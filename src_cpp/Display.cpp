#include "Display.hpp"

#include <cstddef>

#include "hardware/gpio.h"
#include "pico/stdlib.h"

namespace {

constexpr uint32_t kSpiBaud = 8 * 1000 * 1000;
constexpr uint32_t kResetDelayUs = 150000;
constexpr uint32_t kSleepOutDelayUs = 255000;
constexpr uint32_t kNormalOnDelayUs = 10000;
constexpr uint32_t kDisplayOnDelayUs = 100000;

constexpr uint8_t ST7735_SWRESET = 0x01;
constexpr uint8_t ST7735_SLPOUT = 0x11;
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
constexpr uint8_t ST7735_NORON = 0x13;
constexpr uint8_t ST7735_INVOFF = 0x20;

constexpr uint8_t kRotations[4] = {0x00, 0x60, 0xC0, 0xA0};
constexpr uint8_t kRgbBit = 0x00;
constexpr uint8_t kBgrBit = 0x08;

constexpr uint8_t kFontData[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00, 0x14,
    0x7F, 0x14, 0x7F, 0x14, 0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62, 0x36, 0x49,
    0x55, 0x22, 0x50, 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x1C, 0x22, 0x41, 0x00, 0x00, 0x41, 0x22,
    0x1C, 0x00, 0x08, 0x2A, 0x1C, 0x2A, 0x08, 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x50, 0x30, 0x00,
    0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x60, 0x60, 0x00, 0x00, 0x20, 0x10, 0x08, 0x04, 0x02,
    0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x42, 0x7F, 0x40, 0x00, 0x42, 0x61, 0x51, 0x49, 0x46, 0x21,
    0x41, 0x45, 0x4B, 0x31, 0x18, 0x14, 0x12, 0x7F, 0x10, 0x27, 0x45, 0x45, 0x45, 0x39, 0x3C, 0x4A,
    0x49, 0x49, 0x30, 0x01, 0x71, 0x09, 0x05, 0x03, 0x36, 0x49, 0x49, 0x49, 0x36, 0x06, 0x49, 0x49,
    0x29, 0x1E, 0x00, 0x36, 0x36, 0x00, 0x00, 0x00, 0x56, 0x36, 0x00, 0x00, 0x00, 0x08, 0x14, 0x22,
    0x41, 0x14, 0x14, 0x14, 0x14, 0x14, 0x41, 0x22, 0x14, 0x08, 0x00, 0x02, 0x01, 0x51, 0x09, 0x06,
    0x32, 0x49, 0x79, 0x41, 0x3E, 0x7E, 0x11, 0x11, 0x11, 0x7E, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x3E,
    0x41, 0x41, 0x41, 0x22, 0x7F, 0x41, 0x41, 0x22, 0x1C, 0x7F, 0x49, 0x49, 0x49, 0x41, 0x7F, 0x09,
    0x09, 0x09, 0x01, 0x3E, 0x41, 0x41, 0x49, 0x7A, 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x41, 0x7F,
    0x41, 0x00, 0x20, 0x40, 0x41, 0x3F, 0x01, 0x7F, 0x08, 0x14, 0x22, 0x41, 0x7F, 0x40, 0x40, 0x40,
    0x40, 0x7F, 0x02, 0x04, 0x02, 0x7F, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x3E, 0x41, 0x41, 0x41, 0x3E,
    0x7F, 0x09, 0x09, 0x09, 0x06, 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x7F, 0x09, 0x19, 0x29, 0x46, 0x46,
    0x49, 0x49, 0x49, 0x31, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x1F, 0x20,
    0x40, 0x20, 0x1F, 0x7F, 0x20, 0x18, 0x20, 0x7F, 0x63, 0x14, 0x08, 0x14, 0x63, 0x03, 0x04, 0x78,
    0x04, 0x03, 0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x00, 0x7F, 0x41, 0x41, 0x02, 0x04, 0x08, 0x10,
    0x20, 0x41, 0x41, 0x7F, 0x00, 0x00, 0x04, 0x02, 0x01, 0x02, 0x04, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x00, 0x01, 0x02, 0x04, 0x00, 0x20, 0x54, 0x54, 0x54, 0x78, 0x7F, 0x48, 0x44, 0x44, 0x38, 0x38,
    0x44, 0x44, 0x44, 0x20, 0x38, 0x44, 0x44, 0x48, 0x7F, 0x38, 0x54, 0x54, 0x54, 0x18, 0x08, 0x7E,
    0x09, 0x01, 0x02, 0x08, 0x14, 0x54, 0x54, 0x3C, 0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, 0x44, 0x7D,
    0x40, 0x00, 0x20, 0x40, 0x44, 0x3D, 0x00, 0x00, 0x7F, 0x10, 0x28, 0x44, 0x00, 0x41, 0x7F, 0x40,
    0x00, 0x7C, 0x04, 0x18, 0x04, 0x78, 0x7C, 0x08, 0x04, 0x04, 0x78, 0x38, 0x44, 0x44, 0x44, 0x38,
    0x7C, 0x14, 0x14, 0x14, 0x08, 0x08, 0x14, 0x14, 0x18, 0x7C, 0x7C, 0x08, 0x04, 0x04, 0x08, 0x48,
    0x54, 0x54, 0x54, 0x20, 0x04, 0x3F, 0x44, 0x40, 0x20, 0x3C, 0x40, 0x40, 0x20, 0x7C, 0x1C, 0x20,
    0x40, 0x20, 0x1C, 0x3C, 0x40, 0x30, 0x40, 0x3C, 0x44, 0x28, 0x10, 0x28, 0x44, 0x0C, 0x50, 0x50,
    0x50, 0x3C, 0x44, 0x64, 0x54, 0x4C, 0x44, 0x00, 0x08, 0x36, 0x41, 0x00, 0x00, 0x00, 0x7F, 0x00,
    0x00, 0x00, 0x41, 0x36, 0x08, 0x00, 0x08, 0x08, 0x2A, 0x1C, 0x08, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
};

}  // namespace

Display::Display(spi_inst_t* spi,
                 uint pin_sck,
                 uint pin_mosi,
                 uint pin_miso,
                 uint pin_dc,
                 uint pin_rst,
                 uint pin_cs)
    : spi_(spi),
      pin_sck_(pin_sck),
      pin_mosi_(pin_mosi),
      pin_miso_(pin_miso),
      pin_dc_(pin_dc),
      pin_rst_(pin_rst),
      pin_cs_(pin_cs) {}

void Display::select() { gpio_put(pin_cs_, 0); }
void Display::deselect() { gpio_put(pin_cs_, 1); }

void Display::writeCommand(uint8_t command) {
    gpio_put(pin_dc_, 0);
    select();
    spi_write_blocking(spi_, &command, 1);
    deselect();
}

void Display::writeData(const uint8_t* data, size_t len) {
    if (data == nullptr || len == 0) {
        return;
    }
    gpio_put(pin_dc_, 1);
    select();
    spi_write_blocking(spi_, data, static_cast<int>(len));
    deselect();
}

void Display::writeDataByte(uint8_t byte) {
    writeData(&byte, 1);
}

void Display::reset() {
    gpio_put(pin_dc_, 0);
    gpio_put(pin_rst_, 1);
    sleep_us(500);
    gpio_put(pin_rst_, 0);
    sleep_us(500);
    gpio_put(pin_rst_, 1);
    sleep_us(500);
}

void Display::setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    x0 = static_cast<uint16_t>(x0 + x_offset_);
    x1 = static_cast<uint16_t>(x1 + x_offset_);
    y0 = static_cast<uint16_t>(y0 + y_offset_);
    y1 = static_cast<uint16_t>(y1 + y_offset_);

    const uint8_t col[] = {
        static_cast<uint8_t>(x0 >> 8),
        static_cast<uint8_t>(x0 & 0xFF),
        static_cast<uint8_t>(x1 >> 8),
        static_cast<uint8_t>(x1 & 0xFF),
    };
    const uint8_t row[] = {
        static_cast<uint8_t>(y0 >> 8),
        static_cast<uint8_t>(y0 & 0xFF),
        static_cast<uint8_t>(y1 >> 8),
        static_cast<uint8_t>(y1 & 0xFF),
    };

    writeCommand(ST7735_CASET);
    writeData(col, sizeof(col));
    writeCommand(ST7735_RASET);
    writeData(row, sizeof(row));
    writeCommand(ST7735_RAMWR);
}

void Display::writeRepeatColor(uint16_t color, uint32_t pixels) {
    const uint8_t hi = static_cast<uint8_t>(color >> 8);
    const uint8_t lo = static_cast<uint8_t>(color & 0xFF);

    constexpr size_t kChunkPixels = 64;
    uint8_t chunk[kChunkPixels * 2];
    for (size_t i = 0; i < kChunkPixels; ++i) {
        chunk[i * 2] = hi;
        chunk[i * 2 + 1] = lo;
    }

    gpio_put(pin_dc_, 1);
    select();
    while (pixels > 0) {
        const uint32_t batch_pixels = pixels > kChunkPixels ? kChunkPixels : pixels;
        spi_write_blocking(spi_, chunk, static_cast<int>(batch_pixels * 2));
        pixels -= batch_pixels;
    }
    deselect();
}

void Display::init() {
    spi_init(spi_, kSpiBaud);
    gpio_set_function(pin_sck_, GPIO_FUNC_SPI);
    gpio_set_function(pin_mosi_, GPIO_FUNC_SPI);
    gpio_set_function(pin_miso_, GPIO_FUNC_SPI);

    gpio_init(pin_dc_);
    gpio_set_dir(pin_dc_, GPIO_OUT);

    gpio_init(pin_rst_);
    gpio_set_dir(pin_rst_, GPIO_OUT);

    gpio_init(pin_cs_);
    gpio_set_dir(pin_cs_, GPIO_OUT);
    deselect();

    reset();

    writeCommand(ST7735_SWRESET);
    sleep_us(kResetDelayUs);

    writeCommand(ST7735_SLPOUT);
    sleep_us(kSleepOutDelayUs);

    const uint8_t frame[] = {0x01, 0x2C, 0x2D};
    writeCommand(ST7735_FRMCTR1);
    writeData(frame, sizeof(frame));
    writeCommand(ST7735_FRMCTR2);
    writeData(frame, sizeof(frame));

    const uint8_t frame3[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
    writeCommand(ST7735_FRMCTR3);
    writeData(frame3, sizeof(frame3));

    writeCommand(ST7735_INVCTR);
    writeDataByte(0x07);

    const uint8_t pwctr1[] = {0xA2, 0x02, 0x84};
    writeCommand(ST7735_PWCTR1);
    writeData(pwctr1, sizeof(pwctr1));

    writeCommand(ST7735_PWCTR2);
    writeDataByte(0xC5);

    const uint8_t pwctr3[] = {0x0A, 0x00};
    const uint8_t pwctr4[] = {0x8A, 0x2A};
    writeCommand(ST7735_PWCTR3);
    writeData(pwctr3, sizeof(pwctr3));
    writeCommand(ST7735_PWCTR4);
    writeData(pwctr4, sizeof(pwctr4));

    const uint8_t pwctr5[] = {0x8A, 0xEE};
    writeCommand(ST7735_PWCTR5);
    writeData(pwctr5, sizeof(pwctr5));

    writeCommand(ST7735_VMCTR1);
    writeDataByte(0x0E);

    writeCommand(ST7735_INVOFF);

    setRotation(1, true);

    writeCommand(ST7735_COLMOD);
    writeDataByte(0x05);

    const uint8_t gamma_pos[] = {
        0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D,
        0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10,
    };
    const uint8_t gamma_neg[] = {
        0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
        0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10,
    };
    writeCommand(ST7735_GMCTRP1);
    writeData(gamma_pos, sizeof(gamma_pos));
    writeCommand(ST7735_GMCTRN1);
    writeData(gamma_neg, sizeof(gamma_neg));

    writeCommand(ST7735_NORON);
    sleep_us(kNormalOnDelayUs);
    writeCommand(ST7735_DISPON);
    sleep_us(kDisplayOnDelayUs);

    fillScreen(0x0000);
}

void Display::setRotation(uint8_t rotation, bool rgb) {
    rotation &= 0x03;
    const uint8_t madctl = static_cast<uint8_t>(kRotations[rotation] | (rgb ? kRgbBit : kBgrBit));
    writeCommand(ST7735_MADCTL);
    writeDataByte(madctl);
}

void Display::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= kWidth || y >= kHeight) {
        return;
    }

    setAddressWindow(x, y, x, y);
    const uint8_t px[] = {
        static_cast<uint8_t>(color >> 8),
        static_cast<uint8_t>(color & 0xFF),
    };
    writeData(px, sizeof(px));
}

void Display::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (w == 0 || h == 0 || x >= kWidth || y >= kHeight) {
        return;
    }

    const uint16_t x_end = static_cast<uint16_t>((x + w - 1 >= kWidth) ? (kWidth - 1) : (x + w - 1));
    const uint16_t y_end = static_cast<uint16_t>((y + h - 1 >= kHeight) ? (kHeight - 1) : (y + h - 1));

    setAddressWindow(x, y, x_end, y_end);
    const uint32_t pixels = static_cast<uint32_t>(x_end - x + 1) * static_cast<uint32_t>(y_end - y + 1);
    writeRepeatColor(color, pixels);
}

void Display::fillScreen(uint16_t color) {
    fillRect(0, 0, kWidth, kHeight, color);
}

void Display::drawCharOnBg(char ch, uint16_t x, uint16_t y, uint16_t fg, uint16_t bg) {
    const uint8_t code = static_cast<uint8_t>(ch);
    if (code < kFontStart || code > kFontEnd) {
        return;
    }

    if (x >= kWidth || y >= kHeight || x + kFontWidth > kWidth || y + kFontHeight > kHeight) {
        return;
    }

    const size_t glyph_index = static_cast<size_t>(code - kFontStart) * kFontWidth;

    uint8_t glyph[kFontWidth * kFontHeight * 2];
    size_t out = 0;
    for (uint8_t row = 0; row < kFontHeight; ++row) {
        for (uint8_t col = 0; col < kFontWidth; ++col) {
            const uint8_t bits = kFontData[glyph_index + col];
            const uint16_t color = (bits & (1u << row)) ? fg : bg;
            glyph[out++] = static_cast<uint8_t>(color >> 8);
            glyph[out++] = static_cast<uint8_t>(color & 0xFF);
        }
    }

    setAddressWindow(x, y, static_cast<uint16_t>(x + kFontWidth - 1), static_cast<uint16_t>(y + kFontHeight - 1));
    writeData(glyph, sizeof(glyph));
}

void Display::drawTextOnBg(const char* text, uint16_t x, uint16_t y, uint16_t fg, uint16_t bg) {
    if (text == nullptr) {
        return;
    }

    uint16_t px = x;
    while (*text != '\0') {
        if (px + kFontWidth > kWidth) {
            break;
        }
        drawCharOnBg(*text, px, y, fg, bg);
        px = static_cast<uint16_t>(px + kFontWidth + 1);
        ++text;
    }
}
