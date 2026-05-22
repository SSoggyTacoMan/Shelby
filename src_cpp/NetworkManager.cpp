#include "NetworkManager.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <strings.h>

#include "hardware/rtc.h"
#include "lwip/sockets.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

namespace {

constexpr uint32_t kWifiPollIntervalMs = 100;
constexpr uint32_t kNtpUnixDelta = 2208988800u;
constexpr size_t kIoBufSize = 512;

bool epochToDatetime(time_t epoch, datetime_t* out) {
    if (out == nullptr) {
        return false;
    }

    struct tm tm_buf;
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    if (gmtime_r(&epoch, &tm_buf) == nullptr) {
        return false;
    }
#else
    const struct tm* t = gmtime(&epoch);
    if (t == nullptr) {
        return false;
    }
    tm_buf = *t;
#endif

    out->year = static_cast<int16_t>(tm_buf.tm_year + 1900);
    out->month = static_cast<int8_t>(tm_buf.tm_mon + 1);
    out->day = static_cast<int8_t>(tm_buf.tm_mday);
    out->dotw = static_cast<int8_t>((tm_buf.tm_wday + 6) % 7); // RP2040: 0=Mon
    out->hour = static_cast<int8_t>(tm_buf.tm_hour);
    out->min = static_cast<int8_t>(tm_buf.tm_min);
    out->sec = static_cast<int8_t>(tm_buf.tm_sec);
    return true;
}

}  // namespace

bool NetworkManager::init() {
    if (initialized_) {
        return true;
    }

    if (cyw43_arch_init()) {
        return false;
    }

    cyw43_arch_enable_sta_mode();
    initialized_ = true;
    return true;
}

void NetworkManager::shutdown() {
    if (!initialized_) {
        return;
    }

    cyw43_arch_deinit();
    initialized_ = false;
}

bool NetworkManager::connectWiFi(const char* ssid, const char* password, uint32_t timeout_ms) {
    if (!initialized_ || ssid == nullptr || password == nullptr || std::strlen(ssid) == 0) {
        return false;
    }

    if (isWiFiConnected()) {
        return true;
    }

    const int rc = cyw43_arch_wifi_connect_timeout_ms(
        ssid,
        password,
        CYW43_AUTH_WPA2_AES_PSK,
        timeout_ms
    );
    return rc == 0;
}

bool NetworkManager::isWiFiConnected() const {
    if (!initialized_) {
        return false;
    }

    return cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) >= CYW43_LINK_UP;
}

void NetworkManager::poll() {
    if (initialized_) {
        cyw43_arch_poll();
    }
}

bool NetworkManager::syncTimeNtp(const NtpConfig& cfg) {
    if (!initialized_ || !isWiFiConnected()) {
        return false;
    }

    for (const char* host : cfg.hosts) {
        if (host == nullptr || *host == '\0') {
            continue;
        }

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        addrinfo* result = nullptr;
        if (getaddrinfo(host, "123", &hints, &result) != 0 || result == nullptr) {
            continue;
        }

        int sock = lwip_socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sock < 0) {
            freeaddrinfo(result);
            continue;
        }

        timeval tv{};
        tv.tv_sec = static_cast<long>(cfg.timeout_ms / 1000);
        tv.tv_usec = static_cast<long>((cfg.timeout_ms % 1000) * 1000);
        lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        lwip_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        std::array<uint8_t, 48> packet{};
        packet[0] = 0x1B;

        const int sent = lwip_sendto(sock,
                                     packet.data(),
                                     packet.size(),
                                     0,
                                     result->ai_addr,
                                     static_cast<socklen_t>(result->ai_addrlen));
        if (sent == static_cast<int>(packet.size())) {
            sockaddr_storage src{};
            socklen_t src_len = sizeof(src);
            const int recvd = lwip_recvfrom(sock,
                                            packet.data(),
                                            packet.size(),
                                            0,
                                            reinterpret_cast<sockaddr*>(&src),
                                            &src_len);
            if (recvd >= 48) {
                const uint32_t ntp_secs =
                    (static_cast<uint32_t>(packet[40]) << 24) |
                    (static_cast<uint32_t>(packet[41]) << 16) |
                    (static_cast<uint32_t>(packet[42]) << 8) |
                    static_cast<uint32_t>(packet[43]);

                if (ntp_secs > kNtpUnixDelta) {
                    const time_t unix_time = static_cast<time_t>(
                        static_cast<int64_t>(ntp_secs - kNtpUnixDelta) + cfg.timezone_offset_seconds
                    );

                    datetime_t dt{};
                    if (epochToDatetime(unix_time, &dt)) {
                        rtc_set_datetime(&dt);
                        lwip_close(sock);
                        freeaddrinfo(result);
                        return true;
                    }
                }
            }
        }

        lwip_close(sock);
        freeaddrinfo(result);
        sleep_ms(kWifiPollIntervalMs);
    }

    return false;
}

bool NetworkManager::parseHttpStatus(const char* line, int* status_code) {
    if (line == nullptr || status_code == nullptr) {
        return false;
    }

    // Expected: HTTP/1.1 200 OK
    const char* sp1 = std::strchr(line, ' ');
    if (sp1 == nullptr) {
        return false;
    }
    const int code = std::atoi(sp1 + 1);
    if (code <= 0) {
        return false;
    }

    *status_code = code;
    return true;
}

NetworkManager::HttpsResponse NetworkManager::httpsRequest(const HttpsRequest& req,
                                                           BodyChunkCallback on_chunk,
                                                           void* user) {
    HttpsResponse out{};

    if (!initialized_ || !isWiFiConnected() || req.host == nullptr || req.path == nullptr || req.method == nullptr) {
        return out;
    }

    mbedtls_net_context net;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    mbedtls_net_init(&net);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    const char* pers = "shelby_net";
    int rc = mbedtls_ctr_drbg_seed(&ctr_drbg,
                                   mbedtls_entropy_func,
                                   &entropy,
                                   reinterpret_cast<const unsigned char*>(pers),
                                   std::strlen(pers));
    if (rc != 0) {
        goto cleanup;
    }

    char port_str[8];
    std::snprintf(port_str, sizeof(port_str), "%u", static_cast<unsigned>(req.port));

    rc = mbedtls_net_connect(&net, req.host, port_str, MBEDTLS_NET_PROTO_TCP);
    if (rc != 0) {
        goto cleanup;
    }

    rc = mbedtls_ssl_config_defaults(&conf,
                                     MBEDTLS_SSL_IS_CLIENT,
                                     MBEDTLS_SSL_TRANSPORT_STREAM,
                                     MBEDTLS_SSL_PRESET_DEFAULT);
    if (rc != 0) {
        goto cleanup;
    }

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    rc = mbedtls_ssl_setup(&ssl, &conf);
    if (rc != 0) {
        goto cleanup;
    }

    rc = mbedtls_ssl_set_hostname(&ssl, req.host);
    if (rc != 0) {
        goto cleanup;
    }

    mbedtls_ssl_set_bio(&ssl, &net, mbedtls_net_send, mbedtls_net_recv, nullptr);

    do {
        rc = mbedtls_ssl_handshake(&ssl);
        if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
            poll();
            continue;
        }
        if (rc != 0) {
            goto cleanup;
        }
    } while (rc != 0);

    char request_head[1024];
    const int head_len = std::snprintf(
        request_head,
        sizeof(request_head),
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: Shelby-Spade\r\n"
        "Connection: close\r\n"
        "%s"
        "%s"
        "Content-Length: %u\r\n"
        "\r\n",
        req.method,
        req.path,
        req.host,
        (req.extra_headers != nullptr) ? req.extra_headers : "",
        (req.extra_headers != nullptr && std::strlen(req.extra_headers) > 0 &&
         req.extra_headers[std::strlen(req.extra_headers) - 1] != '\n')
            ? "\r\n"
            : "",
        static_cast<unsigned>(req.body_size)
    );

    if (head_len <= 0 || static_cast<size_t>(head_len) >= sizeof(request_head)) {
        goto cleanup;
    }

    size_t written = 0;
    while (written < static_cast<size_t>(head_len)) {
        rc = mbedtls_ssl_write(&ssl,
                               reinterpret_cast<const unsigned char*>(request_head + written),
                               static_cast<size_t>(head_len) - written);
        if (rc > 0) {
            written += static_cast<size_t>(rc);
            continue;
        }
        if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
            poll();
            continue;
        }
        goto cleanup;
    }

    written = 0;
    while (req.body != nullptr && written < req.body_size) {
        const size_t remain = req.body_size - written;
        const size_t chunk = std::min(remain, static_cast<size_t>(256));
        rc = mbedtls_ssl_write(&ssl, req.body + written, chunk);
        if (rc > 0) {
            written += static_cast<size_t>(rc);
            continue;
        }
        if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
            poll();
            continue;
        }
        goto cleanup;
    }

    std::array<char, kIoBufSize> rx{};
    bool parsed_headers = false;
    int content_length = -1;
    bool chunked = false;
    std::array<char, 1024> header_buf{};
    size_t header_len = 0;

    while (true) {
        rc = mbedtls_ssl_read(&ssl, reinterpret_cast<unsigned char*>(rx.data()), rx.size());
        if (rc == 0) {
            break;
        }
        if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
            poll();
            continue;
        }
        if (rc < 0) {
            goto cleanup;
        }

        const char* data = rx.data();
        size_t data_len = static_cast<size_t>(rc);

        if (!parsed_headers) {
            const size_t copy_len = std::min(data_len, header_buf.size() - header_len - 1);
            std::memcpy(header_buf.data() + header_len, data, copy_len);
            header_len += copy_len;
            header_buf[header_len] = '\0';

            char* headers_end = std::strstr(header_buf.data(), "\r\n\r\n");
            if (headers_end == nullptr) {
                if (header_len == header_buf.size() - 1) {
                    goto cleanup;
                }
                continue;
            }

            parsed_headers = true;

            char* line_end = std::strstr(header_buf.data(), "\r\n");
            if (line_end == nullptr) {
                goto cleanup;
            }
            *line_end = '\0';
            if (!parseHttpStatus(header_buf.data(), &out.status_code)) {
                goto cleanup;
            }
            *line_end = '\r';

            const char* headers_scan = line_end + 2;
            while (headers_scan < headers_end) {
                const char* next = std::strstr(headers_scan, "\r\n");
                if (next == nullptr) {
                    break;
                }
                const size_t line_sz = static_cast<size_t>(next - headers_scan);

                if (line_sz >= 16 && strncasecmp(headers_scan, "Content-Length:", 15) == 0) {
                    content_length = std::atoi(headers_scan + 15);
                } else if (line_sz >= 18 && strncasecmp(headers_scan, "Transfer-Encoding:", 18) == 0) {
                    if (std::strstr(headers_scan, "chunked") != nullptr ||
                        std::strstr(headers_scan, "Chunked") != nullptr) {
                        chunked = true;
                    }
                }
                headers_scan = next + 2;
            }

            const char* body_start = headers_end + 4;
            const size_t header_bytes = static_cast<size_t>(body_start - header_buf.data());
            const size_t body_in_header_buf = (header_len > header_bytes) ? (header_len - header_bytes) : 0;
            if (body_in_header_buf > 0) {
                const size_t accepted = std::min(body_in_header_buf, req.max_body_bytes - out.body_bytes);
                if (accepted > 0 && on_chunk != nullptr) {
                    if (!on_chunk(reinterpret_cast<const uint8_t*>(body_start), accepted, user)) {
                        goto cleanup;
                    }
                }
                out.body_bytes += accepted;
            }
            continue;
        }

        const size_t accepted = std::min(data_len, req.max_body_bytes - out.body_bytes);
        if (accepted > 0 && on_chunk != nullptr) {
            if (!on_chunk(reinterpret_cast<const uint8_t*>(data), accepted, user)) {
                goto cleanup;
            }
        }
        out.body_bytes += accepted;

        if ((content_length >= 0 && out.body_bytes >= static_cast<size_t>(content_length)) ||
            out.body_bytes >= req.max_body_bytes) {
            break;
        }

        (void)chunked;
    }

    out.success = (out.status_code >= 200 && out.status_code < 300);

cleanup:
    mbedtls_ssl_close_notify(&ssl);
    mbedtls_net_free(&net);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return out;
}
