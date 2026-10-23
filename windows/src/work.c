#include "common.h"
#include <stdio.h>
#include <wchar.h>
#ifdef BWL_TEST
extern BOOL mock_download(const App *, const Settings *, wchar_t *, size_t);
#define download_image mock_download
#endif

int wallpaper_work(App *a, const Settings *s) {
    prune_files(a, s);
    if (succeeded_today(a)) return 0;
    wchar_t today[16], file[PATH_CAP], name[48], current[PATH_CAP] = {0};
    local_date(today); swprintf(name, 48, L"%ls.jpg", today);
    if (!path_join(file, PATH_CAP, a->cache, name)) return 1;
    /* Recover a crash after SPI succeeded but before its commit record was written. */
    if (SystemParametersInfoW(SPI_GETDESKWALLPAPER, PATH_CAP, current, 0) &&
        !_wcsicmp(current, file) && valid_image(file)) {
        log_line(a, L"Recovered successful wallpaper change from the current desktop path.");
        return mark_success(a, today, file) ? 0 : 1;
    }
    if (!WritePrivateProfileStringW(L"Work", L"Pending", L"1", a->state)) return 1;
    if (!download_image(a, s, file, PATH_CAP)) return 1;
    if (!SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, file, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
        log_line(a, L"Wallpaper update failed: %lu", GetLastError()); return 1;
    }
    local_date(today);
    if (!mark_success(a, today, file)) { log_line(a, L"Wallpaper changed but success record could not be saved."); return 1; }
    log_line(a, L"SUCCESS: wallpaper updated. Worker is exiting.");
    prune_files(a, s); return 0;
}
