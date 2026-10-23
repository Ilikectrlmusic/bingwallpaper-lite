import Foundation
import ImageIO

final class Downloader: NSObject, URLSessionDataDelegate {
    private var bytes = Data()
    private var failure: Error?
    private var limit = 0
    private let done = DispatchSemaphore(value: 0)
    private var session: URLSession?
    private var task: URLSessionDataTask?

    func fetch(_ url: URL, limit: Int) throws -> Data {
        self.limit = limit
        let config = URLSessionConfiguration.ephemeral
        config.timeoutIntervalForRequest = 20
        config.timeoutIntervalForResource = 90
        config.httpAdditionalHeaders = ["User-Agent": "BingWallpaperLite/1.0.0"]
        let session = URLSession(configuration: config, delegate: self, delegateQueue: nil)
        self.session = session
        let task = session.dataTask(with: url)
        self.task = task
        defer { task.cancel(); session.invalidateAndCancel(); self.session = nil; self.task = nil }
        task.resume()
        guard done.wait(timeout: .now() + 95) == .success else { throw AppError("Download timed out") }
        if let failure { throw failure }
        guard !bytes.isEmpty else { throw AppError("Empty response") }
        return bytes
    }
    func urlSession(_ session: URLSession, dataTask: URLSessionDataTask, didReceive response: URLResponse,
                    completionHandler: @escaping (URLSession.ResponseDisposition) -> Void) {
        guard let http = response as? HTTPURLResponse, http.statusCode == 200,
              response.url?.scheme == "https", response.expectedContentLength <= Int64(limit) else {
            failure = AppError("Invalid HTTP status or response size")
            completionHandler(.cancel); return
        }
        completionHandler(.allow)
    }
    func urlSession(_ session: URLSession, dataTask: URLSessionDataTask, didReceive data: Data) {
        if bytes.count + data.count > limit { failure = AppError("Response too large"); dataTask.cancel() }
        else { bytes.append(data) }
    }
    func urlSession(_ session: URLSession, task: URLSessionTask, willPerformHTTPRedirection response: HTTPURLResponse,
                    newRequest request: URLRequest, completionHandler: @escaping (URLRequest?) -> Void) {
        completionHandler(request.url?.scheme == "https" && request.url?.host == "www.bing.com" ? request : nil)
    }
    func urlSession(_ session: URLSession, task: URLSessionTask, didCompleteWithError error: Error?) {
        if failure == nil { failure = error }
        done.signal()
    }
}

struct BingImage {
    struct Archive: Decodable {
        struct Image: Decodable { let urlbase: String }
        let images: [Image]
    }
    static func imageURL(metadata: Data, resolution: String) throws -> URL {
        let archive = try JSONDecoder().decode(Archive.self, from: metadata)
        guard let base = archive.images.first?.urlbase,
              base.hasPrefix("/th?id=OHR."),
              base.range(of: #"^/th\?id=OHR\.[A-Za-z0-9_\-]+$"#, options: .regularExpression) != nil,
              let url = URL(string: "https://www.bing.com\(base)_\(resolution == "4K" ? "UHD" : "1920x1080").jpg") else {
            throw AppError("Invalid Bing image URL")
        }
        return url
    }
    static func validate(_ data: Data, resolution: String) throws {
        guard data.count > 4, data.starts(with: [0xff, 0xd8]), data.suffix(2).elementsEqual([0xff, 0xd9]),
              let source = CGImageSourceCreateWithData(data as CFData, nil),
              CGImageSourceGetStatus(source) == .statusComplete,
              let image = CGImageSourceCreateImageAtIndex(source, 0, [kCGImageSourceShouldCacheImmediately: true] as CFDictionary),
              CGImageSourceGetStatusAtIndex(source, 0) == .statusComplete,
              image.width == (resolution == "4K" ? 3840 : 1920),
              image.height == (resolution == "4K" ? 2160 : 1080) else {
            throw AppError("Image is incomplete or does not match the requested resolution")
        }
    }
    static func download(resolution: String) throws -> Data {
        let url = URL(string: "https://www.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1&mkt=zh-CN")!
        let metadata = try Downloader().fetch(url, limit: 65536)
        let imageURL = try imageURL(metadata: metadata, resolution: resolution)
        let data = try Downloader().fetch(imageURL, limit: 25 * 1024 * 1024)
        try validate(data, resolution: resolution)
        return data
    }
}
