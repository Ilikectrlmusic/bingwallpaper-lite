#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDK="$(xcrun --sdk macosx --show-sdk-path)"
mkdir -p "$ROOT/build/module-cache" "$ROOT/dist"
ICONSET="$ROOT/build/AppIcon.iconset"
mkdir -p "$ICONSET"
for size in 16 32 128 256 512; do
    sips -z "$size" "$size" "$ROOT/Resources/AppIcon.png" --out "$ICONSET/icon_${size}x${size}.png" >/dev/null
    double=$((size * 2))
    sips -z "$double" "$double" "$ROOT/Resources/AppIcon.png" --out "$ICONSET/icon_${size}x${size}@2x.png" >/dev/null
done
iconutil -c icns "$ICONSET" -o "$ROOT/build/AppIcon.icns"
if [[ $# -gt 0 ]]; then architectures=("$@"); else architectures=(arm64 x86_64); fi
for arch in "${architectures[@]}"; do
    case "$arch" in arm64|x86_64) ;; *) echo "Unsupported architecture: $arch" >&2; exit 64;; esac
    APP="$ROOT/build/$arch/Bing Wallpaper.app"
    mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
    cp "$ROOT/Resources/Info.plist" "$APP/Contents/Info.plist"
    cp -R "$ROOT/Resources/en.lproj" "$ROOT/Resources/zh-Hans.lproj" "$APP/Contents/Resources/"
    cp "$ROOT/Resources/github.png" "$ROOT/build/AppIcon.icns" "$APP/Contents/Resources/"
    flags=(-swift-version 5 -O -whole-module-optimization -sdk "$SDK" -target "$arch-apple-macos12.0"
        -module-cache-path "$ROOT/build/module-cache" -framework AppKit)
    xcrun swiftc "${flags[@]}" "$ROOT/Sources/Core.swift" "$ROOT/Sources/GUI.swift" -o "$APP/Contents/MacOS/BingWallpaper"
    xcrun swiftc "${flags[@]}" "$ROOT/Sources/Core.swift" "$ROOT/Sources/Scheduler.swift" "$ROOT/Sources/Network.swift" "$ROOT/Sources/Worker.swift" -o "$APP/Contents/MacOS/BingWallpaperWorker"
    codesign --force --sign - --timestamp=none "$APP/Contents/MacOS/BingWallpaperWorker"
    codesign --force --sign - --timestamp=none "$APP"
    codesign --verify --deep --strict --verbose=2 "$APP"
    STAGE="$ROOT/build/dmg-$arch"
    rm -rf "$STAGE"
    mkdir -p "$STAGE"
    ditto "$APP" "$STAGE/Bing Wallpaper.app"
    ln -sfn /Applications "$STAGE/Applications"
    DMG="$ROOT/dist/BingWallpaperLite-1.0.0-$arch.dmg"
    rm -f "$DMG"
    hdiutil create -volname "Bing Wallpaper 1.0.0" -srcfolder "$STAGE" -ov -format UDZO "$DMG"
    codesign --force --sign - --timestamp=none "$DMG"
    hdiutil verify "$DMG"
done
(cd "$ROOT/dist" && shasum -a 256 ./*.dmg > SHA256SUMS.txt)
