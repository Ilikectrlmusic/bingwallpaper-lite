#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$ROOT/build/module-cache"
xcrun swiftc -swift-version 5 -sdk "$(xcrun --sdk macosx --show-sdk-path)" \
    -module-cache-path "$ROOT/build/module-cache" -framework AppKit \
    "$ROOT/Sources/Core.swift" "$ROOT/Sources/Scheduler.swift" "$ROOT/Sources/Network.swift" \
    "$ROOT/Tests/Tests.swift" -o "$ROOT/build/tests"
"$ROOT/build/tests" "$@"
