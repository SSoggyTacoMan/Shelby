#pragma once

#include <cstddef>
#include <cstdint>

#include "hardware/spi.h"

class Display {
 public:
  struct Pins {
    uint sck;
    uint mosi;
    uint miso;
    uint dc;
    uint rst;
    uint cs;
  };

  static constexpr uint16_t BLACK = 0x0000;
  static constexpr uint16_t WHITE = 0xFFFF;
  static constexpr uint16_t RED = 0xF800;
  static constexpr uint16_t GREEN = 0x07E0;
  static constexpr uint16_t BLUE = 0x001F;

  static constexpr uint16_t c(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
  }

  Display(spi_inst_t* spi, Pins pins);

  void init(uint32_t spi_baud_hz = 8'000'000);

  void set_rotation(uint8_t rotation);
  void set_rgb(bool rgb);

  int width() const { return width_; }
  int height() const { return height_; }

  void fill(uint16_t color565);
  void fill_rect(int x, int y, int w, int h, uint16_t color565);
  void pixel(int x, int y, uint16_t color565);

  void draw_char_on_bg(char ch, int x, int y, uint16_t fg565, uint16_t bg565);
  void draw_text_on_bg(const char* text, int x, int y, uint16_t fg565, uint16_t bg565);

 private:
  void reset_();
  void write_cmd_(uint8_t cmd);
  void write_data_(const uint8_t* data, size_t len);

  void set_madctl_();
  void set_addr_window_(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

  void initg_();

  const uint8_t* glyph_columns_(char ch) const;

  spi_inst_t* spi_;
  Pins pins_;

  uint8_t rotation_ = 0;
  bool rgb_ = true;

  int width_ = 128;
  int height_ = 160;
};

