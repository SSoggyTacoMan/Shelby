#include <cstdio>
#include <cstring>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "Display.hpp"

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

namespace {

void init_wifi() {
    if (cyw43_arch_init()) {
        printf("CYW43 init failed\n");
        return;
    }

    cyw43_arch_enable_sta_mode();

    if (std::strlen(WIFI_SSID) == 0) {
        printf("Wi-Fi credentials not set; skipping connect\n");
        return;
    }

    printf("Connecting to Wi-Fi SSID: %s\n", WIFI_SSID);
    const int connect_rc = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,
        30000
    );

    if (connect_rc) {
        printf("Wi-Fi connect failed: %d\n", connect_rc);
    } else {
        printf("Wi-Fi connected\n");
    }
}

}  // namespace

int main() {
    stdio_init_all();
    sleep_ms(2000);

    printf("Shelby native bootstrap (Step 2)\n");

    Display display;
    display.init();
    display.drawTextOnBg(
        "SHELBY OS",
        8,
        8,
        Display::rgb565(255, 255, 255),
        Display::rgb565(0, 0, 0)
    );
    init_wifi();

    while (true) {
        cyw43_arch_poll();
        sleep_ms(10);
    }
}
