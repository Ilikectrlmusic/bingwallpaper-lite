#include "common.h"
#include <winhttp.h>
#include <wincodec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define MAX_IMAGE (25u * 1024u * 1024u)
static BOOL hex4(const char **cursor, unsigned *value) {
    *value = 0;
    for (int i = 0; i < 4; ++i) {
        unsigned char c = (unsigned char)*(*cursor)++;
        if (c >= '0' && c <= '9') *value = *value * 16 + c - '0';
        else if (c >= 'a' && c <= 'f') *value = *value * 16 + c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') *value = *value * 16 + c - 'A' + 10;
        else return FALSE;
    }
    return TRUE;
}
/* Bounded JSON string reader. Handles the escapes used by Bing; does not execute JSON. */
static const char *json_string(const char *p, char *out, size_t cap) {
    size_t n = 0;
    if (*p++ != '"') return NULL;
    while (*p && *p != '"') {
        unsigned char c = (unsigned char)*p++;
        if (c < 0x20) return NULL;
        if (c == '\\') {
            c = (unsigned char)*p++;
            if (c == 'u') {
                unsigned value;
                if (!hex4(&p, &value) || !value) return NULL;
                if (value >= 0xd800 && value <= 0xdbff) {
                    unsigned low;
                    if (p[0] != '\\' || p[1] != 'u') return NULL;
                    p += 2;
                    if (!hex4(&p, &low) || low < 0xdc00 || low > 0xdfff) return NULL;
                    value = 0x10000 + ((value - 0xd800) << 10) + low - 0xdc00;
                } else if (value >= 0xdc00 && value <= 0xdfff) return NULL;
                if (value > 127) {
                    unsigned char utf8[4]; size_t count;
                    if (value < 0x800) { count = 2; utf8[0] = 0xc0 | (value >> 6); utf8[1] = 0x80 | (value & 63); }
                    else if (value < 0x10000) { count = 3; utf8[0] = 0xe0 | (value >> 12); utf8[1] = 0x80 | ((value >> 6) & 63); utf8[2] = 0x80 | (value & 63); }
                    else { count = 4; utf8[0] = 0xf0 | (value >> 18); utf8[1] = 0x80 | ((value >> 12) & 63); utf8[2] = 0x80 | ((value >> 6) & 63); utf8[3] = 0x80 | (value & 63); }
                    if (n + count >= cap) return NULL;
                    memcpy(out + n, utf8, count); n += count; continue;
                }
                c = (unsigned char)value;
            } else if (c == 'n') c = '\n'; else if (c == 'r') c = '\r'; else if (c == 't') c = '\t';
            else if (c == 'b') c = '\b'; else if (c == 'f') c = '\f';
            else if (c != '"' && c != '\\' && c != '/') return NULL;
        }
        if (n + 1 >= cap) return NULL;
        out[n++] = (char)c;
    }
    if (*p != '"') return NULL;
    out[n] = 0; return p + 1;
}
BOOL json_urlbase(const char *json, wchar_t *out, size_t cap) {
    const char *p = json; char key[4096], value[4096];
    while (*p) {
        if (*p != '"') { ++p; continue; }
        const char *next = json_string(p, key, sizeof(key));
        if (!next) return FALSE;
        p = next;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
        if (strcmp(key, "urlbase") || *p != ':') continue;
        ++p; while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
        if (!json_string(p, value, sizeof(value)) || strncmp(value, "/th?id=OHR.", 11) != 0) return FALSE;
        for (const char *q = value; *q; ++q) if ((unsigned char)*q < 33 || (unsigned char)*q > 126 || *q == '"' || *q == '<' || *q == '>' || *q == '\\') return FALSE;
        return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, out, (int)cap) > 0;
    }
    return FALSE;
}
/* Synchronous WinHTTP lives only in the short-lived worker. No helper shell or service. */
static BOOL get_https(const App *a, const wchar_t *path, BYTE **memory, DWORD *size, const wchar_t *file, DWORD limit) {
    HINTERNET session = NULL, connect = NULL, request = NULL; HANDLE output = INVALID_HANDLE_VALUE;
    BYTE *data = NULL; DWORD total = 0; BOOL ok = FALSE;
    ULONGLONG deadline = GetTickCount64() + 90000;
    session = WinHttpOpen(L"BingWallpaperLite/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) goto done;
    WinHttpSetTimeouts(session, 8000, 10000, 10000, 15000);
    connect = WinHttpConnect(session, L"www.bing.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connect) goto done;
    request = WinHttpOpenRequest(connect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request) goto done;
    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect));
    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(request, NULL)) goto done;
    DWORD status = 0, n = sizeof(status);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &n, WINHTTP_NO_HEADER_INDEX) || status != 200) {
        log_line(a, L"HTTP status %lu", status); goto done;
    }
    DWORD expected = 0; n = sizeof(expected);
    BOOL have_length = WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &expected, &n, WINHTTP_NO_HEADER_INDEX);
    if (have_length && expected > limit) goto done;
    if (file) {
        output = CreateFileW(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (output == INVALID_HANDLE_VALUE) goto done;
    } else { data = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)limit + 1); if (!data) goto done; }
    for (;;) {
        BYTE buffer[32768]; DWORD got = 0, written;
        if (GetTickCount64() > deadline || !WinHttpReadData(request, buffer, sizeof(buffer), &got)) goto done;
        if (!got) break;
        if (got > limit - total) goto done;
        if (file) {
            if (!WriteFile(output, buffer, got, &written, NULL) || written != got) goto done;
        } else memcpy(data + total, buffer, got);
        total += got;
    }
    if (!total || (have_length && total != expected)) goto done;
    if (file) { if (!FlushFileBuffers(output)) goto done; }
    else { data[total] = 0; *memory = data; data = NULL; *size = total; }
    ok = TRUE;
done:
    if (!ok) log_line(a, L"Download failed (Windows error %lu); scheduled retry will run after exit.", GetLastError());
    if (output != INVALID_HANDLE_VALUE) CloseHandle(output);
    if (data) HeapFree(GetProcessHeap(), 0, data);
    if (request) WinHttpCloseHandle(request);
    if (connect) WinHttpCloseHandle(connect);
    if (session) WinHttpCloseHandle(session);
    return ok;
}
static BOOL decode_image(const wchar_t *path, int resolution) {
    HRESULT init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    IWICImagingFactory *factory = NULL; IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL; IWICFormatConverter *converter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void **)&factory);
    if (SUCCEEDED(hr)) hr = IWICImagingFactory_CreateDecoderFromFilename(factory, path, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    if (SUCCEEDED(hr)) hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    UINT w = 0, h = 0;
    if (SUCCEEDED(hr)) hr = IWICBitmapFrameDecode_GetSize(frame, &w, &h);
    if (SUCCEEDED(hr) && (w < 640 || h < 360 || w > 3840 || h > 2160)) hr = E_FAIL;
    if (SUCCEEDED(hr) && resolution &&
        (h != (UINT)resolution || w != (resolution == RESOLUTION_4K ? 3840U : 1920U))) hr = E_FAIL;
    if (SUCCEEDED(hr)) hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    if (SUCCEEDED(hr)) hr = IWICFormatConverter_Initialize(converter, (IWICBitmapSource *)frame,
        &GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0, WICBitmapPaletteTypeCustom);
    if (SUCCEEDED(hr)) {
        UINT bytes = w * h * 4; BYTE *pixels = HeapAlloc(GetProcessHeap(), 0, bytes);
        if (!pixels) hr = E_OUTOFMEMORY;
        else { hr = IWICFormatConverter_CopyPixels(converter, NULL, w * 4, bytes, pixels); HeapFree(GetProcessHeap(), 0, pixels); }
    }
    if (converter) IWICFormatConverter_Release(converter);
    if (frame) IWICBitmapFrameDecode_Release(frame);
    if (decoder) IWICBitmapDecoder_Release(decoder);
    if (factory) IWICImagingFactory_Release(factory);
    if (SUCCEEDED(init)) CoUninitialize();
    return SUCCEEDED(hr);
}
BOOL valid_image(const wchar_t *path) { return decode_image(path, 0); }
BOOL download_image(const App *a, const Settings *s, wchar_t *out, size_t cap) {
    BYTE *metadata = NULL; DWORD bytes = 0; wchar_t base[4096], remote[4600], date[16], name[48], part[PATH_CAP];
    if (!get_https(a, L"/HPImageArchive.aspx?format=js&idx=0&n=1&mkt=zh-CN", &metadata, &bytes, NULL, 65536)) return FALSE;
    BOOL parsed = json_urlbase((const char *)metadata, base, 4096);
    HeapFree(GetProcessHeap(), 0, metadata);
    if (!parsed) { log_line(a, L"Bing metadata missing a valid urlbase."); return FALSE; }
    local_date(date); swprintf(name, 48, L"%ls.jpg", date);
    if (!path_join(out, cap, a->cache, name)) return FALSE;
    swprintf(name, 48, L"%ls.part", date);
    BOOL uhd = s->resolution == RESOLUTION_4K;
    if (!path_join(part, PATH_CAP, a->cache, name) ||
        swprintf(remote, 4600, L"%ls_%ls.jpg", base, uhd ? L"UHD" : L"1920x1080") < 0) return FALSE;
    log_line(a, L"Downloading wallpaper: %ls", uhd ? L"4K (3840x2160)" : L"1080P (1920x1080)");
    if (!get_https(a, remote, NULL, NULL, part, MAX_IMAGE) ||
        !decode_image(part, uhd ? RESOLUTION_4K : RESOLUTION_1080P)) {
        log_line(a, L"Image transfer, requested resolution or full WIC decode failed; keeping the existing wallpaper."); return FALSE;
    }
    if (!MoveFileExW(part, out, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return FALSE;
    return TRUE;
}
