#include "common.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <shldisp.h>
#include <exdisp.h>
#include <servprov.h>
#include <wbemidl.h>
#include <oleauto.h>
#include <shellapi.h>
#include <stdio.h>
#include <wchar.h>
#ifdef ShellExecute
#undef ShellExecute
#endif

static BOOL add_selector(wchar_t *out, size_t *used, size_t cap, const wchar_t *key, const wchar_t *value) {
    size_t k = wcslen(key), n = wcslen(value), trailing = 0;
    while (trailing < n && value[n - trailing - 1] == L'\\') ++trailing;
    if (*used + k + n + trailing + 3 >= cap) return FALSE;
    if (*used) out[(*used)++] = L' ';
    wmemcpy(out + *used, key, k); *used += k; out[(*used)++] = L'"';
    wmemcpy(out + *used, value, n); *used += n;
    for (size_t i = 0; i < trailing; ++i) out[(*used)++] = L'\\';
    out[(*used)++] = L'"'; out[*used] = 0; return TRUE;
}

/* Copy only profile selectors from an already-running browser, never its URLs or arbitrary switches. */
static BOOL profile_arguments(const wchar_t *command, wchar_t *out, size_t cap) {
    int count = 0; LPWSTR *args = CommandLineToArgvW(command, &count);
    if (!args) return FALSE;
    const wchar_t *data = NULL, *profile = NULL; BOOL child_process = FALSE, ok = FALSE;
    for (int i = 1; i < count; ++i) {
        if (!wcsncmp(args[i], L"--type=", 7) || !wcscmp(args[i], L"--type")) child_process = TRUE;
        if (!wcsncmp(args[i], L"--user-data-dir=", 16)) data = args[i] + 16;
        else if (!wcscmp(args[i], L"--user-data-dir") && i + 1 < count) data = args[++i];
        else if (!wcsncmp(args[i], L"--profile-directory=", 20)) profile = args[i] + 20;
        else if (!wcscmp(args[i], L"--profile-directory") && i + 1 < count) profile = args[++i];
    }
    if (child_process || (!data && !profile)) goto done;
    if (data && (!*data || PathIsRelativeW(data) || wcspbrk(data, L"\"\r\n") || wcslen(data) >= PATH_CAP)) goto done;
    if (profile && (!*profile || wcspbrk(profile, L"\"\r\n\\/") || !wcscmp(profile, L".") ||
        !wcscmp(profile, L"..") || wcslen(profile) >= 128)) goto done;
    size_t used = 0;
    ok = (!data || add_selector(out, &used, cap, L"--user-data-dir=", data)) &&
        (!profile || add_selector(out, &used, cap, L"--profile-directory=", profile));
done:
    LocalFree(args); return ok;
}
static BOOL same_user_session(HANDLE process, DWORD pid) {
    DWORD mine, other;
    if (!ProcessIdToSessionId(GetCurrentProcessId(), &mine) || !ProcessIdToSessionId(pid, &other) || mine != other) return FALSE;
    HANDLE current = NULL, target = NULL; BOOL ok = FALSE; DWORD bytes;
    struct { TOKEN_USER user; BYTE sid[SECURITY_MAX_SID_SIZE]; } a, b;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &current) && OpenProcessToken(process, TOKEN_QUERY, &target) &&
        GetTokenInformation(current, TokenUser, &a, sizeof(a), &bytes) &&
        GetTokenInformation(target, TokenUser, &b, sizeof(b), &bytes)) ok = EqualSid(a.user.User.Sid, b.user.User.Sid);
    if (current) CloseHandle(current);
    if (target) CloseHandle(target);
    return ok;
}
/* Caller has initialized COM. Query the registered HTTPS handler, not a hard-coded browser path. */
static BOOL existing_default_profile(wchar_t *executable, size_t exe_cap, wchar_t *arguments, size_t arg_cap) {
    DWORD length = (DWORD)exe_cap;
    if (FAILED(AssocQueryStringW(ASSOCF_IS_PROTOCOL, ASSOCSTR_EXECUTABLE, L"https", L"open", executable, &length))) return FALSE;
    const wchar_t *base = wcsrchr(executable, L'\\'); base = base ? base + 1 : executable;
    if (_wcsicmp(base, L"chrome.exe") && _wcsicmp(base, L"msedge.exe") && _wcsicmp(base, L"brave.exe") &&
        _wcsicmp(base, L"vivaldi.exe") && _wcsicmp(base, L"chromium.exe")) return FALSE;
    IWbemLocator *locator = NULL; IWbemServices *services = NULL; IEnumWbemClassObject *rows = NULL;
    BSTR ns = SysAllocString(L"ROOT\\CIMV2"), language = SysAllocString(L"WQL"), query = NULL;
    wchar_t sql[200]; swprintf(sql, 200, L"SELECT ProcessId,CommandLine FROM Win32_Process WHERE Name='%ls'", base);
    query = SysAllocString(sql);
    HRESULT hr = CoCreateInstance(&CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (void **)&locator);
    if (SUCCEEDED(hr)) hr = IWbemLocator_ConnectServer(locator, ns, NULL, NULL, NULL, WBEM_FLAG_CONNECT_USE_MAX_WAIT, NULL, NULL, &services);
    if (SUCCEEDED(hr)) hr = CoSetProxyBlanket((IUnknown *)services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (SUCCEEDED(hr)) hr = IWbemServices_ExecQuery(services, language, query, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &rows);
    BOOL found = FALSE, ambiguous = FALSE; ULONGLONG deadline = GetTickCount64() + 3000;
    while (SUCCEEDED(hr) && GetTickCount64() < deadline) {
        IWbemClassObject *row = NULL; ULONG returned = 0;
        hr = IEnumWbemClassObject_Next(rows, 1000, 1, &row, &returned);
        if (!returned) break;
        VARIANT id, command; VariantInit(&id); VariantInit(&command);
        if (SUCCEEDED(IWbemClassObject_Get(row, L"ProcessId", 0, &id, NULL, NULL)) &&
            SUCCEEDED(IWbemClassObject_Get(row, L"CommandLine", 0, &command, NULL, NULL)) &&
            (id.vt == VT_I4 || id.vt == VT_UI4) && command.vt == VT_BSTR && command.bstrVal) {
            DWORD pid = id.ulVal;
            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            wchar_t image[PATH_CAP], candidate[PATH_CAP + 256]; DWORD image_length = PATH_CAP;
            if (process && QueryFullProcessImageNameW(process, 0, image, &image_length) && !_wcsicmp(image, executable) &&
                same_user_session(process, pid) && profile_arguments(command.bstrVal, candidate, PATH_CAP + 256)) {
                if (found && _wcsicmp(arguments, candidate)) ambiguous = TRUE;
                if (wcslen(candidate) < arg_cap) { wcscpy(arguments, candidate); found = TRUE; }
            }
            if (process) CloseHandle(process);
        }
        VariantClear(&id); VariantClear(&command); IWbemClassObject_Release(row);
    }
    if (rows) IEnumWbemClassObject_Release(rows);
    if (services) IWbemServices_Release(services);
    if (locator) IWbemLocator_Release(locator);
    SysFreeString(ns); SysFreeString(language); SysFreeString(query);
    return found && !ambiguous;
}
/* Use the desktop Explorer's user/token/environment even when the GUI was launched elevated. */
static HRESULT explorer_execute(const wchar_t *file, const wchar_t *parameters) {
    IShellWindows *windows = NULL; IDispatch *desktop = NULL, *view_dispatch = NULL, *application = NULL;
    IServiceProvider *provider = NULL; IShellBrowser *browser = NULL; IShellView *view = NULL;
    IShellFolderViewDual *folder = NULL; IShellDispatch2 *shell = NULL;
    VARIANT location, empty; VariantInit(&location); VariantInit(&empty); location.vt = VT_I4; location.lVal = CSIDL_DESKTOP;
    LONG handle = 0;
    HRESULT hr = CoCreateInstance(&CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER, &IID_IShellWindows, (void **)&windows);
    if (SUCCEEDED(hr)) hr = IShellWindows_FindWindowSW(windows, &location, &empty, SWC_DESKTOP, &handle, SWFO_NEEDDISPATCH, &desktop);
    if (SUCCEEDED(hr) && !desktop) hr = E_FAIL;
    if (SUCCEEDED(hr)) hr = IDispatch_QueryInterface(desktop, &IID_IServiceProvider, (void **)&provider);
    if (SUCCEEDED(hr)) hr = IServiceProvider_QueryService(provider, &SID_STopLevelBrowser, &IID_IShellBrowser, (void **)&browser);
    if (SUCCEEDED(hr)) hr = IShellBrowser_QueryActiveShellView(browser, &view);
    if (SUCCEEDED(hr)) hr = IShellView_GetItemObject(view, SVGIO_BACKGROUND, &IID_IDispatch, (void **)&view_dispatch);
    if (SUCCEEDED(hr)) hr = IDispatch_QueryInterface(view_dispatch, &IID_IShellFolderViewDual, (void **)&folder);
    if (SUCCEEDED(hr)) hr = IShellFolderViewDual_get_Application(folder, &application);
    if (SUCCEEDED(hr)) hr = IDispatch_QueryInterface(application, &IID_IShellDispatch2, (void **)&shell);
    if (SUCCEEDED(hr)) {
        BSTR path = SysAllocString(file);
        VARIANT args, verb, show; VariantInit(&args); VariantInit(&verb); VariantInit(&show);
        args.vt = VT_BSTR; args.bstrVal = SysAllocString(parameters ? parameters : L"");
        verb.vt = VT_BSTR; verb.bstrVal = SysAllocString(L"open"); show.vt = VT_I4; show.lVal = SW_SHOWNORMAL;
        hr = IShellDispatch2_ShellExecute(shell, path, args, empty, verb, show);
        SysFreeString(path); VariantClear(&args); VariantClear(&verb);
    }
    if (shell) IShellDispatch2_Release(shell);
    if (application) IDispatch_Release(application);
    if (folder) IShellFolderViewDual_Release(folder);
    if (view_dispatch) IDispatch_Release(view_dispatch);
    if (view) IShellView_Release(view);
    if (browser) IShellBrowser_Release(browser);
    if (provider) IServiceProvider_Release(provider);
    if (desktop) IDispatch_Release(desktop);
    if (windows) IShellWindows_Release(windows);
    return hr;
}
BOOL browser_open(HWND owner, const wchar_t *url) {
    if (!url || wcsncmp(url, L"https://", 8) || wcspbrk(url, L"\"\r\n")) return FALSE;
    HRESULT init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED), hr = E_FAIL;
    BOOL com = SUCCEEDED(init) || init == RPC_E_CHANGED_MODE;
    wchar_t executable[PATH_CAP], profile[PATH_CAP + 256], parameters[PATH_CAP + 1024];
    BOOL reuse = com && existing_default_profile(executable, PATH_CAP, profile, PATH_CAP + 256);
    if (reuse && swprintf(parameters, PATH_CAP + 1024, L"%ls \"%ls\"", profile, url) < 0) reuse = FALSE;
    const wchar_t *file = reuse ? executable : url;
    const wchar_t *args = reuse ? parameters : NULL;
    if (com) hr = explorer_execute(file, args);
    BOOL ok = SUCCEEDED(hr);
    if (!ok) {
        SHELLEXECUTEINFOW info = {0}; info.cbSize = sizeof(info); info.fMask = SEE_MASK_FLAG_NO_UI;
        info.hwnd = owner; info.lpVerb = L"open"; info.lpFile = file; info.lpParameters = args; info.nShow = SW_SHOWNORMAL;
        ok = ShellExecuteExW(&info);
    }
    if (SUCCEEDED(init)) CoUninitialize();
    return ok;
}
