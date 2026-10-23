#include "common.h"
#include <shellapi.h>
#include <wchar.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR cmd, int show) {
    (void)instance; (void)previous; (void)cmd; (void)show;
    int argc; LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return 2;
    BOOL uninstall = argc == 2 && !wcscmp(argv[1], L"--uninstall");
    BOOL remove_task = argc == 2 && !wcscmp(argv[1], L"--remove-task");
    BOOL configure = argc == 2 && !wcscmp(argv[1], L"--configure");
    BOOL start = argc == 2 && !wcscmp(argv[1], L"--start");
    BOOL scheduled = argc == 2 && !wcscmp(argv[1], L"--scheduled");
    BOOL invalid = argc > 1 && !uninstall && !remove_task && !configure && !start && !scheduled;
    if (argv) LocalFree(argv);
    if (invalid) return 64;
    App a;
    if (remove_task) return app_identity(&a) && remove_task_only(&a) ? 0 : 3;
    if (!app_init(&a)) return 2;
    if (uninstall) return uninstall_app(&a) ? 0 : 3;
    Settings settings; settings_load(&a, &settings);
    /* Stop an automatic run before waiting for its lock. Unlike uninstall, configure
       must work while the GUI is open and owns its separate GUI mutex. */
    if (configure && !settings.daily_enabled) {
        HRESULT hr = scheduler_remove(&a);
        if (FAILED(hr)) { log_line(&a, L"Task removal failed: 0x%08lx", (unsigned long)hr); return 4; }
    }
    HANDLE mutex = CreateMutexW(NULL, FALSE, a.mutex);
    if (!mutex) return 2;
    DWORD wait = WaitForSingleObject(mutex, configure || start ? 180000 : 0);
    if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED) { CloseHandle(mutex); return configure || start ? 6 : 0; }
    /* Settings may have changed while a previous worker held the lock. */
    settings_load(&a, &settings);
    if (scheduled && !settings.daily_enabled) { ReleaseMutex(mutex); CloseHandle(mutex); return 0; }
    HRESULT hr = S_OK;
    if (settings.daily_enabled || configure) hr = scheduler_ensure(&a, &settings);
    if (FAILED(hr)) log_line(&a, L"Task configuration failed: 0x%08lx", (unsigned long)hr);
    if (configure || (start && settings.daily_enabled)) {
        int result = FAILED(hr) ? 4 : 0;
        if (!result && start) {
            wchar_t today[16]; local_date(today);
            if (!WritePrivateProfileStringW(L"Request", L"Date", today, a.request)) result = 2;
        }
        ReleaseMutex(mutex); CloseHandle(mutex);
        if (!result && start && !launch_worker(&a)) result = 5;
        return result;
    }
    /* Manual updates work with automatic updates off, without creating a retry task. */
    if (scheduled && !GetPrivateProfileIntW(L"Work", L"Pending", 0, a.state)) {
        SYSTEMTIME now; GetLocalTime(&now);
        wchar_t today[16], requested[32]; local_date(today);
        GetPrivateProfileStringW(L"Request", L"Date", L"", requested, 32, a.request);
        if (now.wHour * 60 + now.wMinute < settings.hour * 60 + settings.minute && wcscmp(today, requested)) {
            ReleaseMutex(mutex); CloseHandle(mutex); return FAILED(hr) ? 4 : 0;
        }
    }
    int result = wallpaper_work(&a, &settings);
    ReleaseMutex(mutex); CloseHandle(mutex);
    return result ? result : (FAILED(hr) ? 4 : 0);
}
