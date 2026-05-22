#include <cstdio>
#include <cstdint>
#include <cstring>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/cyw43_arch.h"

#if __has_include("secrets.hpp")
#include "secrets.hpp"
#define SHELBY_HAVE_WIFI_SECRETS 1
#else
#define SHELBY_HAVE_WIFI_SECRETS 0
#endif

namespace {

// Sprig (Pico W) ST7735 wiring (from existing MicroPython code: main.py).
constexpr spi_inst_t* kTftSpi = spi0;
constexpr uint kPinTftSck = 18;
constexpr uint kPinTftMosi = 19;
constexpr uint kPinTftMiso = 16;  // Not used by display, but configured for SPI0.
constexpr uint kPinTftDc = 22;
constexpr uint kPinTftCs = 20;
constexpr uint kPinTftRst = 26;

constexpr uint32_t kTftSpiBaud = 8'000'000;

// Logical size after `rotation(1)` in the MicroPython firmware.
constexpr int kScreenWidth = 160;
constexpr int kScreenHeight = 128;

// ST7735 command set (subset).
constexpr uint8_t ST7735_SWRESET = 0x01;
constexpr uint8_t ST7735_SLPOUT = 0x11;
constexpr uint8_t ST7735_NORON = 0x13;
constexpr uint8_t ST7735_INVOFF = 0x20;
constexpr uint8_t ST7735_DISPON = 0x29;
constexpr uint8_t ST7735_CASET = 0x2A;
constexpr uint8_t ST7735_RASET = 0x2B;
constexpr uint8_t ST7735_RAMWR = 0x2C;
constexpr uint8_t ST7735_MADCTL = 0x36;
constexpr uint8_t ST7735_COLMOD = 0x3A;
constexpr uint8_t ST7735_FRMCTR1 = 0xB1;
constexpr uint8_t ST7735_FRMCTR2 = 0xB2;
constexpr uint8_t ST7735_FRMCTR3 = 0xB3;
constexpr uint8_t ST7735_INVCTR = 0xB4;
constexpr uint8_t ST7735_PWCTR1 = 0xC0;
constexpr uint8_t ST7735_PWCTR2 = 0xC1;
constexpr uint8_t ST7735_PWCTR3 = 0xC2;
constexpr uint8_t ST7735_PWCTR4 = 0xC3;
constexpr uint8_t ST7735_PWCTR5 = 0xC4;
constexpr uint8_t ST7735_VMCTR1 = 0xC5;
constexpr uint8_t ST7735_GMCTRP1 = 0xE0;
constexpr uint8_t ST7735_GMCTRN1 = 0xE1;

constexpr uint8_t kMadctlRot[4] = {0x00, 0x60, 0xC0, 0xA0};
constexpr uint8_t kMadctlBgr = 0x08;

inline void cs_select() { gpio_put(kPinTftCs, 0); }
inline void cs_deselect() { gpio_put(kPinTftCs, 1); }
inline void dc_command() { gpio_put(kPinTftDc, 0); }
inline void dc_data() { gpio_put(kPinTftDc, 1); }

void tft_write_cmd(uint8_t cmd) {
  dc_command();
  cs_select();
  spi_write_blocking(kTftSpi, &cmd, 1);
  cs_deselect();
}

void tft_write_data(const uint8_t* data, size_t len) {
  if (len == 0) return;
  dc_data();
  cs_select();
  spi_write_blocking(kTftSpi, data, static_cast<int>(len));
  cs_deselect();
}

void tft_reset() {
  dc_command();
  gpio_put(kPinTftRst, 1);
  sleep_us(500);
  gpio_put(kPinTftRst, 0);
  sleep_us(500);
  gpio_put(kPinTftRst, 1);
  sleep_us(500);
}

void tft_set_madctl(uint8_t rotation, bool rgb) {
  const uint8_t madctl = static_cast<uint8_t>(kMadctlRot[rotation & 0x3] | (rgb ? 0x00 : kMadctlBgr));
  tft_write_cmd(ST7735_MADCTL);
  tft_write_data(&madctl, 1);
}

void tft_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  uint8_t data[4];

  tft_write_cmd(ST7735_CASET);
  data[0] = static_cast<uint8_t>(x0 >> 8);
  data[1] = static_cast<uint8_t>(x0 & 0xFF);
  data[2] = static_cast<uint8_t>(x1 >> 8);
  data[3] = static_cast<uint8_t>(x1 & 0xFF);
  tft_write_data(data, sizeof(data));

  tft_write_cmd(ST7735_RASET);
  data[0] = static_cast<uint8_t>(y0 >> 8);
  data[1] = static_cast<uint8_t>(y0 & 0xFF);
  data[2] = static_cast<uint8_t>(y1 >> 8);
  data[3] = static_cast<uint8_t>(y1 & 0xFF);
  tft_write_data(data, sizeof(data));

  tft_write_cmd(ST7735_RAMWR);
}

void tft_fill_screen(uint16_t color565) {
  tft_set_addr_window(0, 0, kScreenWidth - 1, kScreenHeight - 1);

  uint8_t px[64 * 2];
  for (size_t i = 0; i < sizeof(px); i += 2) {
    px[i + 0] = static_cast<uint8_t>(color565 >> 8);
    px[i + 1] = static_cast<uint8_t>(color565 & 0xFF);
  }

  const int total_pixels = kScreenWidth * kScreenHeight;
  int remaining = total_pixels;

  dc_data();
  cs_select();
  while (remaining > 0) {
    const int chunk_pixels = remaining > 64 ? 64 : remaining;
    spi_write_blocking(kTftSpi, px, chunk_pixels * 2);
    remaining -= chunk_pixels;
  }
  cs_deselect();
}

// Minimal init sequence matching `st7735.TFT.initg()` used by Shelby OS.
void tft_init_green_tab() {
  tft_reset();

  tft_write_cmd(ST7735_SWRESET);
  sleep_us(150);
  tft_write_cmd(ST7735_SLPOUT);
  sleep_us(255);

  const uint8_t frmctr3[] = {0x01, 0x2C, 0x2D};
  tft_write_cmd(ST7735_FRMCTR1);
  tft_write_data(frmctr3, sizeof(frmctr3));
  tft_write_cmd(ST7735_FRMCTR2);
  tft_write_data(frmctr3, sizeof(frmctr3));

  const uint8_t frmctr6[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
  tft_write_cmd(ST7735_FRMCTR3);
  tft_write_data(frmctr6, sizeof(frmctr6));
  sleep_us(10);

  const uint8_t invctr = 0x07;
  tft_write_cmd(ST7735_INVCTR);
  tft_write_data(&invctr, 1);

  tft_write_cmd(ST7735_PWCTR1);
  const uint8_t pwctr1[] = {0xA2, 0x02, 0x84};
  tft_write_data(pwctr1, sizeof(pwctr1));

  tft_write_cmd(ST7735_PWCTR2);
  const uint8_t pwctr2 = 0xC5;
  tft_write_data(&pwctr2, 1);

  tft_write_cmd(ST7735_PWCTR3);
  const uint8_t pwctr3[] = {0x0A, 0x00};
  tft_write_data(pwctr3, sizeof(pwctr3));

  tft_write_cmd(ST7735_PWCTR4);
  const uint8_t pwctr4[] = {0x8A, 0x2A};
  tft_write_data(pwctr4, sizeof(pwctr4));

  tft_write_cmd(ST7735_PWCTR5);
  const uint8_t pwctr5[] = {0x8A, 0xEE};
  tft_write_data(pwctr5, sizeof(pwctr5));

  tft_write_cmd(ST7735_VMCTR1);
  const uint8_t vmctr1 = 0x0E;
  tft_write_data(&vmctr1, 1);

  tft_write_cmd(ST7735_INVOFF);

  // Shelby OS uses BGR + rotation(1) after init.
  tft_set_madctl(/*rotation=*/1, /*rgb=*/false);

  tft_write_cmd(ST7735_COLMOD);
  const uint8_t colmod = 0x05;  // 16-bit color (RGB565)
  tft_write_data(&colmod, 1);

  tft_write_cmd(ST7735_CASET);
  const uint8_t caset[] = {0x00, 0x01, 0x00, 0x7F};
  tft_write_data(caset, sizeof(caset));

  tft_write_cmd(ST7735_RASET);
  const uint8_t raset[] = {0x00, 0x01, 0x00, 0x9F};
  tft_write_data(raset, sizeof(raset));

  const uint8_t gmctrp1[] = {0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d,
                             0x29, 0x25, 0x2b, 0x39, 0x00, 0x01, 0x03, 0x10};
  tft_write_cmd(ST7735_GMCTRP1);
  tft_write_data(gmctrp1, sizeof(gmctrp1));

  const uint8_t gmctrn1[] = {0x03, 0x1d, 0x07, 0x06, 0x2e, 0x2c, 0x29, 0x2d,
                             0x2e, 0x2e, 0x37, 0x3f, 0x00, 0x00, 0x02, 0x10};
  tft_write_cmd(ST7735_GMCTRN1);
  tft_write_data(gmctrn1, sizeof(gmctrn1));

  tft_write_cmd(ST7735_NORON);
  sleep_us(10);

  tft_write_cmd(ST7735_DISPON);
  sleep_us(100);
}

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

  // SPI0 setup for ST7735.
  spi_init(kTftSpi, kTftSpiBaud);
  spi_set_format(kTftSpi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_set_function(kPinTftSck, GPIO_FUNC_SPI);
  gpio_set_function(kPinTftMosi, GPIO_FUNC_SPI);
  gpio_set_function(kPinTftMiso, GPIO_FUNC_SPI);

  gpio_init(kPinTftCs);
  gpio_set_dir(kPinTftCs, GPIO_OUT);
  cs_deselect();

  gpio_init(kPinTftDc);
  gpio_set_dir(kPinTftDc, GPIO_OUT);
  dc_command();

  gpio_init(kPinTftRst);
  gpio_set_dir(kPinTftRst, GPIO_OUT);

  tft_init_green_tab();
  tft_fill_screen(0x0000);  // Black

  wifi_init_and_connect();

  while (true) {
    // Blink the Pico W LED via CYW43.
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(250);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    sleep_ms(250);
  }
}
