#include "common.h"
#include <shellapi.h>
#include <objbase.h>
#include <wchar.h>

BOOL browser_open(HWND owner, const wchar_t *url) {
    if (!url || wcsncmp(url, L"https://", 8) || !url[8] || wcspbrk(url, L"\"\r\n")) return FALSE;
    HRESULT init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    /* Let Windows resolve the default HTTPS handler and all browser settings. */
    SHELLEXECUTEINFOW info = {0};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_FLAG_NO_UI;
    info.hwnd = owner;
    info.lpVerb = L"open";
    info.lpFile = url;
    info.nShow = SW_SHOWNORMAL;
    BOOL ok = ShellExecuteExW(&info);
    if (SUCCEEDED(init)) CoUninitialize();
    return ok;
}
