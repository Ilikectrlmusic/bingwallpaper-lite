<p align="center">
  <img src="https://raw.githubusercontent.com/Ilikectrlmusic/bingwallpaper-lite/main/windows/assets/app.png" width="96" height="96" alt="Bing Wallpaper 软件图标">
</p>

<h1 align="center">Bing Wallpaper</h1>
<p align="center"><a href="README.md">English</a> · 简体中文</p>

轻量级 Bing 壁纸工具。无托盘、无常驻后台：工作程序每次执行后退出，由操作系统负责定时启动。

## 平台

| 平台 | 状态 | 源码 |
| --- | --- | --- |
| Windows 10/11 x64 | 可用，原生 C / Win32 | [`windows/`](windows/) |
| macOS 12+（苹果自研芯片 / Intel） | 原生 Swift / AppKit，Ad-hoc 签名 DMG | [`macos/`](macos/) |

通用说明文档保留在仓库根目录。

## Windows 下载

**[1.0.0 版本发布页](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/tag/v1.0.0)**

| 版本 | 下载 |
| --- | --- |
| 安装版 | [BingWallpaperLite-Setup-1.0.0-x64.exe](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Setup-1.0.0-x64.exe) |
| 便携版 | [BingWallpaperLite-Portable-1.0.0-x64.zip](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Portable-1.0.0-x64.zip) |

无需额外安装 Python、.NET 或 WebView 运行时。便携版卸载脚本使用 Windows 自带的 Windows PowerShell。

## Windows 使用

- **安装版**：运行安装程序并选择目录，默认 `C:\Program Files\BingWallpaperLite`；完成后打开设置窗口。
- **便携版**：把 ZIP 解压到独立目录，运行 **`BingWallpaper.exe`**。
- 首次启动且每日更新开启时，GUI 调用工作程序建立每日任务，并立即尝试更换一次壁纸。
- GUI 根据 Windows 显示语言选择简体中文或英文，其他语言默认英文。安装器采用相同语言选择，卸载器沿用安装时的语言。

## Windows 设置

| 设置项 | 默认值 | 范围 |
| --- | --- | --- |
| 每日壁纸更新 | 开启 | 开启 / 关闭 |
| 每日启动时间 | 系统本地时间 09:00 | 自行选择时间 |
| 下载壁纸分辨率 | 4K | 1080P 或 4K |
| 缓存与日志保留天数 | 3 天 | 1–365 天，图片缓存和日志共用 |

- **保存设置**：应用更改。关闭每日更新会停止并移除本副本的定时任务，重新开启并保存则重建任务。关闭时保留程序、设置、图片和日志。
- **立即检查 / 更换**：手动请求更新，关闭每日更新后仍可使用，仍遵守每个本地日历日只成功更换一次的限制。每日更新关闭时，手动执行不会创建任务或安排自动重试。
- **打开数据目录**：查看设置、缓存与日志。
- 有未保存更改时关闭窗口，可选择**保存并退出**或**取消**。

Windows 新配置默认使用 4K，已有保存的分辨率选择会保留。分辨率设置在下一次下载时生效，不会触发当天第二次成功更新。已有配置中独立的缓存、日志保留天数会取较长的有效值作为共用天数，直至保存新的数值。

## Windows 运行与重试

每日任务启动 **`BingWallpaperWorker.exe`**；**`BingWallpaper.exe`** 只是设置 GUI，不每日常驻启动。成功更新日期会被记录，当天后续执行不会再次更换壁纸。

每日更新开启时，失败后的重试由 Windows 任务计划按 **30 分钟间隔、最多 48 次**执行，等待期间没有工作进程驻留。登录后 20 秒会补检；未到指定时间且没有待重试或手动请求时，不提前下载。程序不主动唤醒电脑。

## Windows 数据与清理

- 普通权限可写入程序目录：**`程序目录\data\`**。
- 否则：**`%APPDATA%\BingWallpaperLite\<安装路径标识>\`**。

`HKCU\Software\BingWallpaperLite` 中的小型定位项记录数据目录。缓存与日志共用保留天数，按本地日历日期清理。**当前正在使用的壁纸受到保护**，可能超过设定天数。过期清理和卸载均永久删除文件，不经过回收站。

## Windows 卸载与移动

**安装版**：Windows 设置 → 应用 → 已安装的应用 → Bing Wallpaper → 卸载。工作程序先清理本副本的任务、当前用户数据和定位项，再由 Inno Setup 移除程序文件。

**便携版**：关闭 GUI 后，双击 **`卸载便携版.cmd`**（简体中文）或 **`Uninstall-Portable.cmd`**（英文）。脚本直接卸载整个软件，包括任务、用户数据、定位项和独立便携目录，不提供“仅移除任务 / 完整卸载”选择框；执行结束后弹窗报告成功或错误。请保留同目录的 `uninstall-portable.ps1`，不要在便携目录里存放无关文件。

**移动便携目录**：先关闭 GUI 中的**每日壁纸更新**，保存并退出，再移动整个目录。在新位置运行 `BingWallpaper.exe` 后，已保存的关闭状态仍然保留；需要恢复每日更新时，再开启并保存。可写目录中的 `data` 随程序移动。APPDATA 数据不会自动迁移，请先通过“打开数据目录”备份，再复制到新位置对应的数据目录；旧位置的定位项会保留，直到完整卸载旧副本。

## macOS 安装与使用

版本 **1.0.0**，支持 **macOS 12 Monterey 及以上**，分别提供两种架构：

| 芯片 | 下载 |
| --- | --- |
| 苹果自研芯片（M 系列） | [BingWallpaperLite-1.0.0-arm64.dmg](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-1.0.0-arm64.dmg) |
| Intel | [BingWallpaperLite-1.0.0-x86_64.dmg](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-1.0.0-x86_64.dmg) |

DMG 仅包含应用和 Applications 快捷方式。SHA-256 校验值收录在 [v1.0.0 Release](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/tag/v1.0.0) 现有的校验值 TXT 文件中。将 **Bing Wallpaper.app** 拖到**应用程序**，推出 DMG，再打开已安装的应用。不要直接从 DMG 运行。移动安装位置前先关闭每日更新并保存，移到新位置后重新打开并启用。

应用和 DMG 使用 **Ad-hoc 签名**，未使用 Developer ID 签名，也未经过苹果公证。Gatekeeper 可能阻止首次启动。尝试打开后，可在**系统设置 → 隐私与安全性 → 仍要打开**中放行（旧版 macOS 在“系统偏好设置 → 安全性与隐私”中操作）。Ad-hoc 签名用于验证本地代码完整性，不代表开发者身份认证。

点击应用图标启动的是 **GUI：`Contents/MacOS/BingWallpaper`**；实际下载、更换壁纸由独立的 **`Contents/MacOS/BingWallpaperWorker`** 执行。无菜单栏图标、无工作进程常驻，也不需要额外运行时。关闭 GUI 即退出界面，launchd 独立负责后续调度。

原生设置窗口默认开启每日更新、系统本地时间 09:00、4K（可选 1080P）、缓存与日志共用 3 天保留期（1–365 天）。4K 默认值适用于新配置，已有保存的分辨率选择会保留。**保存设置**应用定时任务；**立即检查 / 更换**使用已保存设置，仍遵守每个本地日历日只成功一次。关闭每日更新后仍可手动更新，但不安排自动重试。紧凑设置窗口使用原生分组控件，GitHub 图标用默认浏览器打开本项目。上次成功运行只显示日期，更新状态在同一行右侧显示。退出前有未保存更改会提示保存。系统首选语言为简体中文时显示简体中文，其他语言默认英文。

首次启动建立调度并立即尝试更新。每日 LaunchAgent 只启动工作程序，按指定时间运行，并在登录时补检；未到时间不会提前下载。失败后创建 **30 分钟后的单次日历重试**，向上取整到下一整分钟；再次失败继续安排下一次。当天成功或当天结束后停止重试，次日从配置时间开始。两个重试任务交替使用，避免工作程序更新调度时终止自身。过期任务的 launchd 元数据可能保留到下次每日任务，但**没有进程等待或驻留**，也不做周期轮询，不使用 `KeepAlive`。

软件不会主动唤醒电脑。睡眠期间错过的日历任务由 launchd 在唤醒后合并执行；登录时检查当天任务是否已到时间。自动任务需要用户已登录图形会话，并在 macOS“登录项”中允许本应用的后台项目。壁纸应用于已连接显示器上当前可用的桌面，不修改锁屏，也不保证修改所有未激活空间；空间行为由 macOS 控制。

用户数据位于 **`~/Library/Application Support/BingWallpaperLite/`**，包含 `settings.json`、`state.json`、`cache/` 和 `logs/`。设置与状态原子写入，进程锁防止重叠执行。Bing 元数据使用与 Windows 相同的 `zh-CN` 市场，与界面语言无关。下载限定 HTTPS、大小和超时，并完整解码、校验请求分辨率。缓存和日志按本地日期清理，保护已知正在使用的壁纸；过期文件永久删除。

卸载时，先关闭**每日壁纸更新**并保存，退出后将应用移到废纸篓。此操作移除每日和重试 LaunchAgent，保留设置及图片；之后可自行删除数据目录，但这也会删除可能仍在使用的缓存壁纸。LaunchAgent 位于 `~/Library/LaunchAgents/`，文件名前缀为 `com.ilikectrlmusic.bingwallpaper-lite.`。每个用户支持一个安装副本。

## macOS 构建与测试

需要 Xcode / Command Line Tools、macOS SDK 和 Swift 编译器；没有第三方包依赖。在仓库根目录执行：

```sh
./macos/scripts/test.sh          # 隔离测试：行为、存储、图片校验与调度
./macos/scripts/test.sh --live   # 额外真实下载并解码 Bing 1080P 和 4K 图片
./macos/scripts/build.sh         # 构建、Ad-hoc 签名、生成并验证两种 DMG
# 也可仅构建一种架构：
./macos/scripts/build.sh arm64
./macos/scripts/build.sh x86_64
```

DMG 和 SHA-256 校验值位于 `macos/dist/`，应用位于 `macos/build/<架构>/`。GUI 与工作程序均以 macOS 12.0 为最低目标，可在任一架构 Mac 上交叉编译两种架构。Intel 运行验证需要 Intel Mac 或 Rosetta；编译通过不等于已验证最低系统版本。

运行 `Bing Wallpaper.app/Contents/MacOS/BingWallpaperWorker --diagnose` 可只读输出路径及每日任务定义，不写数据、不安装任务、不下载或更换壁纸。GUI 的 `--preview` 参数用于检查界面，跳过首次启动的调度和下载。

## 源码结构

- `windows/src/`：Win32 GUI、系统默认浏览器跳转、工作程序、存储、任务计划、下载和壁纸更新。
- `windows/assets/`：软件与 GitHub 图标、manifest 和 Windows 资源。
- `windows/packaging/`：中英文便携版卸载入口及共用辅助脚本。
- `macos/Sources/`：原生 AppKit GUI、独立工作程序、launchd 调度与存储。
- `macos/Resources/`：英文 / 简体中文文案和应用资源。
- `macos/scripts/`：双架构构建与行为测试。

Windows 本地测试、构建工具、安装器源码及所有生成的程序包不纳入 Git；macOS 构建脚本和测试纳入 Git。Windows 本地可用 Git 稀疏检出保留根目录文档与 `windows/`，不检出 `macos/`。
