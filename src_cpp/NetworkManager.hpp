#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class NetworkManager {
public:
    struct NtpConfig {
        std::array<const char*, 3> hosts = {"time.google.com", "pool.ntp.org", "time.cloudflare.com"};
        int timezone_offset_seconds = 0;
        uint32_t timeout_ms = 5000;
    };

    struct HttpsRequest {
        const char* host = nullptr;                // e.g. "api.github.com"
        uint16_t port = 443;
        const char* method = "GET";              // "GET" / "POST"
        const char* path = "/";                  // e.g. "/graphql"
        const char* extra_headers = nullptr;      // CRLF-separated key/value lines; no trailing CRLF required
        const uint8_t* body = nullptr;            // optional payload
        size_t body_size = 0;
        uint32_t timeout_ms = 15000;
        size_t max_body_bytes = 16 * 1024;        // safety cap for RAM
    };

    struct HttpsResponse {
        int status_code = -1;
        size_t body_bytes = 0;
        bool success = false;
    };

    using BodyChunkCallback = bool (*)(const uint8_t* data, size_t len, void* user);

    NetworkManager() = default;

    bool init();
    void shutdown();

    bool connectWiFi(const char* ssid, const char* password, uint32_t timeout_ms = 15000);
    bool isWiFiConnected() const;

    // Call from Spade main loop when using poll-based CYW43 mode.
    void poll();

    // Sync RTC from NTP (UTC + timezone offset).
    bool syncTimeNtp(const NtpConfig& cfg = NtpConfig{});

    // Minimal streaming HTTPS primitive for GitHub/Hackatime APIs.
    HttpsResponse httpsRequest(const HttpsRequest& req,
                               BodyChunkCallback on_chunk = nullptr,
                               void* user = nullptr);

private:
    bool initialized_ = false;

    static bool parseHttpStatus(const char* line, int* status_code);
};
