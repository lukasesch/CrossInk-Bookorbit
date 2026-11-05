#pragma once
#include <HalStorage.h>
#include <Stream.h>

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "HttpRequestDiagnostics.h"

/**
 * HTTP client utility for fetching content and downloading files.
 * Streams requests through the configured HTTP transport so large downloads
 * do not need to fit in RAM.
 */
class HttpDownloader {
 public:
  using Header = std::pair<std::string, std::string>;
  using ProgressCallback = std::function<void(size_t downloaded, size_t total)>;
  using CancelCallback = std::function<bool()>;
  // Called with each body chunk as it arrives; return false to abort. Lets a
  // streaming parser consume the response without buffering the whole body.
  using DataCallback = std::function<bool(const uint8_t* data, size_t len)>;

  enum DownloadError {
    OK = 0,
    HTTP_ERROR,
    FILE_ERROR,
    ABORTED,
    INSUFFICIENT_SPACE,
    UNAUTHORIZED,
    FILE_EXISTS,  // Resolved destination needs explicit replacement approval.
  };

  enum class Transport {
    ESP_HTTP,
    WOLFSSL,
  };

  using HeaderList = std::vector<std::pair<std::string, std::string>>;

  struct DownloadOptions {
    explicit DownloadOptions(bool preservePartial = false, bool resumePartial = false,
                             CancelCallback shouldCancel = nullptr, size_t bufferSize = 0,
                             Transport transport = Transport::ESP_HTTP)
        : preservePartial(preservePartial),
          resumePartial(resumePartial),
          shouldCancel(std::move(shouldCancel)),
          bufferSize(bufferSize),
          transport(transport) {}

    // Borrowed for the synchronous call; never forwarded to a different origin.
    const std::vector<Header>* headers = nullptr;
    bool preservePartial;
    bool resumePartial;
    CancelCallback shouldCancel;
    size_t bufferSize;
    Transport transport;

    // Extra request headers, for APIs that authenticate with them (BookOrbit's
    // x-auth-user/x-auth-key). Treated as credentials: see authorizationOrigin.
    HeaderList extraHeaders;

    // esp_http_client's own RX buffer (0 = its default). Worth shrinking for TLS servers on
    // a fragmented heap: body bytes arriving with the headers are cached via realloc in
    // steps of this size, competing with the TLS record buffer for the largest free block.
    size_t clientRxBufferSize = 0;

    // Borrowed only for this synchronous request. Credentials -- Basic auth and
    // extraHeaders alike -- are sent only to this origin; empty keeps the request
    // URL as the credential origin, so a redirect to another host never sees them.
    std::string_view authorizationOrigin;
    // Download to "<destPath>.part" and rename it over destPath only once the
    // transfer succeeds, so a failed or interrupted download never replaces
    // (or deletes) an existing file and never leaves a truncated one behind
    // under the real name.
    bool stageAsPart = false;
    // Opt-in asset contract: success also requires deleting the transaction backup.
    bool requireBackupCleanup = false;
    // Checks the finished file before it is accepted (and, with stageAsPart,
    // before it replaces destPath). Returning false fails the download and
    // removes the file. Catches bodies cut short without a Content-Length.
    bool (*validate)(const std::string& path) = nullptr;
    // Optional context-aware check, also run before publication. Both checks
    // must pass when both are supplied. Context is borrowed for this call.
    bool (*validateWithContext)(const std::string& path, void* context) = nullptr;
    void* validationContext = nullptr;
    // Hard limits include any resumed bytes. Zero leaves the corresponding
    // limit unspecified. Enforced on declared totals and before every body
    // write/callback, including chunked or close-delimited responses.
    size_t maxBytes = 0;
    size_t expectedBytes = 0;
    // Number of redirects after the initial request; zero prohibits redirects.
    uint8_t maxRedirects = 4;
    // Optional fixed endpoint policy. Checked before contacting the initial
    // URL and every redirect target, with an absolute URL. This is a routing
    // restriction, not TLS authentication (WOLFSSL currently uses insecure TLS).
    bool (*allowUrl)(std::string_view url, void* context) = nullptr;
    void* urlContext = nullptr;
    // Once the response length is known, fail with INSUFFICIENT_SPACE before
    // writing anything if the SD card cannot hold the file. The first check
    // can scan the whole FAT, so leave it off for small files.
    bool checkFreeSpace = false;
    // Opt in only for bulk WOLFSSL transfers. Reserve after request text is
    // freed; other transports and SDK builds without retention ignore this.
    bool reserveFullRecord = false;
    // OPDS EPUBs only: choose the final response's safe Content-Disposition
    // basename, falling back to destPath. Requires stageAsPart. A collision
    // returns FILE_EXISTS before touching the partial or complete book.
    bool useServerFilename = false;
    // Borrowed for this synchronous call; approval applies to exactly one path.
    std::string_view overwriteApprovedPath;
    // Receives the chosen path, including when FILE_EXISTS requests approval.
    std::string* resolvedPath = nullptr;
  };

  /**
   * Fetch text content from a URL with optional credentials.
   */
  static bool fetchUrl(const std::string& url, std::string& outContent, const std::string& username = "",
                       const std::string& password = "");

  static bool fetchUrl(const std::string& url, Stream& stream, const std::string& username = "",
                       const std::string& password = "");

  /**
   * Stream the response body to onData as it arrives, without buffering it.
   */
  static bool fetchUrl(const std::string& url, const DataCallback& onData, const std::string& username = "",
                       const std::string& password = "");

  /**
   * Stream a URL with cancellation/progress support and a detailed result.
   * Optional diagnostics are reset on entry and borrowed only during this call.
   */
  static DownloadError streamUrl(const std::string& url, const DataCallback& onData,
                                 ProgressCallback progress = nullptr, const std::string& username = "",
                                 const std::string& password = "", DownloadOptions options = DownloadOptions(),
                                 HttpRequestDiagnostics* diagnostics = nullptr);

  /**
   * Download a file to the SD card with optional credentials.
   */
  /**
   * HTTP status code from the last request, or 0 when no response line was received.
   * Diagnostic only, so callers can tell a 404 (wrong path or server version) from a
   * transport failure.
   */
  static int lastHttpStatus;

  static DownloadError downloadToFile(const std::string& url, const std::string& destPath,
                                      ProgressCallback progress = nullptr, bool* cancelFlag = nullptr,
                                      const std::string& username = "", const std::string& password = "",
                                      DownloadOptions options = DownloadOptions());
};
