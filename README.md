<p align="center">
  <img src="https://raw.githubusercontent.com/Ilikectrlmusic/bingwallpaper-lite/main/windows/assets/app.png" width="96" height="96" alt="Bing Wallpaper icon">
</p>

<h1 align="center">Bing Wallpaper</h1>
<p align="center">English · <a href="README.zh-CN.md">简体中文</a></p>

A lightweight Bing wallpaper app. No tray icon or resident background process: the worker exits after each attempt, and the operating system handles scheduled runs.

## Platforms

| Platform | Status | Source |
| --- | --- | --- |
| Windows 10/11 x64 | Available; native C / Win32 | [`windows/`](windows/) |
| macOS | Planned; no implementation or downloadable package yet | [`macos/`](macos/) |

The platform-independent documentation stays at the repository root.

## Windows downloads

**[Release 1.0.0](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/tag/v1.0.0)**

| Package | Download |
| --- | --- |
| Installer | [BingWallpaperLite-Setup-1.0.0-x64.exe](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Setup-1.0.0-x64.exe) |
| Portable | [BingWallpaperLite-Portable-1.0.0-x64.zip](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Portable-1.0.0-x64.zip) |

The settings below describe the current source. Published 1.0.0 binaries may differ from the development source.

No separate Python, .NET, or WebView runtime is required. Portable uninstall scripts use Windows PowerShell included with Windows.

## Windows: getting started

- **Installer:** Run the installer and choose a directory. The default is `C:\Program Files\BingWallpaperLite`. The settings window opens after installation.
- **Portable:** Extract the ZIP into its own directory and run **`BingWallpaper.exe`**.
- On first launch with daily updates enabled, the GUI asks the worker to create the task and immediately attempt a wallpaper update.
- The GUI supports Simplified Chinese and English, selected from the Windows display language. Other languages use English. The installer uses the same language choices; its uninstaller retains the installation language.

## Windows: settings

| Setting | Default | Options |
| --- | --- | --- |
| Daily wallpaper updates | On | On / off |
| Daily start time | 09:00, local system time | Choose a time |
| Download resolution | 1080P | 1080P or 4K |
| Cache and log retention | 3 days | 1–365 days, shared by cached images and logs |

- **Save settings** — Apply changes. Turning daily updates off stops and removes this copy's scheduled task; turning them on creates it again. Program files, settings, images, and logs remain when updates are off.
- **Check / update** — Request a manual update, including when daily updates are off. The limit of one successful wallpaper change per local calendar day still applies. With daily updates off, a manual attempt does not create a scheduled task or automatic retries.
- **Open data folder** — View settings, cache, and logs.
- Closing with unsaved changes offers **Save and exit** or **Cancel**.

Resolution changes apply to the next download, not another successful update on the same day. Existing separate cache/log retention settings use the longer valid period as the shared value until you save a new value.

## Windows: scheduling and retries

The daily task launches **`BingWallpaperWorker.exe`**. **`BingWallpaper.exe`** is the settings GUI, not a daily background process. A successful update is recorded once per day; later attempts that day exit without another wallpaper change.

While daily updates are enabled, failures are retried by Windows Task Scheduler every **30 minutes, up to 48 times**. There is no worker process waiting between attempts. A check also runs 20 seconds after sign-in. Before the configured time, no download occurs unless a retry or manual request is pending. The app does not wake the computer.

## Windows: data and cleanup

- Writable application directory without administrator permission: **`<application directory>\data\`**.
- Otherwise: **`%APPDATA%\BingWallpaperLite\<installation-path ID>\`**.

A small `HKCU\Software\BingWallpaperLite` location entry records the data directory. Cache and logs use the same retention period and are cleaned up by local calendar date. The wallpaper currently in use is protected and may remain longer. Expired-file cleanup and uninstall deletion are permanent, not sent to the Recycle Bin.

## Windows: uninstalling or moving

**Installed version:** Windows Settings > Apps > Installed apps > Bing Wallpaper > Uninstall. The worker removes this copy's task, current-user data, and location entry before Inno Setup removes the program files.

**Portable version:** Close the GUI and run **`Uninstall-Portable.cmd`** (English) or **`卸载便携版.cmd`** (Simplified Chinese). Each script directly uninstalls the whole application, including the task, user data, location entry, and dedicated portable directory. There is no task-only/full-uninstall chooser. A result popup reports success or errors. Keep `uninstall-portable.ps1` beside the launchers, and do not store unrelated files in the portable directory.

**Moving a portable copy:** Turn off **Daily wallpaper updates**, save, and close the GUI. Move the entire directory, then run `BingWallpaper.exe` at the new location. The saved off setting is retained; enable it and save when you want scheduled updates again. For a writable directory, `data` moves with the program. APPDATA data does not move automatically: back it up via **Open data folder**, then copy it into the corresponding data folder at the new location. The old location entry remains until that old copy is fully uninstalled.

## Source layout

- `windows/src/` — Win32 GUI, system-default browser links, worker, storage, scheduling, downloads, and wallpaper updates.
- `windows/assets/` — App/GitHub icons, manifest, and Windows resources.
- `windows/packaging/` — English and Simplified Chinese portable uninstall launchers and shared helper.
- `macos/` — Reserved for the macOS implementation.

Local tests, build tools, installer sources, and generated binaries are excluded from Git. A Windows-only checkout can use Git sparse checkout to include the root documentation and `windows/`, without checking out `macos/`.
