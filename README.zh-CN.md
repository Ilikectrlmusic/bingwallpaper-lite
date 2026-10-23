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
| macOS | 计划开发，暂无实现和下载包 | [`macos/`](macos/) |

通用说明文档保留在仓库根目录。

## Windows 下载

**[1.0.0 版本发布页](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/tag/v1.0.0)**

| 版本 | 下载 |
| --- | --- |
| 安装版 | [BingWallpaperLite-Setup-1.0.0-x64.exe](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Setup-1.0.0-x64.exe) |
| 便携版 | [BingWallpaperLite-Portable-1.0.0-x64.zip](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Portable-1.0.0-x64.zip) |

以下设置说明对应当前源码；已发布的 1.0.0 安装包可能与开发中的源码不同。

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
| 下载壁纸分辨率 | 1080P | 1080P 或 4K |
| 缓存与日志保留天数 | 3 天 | 1–365 天，图片缓存和日志共用 |

- **保存设置**：应用更改。关闭每日更新会停止并移除本副本的定时任务，重新开启并保存则重建任务。关闭时保留程序、设置、图片和日志。
- **立即检查 / 更换**：手动请求更新，关闭每日更新后仍可使用，仍遵守每个本地日历日只成功更换一次的限制。每日更新关闭时，手动执行不会创建任务或安排自动重试。
- **打开数据目录**：查看设置、缓存与日志。
- 有未保存更改时关闭窗口，可选择**保存并退出**或**取消**。

分辨率设置在下一次下载时生效，不会触发当天第二次成功更新。已有配置中独立的缓存、日志保留天数会取较长的有效值作为共用天数，直至保存新的数值。

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

## 源码结构

- `windows/src/`：Win32 GUI、系统默认浏览器跳转、工作程序、存储、任务计划、下载和壁纸更新。
- `windows/assets/`：软件与 GitHub 图标、manifest 和 Windows 资源。
- `windows/packaging/`：中英文便携版卸载入口及共用辅助脚本。
- `macos/`：为 macOS 实现预留的目录。

本地测试、构建工具、安装器源码和生成的程序包不纳入 Git。Windows 本地可用 Git 稀疏检出保留根目录文档与 `windows/`，不检出 `macos/`。
