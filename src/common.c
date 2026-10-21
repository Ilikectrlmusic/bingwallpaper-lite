#include "common.h"
#include <shlobj.h>
#include <shellapi.h>
#include <sddl.h>
#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>
#include <wctype.h>
#include <taskschd.h>
#include <oleauto.h>

BOOL path_join(wchar_t *out, size_t cap, const wchar_t *dir, const wchar_t *name) {
    int n = swprintf(out, cap, L"%ls\\%ls", dir, name);
    return n > 0 && (size_t)n < cap;
}
static uint64_t hash_text(const wchar_t *p) {
    uint64_t h = UINT64_C(14695981039346656037);
    for (; *p; ++p) { h ^= (uint16_t)towlower(*p); h *= UINT64_C(1099511628211); }
    return h;
}
static BOOL mkdirs(const wchar_t *p) {
    int r = SHCreateDirectoryExW(NULL, p, NULL);
    return r == ERROR_SUCCESS || r == ERROR_ALREADY_EXISTS || r == ERROR_FILE_EXISTS;
}
/* A real write probe, using the non-elevated linked token when UAC-elevated. */
static BOOL writable(const wchar_t *p) {
    HANDLE token = NULL, linked = NULL; TOKEN_ELEVATION elevation = {0}; DWORD n;
    BOOL impersonated = FALSE;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return FALSE;
    if (!GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &n)) {
        CloseHandle(token); return FALSE;
    }
    if (elevation.TokenIsElevated) {
        TOKEN_LINKED_TOKEN lt;
        if (!GetTokenInformation(token, TokenLinkedToken, &lt, sizeof(lt), &n)) {
            CloseHandle(token); return FALSE; /* Never infer ordinary-user access from admin access. */
        }
        linked = lt.LinkedToken;
        impersonated = ImpersonateLoggedOnUser(linked);
        if (!impersonated) { CloseHandle(linked); CloseHandle(token); return FALSE; }
    }
    wchar_t probe[PATH_CAP], name[80];
    swprintf(name, 80, L".bwl-probe-%lu-%llu.tmp", GetCurrentProcessId(), GetTickCount64());
    BOOL ok = path_join(probe, PATH_CAP, p, name);
    HANDLE file = ok ? CreateFileW(probe, GENERIC_WRITE, 0, NULL, CREATE_NEW,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL) : INVALID_HANDLE_VALUE;
    ok = file != INVALID_HANDLE_VALUE;
    if (ok) CloseHandle(file);
    if (impersonated) RevertToSelf();
    if (linked) CloseHandle(linked);
    CloseHandle(token); return ok;
}
static BOOL get_sid(wchar_t *out, size_t cap) {
    HANDLE token; DWORD n = 0; BOOL ok = FALSE;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return FALSE;
    GetTokenInformation(token, TokenUser, NULL, 0, &n);
    TOKEN_USER *user = HeapAlloc(GetProcessHeap(), 0, n);
    if (user && GetTokenInformation(token, TokenUser, user, n, &n)) {
        LPWSTR sid = NULL;
        if (ConvertSidToStringSidW(user->User.Sid, &sid)) {
            ok = wcslen(sid) < cap;
            if (ok) wcscpy(out, sid);
            LocalFree(sid);
        }
    }
    if (user) HeapFree(GetProcessHeap(), 0, user);
    CloseHandle(token); return ok;
}
/* Identity-only initialization must not create or touch user data when removing only a task. */
BOOL app_identity(App *a) {
    ZeroMemory(a, sizeof(*a));
    DWORD n = GetModuleFileNameW(NULL, a->root, PATH_CAP);
    if (!n || n >= PATH_CAP - 1) return FALSE;
    wchar_t *slash = wcsrchr(a->root, L'\\');
    if (!slash || !get_sid(a->sid, 192)) return FALSE;
    *slash = 0;
    swprintf(a->id, 24, L"%016llx", (unsigned long long)hash_text(a->root));
    swprintf(a->task, 256, L"BingWallpaperLite-%ls-%ls", a->id, a->sid);
    swprintf(a->mutex, 300, L"Global\\BingWallpaperLite-%ls-%ls", a->id, a->sid);
    swprintf(a->regkey, 256, L"Software\\BingWallpaperLite\\%ls", a->id);
    return path_join(a->worker, PATH_CAP, a->root, L"BingWallpaperWorker.exe");
}
BOOL app_init(App *a) {
    if (!app_identity(a)) return FALSE;
    wchar_t portable[PATH_CAP], roaming[PATH_CAP], fallback[PATH_CAP], saved[PATH_CAP];
    if (!path_join(portable, PATH_CAP, a->root, L"data") ||
        FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, roaming))) return FALSE;
    if (swprintf(fallback, PATH_CAP, L"%ls\\BingWallpaperLite\\%ls", roaming, a->id) < 0) return FALSE;
    DWORD bytes = sizeof(saved), type;
    BOOL have = RegGetValueW(HKEY_CURRENT_USER, a->regkey, L"DataDir", RRF_RT_REG_SZ,
        &type, saved, &bytes) == ERROR_SUCCESS;
    /* Only our two deterministic locations are accepted. Never trust a registry path for deletion. */
    if (have && (!_wcsicmp(saved, portable) || !_wcsicmp(saved, fallback))) {
        wcscpy(a->data, saved);
        if (!mkdirs(a->data) || !writable(a->data)) return FALSE;
    } else if (writable(a->root) && mkdirs(portable) && writable(portable)) wcscpy(a->data, portable);
    else { wcscpy(a->data, fallback); if (!mkdirs(a->data) || !writable(a->data)) return FALSE; }
    if (!path_join(a->ini, PATH_CAP, a->data, L"settings.ini") ||
        !path_join(a->state, PATH_CAP, a->data, L"state.ini") ||
        !path_join(a->gui, PATH_CAP, a->data, L"gui.ini") ||
        !path_join(a->request, PATH_CAP, a->data, L"request.ini") ||
        !path_join(a->cache, PATH_CAP, a->data, L"cache") ||
        !path_join(a->logs, PATH_CAP, a->data, L"logs") ||
        !path_join(a->worker, PATH_CAP, a->root, L"BingWallpaperWorker.exe") ||
        !mkdirs(a->cache) || !mkdirs(a->logs)) return FALSE;
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, a->regkey, 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL) != ERROR_SUCCESS) return FALSE;
    LONG r = RegSetValueExW(key, L"DataDir", 0, REG_SZ, (BYTE *)a->data, (DWORD)((wcslen(a->data) + 1) * sizeof(wchar_t)));
    RegCloseKey(key); return r == ERROR_SUCCESS;
}
void settings_load(const App *a, Settings *s) {
    s->hour = GetPrivateProfileIntW(L"Schedule", L"Hour", 9, a->ini);
    s->minute = GetPrivateProfileIntW(L"Schedule", L"Minute", 0, a->ini);
    s->cache_days = GetPrivateProfileIntW(L"Storage", L"CacheDays", 3, a->ini);
    s->log_days = GetPrivateProfileIntW(L"Storage", L"LogDays", 3, a->ini);
    wchar_t resolution[32];
    GetPrivateProfileStringW(L"Wallpaper", L"Resolution", L"1080P", resolution, 32, a->ini);
    s->resolution = !_wcsicmp(resolution, L"4K") ? RESOLUTION_4K : RESOLUTION_1080P;
    if (s->hour < 0 || s->hour > 23) s->hour = 9;
    if (s->minute < 0 || s->minute > 59) s->minute = 0;
    if (s->cache_days < 1 || s->cache_days > 365) s->cache_days = 3;
    if (s->log_days < 1 || s->log_days > 365) s->log_days = 3;
}
BOOL settings_save(const App *a, const Settings *s) {
    wchar_t tmp[PATH_CAP];
    if (!path_join(tmp, PATH_CAP, a->data, L"settings.new")) return FALSE;
    HANDLE f = CreateFileW(tmp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return FALSE;
    wchar_t text[512]; DWORD written; WORD bom = 0xfeff;
    int n = swprintf(text, 512, L"[Schedule]\r\nHour=%d\r\nMinute=%d\r\n[Storage]\r\nCacheDays=%d\r\nLogDays=%d\r\n[Wallpaper]\r\nResolution=%ls\r\n",
        s->hour, s->minute, s->cache_days, s->log_days, s->resolution == RESOLUTION_4K ? L"4K" : L"1080P");
    BOOL ok = n > 0 && WriteFile(f, &bom, 2, &written, NULL) && written == 2 &&
        WriteFile(f, text, (DWORD)n * 2, &written, NULL) && written == (DWORD)n * 2 && FlushFileBuffers(f);
    CloseHandle(f);
    if (ok) ok = MoveFileExW(tmp, a->ini, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    WritePrivateProfileStringW(NULL, NULL, NULL, a->ini); return ok;
}
void local_date(wchar_t out[16]) {
    SYSTEMTIME t; GetLocalTime(&t);
    swprintf(out, 16, L"%04u-%02u-%02u", t.wYear, t.wMonth, t.wDay);
}
BOOL succeeded_today(const App *a) {
    wchar_t date[16], last[32]; local_date(date);
    GetPrivateProfileStringW(L"Work", L"LastSuccess", L"", last, 32, a->state);
    return wcscmp(last, date) == 0;
}
BOOL mark_success(const App *a, const wchar_t *date, const wchar_t *image) {
    wchar_t temp[PATH_CAP], text[PATH_CAP + 160];
    if (!path_join(temp, PATH_CAP, a->data, L"state.new")) return FALSE;
    int n = swprintf(text, PATH_CAP + 160, L"[Work]\r\nLastSuccess=%ls\r\nImage=%ls\r\nPending=0\r\n", date, image);
    if (n < 0) return FALSE;
    HANDLE f = CreateFileW(temp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return FALSE;
    DWORD written; WORD bom = 0xfeff;
    BOOL ok = WriteFile(f, &bom, 2, &written, NULL) && written == 2 &&
        WriteFile(f, text, (DWORD)n * 2, &written, NULL) && written == (DWORD)n * 2 && FlushFileBuffers(f);
    CloseHandle(f);
    WritePrivateProfileStringW(NULL, NULL, NULL, a->state);
    return ok && MoveFileExW(temp, a->state, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}
void log_line(const App *a, const wchar_t *fmt, ...) {
    wchar_t date[16], name[48], path[PATH_CAP], body[1500], line[1700];
    local_date(date); swprintf(name, 48, L"%ls.log", date);
    if (!path_join(path, PATH_CAP, a->logs, name)) return;
    va_list ap; va_start(ap, fmt); vswprintf(body, 1500, fmt, ap); va_end(ap);
    SYSTEMTIME t; GetLocalTime(&t);
    swprintf(line, 1700, L"%ls %02u:%02u:%02u %ls\r\n", date, t.wHour, t.wMinute, t.wSecond, body);
    char utf8[7000]; int n = WideCharToMultiByte(CP_UTF8, 0, line, -1, utf8, sizeof(utf8), NULL, NULL);
    HANDLE f = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f != INVALID_HANDLE_VALUE) { DWORD written; if (n > 1) WriteFile(f, utf8, (DWORD)n - 1, &written, NULL); CloseHandle(f); }
}
BOOL recycle_path(const wchar_t *path) {
    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) return TRUE;
    if (wcslen(path) >= PATH_CAP) return FALSE;
    HRESULT init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(init) && init != RPC_E_CHANGED_MODE) return FALSE;
    IFileOperation *op = NULL; IShellItem *item = NULL; BOOL aborted = TRUE;
    HRESULT hr = CoCreateInstance(&CLSID_FileOperation, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOperation, (void **)&op);
    if (SUCCEEDED(hr)) hr = IFileOperation_SetOperationFlags(op, FOFX_RECYCLEONDELETE | FOFX_EARLYFAILURE |
        FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT);
    if (SUCCEEDED(hr)) hr = SHCreateItemFromParsingName(path, NULL, &IID_IShellItem, (void **)&item);
    if (SUCCEEDED(hr)) hr = IFileOperation_DeleteItem(op, item, NULL);
    if (SUCCEEDED(hr)) hr = IFileOperation_PerformOperations(op);
    if (SUCCEEDED(hr)) hr = IFileOperation_GetAnyOperationsAborted(op, &aborted);
    if (item) IShellItem_Release(item);
    if (op) IFileOperation_Release(op);
    if (SUCCEEDED(init)) CoUninitialize();
    return SUCCEEDED(hr) && !aborted;
}
static void prune_dir(const wchar_t *dir, const wchar_t *pattern, int days, const wchar_t *keep) {
    DWORD attributes = GetFileAttributesW(dir);
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return;
    wchar_t glob[PATH_CAP], path[PATH_CAP]; WIN32_FIND_DATAW fd;
    if (!path_join(glob, PATH_CAP, dir, pattern)) return;
    HANDLE h = FindFirstFileW(glob, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    SYSTEMTIME now; GetLocalTime(&now); now.wHour = now.wMinute = now.wSecond = now.wMilliseconds = 0;
    FILETIME midnight; SystemTimeToFileTime(&now, &midnight);
    ULARGE_INTEGER cut; cut.LowPart = midnight.dwLowDateTime; cut.HighPart = midnight.dwHighDateTime;
    cut.QuadPart -= (uint64_t)(days - 1) * UINT64_C(864000000000);
    do {
        if (fd.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) continue;
        unsigned y, m, d; int used = 0;
        if (swscanf(fd.cFileName, L"%4u-%2u-%2u%n", &y, &m, &d, &used) != 3 || used != 10) continue;
        SYSTEMTIME day = {0}; day.wYear = (WORD)y; day.wMonth = (WORD)m; day.wDay = (WORD)d;
        FILETIME ft;
        if (!SystemTimeToFileTime(&day, &ft)) continue;
        ULARGE_INTEGER stamp; stamp.LowPart = ft.dwLowDateTime; stamp.HighPart = ft.dwHighDateTime;
        if (stamp.QuadPart < cut.QuadPart && path_join(path, PATH_CAP, dir, fd.cFileName) &&
            (!keep || _wcsicmp(path, keep))) recycle_path(path);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}
void prune_files(const App *a, const Settings *s) {
    wchar_t current[PATH_CAP] = {0};
    SystemParametersInfoW(SPI_GETDESKWALLPAPER, PATH_CAP, current, 0);
    prune_dir(a->cache, L"????" L"-??" L"-??.jpg", s->cache_days, current);
    prune_dir(a->cache, L"????" L"-??" L"-??.part", s->cache_days, NULL);
    prune_dir(a->logs, L"????" L"-??" L"-??.log", s->log_days, NULL);
}
