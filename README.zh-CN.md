<p align="center">
  <img src="assets/app.png" width="96" height="96" alt="Bing Wallpaper 软件图标">
</p>

<h1 align="center">Bing Wallpaper</h1>

<p align="center"><a href="README.md">English</a> · 简体中文</p>

适用于 **Windows 10/11 x64** 的极简 Bing 壁纸工具，使用原生 **C / Win32** 编写。无托盘、无常驻后台，每次执行后即彻底退出；失败后的重试交给 Windows 任务计划，不在后台等待。

> 本项目为非官方工具。目前设置界面和便携卸载弹窗使用简体中文。

## 下载

**[1.0.0 版本发布页](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/tag/v1.0.0)**

| 版本 | 下载 |
| --- | --- |
| 安装版 | [BingWallpaperLite-Setup-1.0.0-x64.exe](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Setup-1.0.0-x64.exe) |
| 便携版 | [BingWallpaperLite-Portable-1.0.0-x64.zip](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/download/v1.0.0/BingWallpaperLite-Portable-1.0.0-x64.zip) |

无需额外安装 Python、.NET 或 WebView 运行时。便携版卸载脚本使用 Windows 自带的 Windows PowerShell。

## 使用

- **安装版**：运行安装程序并选择目录。安装程序会申请管理员权限，默认目录为 `C:\Program Files\BingWallpaperLite`；完成后自动打开设置窗口，软件本身使用普通用户权限运行。
- **便携版**：把 ZIP 中整个 `BingWallpaperLite-Portable` 文件夹解压到一个**独立目录**，运行 **`BingWallpaper.exe`**。请保留同目录下的工作程序和卸载脚本。
- 首次打开设置时，主程序会调用工作程序建立每日任务，并立即尝试更换一次壁纸。之后可直接关闭设置窗口，不需要保持窗口开启，主程序也不会每天启动。

## 设置

| 设置项 | 默认值 | 可选范围 |
| --- | --- | --- |
| 每日启动时间 | 系统本地时间 09:00 | 自行选择时间 |
| 图片缓存保留天数 | 3 天 | 1–365 天 |
| 运行日志保留天数 | 3 天 | 1–365 天 |
| 下载壁纸分辨率 | 1080P | 1080P（1920×1080）或 4K（3840×2160） |

- **保存设置**：保存更改，并由工作程序更新定时任务。
- **立即检查 / 更换**：立即请求执行，仍遵守每天只成功更换一次的限制。
- **打开数据目录**：查看设置、缓存和日志文件。
- 点击 GitHub 图标，在系统默认浏览器中打开 [项目仓库](https://github.com/Ilikectrlmusic/bingwallpaper-lite)。

有未保存的更改时关闭窗口，会提示 **“保存并退出” / “取消”**。选择保存后，只有写入设置和更新任务均成功，窗口才会自动关闭。

分辨率设置供下一次下载使用。当天已经成功更换壁纸后，切换分辨率不会再次下载或更换，下一天下载时生效。

## 运行与重试

每日任务启动的是 **`BingWallpaperWorker.exe`**，不是设置界面。任务使用当前已登录用户的身份，无需保存密码。同一本地日历日成功更换一次后，后续触发会直接退出，不重复下载或更换。

失败时，工作程序会返回错误并退出，Windows 任务计划按 **30 分钟间隔、最多 48 次**重试。等待重试时没有工作进程驻留。登录后 20 秒还会补检；未到设定时间且没有待重试或手动请求时，不会提前下载。程序不主动唤醒电脑。

失败会保留原壁纸。下载后会完整解码并核对图片尺寸，不会把低分辨率图片当作 4K。Bing 图片接口或网络连接异常可能导致更新失败。

## 数据位置与清理

- 普通权限可写入安装目录：**`程序目录\data\`**。
- 普通权限不可写入安装目录：**`%APPDATA%\BingWallpaperLite\<安装路径标识>\`**。

`HKCU\Software\BingWallpaperLite` 中的小型定位项记录数据目录。因此便携版也会创建定时任务和定位注册项，不属于“零系统痕迹”软件。

缓存与日志按本地日历日期清理，**当前正在使用的壁纸受到保护**，可能超过设定的缓存保留天数。运行时清理和便携卸载使用回收站，清理失败会报错。

## 卸载与移动目录

**安装版**：Windows 设置 → 应用 → 已安装的应用 → Bing Wallpaper → 卸载。工作程序先删除本副本的任务、当前用户的数据和定位项，再由 Inno Setup 移除程序文件。

**便携版**：先关闭设置窗口，再双击 **`卸载便携版.cmd`**。首先弹窗选择：

- **卸载**：清理任务、用户数据和定位项，再把整个独立便携目录（包含卸载脚本）移到回收站。
- **仅卸载定时任务**：停止并移除当前目录、当前用户的任务，保留程序、设置、缓存、日志和定位项。完成后可移动整个目录，再打开新位置的 `BingWallpaper.exe` 重建任务。
- **取消**：不进行更改。

两种卸载操作均会弹窗显示成功或具体错误。请使用**最初设置此软件的同一 Windows 用户**卸载。卸载可能移除当前壁纸文件，但不会替你选择其他系统壁纸；如有需要，请先另选壁纸。

普通可写便携目录中的 `data` 会随整个目录一起移动。如果数据原本存放在 APPDATA，“仅卸载定时任务”仍会保留这些外部数据，移动程序**不会自动迁移**它们。需要保留时，请先用“打开数据目录”备份，再复制到新位置对应的数据目录；旧位置的定位项也会按“仅卸载任务”的语义保留。

## 源码结构

- `src/`：设置界面、浏览器启动、工作程序、数据存储、任务计划、下载和壁纸替换。
- `assets/`：软件与 GitHub 图标、Windows manifest 和资源文件。
- `packaging/`：便携版卸载脚本。

本地开发说明、测试、构建辅助文件和生成的安装包已按项目要求排除，不提交到仓库。可直接运行的程序请到 [Releases](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases) 下载。
