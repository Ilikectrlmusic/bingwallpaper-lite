<p align="center">
  <img src="assets/app.png" width="96" height="96" alt="Bing Wallpaper icon">
</p>

<h1 align="center">Bing Wallpaper</h1>

<p align="center">English · <a href="README.zh-CN.md">简体中文</a></p>

A minimal Bing wallpaper app for **Windows 10/11 x64**, written in native **C / Win32**. No tray icon or resident background process: the worker exits after every attempt, and Windows Task Scheduler handles later retries.

> An unofficial project. The settings interface automatically follows the current user's Windows display language at startup: Simplified Chinese display languages use Simplified Chinese; English and all other languages use English. The portable ZIP includes separate English and Simplified Chinese uninstall launchers.

## Download

**[Release 1.0.0](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/tag/v1.0.0)**

| Package | Download |
| --- | --- |
| Installer | [BingWallpaperLite-Setup-1.0.0-x64.exe](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Setup-1.0.0-x64.exe) |
| Portable ZIP | [BingWallpaperLite-Portable-1.0.0-x64.zip](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Portable-1.0.0-x64.zip) |

No separate Python, .NET, or WebView runtime is required. Portable uninstall uses the Windows PowerShell included with Windows.

## Getting started

- **Installer:** Run the installer and choose a directory. It requests administrator permission and defaults to `C:\Program Files\BingWallpaperLite`. The settings window opens after installation; the app itself runs with ordinary user permissions.
- **Portable:** Extract the entire `BingWallpaperLite-Portable` folder into a **dedicated directory**, then run **`BingWallpaper.exe`**. Keep the worker and uninstall scripts alongside it.
- On first launch, the settings app asks the worker to create the daily task and immediately attempt a wallpaper update. You can then close the settings window. It does not need to stay open or launch every day.

## Settings

| Setting | Default | Options |
| --- | --- | --- |
| Daily start time | 09:00, local system time | Choose a time of day |
| Image cache retention | 3 days | 1–365 days |
| Log retention | 3 days | 1–365 days |
| Download resolution | 1080P | 1080P (1920×1080) or 4K (3840×2160) |

- **Save settings / 保存设置** — Save settings.
- **Check / update / 立即检查 / 更换** — Request an update now, still respecting the once-per-day success limit.
- **Open data folder / 打开数据目录** — Open the data directory.
- The GitHub icon opens [this repository](https://github.com/Ilikectrlmusic/bingwallpaper-lite) in the default browser.

With unsaved changes, closing the window offers **Save and exit / 保存并退出** or **Cancel / 取消** in the interface language. Saving closes the window automatically only after the settings and scheduled task are updated successfully.

Resolution changes apply to the next download. If today's wallpaper was already applied successfully, changing resolution does not trigger a second update that day.

## Scheduling and retries

The daily task launches **`BingWallpaperWorker.exe`**, not the settings app. It uses the current signed-in user's identity without storing a password. After one successful update per local calendar day, later attempts exit without downloading or changing the wallpaper again.

On failure, the worker exits with an error and Task Scheduler retries at **30-minute intervals, up to 48 retries**. No worker process remains running between attempts. A check 20 seconds after sign-in helps catch up; before the configured time, it exits unless a retry or manual request is pending. The app does not wake the computer.

Failed downloads preserve the existing wallpaper. Downloaded images are fully decoded and their dimensions checked; lower-resolution images are not silently accepted as 4K. Updates depend on Bing's image endpoint and your network connection.

## Data and cleanup

- Writable without administrator permission: **`<application directory>\data\`**.
- Otherwise: **`%APPDATA%\BingWallpaperLite\<installation-path ID>\`**.

A small entry under `HKCU\Software\BingWallpaperLite` remembers the data location. The portable version therefore creates a task and a registry entry; it is not a zero-trace application.

Retention follows local calendar dates. The wallpaper currently in use is protected from cache cleanup and may remain longer than the selected retention period. Runtime cleanup and portable removal use the Recycle Bin and report failures.

## Uninstalling or moving

**Installed version:** Windows Settings → Apps → Installed apps → Bing Wallpaper → Uninstall. The worker removes this installation's task, current-user data, and location entry before Inno Setup removes the program files.

**Portable version:** Close the settings window, then double-click **`Uninstall-Portable.cmd`** for English dialogs or **`卸载便携版.cmd`** for Chinese dialogs. Both scripts are included in the ZIP and offer the same actions:

- **Uninstall / 卸载:** Remove the task, user data, and location entry, then move the entire portable directory, including the scripts, to the Recycle Bin.
- **Remove scheduled task only / 仅卸载定时任务:** Stop and remove the task for the current directory and user. Keep the program, settings, cache, logs, and location entry. Move the whole folder, then run `BingWallpaper.exe` at the new location to register a new task.
- **Cancel / 取消:** Make no changes.

Both removal modes display success or error details in the launcher's language. Keep the shared `uninstall-portable.ps1` helper alongside both CMD files. Use the **same Windows account** that originally configured the app. Uninstalling may remove the wallpaper file but does not choose another system wallpaper; select a replacement first if needed.

For a writable portable directory, `data` moves with the folder. If data was stored in APPDATA, task-only removal preserves it there; moving the program **does not automatically migrate it**. Use **Open data folder / 打开数据目录** to back it up and copy it into the new installation's data directory if needed. Task-only removal also retains the old location entry.

## Source layout

- `src/` — Settings GUI, browser launching, worker, storage, scheduling, downloading, and wallpaper updates.
- `assets/` — Icons, Windows manifest, and resources.
- `packaging/` — Portable uninstall helpers.

Local development notes, tests, build helpers, and generated packages are intentionally excluded from this repository. Ready-to-run packages are available from [Releases](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases).
