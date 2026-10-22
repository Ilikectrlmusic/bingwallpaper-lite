<p align="center">
  <img src="assets/app.png" width="96" height="96" alt="Bing Wallpaper 软件图标">
</p>

<h1 align="center">Bing Wallpaper</h1>

<p align="center"><a href="README.md">English</a> · 简体中文</p>

适用于 **Windows 10/11 x64** 的极简 Bing 壁纸工具，使用原生 **C / Win32** 编写。无托盘、无常驻后台，每次执行后即彻底退出；失败后的重试交给 Windows 任务计划，不在后台等待。


## 下载

**[1.0.0 版本发布页](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/tag/v1.0.0)**

| 版本 | 下载 |
| --- | --- |
| 安装版 | [BingWallpaperLite-Setup-1.0.0-x64.exe](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Setup-1.0.0-x64.exe) |
| 便携版 | [BingWallpaperLite-Portable-1.0.0-x64.zip](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Portable-1.0.0-x64.zip) |

无需额外安装 Python、.NET 或 WebView 运行时。便携版卸载脚本使用 Windows 自带的 Windows PowerShell。

## 使用

- **安装版**：运行安装程序并选择目录，默认目录为 `C:\Program Files\BingWallpaperLite`；完成后会自动打开设置窗口。
- **便携版**：解压 ZIP，运行 **`BingWallpaper.exe`**。
- 首次打开设置时，主程序会调用工作程序建立每日任务，并立即尝试更换一次壁纸。

## 设置

| 设置项 | 默认值 | 可选范围 |
| --- | --- | --- |
| 每日启动时间 | 系统本地时间 09:00 | 自行选择时间 |
| 图片缓存保留天数 | 3 天 | 1–365 天 |
| 运行日志保留天数 | 3 天 | 1–365 天 |
| 下载壁纸分辨率 | 1080P | 1080P 或 4K |

- **保存设置**：保存更改，并更新定时任务。
- **立即检查 / 更换**：立即请求执行，仍遵守每天只成功更换一次的限制。
- **打开数据目录**：查看设置、缓存和日志文件。


分辨率设置供下一次下载使用。当天已经成功更换壁纸后，切换分辨率不会再次下载或更换，下一天下载时生效。

## 运行与重试

每日任务启动的是 **`BingWallpaperWorker.exe`**， **`BingWallpaper.exe`** 只是设置GUI。每天成功更换一次后，**`BingWallpaperWorker.exe`** 后续触发会直接退出。

失败时，工作程序会退出，然后 Windows 任务计划按 **30 分钟间隔、最多 48 次**重试。等待重试时没有工作进程驻留。登录后 20 秒还会补检；未到设定时间且没有待重试或手动请求时，不会提前下载。程序不主动唤醒电脑。


## 数据位置与清理

- 普通权限可写入安装目录：**`程序目录\data\`**。
- 普通权限不可写入安装目录：**`%APPDATA%\BingWallpaperLite\<安装路径标识>\`**。

`HKCU\Software\BingWallpaperLite` 中的小型定位项记录数据目录。因此便携版也会创建定时任务和定位注册项。

缓存与日志按本地日历日期清理，**当前正在使用的壁纸受到保护**，可能超过设定的缓存保留天数。

## 卸载与移动目录

**安装版**：Windows 设置 → 应用 → 已安装的应用 → Bing Wallpaper → 卸载。工作程序先删除本副本的任务、当前用户的数据和定位项，再由 Inno Setup 移除程序文件。

**便携版**：先确保关闭设置窗口，再双击 **`卸载便携版.cmd`** 。ZIP 中包含这两个入口，均可选择：

- **卸载**：清理任务、用户数据和定位项，再把整个独立便携目录（包含卸载脚本）删除。
- **仅卸载定时任务**：停止并移除当前目录的任务，保留程序、设置、缓存、日志和定位项。完成后可移动整个目录，再打开新位置的 `BingWallpaper.exe` 重建任务。
- **取消**

请保留同目录下共用的 `uninstall-portable.ps1` 辅助脚本。

普通可写便携目录中的 `data` 会随整个目录一起移动。如果数据原本存放在 APPDATA，“仅卸载定时任务”仍会保留这些外部数据，移动程序**不会自动迁移**它们。需要保留时，请先用“打开数据目录”备份，再复制到新位置对应的数据目录；旧位置的定位项也会按“仅卸载任务”的语义保留。

## 源码结构

- `src/`：设置界面、浏览器启动、工作程序、数据存储、任务计划、下载和壁纸替换。
- `assets/`：软件与 GitHub 图标、Windows manifest 和资源文件。
- `packaging/`：便携版卸载脚本。
