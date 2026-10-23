<p align="center"><img src="https://raw.githubusercontent.com/Ilikectrlmusic/bingwallpaper-lite/main/windows/assets/app.png" width="96" alt="Bing Wallpaper"></p>

<h1 align="center">Bing Wallpaper Lite</h1>
<p align="center"><a href="README.md">English</a> · 简体中文</p>

轻量级 Bing 每日壁纸工具。每天更新一次桌面壁纸，完成后自动退出，**没有托盘图标，也不常驻后台**。

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

**macOS：** 打开 DMG，将 **Bing Wallpaper.app** 拖入“应用程序”文件夹，再从该文件夹启动。

首次启动会设置每日任务，并尝试更新当天壁纸。可在设置界面调整时间、分辨率和保留天数。默认 **09:00、4K、缓存及日志保留 3 天**。

每天最多成功更换一次壁纸，包括手动更新。定时任务由系统负责，工作程序执行完即退出。

**macOS 注意：** 应用采用 Ad-hoc 签名，未经过苹果公证。如首次启动被拦截，尝试打开应用后，进入 **系统设置 → 隐私与安全性 → 仍要打开**。

## 卸载

- **Windows 安装版：** 在 Windows 设置 → 应用中卸载。
- **Windows 便携版：** 运行 `卸载便携版.cmd` 或 `Uninstall-Portable.cmd`，会同时删除便携目录和应用数据，请勿在该目录放置其他文件。
- **macOS：** 先关闭每日更新并保存，再将应用移到废纸篓。如需删除用户数据，可手动清理 `~/Library/Application Support/BingWallpaperLite/`。

## 源码

- [Windows](windows/)
- [macOS](macos/) — 使用 `./macos/scripts/build.sh` 构建，`./macos/scripts/test.sh` 测试

各版本 SHA-256 校验值见 Release 页面。
