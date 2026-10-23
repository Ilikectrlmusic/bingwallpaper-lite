import AppKit

func L(_ key: String) -> String {
    let language = Locale.preferredLanguages.first?.lowercased() ?? "en"
    let simplified = language.hasPrefix("zh-hans") || language == "zh-cn" || language == "zh-sg"
    let path = Bundle.main.path(forResource: simplified ? "zh-Hans" : "en", ofType: "lproj")
    return path.flatMap(Bundle.init(path:))?.localizedString(forKey: key, value: nil, table: nil) ?? key
}

@main
final class AppDelegate: NSObject, NSApplicationDelegate, NSWindowDelegate {
    private var window: NSWindow!
    private let store = Store()
    private var saved = Settings()
    private var daily: NSSwitch!
    private var time: NSDatePicker!
    private var resolution: NSPopUpButton!
    private var retention: NSTextField!
    private var status: NSTextField!
    private var last: NSTextField!
    private var actions: [NSButton] = []
    private var busy = false
    private var timer: Timer?
    private var guiLock: FileLock?
    private let preview = CommandLine.arguments.contains("--preview")
    private var worker: URL { Bundle.main.bundleURL.appendingPathComponent("Contents/MacOS/BingWallpaperWorker") }

    static func main() {
        let app = NSApplication.shared
        let delegate = AppDelegate()
        app.delegate = delegate
        app.setActivationPolicy(.regular)
        withExtendedLifetime(delegate) { app.run() }
    }
    func applicationDidFinishLaunching(_ notification: Notification) {
        do {
            if !preview {
                try store.prepare()
                guiLock = try FileLock(store.root.appendingPathComponent("gui.lock"))
            }
            saved = try store.settings()
        } catch {
            let alert = NSAlert(); alert.messageText = L("error"); alert.informativeText = error.localizedDescription
            alert.runModal(); NSApp.terminate(nil); return
        }
        buildMenu(); buildWindow(); refresh()
        timer = Timer.scheduledTimer(withTimeInterval: 3, repeats: true) { [weak self] _ in self?.refresh() }
        if !preview {
            let first = !FileManager.default.fileExists(atPath: store.root.appendingPathComponent("settings.json").path)
            perform {
                if first { try self.store.write(self.saved, "settings.json") }
                try self.runWorker("--configure")
                if first && self.saved.dailyEnabled { try self.runWorker("--manual") }
            }
        }
    }
    private func buildMenu() {
        let bar = NSMenu()
        let appItem = NSMenuItem(); bar.addItem(appItem)
        let menu = NSMenu(); appItem.submenu = menu
        menu.addItem(withTitle: L("quit"), action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")
        let editItem = NSMenuItem(); bar.addItem(editItem)
        let edit = NSMenu(title: L("edit")); editItem.submenu = edit
        for (title, action, key) in [("cut", #selector(NSText.cut(_:)), "x"), ("copy", #selector(NSText.copy(_:)), "c"),
                                     ("paste", #selector(NSText.paste(_:)), "v"), ("selectAll", #selector(NSText.selectAll(_:)), "a")] {
            edit.addItem(withTitle: L(title), action: action, keyEquivalent: key)
        }
        NSApp.mainMenu = bar
    }
    private func label(_ text: String, size: CGFloat = 13, color: NSColor = .labelColor) -> NSTextField {
        let v = NSTextField(labelWithString: text); v.font = .systemFont(ofSize: size); v.textColor = color
        return v
    }
    private func buildWindow() {
        window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 460, height: 450),
                          styleMask: [.titled, .closable, .miniaturizable], backing: .buffered, defer: false)
        window.title = "Bing Wallpaper"; window.delegate = self; window.isReleasedWhenClosed = false
        let root = NSStackView(); root.orientation = .vertical; root.alignment = .leading; root.spacing = 14
        root.translatesAutoresizingMaskIntoConstraints = false
        window.contentView!.addSubview(root)
        NSLayoutConstraint.activate([root.leadingAnchor.constraint(equalTo: window.contentView!.leadingAnchor, constant: 24),
            root.trailingAnchor.constraint(equalTo: window.contentView!.trailingAnchor, constant: -24),
            root.topAnchor.constraint(equalTo: window.contentView!.topAnchor, constant: 20)])
        func fillWidth(_ view: NSView) {
            view.widthAnchor.constraint(equalTo: root.widthAnchor).isActive = true
        }
        let icon = NSImageView(image: NSImage(named: NSImage.applicationIconName)!)
        icon.widthAnchor.constraint(equalToConstant: 48).isActive = true
        icon.heightAnchor.constraint(equalToConstant: 48).isActive = true
        let heading = label("Bing Wallpaper", size: 23)
        heading.font = .systemFont(ofSize: 23, weight: .semibold)
        let title = NSStackView(views: [heading, label(L("tagline"), size: 12, color: .secondaryLabelColor)])
        title.orientation = .vertical; title.alignment = .leading; title.spacing = 4
        let header = NSStackView(views: [icon, title]); header.spacing = 12
        root.addArrangedSubview(header); root.setCustomSpacing(20, after: header)

        daily = NSSwitch(); daily.controlSize = .small
        daily.state = saved.dailyEnabled ? .on : .off
        daily.setAccessibilityLabel(L("updates"))
        time = NSDatePicker(); time.datePickerStyle = .textFieldAndStepper
        time.datePickerElements = [.hourMinute]; time.locale = Locale(identifier: "en_GB")
        time.calendar = Calendar.current; time.timeZone = .current
        time.dateValue = Calendar.current.date(from: DateComponents(year: 2001, month: 1, day: 1, hour: saved.hour, minute: saved.minute))!
        time.widthAnchor.constraint(equalToConstant: 76).isActive = true
        resolution = NSPopUpButton(); resolution.addItems(withTitles: ["1080P", "4K"])
        resolution.selectItem(withTitle: saved.resolution)
        resolution.widthAnchor.constraint(equalToConstant: 76).isActive = true
        retention = NSTextField(string: String(saved.retentionDays)); retention.alignment = .center
        retention.widthAnchor.constraint(equalToConstant: 44).isActive = true
        let days = NSStackView(views: [retention, label(L("days"), color: .secondaryLabelColor)]); days.spacing = 6

        let box = NSBox(); box.boxType = .custom; box.titlePosition = .noTitle
        box.borderWidth = 0.5; box.cornerRadius = 10
        box.borderColor = .separatorColor; box.fillColor = .controlBackgroundColor
        box.contentViewMargins = .zero
        let content = NSView(); box.contentView = content
        let form = NSStackView(); form.orientation = .vertical; form.alignment = .leading; form.spacing = 0
        form.translatesAutoresizingMaskIntoConstraints = false; content.addSubview(form)
        NSLayoutConstraint.activate([form.leadingAnchor.constraint(equalTo: content.leadingAnchor, constant: 14),
            form.trailingAnchor.constraint(equalTo: content.trailingAnchor, constant: -14),
            form.topAnchor.constraint(equalTo: content.topAnchor, constant: 5),
            form.bottomAnchor.constraint(equalTo: content.bottomAnchor, constant: -5)])
        let fields: [(String, NSView)] = [("updates", daily), ("time", time), ("resolution", resolution), ("retention", days)]
        for (index, field) in fields.enumerated() {
            let spacer = NSView(); spacer.setContentHuggingPriority(.defaultLow, for: .horizontal)
            let row = NSStackView(views: [label(L(field.0)), spacer, field.1]); row.spacing = 10
            form.addArrangedSubview(row)
            row.widthAnchor.constraint(equalTo: form.widthAnchor).isActive = true
            row.heightAnchor.constraint(equalToConstant: 36).isActive = true
            if index < fields.count - 1 {
                let line = NSBox(); line.boxType = .separator
                form.addArrangedSubview(line)
                line.widthAnchor.constraint(equalTo: form.widthAnchor).isActive = true
                line.heightAnchor.constraint(equalToConstant: 1).isActive = true
            }
        }
        root.addArrangedSubview(box); fillWidth(box)
        box.heightAnchor.constraint(equalToConstant: 157).isActive = true
        let buttons = NSStackView(); buttons.spacing = 8; buttons.distribution = .fillEqually
        for (key, selector) in [("save", #selector(save)), ("update", #selector(update)), ("folder", #selector(openFolder))] {
            let b = NSButton(title: L(key), target: self, action: selector); b.bezelStyle = .rounded
            if key == "save" { b.bezelColor = .controlAccentColor; b.contentTintColor = .white }
            actions.append(b); buttons.addArrangedSubview(b)
        }
        root.addArrangedSubview(buttons); fillWidth(buttons)
        status = label(L("ready"), size: 11, color: .secondaryLabelColor); status.alignment = .right
        last = label("", size: 11, color: .secondaryLabelColor)
        let statusSpacer = NSView(); statusSpacer.setContentHuggingPriority(.defaultLow, for: .horizontal)
        let footer = NSStackView(views: [last, statusSpacer, status]); footer.spacing = 10
        root.addArrangedSubview(footer); fillWidth(footer)
        root.setCustomSpacing(16, after: buttons)
        let github = NSButton(title: "GitHub", target: self, action: #selector(openGitHub))
        github.image = NSImage(contentsOf: Bundle.main.url(forResource: "github", withExtension: "png")!)
        github.image?.size = NSSize(width: 18, height: 18)
        github.image?.isTemplate = false; github.imageScaling = .scaleProportionallyDown
        github.imagePosition = .imageLeading; github.font = .systemFont(ofSize: 11)
        github.bezelStyle = .inline; github.isBordered = false
        github.contentTintColor = .secondaryLabelColor; github.toolTip = L("github")
        github.setAccessibilityLabel(L("github"))
        github.heightAnchor.constraint(equalToConstant: 24).isActive = true
        let spacer = NSView(); spacer.setContentHuggingPriority(.defaultLow, for: .horizontal)
        let bottom = NSStackView(views: [github, spacer, label("v\(AppInfo.version)", size: 11, color: .tertiaryLabelColor)])
        root.addArrangedSubview(bottom); fillWidth(bottom)
        for (view, key) in [(time as NSView, "time"), (resolution as NSView, "resolution"), (retention as NSView, "retention")] {
            view.setAccessibilityLabel(L(key))
        }
        root.layoutSubtreeIfNeeded()
        window.setContentSize(NSSize(width: 460, height: ceil(root.fittingSize.height) + 40))
        window.center(); window.makeKeyAndOrderFront(nil); NSApp.activate(ignoringOtherApps: true)
    }
    private func current() throws -> Settings {
        let c = Calendar.current.dateComponents([.hour, .minute], from: time.dateValue)
        guard let days = Int(retention.stringValue), (1...365).contains(days) else { throw AppError(L("invalidRetention")) }
        return Settings(dailyEnabled: daily.state == .on, hour: c.hour!, minute: c.minute!,
                        resolution: resolution.titleOfSelectedItem ?? "4K", retentionDays: days)
    }
    private func refresh() {
        let s = try? store.state()
        let date = s?.lastSuccess.map { DateFormatter.localizedString(from: $0, dateStyle: .medium, timeStyle: .none) } ?? L("never")
        last.stringValue = "\(L("last"))\(date)"
        if !busy, let retry = s?.retryAt, retry > Date() {
            status.stringValue = "\(L("retry"))\(DateFormatter.localizedString(from: retry, dateStyle: .none, timeStyle: .short))"
        } else if !busy && s?.lastSuccessDay == dayKey() {
            status.stringValue = L("upToDate")
        }
    }
    private func runWorker(_ argument: String) throws {
        let p = Process(); p.executableURL = worker; p.arguments = [argument]
        p.standardOutput = FileHandle.nullDevice; p.standardError = FileHandle.nullDevice
        try p.run(); p.waitUntilExit()
        if p.terminationStatus == 75 { throw AppError(L("alreadyRunning")) }
        guard p.terminationStatus == 0 else { throw AppError(L("operationFailed")) }
    }
    private func perform(_ operation: @escaping () throws -> Void, completion: (() -> Void)? = nil) {
        guard !busy else { return }
        busy = true; status.stringValue = L("working")
        actions.forEach { $0.isEnabled = false }
        [daily as NSControl, time as NSControl, resolution as NSControl, retention as NSControl].forEach { $0.isEnabled = false }
        DispatchQueue.global(qos: .userInitiated).async {
            let result = Result { try operation() }
            DispatchQueue.main.async {
                self.busy = false
                self.actions.forEach { $0.isEnabled = true }
                [self.daily as NSControl, self.time as NSControl, self.resolution as NSControl, self.retention as NSControl].forEach { $0.isEnabled = true }
                switch result {
                case .success: self.status.stringValue = L("done"); completion?()
                case .failure(let error): self.status.stringValue = L("failed"); self.show(error)
                }
                self.refresh()
            }
        }
    }
    private func saveSettings(completion: (() -> Void)? = nil) {
        do {
            let settings = try current()
            perform({
                let lock = try FileLock(self.store.root.appendingPathComponent("worker.lock"), wait: true)
                try self.store.write(settings, "settings.json")
                withExtendedLifetime(lock) {} // Release before configuring in the separate worker.
            }, completion: { self.perform({ try self.runWorker("--configure") }, completion: {
                self.saved = settings; completion?()
            }) })
        } catch { show(error) }
    }
    @objc private func save() { saveSettings() }
    @objc private func update() { perform { try self.runWorker("--manual") } }
    @objc private func openFolder() { NSWorkspace.shared.open(store.root) }
    @objc private func openGitHub() { NSWorkspace.shared.open(AppInfo.github) }
    private func show(_ error: Error) {
        let a = NSAlert(); a.messageText = L("error"); a.informativeText = error.localizedDescription
        a.beginSheetModal(for: window)
    }
    func windowShouldClose(_ sender: NSWindow) -> Bool {
        if busy { NSSound.beep(); return false }
        if (try? current()) != saved {
            let a = NSAlert(); a.messageText = L("unsaved"); a.addButton(withTitle: L("saveExit")); a.addButton(withTitle: L("cancel"))
            if a.runModal() == .alertFirstButtonReturn { saveSettings { NSApp.terminate(nil) } }
            return false
        }
        return true
    }
    func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply {
        windowShouldClose(window) ? .terminateNow : .terminateCancel
    }
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }
    func applicationShouldHandleReopen(_ sender: NSApplication, hasVisibleWindows flag: Bool) -> Bool {
        window.makeKeyAndOrderFront(nil); return true
    }
}
