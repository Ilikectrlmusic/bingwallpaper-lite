<p align="center"><img src="https://raw.githubusercontent.com/Ilikectrlmusic/bingwallpaper-lite/main/windows/assets/app.png" width="96" alt="Bing Wallpaper"></p>

<h1 align="center">Bing Wallpaper Lite</h1>
<p align="center">English · <a href="README.zh-CN.md">简体中文</a></p>

A lightweight alternative to Microsoft's Bing Wallpaper. Updates your desktop wallpaper once a day and exits—no tray icon, no always-running background process.

- Native Windows (C/Win32) and macOS (Swift/AppKit) apps
- 1080p or 4K wallpapers
- Custom update time and cache/log retention
- Automatic retries on failure
- English and Simplified Chinese UI

## Download

[**Latest release**](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/latest)

| Platform | Package |
| --- | --- |
| Windows 10/11 (x64) | Installer (`.exe`) or portable (`.zip`) |
| macOS 12+ (Apple silicon) | `arm64.dmg` |
| macOS 12+ (Intel) | `x86_64.dmg` |

## Getting started

**Windows:** Run the installer, or extract the portable ZIP and open `BingWallpaper.exe`.

**macOS:** Open the DMG, drag **Bing Wallpaper.app** to Applications, then launch it from there.

On first launch, the app sets up daily updates and attempts to download today's wallpaper. You can change the schedule, resolution and retention period in Settings. Defaults: **9:00 AM**, **4K**, **3 days** of cache/log retention.

Only one successful wallpaper change is made per day, including manual checks. The operating system runs the worker on schedule; it exits after finishing.

**macOS note:** Builds are ad-hoc signed, not Apple-notarized. If macOS blocks the app, try opening it, then go to **System Settings → Privacy & Security → Open Anyway**.

## Uninstall

- **Windows installer:** Uninstall from Windows Settings → Apps.
- **Windows portable:** Run `Uninstall-Portable.cmd` or `卸载便携版.cmd`. This also removes the portable folder and app data; keep unrelated files elsewhere.
- **macOS:** Turn off daily updates and save before moving the app to Trash. Optional data folder: `~/Library/Application Support/BingWallpaperLite/`.

## Source

- [Windows](windows/)
- [macOS](macos/)

SHA-256 checksums are available with each release.
