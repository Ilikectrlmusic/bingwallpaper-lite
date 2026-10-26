<p align="center"><img src="https://raw.githubusercontent.com/Ilikectrlmusic/bingwallpaper-lite/main/windows/assets/app.png" width="96" alt="Bing Wallpaper"></p>

<h1 align="center">Bing Wallpaper Lite</h1>
<p align="center"><a href="README.md">English</a> · 简体中文</p>

## 为什么选择 Lite？

**极致轻量化。** 系统定时任务触发，然后就退出。无托盘，也不常驻后台。

| 对比项目 | Bing Wallpaper Lite v1.0.0 | Bing Wallpaper v1.1.460.0 |
| --- | --- | --- |
| macOS支持 | ✅ | ❌ |
| exe安装包大小 | **2.2 MB** | **218.5 MB** |
| 常驻后台 | **否，更新完成后退出** | 是，常驻系统托盘 |
| 失败重试 | ✅ | ✅ |


## 功能介绍

- Windows 原生 C / Win32，macOS 原生 Swift / AppKit
- 支持 1080P / 4K 壁纸
- 可设置每日更新时间、缓存和日志保留天数
- 下载失败自动重试
- 简体中文 / 英文界面


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

---

各版本 SHA-256 校验值见 Release 页面。
