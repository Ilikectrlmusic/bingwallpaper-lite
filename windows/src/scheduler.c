#include "common.h"
#include <taskschd.h>
#include <oleauto.h>
#include <stdio.h>
#include <wchar.h>

static BOOL xml_escape(const wchar_t *in, wchar_t *out, size_t cap) {
    size_t n = 0;
    for (; *in; ++in) {
        const wchar_t *s = *in == L'&' ? L"&amp;" : *in == L'<' ? L"&lt;" : *in == L'>' ? L"&gt;" : *in == L'\"' ? L"&quot;" : NULL;
        size_t k = s ? wcslen(s) : 1;
        if (n + k >= cap) return FALSE;
        if (s) wmemcpy(out + n, s, k); else out[n] = *in;
        n += k;
    }
    out[n] = 0; return TRUE;
}
static HRESULT scheduler(const App *a, const Settings *s, int operation) {
    HRESULT init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(init) && init != RPC_E_CHANGED_MODE) return init;
    ITaskService *svc = NULL; ITaskFolder *folder = NULL; IRegisteredTask *task = NULL;
    VARIANT empty; VariantInit(&empty);
    BSTR root = SysAllocString(L"\\"), name = SysAllocString(a->task), xml = NULL, existing = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&svc);
    if (SUCCEEDED(hr)) hr = ITaskService_Connect(svc, empty, empty, empty, empty);
    if (SUCCEEDED(hr)) hr = ITaskService_GetFolder(svc, root, &folder);
    if (FAILED(hr)) goto done;
    hr = ITaskFolder_GetTask(folder, name, &task);
    if (operation == 1) {
        if (SUCCEEDED(hr)) {
            HRESULT stop = IRegisteredTask_Stop(task, 0);
            (void)stop;
            hr = ITaskFolder_DeleteTask(folder, name, 0);
        } else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) hr = S_OK;
        goto done;
    }
    wchar_t exe[PATH_CAP * 6], cwd[PATH_CAP * 6], text[20000], signature[100];
    if (!xml_escape(a->worker, exe, PATH_CAP * 6) || !xml_escape(a->root, cwd, PATH_CAP * 6)) { hr = E_INVALIDARG; goto done; }
    swprintf(signature, 100, L"BWL-v2-%ls-%02d%02d", a->id, s->hour, s->minute);
    int n = swprintf(text, 20000,
        L"<?xml version=\"1.0\" encoding=\"UTF-16\"?>"
        L"<Task version=\"1.3\" xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">"
        L"<RegistrationInfo><Description>%ls</Description></RegistrationInfo>"
        L"<Triggers><CalendarTrigger><StartBoundary>2020-01-01T%02d:%02d:00</StartBoundary><Enabled>true</Enabled>"
        L"<ScheduleByDay><DaysInterval>1</DaysInterval></ScheduleByDay></CalendarTrigger>"
        L"<LogonTrigger><Enabled>true</Enabled><Delay>PT20S</Delay><UserId>%ls</UserId></LogonTrigger></Triggers>"
        L"<Principals><Principal id=\"User\"><UserId>%ls</UserId><LogonType>InteractiveToken</LogonType><RunLevel>LeastPrivilege</RunLevel></Principal></Principals>"
        L"<Settings><MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy><DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>"
        L"<StopIfGoingOnBatteries>false</StopIfGoingOnBatteries><StartWhenAvailable>true</StartWhenAvailable>"
        L"<RunOnlyIfNetworkAvailable>false</RunOnlyIfNetworkAvailable><AllowStartOnDemand>true</AllowStartOnDemand>"
        L"<Enabled>true</Enabled><ExecutionTimeLimit>PT4M</ExecutionTimeLimit>"
        L"<RestartOnFailure><Interval>PT30M</Interval><Count>48</Count></RestartOnFailure></Settings>"
        L"<Actions Context=\"User\"><Exec><Command>%ls</Command><Arguments>--scheduled</Arguments><WorkingDirectory>%ls</WorkingDirectory></Exec></Actions></Task>",
        signature, s->hour, s->minute, a->sid, a->sid, exe, cwd);
    if (n < 0) { hr = E_INVALIDARG; goto done; }
    wchar_t boundary[80]; swprintf(boundary, 80, L"<StartBoundary>2020-01-01T%02d:%02d:00</StartBoundary>", s->hour, s->minute);
    if (operation == 0 && task && SUCCEEDED(IRegisteredTask_get_Xml(task, &existing)) &&
        wcsstr(existing, signature) && wcsstr(existing, exe) && wcsstr(existing, boundary) &&
        wcsstr(existing, L"<Interval>PT30M</Interval>") && wcsstr(existing, L"<Count>48</Count>") &&
        wcsstr(existing, L"<LogonType>InteractiveToken</LogonType>") && !wcsstr(existing, L"<RunLevel>HighestAvailable</RunLevel>") &&
        wcsstr(existing, L"<Arguments>--scheduled</Arguments>") && wcsstr(existing, L"<DaysInterval>1</DaysInterval>") &&
        wcsstr(existing, L"<StartWhenAvailable>true</StartWhenAvailable>") && wcsstr(existing, L"<Delay>PT20S</Delay>") &&
        wcsstr(existing, L"<ExecutionTimeLimit>PT4M</ExecutionTimeLimit>")) {
        hr = IRegisteredTask_put_Enabled(task, VARIANT_TRUE); goto done;
    }
    if (task) { IRegisteredTask_Release(task); task = NULL; }
    xml = SysAllocString(text);
    VARIANT user; VariantInit(&user); user.vt = VT_BSTR; user.bstrVal = SysAllocString(a->sid);
    hr = ITaskFolder_RegisterTask(folder, name, xml, operation == 2 ? TASK_VALIDATE_ONLY : TASK_CREATE_OR_UPDATE,
        user, empty, TASK_LOGON_INTERACTIVE_TOKEN, empty, &task);
    VariantClear(&user);
done:
    if (existing) SysFreeString(existing);
    if (xml) SysFreeString(xml);
    SysFreeString(root); SysFreeString(name);
    if (task) IRegisteredTask_Release(task);
    if (folder) ITaskFolder_Release(folder);
    if (svc) ITaskService_Release(svc);
    if (SUCCEEDED(init)) CoUninitialize();
    return hr;
}
HRESULT scheduler_ensure(const App *a, const Settings *s) { return scheduler(a, s->daily_enabled ? s : NULL, s->daily_enabled ? 0 : 1); }
HRESULT scheduler_remove(const App *a) { return scheduler(a, NULL, 1); }
HRESULT scheduler_validate(const App *a, const Settings *s) { return scheduler(a, s, 2); }
BOOL launch_worker(const App *a) {
    /* Start the registered task, not an unmanaged child: its failure-restart policy then applies immediately. */
    HRESULT init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ITaskService *svc = NULL; ITaskFolder *folder = NULL; IRegisteredTask *task = NULL; IRunningTask *running = NULL;
    VARIANT empty; VariantInit(&empty); BSTR root = SysAllocString(L"\\"), name = SysAllocString(a->task);
    HRESULT hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&svc);
    if (SUCCEEDED(hr)) hr = ITaskService_Connect(svc, empty, empty, empty, empty);
    if (SUCCEEDED(hr)) hr = ITaskService_GetFolder(svc, root, &folder);
    if (SUCCEEDED(hr)) hr = ITaskFolder_GetTask(folder, name, &task);
    if (SUCCEEDED(hr)) hr = IRegisteredTask_Run(task, empty, &running);
    if (running) IRunningTask_Release(running);
    if (task) IRegisteredTask_Release(task);
    if (folder) ITaskFolder_Release(folder);
    if (svc) ITaskService_Release(svc);
    SysFreeString(root); SysFreeString(name); if (SUCCEEDED(init)) CoUninitialize();
    return SUCCEEDED(hr);
}
static BOOL remove_installation(App *a, BOOL data_too) {
    wchar_t gui_name[320]; swprintf(gui_name, 320, L"%ls-GUI", a->mutex);
    HANDLE gui = CreateMutexW(NULL, FALSE, gui_name);
    if (!gui) return FALSE;
    DWORD gui_wait = WaitForSingleObject(gui, 0);
    if (gui_wait != WAIT_OBJECT_0 && gui_wait != WAIT_ABANDONED) { CloseHandle(gui); return FALSE; }
    HRESULT hr = scheduler_remove(a);
    if (FAILED(hr)) {
        if (data_too) log_line(a, L"Uninstall: task removal failed 0x%08lx", (unsigned long)hr);
        ReleaseMutex(gui); CloseHandle(gui); return FALSE;
    }
    HANDLE mutex = CreateMutexW(NULL, FALSE, a->mutex);
    if (!mutex) { ReleaseMutex(gui); CloseHandle(gui); return FALSE; }
    DWORD wait = WaitForSingleObject(mutex, 180000);
    if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED) { CloseHandle(mutex); ReleaseMutex(gui); CloseHandle(gui); return FALSE; }
    /* A previously running worker may have repaired the task while we waited for it.
       Remove once more under the worker lock so moving the directory leaves no old task. */
    if (FAILED(scheduler_remove(a))) {
        ReleaseMutex(mutex); CloseHandle(mutex); ReleaseMutex(gui); CloseHandle(gui); return FALSE;
    }
    BOOL ok = !data_too || delete_path(a->data);
    if (ok && data_too) {
        LONG r = RegDeleteTreeW(HKEY_CURRENT_USER, a->regkey); ok = r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND;
        HKEY parent; DWORD subkeys = 1, values = 1;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\BingWallpaperLite", 0, KEY_READ, &parent) == ERROR_SUCCESS) {
            RegQueryInfoKeyW(parent, NULL, NULL, NULL, &subkeys, NULL, NULL, &values, NULL, NULL, NULL, NULL);
            RegCloseKey(parent);
            if (!subkeys && !values) RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\BingWallpaperLite");
        }
        wchar_t parent_path[PATH_CAP]; wcscpy(parent_path, a->data);
        wchar_t *slash = wcsrchr(parent_path, L'\\');
        if (slash) {
            *slash = 0;
            wchar_t *base = wcsrchr(parent_path, L'\\');
            if (base && !_wcsicmp(base + 1, L"BingWallpaperLite")) {
                wchar_t glob[PATH_CAP]; path_join(glob, PATH_CAP, parent_path, L"*");
                WIN32_FIND_DATAW fd; HANDLE search = FindFirstFileW(glob, &fd); BOOL empty = TRUE;
                if (search != INVALID_HANDLE_VALUE) {
                    do { if (wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")) empty = FALSE; } while (FindNextFileW(search, &fd));
                    FindClose(search);
                    /* Non-recursive: another installation may create data after the empty check. */
                    if (empty && !RemoveDirectoryW(parent_path)) {
                        DWORD error = GetLastError();
                        if (error != ERROR_DIR_NOT_EMPTY && error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) ok = FALSE;
                    }
                }
            }
        }
    }
    ReleaseMutex(mutex); CloseHandle(mutex); ReleaseMutex(gui); CloseHandle(gui); return ok;
}
BOOL uninstall_app(App *a) { return remove_installation(a, TRUE); }
BOOL remove_task_only(App *a) { return remove_installation(a, FALSE); }
