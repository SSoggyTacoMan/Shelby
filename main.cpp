#include <cstdio>
#include <cstdint>
#include <cstring>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "Display.hpp"

#if __has_include("secrets.hpp")
#include "secrets.hpp"
#define SHELBY_HAVE_WIFI_SECRETS 1
#else
#define SHELBY_HAVE_WIFI_SECRETS 0
#endif

namespace {

bool wifi_init_and_connect() {
  if (cyw43_arch_init()) {
    printf("cyw43_arch_init failed\n");
    return false;
  }

  cyw43_arch_enable_sta_mode();

  // Optional local secrets header (do not commit): define WIFI_SSID and WIFI_PASSWORD.
#if SHELBY_HAVE_WIFI_SECRETS
  const char* ssid = WIFI_SSID;
  const char* password = WIFI_PASSWORD;
#else
  const char* ssid = "";
  const char* password = "";
#endif

  if (ssid[0] == '\0') {
    printf("WiFi credentials not set (add secrets.hpp with WIFI_SSID/WIFI_PASSWORD)\n");
    return true;  // Wi-Fi stack initialized, just not connected.
  }

  printf("Connecting to WiFi...\n");
  const int err = cyw43_arch_wifi_connect_timeout_ms(
      ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30'000);
  if (err) {
    printf("WiFi connect failed: %d\n", err);
    return false;
  }

  printf("WiFi connected\n");
  return true;
}

}  // namespace

int main() {
  stdio_init_all();
  sleep_ms(200);
  printf("Shelby Native booting...\n");

  // Sprig (Pico W) ST7735 wiring (from existing MicroPython code: main.py).
  Display display(spi0, Display::Pins{
                            .sck = 18,
                            .mosi = 19,
                            .miso = 16,
                            .dc = 22,
                            .rst = 26,
                            .cs = 20,
                        });
  display.init(8'000'000);
  display.fill(Display::BLACK);

  wifi_init_and_connect();

  while (true) {
    // Blink the Pico W LED via CYW43.
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(250);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    sleep_ms(250);
  }
}
