#include "Display.hpp"

#include <algorithm>
#include <cstring>

#include "pico/stdlib.h"

namespace {

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

constexpr uint8_t kTftRotations[4] = {0x00, 0x60, 0xC0, 0xA0};
constexpr uint8_t kTftBgr = 0x08;

constexpr uint8_t kFontWidth = 5;
constexpr uint8_t kFontHeight = 8;
constexpr uint8_t kFontStart = 32;
constexpr uint8_t kFontEnd = 127;

// Boochow-compatible 5x8 font dict, ASCII 32-127
// Columns are bit-packed: bit0 = top pixel, bit7 = bottom pixel
constexpr uint8_t kFont5x8[] = {
    0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x5F,0x00,0x00, 0x00,0x07,0x00,0x07,0x00,
    0x14,0x7F,0x14,0x7F,0x14, 0x24,0x2A,0x7F,0x2A,0x12, 0x23,0x13,0x08,0x64,0x62,
    0x36,0x49,0x55,0x22,0x50, 0x00,0x05,0x03,0x00,0x00, 0x00,0x1C,0x22,0x41,0x00,
    0x00,0x41,0x22,0x1C,0x00, 0x08,0x2A,0x1C,0x2A,0x08, 0x08,0x08,0x3E,0x08,0x08,
    0x00,0x50,0x30,0x00,0x00, 0x08,0x08,0x08,0x08,0x08, 0x00,0x60,0x60,0x00,0x00,
    0x20,0x10,0x08,0x04,0x02, 0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00,
    0x42,0x61,0x51,0x49,0x46, 0x21,0x41,0x45,0x4B,0x31, 0x18,0x14,0x12,0x7F,0x10,
    0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x30, 0x01,0x71,0x09,0x05,0x03,
    0x36,0x49,0x49,0x49,0x36, 0x06,0x49,0x49,0x29,0x1E, 0x00,0x36,0x36,0x00,0x00,
    0x00,0x56,0x36,0x00,0x00, 0x00,0x08,0x14,0x22,0x41, 0x14,0x14,0x14,0x14,0x14,
    0x41,0x22,0x14,0x08,0x00, 0x02,0x01,0x51,0x09,0x06, 0x32,0x49,0x79,0x41,0x3E,
    0x7E,0x11,0x11,0x11,0x7E, 0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22,
    0x7F,0x41,0x41,0x22,0x1C, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01,
    0x3E,0x41,0x41,0x49,0x7A, 0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00,
    0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41, 0x7F,0x40,0x40,0x40,0x40,
    0x7F,0x02,0x04,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E,
    0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46,
    0x46,0x49,0x49,0x49,0x31, 0x01,0x01,0x7F,0x01,0x01, 0x3F,0x40,0x40,0x40,0x3F,
    0x1F,0x20,0x40,0x20,0x1F, 0x7F,0x20,0x18,0x20,0x7F, 0x63,0x14,0x08,0x14,0x63,
    0x03,0x04,0x78,0x04,0x03, 0x61,0x51,0x49,0x45,0x43, 0x00,0x00,0x7F,0x41,0x41,
    0x02,0x04,0x08,0x10,0x20, 0x41,0x41,0x7F,0x00,0x00, 0x04,0x02,0x01,0x02,0x04,
    0x40,0x40,0x40,0x40,0x40, 0x00,0x01,0x02,0x04,0x00, 0x20,0x54,0x54,0x54,0x78,
    0x7F,0x48,0x44,0x44,0x38, 0x38,0x44,0x44,0x44,0x20, 0x38,0x44,0x44,0x48,0x7F,
    0x38,0x54,0x54,0x54,0x18, 0x08,0x7E,0x09,0x01,0x02, 0x08,0x14,0x54,0x54,0x3C,
    0x7F,0x08,0x04,0x04,0x78, 0x00,0x44,0x7D,0x40,0x00, 0x20,0x40,0x44,0x3D,0x00,
    0x00,0x7F,0x10,0x28,0x44, 0x00,0x41,0x7F,0x40,0x00, 0x7C,0x04,0x18,0x04,0x78,
    0x7C,0x08,0x04,0x04,0x78, 0x38,0x44,0x44,0x44,0x38, 0x7C,0x14,0x14,0x14,0x08,
    0x08,0x14,0x14,0x18,0x7C, 0x7C,0x08,0x04,0x04,0x08, 0x48,0x54,0x54,0x54,0x20,
    0x04,0x3F,0x44,0x40,0x20, 0x3C,0x40,0x40,0x20,0x7C, 0x1C,0x20,0x40,0x20,0x1C,
    0x3C,0x40,0x30,0x40,0x3C, 0x44,0x28,0x10,0x28,0x44, 0x0C,0x50,0x50,0x50,0x3C,
    0x44,0x64,0x54,0x4C,0x44, 0x00,0x08,0x36,0x41,0x00, 0x00,0x00,0x7F,0x00,0x00,
    0x00,0x41,0x36,0x08,0x00, 0x08,0x08,0x2A,0x1C,0x08, 0x7F,0x7F,0x7F,0x7F,0x7F,
};

inline void gpio_write(uint pin, bool value) {
  gpio_put(pin, value ? 1 : 0);
}

}  // namespace

Display::Display(spi_inst_t* spi, Pins pins) : spi_(spi), pins_(pins) {}

void Display::init(uint32_t spi_baud_hz) {
  spi_init(spi_, spi_baud_hz);
  spi_set_format(spi_, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

  gpio_set_function(pins_.sck, GPIO_FUNC_SPI);
  gpio_set_function(pins_.mosi, GPIO_FUNC_SPI);
  gpio_set_function(pins_.miso, GPIO_FUNC_SPI);

  gpio_init(pins_.cs);
  gpio_set_dir(pins_.cs, GPIO_OUT);
  gpio_write(pins_.cs, true);

  gpio_init(pins_.dc);
  gpio_set_dir(pins_.dc, GPIO_OUT);
  gpio_write(pins_.dc, false);

  gpio_init(pins_.rst);
  gpio_set_dir(pins_.rst, GPIO_OUT);
  gpio_write(pins_.rst, true);

  initg_();
}

void Display::set_rotation(uint8_t rotation) {
  rotation_ = rotation & 0x3;

  const bool swap_wh = (rotation_ & 1) != 0;
  width_ = swap_wh ? 160 : 128;
  height_ = swap_wh ? 128 : 160;

  set_madctl_();
}

void Display::set_rgb(bool rgb) {
  rgb_ = rgb;
  set_madctl_();
}

void Display::reset_() {
  gpio_write(pins_.dc, false);
  gpio_write(pins_.rst, true);
  sleep_us(500);
  gpio_write(pins_.rst, false);
  sleep_us(500);
  gpio_write(pins_.rst, true);
  sleep_us(500);
}

void Display::write_cmd_(uint8_t cmd) {
  gpio_write(pins_.dc, false);
  gpio_write(pins_.cs, false);
  spi_write_blocking(spi_, &cmd, 1);
  gpio_write(pins_.cs, true);
}

void Display::write_data_(const uint8_t* data, size_t len) {
  if (len == 0) return;
  gpio_write(pins_.dc, true);
  gpio_write(pins_.cs, false);
  spi_write_blocking(spi_, data, static_cast<int>(len));
  gpio_write(pins_.cs, true);
}

void Display::set_madctl_() {
  write_cmd_(ST7735_MADCTL);
  const uint8_t rgb = rgb_ ? 0x00 : kTftBgr;
  const uint8_t value = static_cast<uint8_t>(kTftRotations[rotation_] | rgb);
  write_data_(&value, 1);
}

void Display::set_addr_window_(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  uint8_t data[4];

  write_cmd_(ST7735_CASET);
  data[0] = static_cast<uint8_t>(x0 >> 8);
  data[1] = static_cast<uint8_t>(x0 & 0xFF);
  data[2] = static_cast<uint8_t>(x1 >> 8);
  data[3] = static_cast<uint8_t>(x1 & 0xFF);
  write_data_(data, sizeof(data));

  write_cmd_(ST7735_RASET);
  data[0] = static_cast<uint8_t>(y0 >> 8);
  data[1] = static_cast<uint8_t>(y0 & 0xFF);
  data[2] = static_cast<uint8_t>(y1 >> 8);
  data[3] = static_cast<uint8_t>(y1 & 0xFF);
  write_data_(data, sizeof(data));

  write_cmd_(ST7735_RAMWR);
}

void Display::fill(uint16_t color565) {
  fill_rect(0, 0, width_, height_, color565);
}

void Display::fill_rect(int x, int y, int w, int h, uint16_t color565) {
  if (w <= 0 || h <= 0) return;
  if (x >= width_ || y >= height_) return;

  const int x0 = std::max(0, x);
  const int y0 = std::max(0, y);
  const int x1 = std::min(width_ - 1, x + w - 1);
  const int y1 = std::min(height_ - 1, y + h - 1);
  if (x1 < x0 || y1 < y0) return;

  set_addr_window_(static_cast<uint16_t>(x0), static_cast<uint16_t>(y0),
                   static_cast<uint16_t>(x1), static_cast<uint16_t>(y1));

  uint8_t px[64 * 2];
  for (size_t i = 0; i < sizeof(px); i += 2) {
    px[i + 0] = static_cast<uint8_t>(color565 >> 8);
    px[i + 1] = static_cast<uint8_t>(color565 & 0xFF);
  }

  int remaining = (x1 - x0 + 1) * (y1 - y0 + 1);
  gpio_write(pins_.dc, true);
  gpio_write(pins_.cs, false);
  while (remaining > 0) {
    const int chunk = remaining > 64 ? 64 : remaining;
    spi_write_blocking(spi_, px, chunk * 2);
    remaining -= chunk;
  }
  gpio_write(pins_.cs, true);
}

void Display::pixel(int x, int y, uint16_t color565) {
  if (x < 0 || y < 0 || x >= width_ || y >= height_) return;

  set_addr_window_(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                   static_cast<uint16_t>(x), static_cast<uint16_t>(y));
  const uint8_t data[] = {static_cast<uint8_t>(color565 >> 8), static_cast<uint8_t>(color565 & 0xFF)};
  write_data_(data, sizeof(data));
}

const uint8_t* Display::glyph_columns_(char ch) const {
  const uint8_t ci = static_cast<uint8_t>(ch);
  if (ci < kFontStart || ci > kFontEnd) return nullptr;
  const size_t index = static_cast<size_t>(ci - kFontStart) * kFontWidth;
  return &kFont5x8[index];
}

void Display::draw_char_on_bg(char ch, int x, int y, uint16_t fg565, uint16_t bg565) {
  const uint8_t* cols = glyph_columns_(ch);
  if (!cols) return;
  if (x >= width_ || y >= height_) return;
  if (x + kFontWidth - 1 < 0 || y + kFontHeight - 1 < 0) return;

  uint8_t buf[kFontWidth * kFontHeight * 2];
  size_t o = 0;
  for (uint8_t row = 0; row < kFontHeight; ++row) {
    for (uint8_t col = 0; col < kFontWidth; ++col) {
      const bool on = ((cols[col] >> row) & 0x01) != 0;
      const uint16_t c565 = on ? fg565 : bg565;
      buf[o++] = static_cast<uint8_t>(c565 >> 8);
      buf[o++] = static_cast<uint8_t>(c565 & 0xFF);
    }
  }

  const int x0 = std::max(0, x);
  const int y0 = std::max(0, y);
  const int x1 = std::min(width_ - 1, x + static_cast<int>(kFontWidth) - 1);
  const int y1 = std::min(height_ - 1, y + static_cast<int>(kFontHeight) - 1);

  if (x0 != x || y0 != y || x1 != x + static_cast<int>(kFontWidth) - 1 ||
      y1 != y + static_cast<int>(kFontHeight) - 1) {
    for (uint8_t row = 0; row < kFontHeight; ++row) {
      for (uint8_t col = 0; col < kFontWidth; ++col) {
        const bool on = ((cols[col] >> row) & 0x01) != 0;
        pixel(x + col, y + row, on ? fg565 : bg565);
      }
    }
    return;
  }

  set_addr_window_(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                   static_cast<uint16_t>(x + kFontWidth - 1),
                   static_cast<uint16_t>(y + kFontHeight - 1));

  gpio_write(pins_.dc, true);
  gpio_write(pins_.cs, false);
  spi_write_blocking(spi_, buf, static_cast<int>(sizeof(buf)));
  gpio_write(pins_.cs, true);
}

void Display::draw_text_on_bg(const char* text, int x, int y, uint16_t fg565, uint16_t bg565) {
  if (!text) return;
  int px = x;
  for (const char* p = text; *p; ++p) {
    draw_char_on_bg(*p, px, y, fg565, bg565);
    px += static_cast<int>(kFontWidth) + 1;
  }
}

void Display::initg_() {
  reset_();

  write_cmd_(ST7735_SWRESET);
  sleep_us(150);
  write_cmd_(ST7735_SLPOUT);
  sleep_us(255);

  const uint8_t frmctr3[] = {0x01, 0x2C, 0x2D};
  write_cmd_(ST7735_FRMCTR1);
  write_data_(frmctr3, sizeof(frmctr3));

  write_cmd_(ST7735_FRMCTR2);
  write_data_(frmctr3, sizeof(frmctr3));

  const uint8_t frmctr6[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
  write_cmd_(ST7735_FRMCTR3);
  write_data_(frmctr6, sizeof(frmctr6));
  sleep_us(10);

  write_cmd_(ST7735_INVCTR);
  const uint8_t invctr = 0x07;
  write_data_(&invctr, 1);

  write_cmd_(ST7735_PWCTR1);
  const uint8_t pwctr1[] = {0xA2, 0x02, 0x84};
  write_data_(pwctr1, sizeof(pwctr1));

  write_cmd_(ST7735_PWCTR2);
  const uint8_t pwctr2 = 0xC5;
  write_data_(&pwctr2, 1);

  write_cmd_(ST7735_PWCTR3);
  const uint8_t pwctr3[] = {0x0A, 0x00};
  write_data_(pwctr3, sizeof(pwctr3));

  write_cmd_(ST7735_PWCTR4);
  const uint8_t pwctr4[] = {0x8A, 0x2A};
  write_data_(pwctr4, sizeof(pwctr4));

  write_cmd_(ST7735_PWCTR5);
  const uint8_t pwctr5[] = {0x8A, 0xEE};
  write_data_(pwctr5, sizeof(pwctr5));

  write_cmd_(ST7735_VMCTR1);
  const uint8_t vmctr1 = 0x0E;
  write_data_(&vmctr1, 1);

  write_cmd_(ST7735_INVOFF);

  // Match Shelby OS defaults: BGR + rotation(1) after init.
  rgb_ = false;
  rotation_ = 1;
  width_ = 160;
  height_ = 128;
  set_madctl_();

  write_cmd_(ST7735_COLMOD);
  const uint8_t colmod = 0x05;
  write_data_(&colmod, 1);

  write_cmd_(ST7735_CASET);
  const uint8_t caset[] = {0x00, 0x01, 0x00, 0x7F};
  write_data_(caset, sizeof(caset));

  write_cmd_(ST7735_RASET);
  const uint8_t raset[] = {0x00, 0x01, 0x00, 0x9F};
  write_data_(raset, sizeof(raset));

  write_cmd_(ST7735_GMCTRP1);
  const uint8_t gmctrp1[] = {0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d,
                             0x29, 0x25, 0x2b, 0x39, 0x00, 0x01, 0x03, 0x10};
  write_data_(gmctrp1, sizeof(gmctrp1));

  write_cmd_(ST7735_GMCTRN1);
  const uint8_t gmctrn1[] = {0x03, 0x1d, 0x07, 0x06, 0x2e, 0x2c, 0x29, 0x2d,
                             0x2e, 0x2e, 0x37, 0x3f, 0x00, 0x00, 0x02, 0x10};
  write_data_(gmctrn1, sizeof(gmctrn1));

  write_cmd_(ST7735_NORON);
  sleep_us(10);

  write_cmd_(ST7735_DISPON);
  sleep_us(100);
}

