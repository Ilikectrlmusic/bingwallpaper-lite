<p align="center"><img src="https://raw.githubusercontent.com/Ilikectrlmusic/bingwallpaper-lite/main/windows/assets/app.png" width="96" alt="Bing Wallpaper"></p>

<h1 align="center">Bing Wallpaper Lite</h1>
<p align="center"><a href="README.md">English</a> · 简体中文</p>

轻量级 Bing 每日壁纸工具。每天更新一次桌面壁纸，完成后自动退出，**没有托盘图标，也不常驻后台**。

- Windows 原生 C / Win32，macOS 原生 Swift / AppKit
- 支持 1080P / 4K 壁纸
- 可设置每日更新时间、缓存和日志保留天数
- 下载失败自动重试
- 简体中文 / 英文界面

## 为什么选择 Lite？

**极致轻量化。** 只做一件事：每天更新 Bing 壁纸，然后退出。无需让壁纸工具全天驻留后台。

| 对比项目（Windows） | Bing Wallpaper Lite v1.0.0 | 微软 Bing Wallpaper v1.1.460.0 |
| --- | --- | --- |
| 安装包大小 | **约 2.2 MB** | **218.5 MB** |
| 大小差距 | **约为官方的 1%，小约 99 倍** | — |
| 常驻后台 | **否，更新完成后退出** | 是，提供常驻系统托盘程序 |
| 每日壁纸更新 | 系统定时任务触发 | 官方应用自动管理 |

> 大小比较基于本项目 [v1.0.0 Windows 安装包](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/tag/v1.0.0) 和 [微软官方下载中心](https://www.microsoft.com/en-us/download/details.aspx?id=101202) 标注的 2026-08-24 版本；仅比较下载包大小，不代表安装后占用或内存使用量。

## 下载

[**下载最新版本**](https://github.com/Ilikectrlmusic/bingwallpaper-lite/releases/latest)

| 平台 | 安装包 |
| --- | --- |
| Windows 10/11（x64） | 安装版（`.exe`）或便携版（`.zip`） |
| macOS 12+（Apple 芯片） | `arm64.dmg` |
| macOS 12+（Intel） | `x86_64.dmg` |

## 使用

**Windows：** 运行安装包，或解压便携版 ZIP 后打开 `BingWallpaper.exe`。

**macOS：** 打开 DMG，将 **Bing Wallpaper.app** 拖入“应用程序”文件夹。采用 Ad-hoc 签名，如首次启动被拦截，进入 **系统设置 → 隐私与安全性 → 仍要打开**。

首次启动会设置每日任务，并尝试更新当天壁纸。可在设置界面调整时间、分辨率和保留天数。默认 **09:00、4K、缓存及日志保留 3 天**。

每天最多成功更换一次壁纸，包括手动更新。定时任务由系统负责，工作程序执行完即退出。


## 卸载

- **Windows 安装版：** 在 Windows 设置 → 应用中卸载。
- **Windows 便携版：** 运行 `卸载便携版.cmd` ，会同时删除便携目录和应用数据。
- **macOS：** 先关闭每日更新并保存，再将应用移到废纸篓。如需删除用户数据，可手动清理 `~/Library/Application Support/BingWallpaperLite/`。


各版本 SHA-256 校验值见 Release 页面。
