<p align="center"><img src="https://raw.githubusercontent.com/Ilikectrlmusic/bingwallpaper-lite/main/windows/assets/app.png" width="96" alt="Bing Wallpaper"></p>

<h1 align="center">Bing Wallpaper Lite</h1>
<p align="center">English · <a href="README.zh-CN.md">简体中文</a></p>

## Why Lite?

**Ultra-lightweight by design.** A system-scheduled task updates your wallpaper and then exits. No tray icon, no always-running background process.

| Comparison | Bing Wallpaper Lite v1.0.0 | Bing Wallpaper v1.1.460.0 |
| --- | --- | --- |
| macOS support | ✅ | ❌ |
| Windows package size | **2.2 MB (installer) / 127 KB (ZIP)** | **218.5 MB** |
| Always running in background | **No — exits after updating** | Yes — stays in the system tray |
| Retry on failure | ✅ | ✅ |

## Features

- Native Windows app (C / Win32) and macOS app (Swift / AppKit)
- 1080p / 4K wallpapers
- Custom daily update time and cache/log retention
- Automatic retries on download failure
- Simplified Chinese / English UI

## Download

[**Download the latest release**](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/latest)

| Platform | Package |
| --- | --- |
| Windows 10/11 (x64) | Installer (`.exe`) or portable (`.zip`) |
| macOS 12+ (Apple silicon) | `arm64.dmg` |
| macOS 12+ (Intel) | `x86_64.dmg` |

## Usage

**Windows:** Run the installer, or extract the portable ZIP and open `BingWallpaper.exe`.

**macOS:** Open the DMG and drag **Bing Wallpaper.app** into the Applications folder. The app is ad-hoc signed. If macOS blocks it on first launch, go to **System Settings → Privacy & Security → Open Anyway**.

On first launch, the app sets up a daily task and attempts to update today's wallpaper. You can adjust the time, resolution, and retention settings. Defaults: **9:00 AM, 4K, and 3 days of cache/log retention**.

The wallpaper is changed successfully at most once a day, including manual updates. The operating system handles scheduling, and the worker exits when finished.

## Uninstall

- **Windows installer:** Uninstall via Windows Settings → Apps.
- **Windows portable:** Run `Uninstall-Portable.cmd` to remove the portable directory and app data.
- **macOS:** Disable daily updates and save the setting before moving the app to Trash. To remove user data as well, manually delete `~/Library/Application Support/BingWallpaperLite/`.

---

SHA-256 checksums for each build are available on the Releases page.
