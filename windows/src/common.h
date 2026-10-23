#ifndef BWL_COMMON_H
#define BWL_COMMON_H
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stddef.h>

#define BWL_VERSION L"1.0.0"
#define PATH_CAP 1024
#define GITHUB_URL L"https://github.com/Ilikectrlmusic/bingwallpaper-lite"
typedef struct {
    wchar_t root[PATH_CAP], data[PATH_CAP], ini[PATH_CAP], state[PATH_CAP], gui[PATH_CAP], request[PATH_CAP];
    wchar_t cache[PATH_CAP], logs[PATH_CAP], worker[PATH_CAP];
    wchar_t id[24], sid[192], task[256], regkey[256], mutex[300];
} App;
enum { RESOLUTION_1080P = 1080, RESOLUTION_4K = 2160 };
typedef struct { int hour, minute, retention_days, resolution; BOOL daily_enabled; } Settings;

BOOL path_join(wchar_t *out, size_t cap, const wchar_t *dir, const wchar_t *name);
BOOL app_init(App *a);
BOOL app_identity(App *a);
void settings_load(const App *a, Settings *s);
BOOL settings_save(const App *a, const Settings *s);
void local_date(wchar_t out[16]);
BOOL succeeded_today(const App *a);
BOOL mark_success(const App *a, const wchar_t *date, const wchar_t *image);
void log_line(const App *a, const wchar_t *fmt, ...);
void prune_files(const App *a, const Settings *s);
HRESULT scheduler_ensure(const App *a, const Settings *s);
HRESULT scheduler_remove(const App *a);
HRESULT scheduler_validate(const App *a, const Settings *s);
BOOL launch_worker(const App *a);
BOOL delete_path(const wchar_t *path);
BOOL uninstall_app(App *a);
BOOL remove_task_only(App *a);
int wallpaper_work(App *a, const Settings *s);
BOOL json_urlbase(const char *json, wchar_t *out, size_t cap);
BOOL download_image(const App *a, const Settings *s, wchar_t *out, size_t cap);
BOOL valid_image(const wchar_t *path);
BOOL browser_open(HWND owner, const wchar_t *url);
#ifdef BWL_TEST
BOOL WINAPI test_spi(UINT action, UINT param, PVOID data, UINT flags);
#define SystemParametersInfoW test_spi
#endif
#endif
