#include "common.h"
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <wchar.h>

enum { ID_TIME = 100, ID_CACHE, ID_LOG, ID_SAVE, ID_RUN, ID_GITHUB, ID_OPEN, ID_VERSION, ID_RESOLUTION };
enum { CLIENT_WIDTH = 420, CLIENT_HEIGHT = 366, INPUT_WIDTH = 68, INPUT_HEIGHT = 24 };
enum { ID_SAVE_EXIT = 200 };
static App app;
static HFONT font, title_font, small_font;
static HBRUSH background;
static HANDLE child;
static BOOL first_request;
static Settings saved_settings, pending_settings;
static BOOL save_inflight, save_pending, exit_after_save;
static HWND window, time_control, cache_control, log_control, resolution_control, last_control;
static HICON github_icon;
static BOOL ui_chinese;
static BOOL chinese_language(LANGID language) {
    return language == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED) ||
        language == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE) || language == 0x7804; /* zh-Hans */
}
#define UI(zh, en) (ui_chinese ? (zh) : (en))
static void init_language(void) {
    ui_chinese = chinese_language(GetUserDefaultUILanguage());
    /* Current user's Windows display language, not regional number/date formatting. */
    SetThreadUILanguage(ui_chinese ? MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED) : MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
}
static int dpi = 96;
static int px(int n) { return MulDiv(n, dpi, 96); }
static HWND control(const wchar_t *class_name, const wchar_t *text, DWORD style, int x, int y, int w, int h, int id) {
    HWND c = CreateWindowExW(0, class_name, text, WS_CHILD | WS_VISIBLE | style,
        px(x), px(y), px(w), px(h), window, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(c, WM_SETFONT, (WPARAM)font, TRUE); return c;
}
/* Errors are exceptional dialogs, not a permanent status row in the compact layout. */
static void status(const wchar_t *text) { MessageBoxW(window, text, L"Bing Wallpaper", MB_OK | MB_ICONERROR); }
static LRESULT CALLBACK github_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR data) {
    (void)id; (void)data;
    if (msg == WM_SETCURSOR) { SetCursor(LoadCursorW(NULL, IDC_HAND)); return TRUE; }
    return DefSubclassProc(hwnd, msg, wp, lp);
}
static LRESULT CALLBACK resolution_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR data) {
    (void)id; (void)data;
    LRESULT result = DefSubclassProc(hwnd, msg, wp, lp);
    /* Native combos can omit their selected item when printed with a hidden parent.
       Use the same owner-drawing path for print/preview as for normal on-screen painting. */
    if (msg == WM_PRINTCLIENT || (msg == WM_PRINT && (lp & PRF_CLIENT))) {
        COMBOBOXINFO info = {0}; info.cbSize = sizeof(info);
        if (GetComboBoxInfo(hwnd, &info)) {
            DRAWITEMSTRUCT item = {0}; item.CtlType = ODT_COMBOBOX; item.CtlID = ID_RESOLUTION;
            item.itemID = (UINT)SendMessageW(hwnd, CB_GETCURSEL, 0, 0);
            item.itemAction = ODA_DRAWENTIRE; item.itemState = ODS_COMBOBOXEDIT;
            if (!IsWindowEnabled(hwnd)) item.itemState |= ODS_DISABLED;
            item.hwndItem = hwnd; item.hDC = (HDC)wp; item.rcItem = info.rcItem;
            SendMessageW(GetParent(hwnd), WM_DRAWITEM, ID_RESOLUTION, (LPARAM)&item);
        }
    }
    return result;
}
static void refresh_last(void) {
    wchar_t last[32], text[140];
    GetPrivateProfileStringW(L"Work", L"LastSuccess", UI(L"尚未成功", L"Never"), last, 32, app.state);
    swprintf(text, 140, UI(L"上次成功运行：%ls", L"Last successful run: %ls"), last);
    SetWindowTextW(last_control, text);
}
/* The GUI only starts the worker. It never calls any Task Scheduler API. */
static BOOL worker_request(const wchar_t *mode, BOOL initial) {
    if (child) { status(UI(L"工作程序正在处理上一个设置请求，请稍候。", L"The worker is handling the previous request. Please wait.")); return FALSE; }
    wchar_t cmd[PATH_CAP + 64]; swprintf(cmd, PATH_CAP + 64, L"\"%ls\" %ls", app.worker, mode);
    STARTUPINFOW si = {0}; si.cb = sizeof(si); si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(app.worker, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, app.root, &si, &pi)) {
        status(UI(L"无法启动工作程序，请确认 BingWallpaperWorker.exe 在同一目录。", L"Cannot start the worker. Make sure BingWallpaperWorker.exe is in the same folder.")); return FALSE;
    }
    CloseHandle(pi.hThread); child = pi.hProcess; first_request = initial;
    EnableWindow(GetDlgItem(window, ID_SAVE), FALSE); EnableWindow(GetDlgItem(window, ID_RUN), FALSE);
    return TRUE;
}
static BOOL read_settings(Settings *s) {
    SYSTEMTIME t; if (SendMessageW(time_control, DTM_GETSYSTEMTIME, 0, (LPARAM)&t) != GDT_VALID) return FALSE;
    BOOL a, b;
    s->hour = t.wHour; s->minute = t.wMinute;
    s->cache_days = (int)GetDlgItemInt(window, ID_CACHE, &a, FALSE);
    s->log_days = (int)GetDlgItemInt(window, ID_LOG, &b, FALSE);
    LRESULT selected = SendMessageW(resolution_control, CB_GETCURSEL, 0, 0);
    s->resolution = selected == 1 ? RESOLUTION_4K : RESOLUTION_1080P;
    return (selected == 0 || selected == 1) && a && b && s->cache_days >= 1 && s->cache_days <= 365 && s->log_days >= 1 && s->log_days <= 365;
}
static BOOL settings_dirty(void) {
    Settings s;
    return !read_settings(&s) || s.hour != saved_settings.hour || s.minute != saved_settings.minute ||
        s.cache_days != saved_settings.cache_days || s.log_days != saved_settings.log_days || s.resolution != saved_settings.resolution;
}
static void enable_form(BOOL enable) {
    const int ids[] = {ID_TIME, ID_CACHE, ID_LOG, ID_RESOLUTION, ID_SAVE, ID_RUN};
    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) EnableWindow(GetDlgItem(window, ids[i]), enable);
}
static BOOL start_pending_save(void) {
    save_pending = FALSE;
    if (!worker_request(L"--configure", FALSE)) {
        save_inflight = FALSE; exit_after_save = FALSE; enable_form(TRUE); return FALSE;
    }
    save_inflight = TRUE; enable_form(FALSE); return TRUE;
}
static BOOL save_current(BOOL and_exit) {
    Settings s;
    if (!read_settings(&s)) {
        MessageBoxW(window, UI(L"缓存和日志保留天数请输入 1 到 365 的整数。", L"Enter whole numbers from 1 to 365 for cache and log retention."), UI(L"请检查设置", L"Check settings"), MB_OK | MB_ICONWARNING);
        return FALSE;
    }
    if (!settings_save(&app, &s)) { status(UI(L"设置保存失败，请检查数据目录的权限和磁盘空间。", L"Cannot save settings. Check data folder permissions and free disk space.")); return FALSE; }
    pending_settings = s; exit_after_save = and_exit; enable_form(FALSE);
    if (child) { save_pending = TRUE; return TRUE; }
    return start_pending_save();
}
static int ask_save_before_exit(void) {
    const TASKDIALOG_BUTTON buttons[] = {
        {ID_SAVE_EXIT, UI(L"保存并退出", L"Save and exit")}, {IDCANCEL, UI(L"取消", L"Cancel")}
    };
    TASKDIALOGCONFIG dialog = {0}; dialog.cbSize = sizeof(dialog); dialog.hwndParent = window;
    dialog.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
    dialog.pszWindowTitle = UI(L"未保存的设置", L"Unsaved settings"); dialog.pszMainIcon = TD_WARNING_ICON;
    dialog.pszMainInstruction = UI(L"设置尚未保存", L"You have unsaved changes"); dialog.pszContent = UI(L"是否保存更改后退出？", L"Save your changes before exiting?");
    dialog.cButtons = 2; dialog.pButtons = buttons; dialog.nDefaultButton = ID_SAVE_EXIT;
    int selected = IDCANCEL;
    if (FAILED(TaskDialogIndirect(&dialog, &selected, NULL, NULL))) {
        MessageBoxW(window, UI(L"无法显示保存确认窗口，已取消退出。请点击“保存设置”后再退出。", L"Cannot show the confirmation dialog. Exit was cancelled. Click Save settings before closing."),
            UI(L"未保存的设置", L"Unsaved settings"), MB_OK | MB_ICONWARNING);
    }
    return selected;
}
static LRESULT CALLBACK procedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        window = hwnd;
        init_language();
        SetWindowTextW(hwnd, UI(L"Bing Wallpaper · 设置", L"Bing Wallpaper · Settings"));
        save_inflight = save_pending = exit_after_save = FALSE;
        HDC dc = GetDC(hwnd); dpi = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(hwnd, dc);
        const wchar_t *face = UI(L"Microsoft YaHei UI", L"Segoe UI");
        font = CreateFontW(-px(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, face);
        title_font = CreateFontW(-px(22), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, face);
        small_font = CreateFontW(-px(11), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, face);
        background = CreateSolidBrush(RGB(246, 248, 251));
        HWND icon = control(L"STATIC", L"", SS_ICON, 24, 22, 36, 36, 0);
        HICON large = LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(1), IMAGE_ICON, px(36), px(36), LR_DEFAULTCOLOR | LR_SHARED);
        SendMessageW(icon, STM_SETICON, (WPARAM)large, 0);
        HWND title = control(L"STATIC", L"Bing Wallpaper", 0, 74, 18, 320, 32, 0);
        SendMessageW(title, WM_SETFONT, (WPARAM)title_font, TRUE);
        control(L"STATIC", UI(L"每日一张风景。", L"A new view every day."), 0, 76, 52, 300, 20, 0);
        control(L"STATIC", UI(L"每日启动时间", L"Daily start time"), 0, 24, 103, 230, 20, 0);
        time_control = control(DATETIMEPICK_CLASSW, L"", DTS_TIMEFORMAT | WS_TABSTOP, 290, 100, INPUT_WIDTH, INPUT_HEIGHT, ID_TIME);
        SendMessageW(time_control, DTM_SETFORMATW, 0, (LPARAM)L"HH:mm");
        control(L"STATIC", UI(L"图片缓存保留天数", L"Image cache retention"), 0, 24, 141, 230, 20, 0);
        cache_control = control(L"EDIT", L"", WS_BORDER | ES_NUMBER | WS_TABSTOP, 290, 138, INPUT_WIDTH, INPUT_HEIGHT, ID_CACHE);
        SendMessageW(cache_control, EM_SETLIMITTEXT, 3, 0);
        control(L"STATIC", UI(L"天", L"days"), 0, 302 + INPUT_WIDTH, 141, UI(16, 32), 20, 0);
        control(L"STATIC", UI(L"运行日志保留天数", L"Log retention"), 0, 24, 179, 230, 20, 0);
        log_control = control(L"EDIT", L"", WS_BORDER | ES_NUMBER | WS_TABSTOP, 290, 176, INPUT_WIDTH, INPUT_HEIGHT, ID_LOG);
        SendMessageW(log_control, EM_SETLIMITTEXT, 3, 0);
        control(L"STATIC", UI(L"天", L"days"), 0, 302 + INPUT_WIDTH, 179, UI(16, 32), 20, 0);
        control(L"STATIC", UI(L"下载壁纸分辨率", L"Download resolution"), 0, 24, 217, 230, 20, 0);
        resolution_control = control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, 290, 214, INPUT_WIDTH, 100, ID_RESOLUTION);
        SetWindowSubclass(resolution_control, resolution_proc, 1, 0);
        SendMessageW(resolution_control, CB_ADDSTRING, 0, (LPARAM)L"1080P");
        SendMessageW(resolution_control, CB_ADDSTRING, 0, (LPARAM)L"4K");
        SendMessageW(resolution_control, CB_SETITEMHEIGHT, (WPARAM)-1, px(INPUT_HEIGHT) - 6);
        SendMessageW(resolution_control, CB_SETITEMHEIGHT, 0, px(20));
        control(L"BUTTON", UI(L"保存设置", L"Save settings"), WS_TABSTOP | BS_DEFPUSHBUTTON, 24, 254, 100, 28, ID_SAVE);
        control(L"BUTTON", UI(L"立即检查 / 更换", L"Check / update"), WS_TABSTOP, 136, 254, 128, 28, ID_RUN);
        control(L"BUTTON", UI(L"打开数据目录", L"Open data folder"), WS_TABSTOP, 276, 254, 120, 28, ID_OPEN);
        last_control = control(L"STATIC", L"", 0, 24, 298, 372, 20, 0);
        SendMessageW(last_control, WM_SETFONT, (WPARAM)small_font, TRUE);
        /* Accessible button name retained; owner drawing renders only the icon, never text or a border. */
        HWND link = control(L"BUTTON", L"GitHub", WS_TABSTOP | BS_OWNERDRAW, 22, 324, 36, 32, ID_GITHUB);
        github_icon = LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(2), IMAGE_ICON, px(28), px(28), LR_SHARED);
        SetWindowSubclass(link, github_proc, 1, 0);
        HWND version = control(L"STATIC", L"V1.0.0", SS_RIGHT, 316, 334, 80, 18, ID_VERSION);
        SendMessageW(version, WM_SETFONT, (WPARAM)small_font, TRUE);
        Settings s; settings_load(&app, &s);
        SYSTEMTIME t; GetLocalTime(&t); t.wHour = (WORD)s.hour; t.wMinute = (WORD)s.minute; t.wSecond = 0;
        SendMessageW(time_control, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&t);
        SetDlgItemInt(hwnd, ID_CACHE, s.cache_days, FALSE); SetDlgItemInt(hwnd, ID_LOG, s.log_days, FALSE);
        SendMessageW(resolution_control, CB_SETCURSEL, s.resolution == RESOLUTION_4K ? 1 : 0, 0);
        saved_settings = s;
        refresh_last();
        SetTimer(hwnd, 1, 500, NULL);
        PostMessageW(hwnd, WM_APP, 0, 0);
        return 0;
    }
    case WM_APP: {
        BOOL first = !GetPrivateProfileIntW(L"GUI", L"InitialRequested", 0, app.gui);
        worker_request(first ? L"--start" : L"--configure", first); return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_SAVE: save_current(FALSE); return 0;
        case ID_RUN: worker_request(L"--start", FALSE); return 0;
        case ID_OPEN: ShellExecuteW(hwnd, L"open", app.data, NULL, NULL, SW_SHOWNORMAL); return 0;
        case ID_GITHUB:
            if (!browser_open(hwnd, GITHUB_URL)) status(UI(L"无法打开系统默认浏览器，请检查默认 HTTPS 应用设置。", L"Cannot open the default browser. Check the default HTTPS app in Windows settings."));
            return 0;
        }
        break;
    case WM_TIMER:
        refresh_last();
        if (child && WaitForSingleObject(child, 0) == WAIT_OBJECT_0) {
            DWORD code = 1; GetExitCodeProcess(child, &code); CloseHandle(child); child = NULL;
            BOOL was_save = save_inflight; save_inflight = FALSE;
            if (code == 0) {
                if (first_request) WritePrivateProfileStringW(L"GUI", L"InitialRequested", L"1", app.gui);
                if (was_save) saved_settings = pending_settings;
            } else {
                if (was_save || !save_pending) exit_after_save = FALSE;
                wchar_t text[512]; swprintf(text, 512, UI(L"工作程序返回错误 %lu。请打开数据目录查看 logs；任务设置失败时无法自动重试。", L"The worker returned error %lu. Open the data folder and check logs. Automatic retries require a working scheduled task."), code); status(text);
            }
            if (save_pending) { start_pending_save(); return 0; }
            enable_form(TRUE);
            if (was_save && code == 0 && exit_after_save) { DestroyWindow(hwnd); return 0; }
        }
        return 0;
    case WM_MEASUREITEM: {
        MEASUREITEMSTRUCT *item = (MEASUREITEMSTRUCT *)lp;
        if (item->CtlID == ID_RESOLUTION) { item->itemHeight = (UINT)px(20); return TRUE; }
        break;
    }
    case WM_DRAWITEM: {
        const DRAWITEMSTRUCT *item = (const DRAWITEMSTRUCT *)lp;
        if (item->CtlID == ID_RESOLUTION) {
            /* Draw both the closed selection and list items explicitly, including the initial 1080P value. */
            BOOL highlighted = (item->itemState & ODS_SELECTED) && !(item->itemState & ODS_COMBOBOXEDIT);
            int bg = highlighted ? COLOR_HIGHLIGHT : COLOR_WINDOW;
            int fg = item->itemState & ODS_DISABLED ? COLOR_GRAYTEXT : highlighted ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT;
            int saved_dc = SaveDC(item->hDC);
            FillRect(item->hDC, &item->rcItem, GetSysColorBrush(bg));
            SetTextColor(item->hDC, GetSysColor(fg)); SetBkMode(item->hDC, TRANSPARENT);
            HGDIOBJ previous = SelectObject(item->hDC, font);
            RECT text = item->rcItem; text.left += px(4);
            DrawTextW(item->hDC, item->itemID == 1 ? L"4K" : L"1080P", -1, &text, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
            SelectObject(item->hDC, previous);
            if ((item->itemState & ODS_FOCUS) && !(item->itemState & ODS_NOFOCUSRECT)) DrawFocusRect(item->hDC, &item->rcItem);
            RestoreDC(item->hDC, saved_dc);
            return TRUE;
        }
        if (item->CtlID == ID_GITHUB) {
            FillRect(item->hDC, &item->rcItem, background);
            int size = px(28);
            int x = item->rcItem.left + (item->rcItem.right - item->rcItem.left - size) / 2;
            int y = item->rcItem.top + (item->rcItem.bottom - item->rcItem.top - size) / 2;
            DrawIconEx(item->hDC, x, y, github_icon, size, size, 0, NULL, DI_NORMAL);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
        SetTextColor((HDC)wp, RGB(41, 55, 74)); SetBkColor((HDC)wp, RGB(246, 248, 251)); return (LRESULT)background;
    case WM_ERASEBKGND: { RECT r; GetClientRect(hwnd, &r); FillRect((HDC)wp, &r, background); return 1; }
    case WM_CLOSE:
        if (exit_after_save) return 0;
        if (save_inflight || save_pending) { exit_after_save = TRUE; return 0; }
        if (!settings_dirty()) { DestroyWindow(hwnd); return 0; }
        switch (ask_save_before_exit()) {
        case ID_SAVE_EXIT: save_current(TRUE); break;
        default: break;
        }
        return 0;
    case WM_DESTROY: {
        KillTimer(hwnd, 1);
        if (child) { CloseHandle(child); child = NULL; }
        DeleteObject(font); DeleteObject(title_font); DeleteObject(small_font); DeleteObject(background);
        PostQuitMessage(0); return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR cmd, int show) {
    (void)previous; (void)cmd;
    init_language();
    if (!app_init(&app)) { MessageBoxW(NULL, UI(L"无法创建可写的数据目录，请检查目录权限或磁盘空间。", L"Cannot create a writable data folder. Check folder permissions and free disk space."), L"Bing Wallpaper", MB_ICONERROR); return 2; }
    wchar_t name[320]; swprintf(name, 320, L"%ls-GUI", app.mutex);
    HANDLE mutex = CreateMutexW(NULL, TRUE, name);
    if (!mutex) return 2;
    if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(mutex); return 0; }
    INITCOMMONCONTROLSEX cc = {sizeof(cc), ICC_DATE_CLASSES | ICC_STANDARD_CLASSES}; InitCommonControlsEx(&cc);
    WNDCLASSW wc = {0}; wc.lpfnWndProc = procedure; wc.hInstance = instance;
    wc.lpszClassName = L"BingWallpaperLiteSettings"; wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    RegisterClassW(&wc);
    HDC dc = GetDC(NULL); dpi = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(NULL, dc);
    RECT rect = {0, 0, px(CLIENT_WIDTH), px(CLIENT_HEIGHT)};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRectEx(&rect, style, FALSE, 0);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, UI(L"Bing Wallpaper · 设置", L"Bing Wallpaper · Settings"), style,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, instance, NULL);
    if (!hwnd) { CloseHandle(mutex); return 2; }
    ShowWindow(hwnd, show); UpdateWindow(hwnd);
    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    ReleaseMutex(mutex); CloseHandle(mutex); return 0;
}
