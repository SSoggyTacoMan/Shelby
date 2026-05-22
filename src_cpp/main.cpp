#include <cstdio>
#include <cstring>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

namespace {

constexpr spi_inst_t* kDisplaySpi = spi0;
constexpr uint32_t kSpiBaud = 8 * 1000 * 1000;

// Mirrors the MicroPython pin mapping in main.py
constexpr uint kPinSck = 18;
constexpr uint kPinMosi = 19;
constexpr uint kPinMiso = 16;
constexpr uint kPinDc = 22;
constexpr uint kPinRst = 26;
constexpr uint kPinCs = 20;

constexpr uint8_t ST7735_SWRESET = 0x01;
constexpr uint8_t ST7735_SLPOUT = 0x11;
constexpr uint8_t ST7735_COLMOD = 0x3A;
constexpr uint8_t ST7735_MADCTL = 0x36;
constexpr uint8_t ST7735_CASET = 0x2A;
constexpr uint8_t ST7735_RASET = 0x2B;
constexpr uint8_t ST7735_RAMWR = 0x2C;
constexpr uint8_t ST7735_DISPON = 0x29;

void tft_select() { gpio_put(kPinCs, 0); }
void tft_deselect() { gpio_put(kPinCs, 1); }

void tft_write_command(uint8_t command) {
    gpio_put(kPinDc, 0);
    tft_select();
    spi_write_blocking(kDisplaySpi, &command, 1);
    tft_deselect();
}

void tft_write_data(const uint8_t* data, size_t len) {
    if (len == 0) {
        return;
    }
    gpio_put(kPinDc, 1);
    tft_select();
    spi_write_blocking(kDisplaySpi, data, static_cast<int>(len));
    tft_deselect();
}

void tft_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t col[] = {
        static_cast<uint8_t>(x0 >> 8),
        static_cast<uint8_t>(x0 & 0xFF),
        static_cast<uint8_t>(x1 >> 8),
        static_cast<uint8_t>(x1 & 0xFF),
    };
    uint8_t row[] = {
        static_cast<uint8_t>(y0 >> 8),
        static_cast<uint8_t>(y0 & 0xFF),
        static_cast<uint8_t>(y1 >> 8),
        static_cast<uint8_t>(y1 & 0xFF),
    };

    tft_write_command(ST7735_CASET);
    tft_write_data(col, sizeof(col));

    tft_write_command(ST7735_RASET);
    tft_write_data(row, sizeof(row));

    tft_write_command(ST7735_RAMWR);
}

void tft_fill_screen(uint16_t color565, uint16_t width, uint16_t height) {
    tft_set_addr_window(0, 0, width - 1, height - 1);

    uint8_t pixel[2] = {
        static_cast<uint8_t>(color565 >> 8),
        static_cast<uint8_t>(color565 & 0xFF),
    };

    gpio_put(kPinDc, 1);
    tft_select();
    for (uint32_t i = 0; i < static_cast<uint32_t>(width) * height; ++i) {
        spi_write_blocking(kDisplaySpi, pixel, 2);
    }
    tft_deselect();
}

void init_display_st7735() {
    spi_init(kDisplaySpi, kSpiBaud);
    gpio_set_function(kPinSck, GPIO_FUNC_SPI);
    gpio_set_function(kPinMosi, GPIO_FUNC_SPI);
    gpio_set_function(kPinMiso, GPIO_FUNC_SPI);

    gpio_init(kPinDc);
    gpio_set_dir(kPinDc, GPIO_OUT);

    gpio_init(kPinRst);
    gpio_set_dir(kPinRst, GPIO_OUT);

    gpio_init(kPinCs);
    gpio_set_dir(kPinCs, GPIO_OUT);
    tft_deselect();

    // Hardware reset
    gpio_put(kPinRst, 1);
    sleep_ms(5);
    gpio_put(kPinRst, 0);
    sleep_ms(20);
    gpio_put(kPinRst, 1);
    sleep_ms(150);

    tft_write_command(ST7735_SWRESET);
    sleep_ms(150);

    tft_write_command(ST7735_SLPOUT);
    sleep_ms(120);

    // 16-bit color (RGB565)
    tft_write_command(ST7735_COLMOD);
    const uint8_t colmod = 0x05;
    tft_write_data(&colmod, 1);

    // Rotation/color order placeholder (to be refined in Step 2)
    tft_write_command(ST7735_MADCTL);
    const uint8_t madctl = 0x60;
    tft_write_data(&madctl, 1);

    tft_write_command(ST7735_DISPON);
    sleep_ms(20);

    // Landscape clear (Sprig: 160x128)
    tft_fill_screen(0x0000, 160, 128);
}

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

    printf("Shelby native bootstrap (Step 1)\n");

    init_display_st7735();
    init_wifi();

    while (true) {
        cyw43_arch_poll();
        sleep_ms(10);
    }
}
