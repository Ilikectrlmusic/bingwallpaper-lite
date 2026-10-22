<p align="center">
  <img src="assets/app.png" width="96" height="96" alt="Bing Wallpaper icon">
</p>

<h1 align="center">Bing Wallpaper</h1>

<p align="center">English · <a href="README.zh-CN.md">简体中文</a></p>

A minimal Bing wallpaper app for **Windows 10/11 x64**, written in native **C / Win32**. No tray icon or resident background process: each run exits completely, and Windows Task Scheduler handles retries after failures.

## Download

**[Release 1.0.0](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/tag/v1.0.0)**

| Package | Download |
| --- | --- |
| Installer | [BingWallpaperLite-Setup-1.0.0-x64.exe](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Setup-1.0.0-x64.exe) |
| Portable | [BingWallpaperLite-Portable-1.0.0-x64.zip](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Portable-1.0.0-x64.zip) |

No separate Python, .NET, or WebView runtime is required. The portable uninstall scripts use Windows PowerShell included with Windows.

## Getting started

- **Installer:** Run the installer and choose a directory. The default is `C:\Program Files\BingWallpaperLite`. The settings window opens automatically after installation.
- **Portable:** Extract the ZIP and run **`BingWallpaper.exe`**.
- On first launch, the settings app asks the worker to create the daily task and immediately attempt a wallpaper update.

## Settings

| Setting | Default | Options |
| --- | --- | --- |
| Daily start time | 09:00, local system time | Choose a time |
| Image cache retention | 3 days | 1–365 days |
| Log retention | 3 days | 1–365 days |
| Download resolution | 1080P | 1080P or 4K |

- **Save settings** — Save changes and update the scheduled task.
- **Check / update** — Request an update immediately, still respecting the limit of one successful wallpaper change per day.
- **Open data folder** — View settings, cache, and log files.

Resolution changes apply to the next download. If the wallpaper has already been changed successfully that day, changing the resolution will not trigger another download or update; the new setting takes effect on the next day's download.

## Scheduling and retries

The daily task launches **`BingWallpaperWorker.exe`**; **`BingWallpaper.exe`** is only the settings GUI. After one successful wallpaper change per day, subsequent worker runs exit immediately.

On failure, the worker exits and Windows Task Scheduler retries at **30-minute intervals, up to 48 times**. No worker process remains running while waiting to retry. A check also runs 20 seconds after sign-in. Before the configured time, no download occurs unless a retry or manual request is pending. The app does not wake the computer.

## Data and cleanup

- If the application directory is writable without administrator permission: **`<application directory>\data\`**.
- Otherwise: **`%APPDATA%\BingWallpaperLite\<installation-path ID>\`**.

A small entry in `HKCU\Software\BingWallpaperLite` records the data directory. Therefore, the portable version also creates a scheduled task and a registry location entry.

Cache and logs are cleaned up by local calendar date. **The wallpaper currently in use is protected** and may remain longer than the configured cache retention period.

## Uninstalling or moving

**Installed version:** Windows Settings > Apps > Installed apps > Bing Wallpaper > Uninstall. The worker first removes this copy's task, current-user data, and location entry; Inno Setup then removes the program files.

**Portable version:** Close the settings window, then double-click **`Uninstall-Portable.cmd`**. The ZIP includes both launcher scripts. Choose one of the following actions:

- **Uninstall:** Clean up the task, user data, and location entry, then delete the entire dedicated portable directory, including the uninstall scripts.
- **Remove scheduled task only:** Stop and remove the task for the current directory, while keeping the program, settings, cache, logs, and location entry. Afterward, you can move the entire directory and run `BingWallpaper.exe` from its new location to recreate the task.
- **Cancel**

Keep the shared `uninstall-portable.ps1` helper in the same directory.

For a writable portable directory, `data` moves with the directory. If the data was stored in APPDATA, **Remove scheduled task only** preserves that external data; moving the program does not migrate it automatically. To keep the data, back it up using **Open data folder** first, then copy it to the corresponding data directory at the new location. The old location entry is also retained when removing only the task.

## Source layout

- `src/` — Settings GUI, browser launching, worker, data storage, scheduling, downloading, and wallpaper updates.
- `assets/` — App and GitHub icons, Windows manifest, and resources.
- `packaging/` — Portable uninstall scripts.
