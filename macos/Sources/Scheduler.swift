import Foundation
import Darwin

struct Scheduler {
    let worker: URL
    let agents: URL
    let domain: String
    let identifier: String
    init(worker: URL, agents: URL? = nil, identifier: String = AppInfo.identifier) {
        self.worker = worker
        self.identifier = identifier
        self.agents = agents ?? FileManager.default.homeDirectoryForCurrentUser.appendingPathComponent("Library/LaunchAgents")
        domain = "gui/\(getuid())"
    }
    func label(_ slot: String) -> String { "\(identifier).\(slot)" }
    func file(_ slot: String) -> URL { agents.appendingPathComponent("\(label(slot)).plist") }
    @discardableResult
    func control(_ args: [String], required: Bool = true) throws -> Int32 {
        let p = Process()
        p.executableURL = URL(fileURLWithPath: "/bin/launchctl")
        p.arguments = args
        let pipe = Pipe()
        p.standardOutput = pipe; p.standardError = pipe
        try p.run()
        let output = pipe.fileHandleForReading.readDataToEndOfFile()
        p.waitUntilExit()
        if required && p.terminationStatus != 0 {
            throw AppError("launchctl \(args.first ?? ""): \(String(decoding: output, as: UTF8.self))")
        }
        return p.terminationStatus
    }
    func definition(slot: String, calendar: [String: Int], runAtLoad: Bool) -> [String: Any] {
        ["Label": label(slot), "ProgramArguments": [worker.path, slot == "daily" ? "--scheduled" : "--\(slot)"],
         "StartCalendarInterval": calendar, "RunAtLoad": runAtLoad,
         "ProcessType": "Background", "LimitLoadToSessionType": "Aqua",
         "ExitTimeOut": 10, "ThrottleInterval": 30, "AssociatedBundleIdentifiers": [AppInfo.identifier]]
    }
    func install(slot: String, calendar: [String: Int], runAtLoad: Bool) throws {
        try remove(slot)
        try FileManager.default.createDirectory(at: agents, withIntermediateDirectories: true)
        let data = try PropertyListSerialization.data(fromPropertyList:
            definition(slot: slot, calendar: calendar, runAtLoad: runAtLoad), format: .xml, options: 0)
        try data.write(to: file(slot), options: .atomic)
        try FileManager.default.setAttributes([.posixPermissions: 0o600], ofItemAtPath: file(slot).path)
        try control(["enable", "\(domain)/\(label(slot))"])
        try control(["bootstrap", domain, file(slot).path])
    }
    func remove(_ slot: String, currentSlot: String? = nil) throws {
        // A retry never bootouts itself: launchd would kill it before it can commit state.
        // Its expired calendar entry is reaped by the next daily or opposite-slot run.
        if slot != currentSlot {
            let target = "\(domain)/\(label(slot))"
            if try control(["print", target], required: false) == 0 {
                try control(["bootout", target])
            }
        }
        if FileManager.default.fileExists(atPath: file(slot).path) {
            try FileManager.default.removeItem(at: file(slot))
        }
    }
    func clearRetries(currentSlot: String? = nil) throws {
        for slot in ["retry-a", "retry-b"] { try remove(slot, currentSlot: currentSlot) }
    }
    func configure(_ settings: Settings) throws {
        try clearRetries()
        if settings.dailyEnabled {
            try install(slot: "daily", calendar: ["Hour": settings.hour, "Minute": settings.minute], runAtLoad: true)
        } else { try remove("daily") }
    }
    func retry(at date: Date?, currentSlot: String?) throws {
        guard let date else { try clearRetries(currentSlot: currentSlot); return }
        let nextSlot = currentSlot == "retry-a" ? "retry-b" : "retry-a"
        let c = Calendar.current.dateComponents([.month, .day, .hour, .minute], from: date)
        try install(slot: nextSlot, calendar: ["Month": c.month!, "Day": c.day!, "Hour": c.hour!, "Minute": c.minute!], runAtLoad: false)
        if let currentSlot { try remove(currentSlot, currentSlot: currentSlot) }
        else { try remove("retry-b") }
    }
}
