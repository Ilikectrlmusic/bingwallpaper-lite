import AppKit

@main
enum Worker {
    static func main() {
        let args = Array(CommandLine.arguments.dropFirst())
        let allowed = ["--manual", "--scheduled", "--retry-a", "--retry-b", "--configure", "--diagnose"]
        guard args.count == 1, allowed.contains(args[0]) else { exit(64) }
        let store = Store()
        let workerURL = URL(fileURLWithPath: CommandLine.arguments[0]).standardizedFileURL
        let scheduler = Scheduler(worker: workerURL)
        if args[0] == "--diagnose" {
            do {
                let settings = try store.settings()
                let object: [String: Any] = ["version": AppInfo.version, "dataDirectory": store.root.path,
                    "dailyAgent": scheduler.definition(slot: "daily", calendar: ["Hour": settings.hour, "Minute": settings.minute], runAtLoad: true)]
                let data = try JSONSerialization.data(withJSONObject: object, options: [.prettyPrinted, .sortedKeys])
                print(String(decoding: data, as: UTF8.self)); return
            } catch { fputs("\(error.localizedDescription)\n", stderr); exit(1) }
        }
        do {
            try store.prepare()
            let lock: FileLock
            do { lock = try FileLock(store.root.appendingPathComponent("worker.lock"), wait: args[0] == "--manual" || args[0] == "--configure") }
            catch { exit(75) }
            defer { withExtendedLifetime(lock) {} }
            let settings = try store.settings()
            if args[0] == "--configure" {
                try scheduler.configure(settings)
                var state = try store.state()
                if !settings.dailyEnabled {
                    state.retryAt = nil; state.pendingDay = nil; state.pendingImage = nil
                } else if state.pendingDay == dayKey() && state.lastSuccessDay != dayKey() {
                    let future = state.retryAt.flatMap { $0 > Date() ? $0 : nil } ?? WorkPolicy.nextRetry()
                    state.retryAt = future
                    try scheduler.retry(at: future, currentSlot: nil)
                }
                try store.write(state, "state.json")
                return
            }
            let slot = args[0].hasPrefix("--retry-") ? String(args[0].dropFirst(2)) : nil
            var state = try store.state()
            let now = Date()
            if !WorkPolicy.shouldRun(settings: settings, state: state, manual: args[0] == "--manual", retry: slot != nil, now: now) {
                if state.lastSuccessDay == dayKey(now) || !settings.dailyEnabled || state.pendingDay != dayKey(now) {
                    try scheduler.clearRetries(currentSlot: slot)
                }
                return
            }
            let today = dayKey(now)
            let image = store.cache.appendingPathComponent("\(today).jpg")
            let screens = NSScreen.screens
            let workspace = NSWorkspace.shared
            var protected = Set(screens.compactMap { workspace.desktopImageURL(for: $0)?.path })
            if let path = state.image { protected.insert(path) }
            if let path = state.pendingImage { protected.insert(path) }
            // If any display cannot be queried, retain every cached JPG conservatively.
            if screens.contains(where: { workspace.desktopImageURL(for: $0) == nil }) {
                for file in (try? FileManager.default.contentsOfDirectory(at: store.cache, includingPropertiesForKeys: nil)) ?? [] {
                    if file.pathExtension == "jpg" { protected.insert(file.path) }
                }
            }
            store.prune(days: settings.retentionDays, protectedPaths: protected)
            do {
                guard !screens.isEmpty else { throw AppError("No desktop session is available") }
                var recovering = false
                if state.pendingDay == today && state.pendingImage == image.path &&
                    screens.allSatisfy({ workspace.desktopImageURL(for: $0)?.standardizedFileURL == image.standardizedFileURL }),
                    let data = try? Data(contentsOf: image) {
                    recovering = (try? BingImage.validate(data, resolution: settings.resolution)) != nil
                }
                if !recovering {
                    state.pendingDay = today; state.pendingImage = image.path; state.retryAt = nil
                    try store.write(state, "state.json")
                    store.log("Downloading \(settings.resolution)")
                    let data = try BingImage.download(resolution: settings.resolution)
                    guard dayKey() == today else { throw AppError("Local date changed during download") }
                    try data.write(to: image, options: .atomic)
                    for screen in screens {
                        try workspace.setDesktopImageURL(image, for: screen,
                            options: [.imageScaling: NSImageScaling.scaleProportionallyUpOrDown.rawValue,
                                      .allowClipping: true])
                    }
                }
                guard dayKey() == today else { throw AppError("Local date changed during wallpaper update") }
                state.lastSuccessDay = today; state.lastSuccess = Date(); state.image = image.path
                state.pendingDay = nil; state.pendingImage = nil; state.retryAt = nil; state.lastError = nil
                try store.write(state, "state.json")
                store.log(recovering ? "SUCCESS: recovered previous wallpaper update" : "SUCCESS: wallpaper updated; worker exiting")
                try scheduler.clearRetries(currentSlot: slot)
            } catch {
                state.pendingDay = today
                state.lastError = error.localizedDescription
                state.retryAt = settings.dailyEnabled && dayKey() == today ? WorkPolicy.nextRetry() : nil
                try store.write(state, "state.json")
                store.log("FAILED: \(error.localizedDescription)")
                if settings.dailyEnabled { try scheduler.retry(at: state.retryAt, currentSlot: slot) }
                exit(1)
            }
        } catch {
            store.log("Worker: \(error.localizedDescription)")
            fputs("\(error.localizedDescription)\n", stderr)
            exit(1)
        }
    }
}
