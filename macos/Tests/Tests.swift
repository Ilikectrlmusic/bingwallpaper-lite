import Foundation
import AppKit
import ImageIO

@main
enum Tests {
    static var checks = 0
    static func expect(_ value: Bool, _ message: String) {
        checks += 1
        if !value { fatalError("FAIL: \(message)") }
    }
    static func rejects(_ message: String, _ operation: () throws -> Void) {
        do { try operation(); fatalError("FAIL: \(message)") } catch { checks += 1 }
    }
    static func date(_ text: String) -> Date {
        let f = DateFormatter(); f.locale = Locale(identifier: "en_US_POSIX"); f.timeZone = .current
        f.dateFormat = "yyyy-MM-dd HH:mm:ss"; return f.date(from: text)!
    }
    static func main() throws {
        let now = date("2026-10-23 10:12:34")
        var settings = Settings()
        var state = WorkState()
        expect(WorkPolicy.shouldRun(settings: settings, state: state, manual: false, now: now), "due daily run")
        expect(!WorkPolicy.shouldRun(settings: settings, state: state, manual: false, now: date("2026-10-23 08:59:59")), "no early daily run")
        expect(WorkPolicy.shouldRun(settings: settings, state: state, manual: true, now: date("2026-10-23 08:00:00")), "manual before schedule")
        state.lastSuccessDay = dayKey(now)
        expect(!WorkPolicy.shouldRun(settings: settings, state: state, manual: true, now: now), "manual cannot change twice")
        state.lastSuccessDay = "2026-10-22"
        expect(WorkPolicy.shouldRun(settings: settings, state: state, manual: false, now: now), "next local day runs")
        settings.dailyEnabled = false
        expect(!WorkPolicy.shouldRun(settings: settings, state: state, manual: false, now: now), "disabled automatic run")
        expect(WorkPolicy.shouldRun(settings: settings, state: state, manual: true, now: now), "disabled manual run")
        settings.dailyEnabled = true
        state.pendingDay = dayKey(now); state.retryAt = now.addingTimeInterval(1800)
        expect(!WorkPolicy.shouldRun(settings: settings, state: state, manual: false, retry: true, now: now), "no early retry")
        expect(WorkPolicy.shouldRun(settings: settings, state: state, manual: false, retry: true, now: now.addingTimeInterval(1800)), "retry at deadline")
        expect(!WorkPolicy.shouldRun(settings: settings, state: state, manual: false, retry: true, now: date("2026-10-24 11:00:00")), "yesterday retry expires")
        expect(WorkPolicy.nextRetry(now: now) == date("2026-10-23 10:43:00"), "30 minute retry rounds up")
        expect(WorkPolicy.nextRetry(now: date("2026-10-23 23:40:00")) == nil, "retry cannot cross midnight")
        expect(WorkPolicy.nextRetry(now: date("2026-10-23 23:29:00")) == date("2026-10-23 23:59:00"), "last retry of day")
        settings.retentionDays = 0; rejects("invalid retention") { try settings.validate() }
        settings = Settings(); settings.resolution = "8K"; rejects("invalid resolution") { try settings.validate() }
        let root = FileManager.default.temporaryDirectory.appendingPathComponent("bwl-tests-\(UUID())")
        defer { try? FileManager.default.removeItem(at: root) }
        let store = Store(root: root); try store.prepare()
        try store.write(Settings(), "settings.json")
        expect(try store.settings() == Settings(), "settings round trip")
        try store.write(state, "state.json")
        expect(try store.state().retryAt == state.retryAt, "atomic state round trip")
        let corrupt = root.appendingPathComponent("settings.json")
        try Data("bad".utf8).write(to: corrupt)
        rejects("corrupt settings must not silently reset") { _ = try store.settings() }
        let protected = store.cache.appendingPathComponent("2026-10-01.jpg")
        let expired = store.cache.appendingPathComponent("2026-10-02.jpg")
        let retained = store.cache.appendingPathComponent("2026-10-21.jpg")
        let boundary = store.logs.appendingPathComponent("2026-10-20.log")
        let unrelated = store.cache.appendingPathComponent("personal.jpg")
        let link = store.cache.appendingPathComponent("2026-10-03.jpg")
        for file in [protected, expired, retained, boundary, unrelated] { try Data([1]).write(to: file) }
        try FileManager.default.createSymbolicLink(at: link, withDestinationURL: unrelated)
        store.prune(days: 3, protectedPaths: [protected.path], now: now)
        expect(FileManager.default.fileExists(atPath: protected.path), "active wallpaper survives retention")
        expect(!FileManager.default.fileExists(atPath: expired.path), "expired image removed")
        expect(FileManager.default.fileExists(atPath: retained.path), "retention includes today and two prior dates")
        expect(!FileManager.default.fileExists(atPath: boundary.path), "expired log removed")
        expect(FileManager.default.fileExists(atPath: unrelated.path), "unrelated file preserved")
        expect(FileManager.default.fileExists(atPath: link.path), "symlink not followed or pruned")
        let lockURL = root.appendingPathComponent("lock")
        do {
            let lock = try FileLock(lockURL)
            rejects("simultaneous worker blocked") { _ = try FileLock(lockURL) }
            withExtendedLifetime(lock) {}
        }
        _ = try FileLock(lockURL)
        checks += 1
        let scheduler = Scheduler(worker: URL(fileURLWithPath: "/Applications/Bing Wallpaper.app/Contents/MacOS/BingWallpaperWorker"), agents: root)
        let daily = scheduler.definition(slot: "daily", calendar: ["Hour": 9, "Minute": 0], runAtLoad: true)
        expect((daily["ProgramArguments"] as! [String])[1] == "--scheduled", "agent launches worker")
        expect(daily["KeepAlive"] == nil && daily["StartInterval"] == nil, "no resident process or periodic polling")
        expect((daily["RunAtLoad"] as? Bool) == true, "sign in catchup")
        let encoded = try PropertyListSerialization.data(fromPropertyList: daily, format: .xml, options: 0)
        expect(!encoded.isEmpty, "valid launchd plist")
        let metadata = Data(#"{"images":[{"urlbase":"/th?id=OHR.Example_EN-US1234567890"}]}"#.utf8)
        expect(try BingImage.imageURL(metadata: metadata, resolution: "4K").absoluteString.hasSuffix("_UHD.jpg"), "4K endpoint")
        expect(try BingImage.imageURL(metadata: metadata, resolution: "1080P").absoluteString.hasSuffix("_1920x1080.jpg"), "1080P endpoint")
        for base in ["https://evil.example/image", "/th?id=OHR.X&other=1", "/th?id=OHR.XX/../../bad"] {
            let data = try JSONSerialization.data(withJSONObject: ["images": [["urlbase": base]]])
            rejects("untrusted URL rejected") { _ = try BingImage.imageURL(metadata: data, resolution: "4K") }
        }
        rejects("corrupt image rejected") { try BingImage.validate(Data([1, 2, 3]), resolution: "1080P") }
        let bitmap = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: 1920, pixelsHigh: 1080,
            bitsPerSample: 8, samplesPerPixel: 3, hasAlpha: false, isPlanar: false, colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)!
        let jpeg = bitmap.representation(using: .jpeg, properties: [:])!
        try BingImage.validate(jpeg, resolution: "1080P"); checks += 1
        rejects("wrong dimensions rejected") { try BingImage.validate(jpeg, resolution: "4K") }
        rejects("truncated JPEG rejected") { try BingImage.validate(jpeg.prefix(jpeg.count / 2), resolution: "1080P") }
        if CommandLine.arguments.contains("--launchd") {
            let live = Scheduler(worker: URL(fileURLWithPath: "/usr/bin/true"), agents: root.appendingPathComponent("agents"),
                                 identifier: "com.ilikectrlmusic.bwl-test-\(UUID().uuidString.lowercased())")
            defer { try? live.remove("daily"); try? live.clearRetries() }
            try live.configure(Settings())
            expect(try live.control(["print", "\(live.domain)/\(live.label("daily"))"], required: false) == 0, "daily agent bootstraps")
            try live.retry(at: Date().addingTimeInterval(1800), currentSlot: nil)
            expect(try live.control(["print", "\(live.domain)/\(live.label("retry-a"))"], required: false) == 0, "first retry bootstraps")
            try live.retry(at: Date().addingTimeInterval(3600), currentSlot: "retry-a")
            expect(try live.control(["print", "\(live.domain)/\(live.label("retry-b"))"], required: false) == 0, "retry alternates slots")
            expect(!FileManager.default.fileExists(atPath: live.file("retry-a").path), "old retry file removed")
            var disabled = Settings(); disabled.dailyEnabled = false
            try live.configure(disabled)
            for slot in ["daily", "retry-a", "retry-b"] {
                expect(try live.control(["print", "\(live.domain)/\(live.label(slot))"], required: false) != 0, "disabled removes \(slot)")
            }
            print("Live launchd: daily / alternating retries / disable cleanup verified")
        }
        if CommandLine.arguments.contains("--live") {
            for resolution in ["1080P", "4K"] {
                let bytes = try BingImage.download(resolution: resolution)
                print("Live Bing \(resolution): \(bytes.count) bytes, fully decoded")
                checks += 1
            }
        }
        print("PASS: \(checks) checks")
    }
}
