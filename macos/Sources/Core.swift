import Foundation
import Darwin

enum AppInfo {
    static let version = "1.0.0"
    static let identifier = "com.ilikectrlmusic.bingwallpaper-lite"
    static let github = URL(string: "https://github.com/Ilikectrlmusic/bingwallpaper-lite")!
}

struct AppError: LocalizedError {
    let message: String
    var errorDescription: String? { message }
    init(_ message: String) { self.message = message }
}

struct Settings: Codable, Equatable {
    var dailyEnabled = true
    var hour = 9
    var minute = 0
    var resolution = "4K"
    var retentionDays = 3
    func validate() throws {
        guard (0...23).contains(hour), (0...59).contains(minute),
              (1...365).contains(retentionDays), ["1080P", "4K"].contains(resolution) else {
            throw AppError("Invalid settings")
        }
    }
}

struct WorkState: Codable {
    var lastSuccessDay: String?
    var lastSuccess: Date?
    var image: String?
    var pendingDay: String?
    var pendingImage: String?
    var retryAt: Date?
    var lastError: String?
}

func dayKey(_ date: Date = Date()) -> String {
    let f = DateFormatter()
    f.locale = Locale(identifier: "en_US_POSIX")
    f.calendar = Calendar(identifier: .gregorian)
    f.timeZone = .current
    f.dateFormat = "yyyy-MM-dd"
    return f.string(from: date)
}

enum WorkPolicy {
    static func shouldRun(settings: Settings, state: WorkState, manual: Bool,
                          retry: Bool = false, now: Date = Date(), calendar: Calendar = .current) -> Bool {
        if state.lastSuccessDay == dayKey(now) { return false }
        if manual { return true }
        if !settings.dailyEnabled { return false }
        if state.pendingDay == dayKey(now) {
            return state.retryAt == nil || now >= state.retryAt!
        }
        if retry { return false } // Never carry yesterday's retries into a new day.
        let c = calendar.dateComponents([.hour, .minute], from: now)
        return c.hour! * 60 + c.minute! >= settings.hour * 60 + settings.minute
    }
    static func nextRetry(now: Date = Date()) -> Date? {
        // Calendar triggers have minute precision. Round up, never retry early.
        let next = Date(timeIntervalSince1970: ceil((now.timeIntervalSince1970 + 1800) / 60) * 60)
        return dayKey(next) == dayKey(now) ? next : nil
    }
}

struct Store {
    let root: URL
    init(root: URL? = nil) {
        self.root = root ?? FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent("Library/Application Support/BingWallpaperLite", isDirectory: true)
    }
    var cache: URL { root.appendingPathComponent("cache", isDirectory: true) }
    var logs: URL { root.appendingPathComponent("logs", isDirectory: true) }
    func prepare() throws {
        for dir in [root, cache, logs] {
            try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true,
                                                    attributes: [.posixPermissions: 0o700])
        }
    }
    func read<T: Decodable>(_ type: T.Type, _ name: String, fallback: T) throws -> T {
        let url = root.appendingPathComponent(name)
        guard FileManager.default.fileExists(atPath: url.path) else { return fallback }
        return try JSONDecoder().decode(type, from: Data(contentsOf: url))
    }
    func write<T: Encodable>(_ value: T, _ name: String) throws {
        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        try encoder.encode(value).write(to: root.appendingPathComponent(name), options: .atomic)
    }
    func settings() throws -> Settings {
        let s = try read(Settings.self, "settings.json", fallback: Settings())
        try s.validate()
        return s
    }
    func state() throws -> WorkState { try read(WorkState.self, "state.json", fallback: WorkState()) }
    func log(_ message: String) {
        let url = logs.appendingPathComponent("\(dayKey()).log")
        let line = "\(ISO8601DateFormatter().string(from: Date())) \(message)\n"
        guard let bytes = line.data(using: .utf8) else { return }
        if !FileManager.default.fileExists(atPath: url.path) { try? bytes.write(to: url) }
        else if let h = try? FileHandle(forWritingTo: url) {
            defer { try? h.close() }
            do { try h.seekToEnd(); try h.write(contentsOf: bytes) } catch { }
        }
    }
    func prune(days: Int, protectedPaths: Set<String>, now: Date = Date()) {
        let cutoff = Calendar.current.date(byAdding: .day, value: -(days - 1),
                                           to: Calendar.current.startOfDay(for: now))!
        let cutoffKey = dayKey(cutoff)
        let protected = Set(protectedPaths.map { URL(fileURLWithPath: $0).resolvingSymlinksInPath().path })
        for (directory, extensions) in [(cache, ["jpg", "part"]), (logs, ["log"])] {
            guard let files = try? FileManager.default.contentsOfDirectory(at: directory,
                includingPropertiesForKeys: [.isRegularFileKey, .isSymbolicLinkKey]) else { continue }
            for file in files {
                let name = file.deletingPathExtension().lastPathComponent
                guard name.range(of: #"^\d{4}-\d{2}-\d{2}$"#, options: .regularExpression) != nil,
                      extensions.contains(file.pathExtension), name < cutoffKey,
                      !protected.contains(file.resolvingSymlinksInPath().path),
                      let values = try? file.resourceValues(forKeys: [.isRegularFileKey, .isSymbolicLinkKey]),
                      values.isRegularFile == true, values.isSymbolicLink != true else { continue }
                do { try FileManager.default.removeItem(at: file) }
                catch { log("Cleanup: \(error.localizedDescription)") }
            }
        }
    }
}

final class FileLock {
    private let fd: Int32
    init(_ url: URL, wait: Bool = false) throws {
        fd = open(url.path, O_CREAT | O_RDWR | O_CLOEXEC, 0o600)
        guard fd >= 0 else { throw AppError("Cannot open lock") }
        guard flock(fd, LOCK_EX | (wait ? 0 : LOCK_NB)) == 0 else {
            close(fd)
            throw AppError("Another operation is running")
        }
    }
    deinit { flock(fd, LOCK_UN); close(fd) }
}
