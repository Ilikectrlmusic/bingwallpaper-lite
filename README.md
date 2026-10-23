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
| macOS 12+ (Apple silicon / Intel) | Native Swift / AppKit; Ad-hoc DMG builds | [`macos/`](macos/) |

The platform-independent documentation stays at the repository root.

## Windows downloads

**[Release 1.0.0](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/tag/v1.0.0)**

| Package | Download |
| --- | --- |
| Installer | [BingWallpaperLite-Setup-1.0.0-x64.exe](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Setup-1.0.0-x64.exe) |
| Portable | [BingWallpaperLite-Portable-1.0.0-x64.zip](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Portable-1.0.0-x64.zip) |

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
| Download resolution | 4K | 1080P or 4K |
| Cache and log retention | 3 days | 1–365 days, shared by cached images and logs |

- **Save settings** — Apply changes. Turning daily updates off stops and removes this copy's scheduled task; turning them on creates it again. Program files, settings, images, and logs remain when updates are off.
- **Check / update** — Request a manual update, including when daily updates are off. The limit of one successful wallpaper change per local calendar day still applies. With daily updates off, a manual attempt does not create a scheduled task or automatic retries.
- **Open data folder** — View settings, cache, and logs.
- Closing with unsaved changes offers **Save and exit** or **Cancel**.

New Windows settings default to 4K; existing saved resolution choices are preserved. Resolution changes apply to the next download, not another successful update on the same day. Existing separate cache/log retention settings use the longer valid period as the shared value until you save a new value.

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

## macOS: install and use

Version **1.0.0** supports **macOS 12 Monterey or later**, with separate packages:

| Chip | Download |
| --- | --- |
| Apple silicon (M series) | [BingWallpaperLite-1.0.0-arm64.dmg](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-1.0.0-arm64.dmg) |
| Intel | [BingWallpaperLite-1.0.0-x86_64.dmg](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-1.0.0-x86_64.dmg) |

Each DMG contains only the app and an Applications shortcut. SHA-256 checksums are included in the existing checksum text file on the [v1.0.0 release page](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/tag/v1.0.0). Drag **Bing Wallpaper.app** into **Applications**, eject the DMG, then open the installed app. Do not run from the DMG or move an enabled installation; disable daily updates and save before moving, then reopen and enable them at the new location.

The app and DMGs use **Ad-hoc signatures**, without Developer ID signing or Apple notarization. Gatekeeper can block the first launch. After attempting to open the app, use **System Settings > Privacy & Security > Open Anyway** (System Preferences > Security & Privacy on older macOS). Ad-hoc signing verifies local code integrity; it does not establish a verified developer identity.

The app icon launches the **GUI**, `Contents/MacOS/BingWallpaper`. It delegates wallpaper work to **`Contents/MacOS/BingWallpaperWorker`**. There is no menu bar item, resident worker, or third-party runtime. Closing the GUI quits it; macOS launchd continues scheduling independently.

The native settings window defaults to daily updates enabled, 09:00 local time, 4K (or 1080P), and 3 days of shared cache/log retention (1–365). The 4K default applies to new settings; existing saved resolution choices are preserved. **Save settings** applies the schedule; **Check / update** uses saved settings and still allows only one success per local calendar day. Manual updates also work with daily updates disabled, without automatic retries. The compact settings window groups native controls, and its GitHub icon opens this repository in the default browser. The last successful run shows only the date, with the update status on the right. Unsaved edits prompt to save before quitting. The UI uses Simplified Chinese when the primary system language is Simplified Chinese; all other languages use English.

On first launch, the GUI installs the schedule and requests an immediate update. The daily LaunchAgent runs only the worker, at the chosen local time and at sign-in (checking the time before downloading). A failed attempt schedules a **single calendar retry 30 minutes later**, rounded up to the next whole minute. Subsequent failures schedule the next retry. Retries stop after success or at the end of the local day. The following day starts at the configured time. Retry jobs alternate between two labels so a running worker never kills itself while updating its schedule. An expired, loaded job may remain as launchd metadata until the next daily run; **no worker process remains running**. No periodic polling or `KeepAlive` is used.

Sleeping computers are not actively woken. launchd coalesces missed calendar events after wake; at sign-in it checks whether today's update is due. Daily updates require a logged-in graphical session and permission for the app's background item in macOS Login Items settings. Wallpaper changes apply to the current desktops of connected displays, not the Lock Screen or every inactive Space. macOS controls desktop/Space behavior.

Data lives in **`~/Library/Application Support/BingWallpaperLite/`**: `settings.json`, `state.json`, `cache/`, and `logs/`. Settings/state writes are atomic; a process lock prevents overlapping workers. Bing metadata uses the same `zh-CN` market as Windows, independent of UI language. Downloads use HTTPS, bounded sizes/timeouts, and fully decoded images of the requested dimensions. Cache/log cleanup uses local calendar dates and preserves known active wallpapers. Expired files are permanently deleted.

To uninstall, disable **Daily wallpaper updates**, save, quit, then move the app to Trash. This removes its daily/retry LaunchAgents while preserving downloaded wallpaper and settings. Optionally delete the data folder afterward; this also removes cached images that may still be used as wallpaper. LaunchAgent filenames begin with `com.ilikectrlmusic.bingwallpaper-lite.` in `~/Library/LaunchAgents/`. One installed copy per user is supported.

## macOS: build and test

Requires Xcode / Command Line Tools with a macOS SDK and Swift compiler; no package dependencies. From the repository root:

```sh
./macos/scripts/test.sh          # isolated behavior, storage, image and schedule checks
./macos/scripts/test.sh --live   # also fetch and decode actual Bing 1080P / 4K images
./macos/scripts/build.sh         # compile, Ad-hoc sign, create and verify both DMGs
# Or build one architecture:
./macos/scripts/build.sh arm64
./macos/scripts/build.sh x86_64
```

Outputs and SHA-256 checksums are in `macos/dist/`; app bundles are in `macos/build/<architecture>/`. GUI and worker target macOS 12.0. The scripts cross-compile both architectures on either Mac architecture. Intel runtime testing requires an Intel Mac or Rosetta; compiling does not substitute for testing on the minimum supported OS.

`Bing Wallpaper.app/Contents/MacOS/BingWallpaperWorker --diagnose` prints paths and the proposed daily LaunchAgent without writing data, installing jobs, downloading, or changing wallpaper. The GUI's `--preview` argument opens its window without first-launch scheduling or downloads; use it only to inspect the UI.

## Source layout

- `windows/src/` — Win32 GUI, system-default browser links, worker, storage, scheduling, downloads, and wallpaper updates.
- `windows/assets/` — App/GitHub icons, manifest, and Windows resources.
- `windows/packaging/` — English and Simplified Chinese portable uninstall launchers and shared helper.
- `macos/Sources/` — Native AppKit GUI, standalone worker, launchd scheduling, and storage.
- `macos/Resources/` — English / Simplified Chinese strings and app resources.
- `macos/scripts/` — Reproducible dual-architecture build and behavioral tests.

Windows local tests/build tools/installer sources and all generated binaries are excluded from Git. macOS build scripts and tests are included. A Windows-only checkout can use Git sparse checkout to include the root documentation and `windows/`, without checking out `macos/`.
