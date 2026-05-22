#pragma once

#include <cstdint>

#include "hardware/spi.h"

class Display {
public:
    static constexpr uint16_t kWidth = 160;
    static constexpr uint16_t kHeight = 128;

    static constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
        return static_cast<uint16_t>(((r & 0xF8u) << 8) | ((g & 0xFCu) << 3) | (b >> 3));
    }

    Display(spi_inst_t* spi = spi0,
            uint pin_sck = 18,
            uint pin_mosi = 19,
            uint pin_miso = 16,
            uint pin_dc = 22,
            uint pin_rst = 26,
            uint pin_cs = 20);

    void init();
    void setRotation(uint8_t rotation, bool rgb = true);

    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    void fillScreen(uint16_t color);

    void drawCharOnBg(char ch, uint16_t x, uint16_t y, uint16_t fg, uint16_t bg);
    void drawTextOnBg(const char* text, uint16_t x, uint16_t y, uint16_t fg, uint16_t bg);

private:
    static constexpr uint8_t kFontWidth = 5;
    static constexpr uint8_t kFontHeight = 8;
    static constexpr uint8_t kFontStart = 32;
    static constexpr uint8_t kFontEnd = 127;

    spi_inst_t* spi_;
    uint pin_sck_;
    uint pin_mosi_;
    uint pin_miso_;
    uint pin_dc_;
    uint pin_rst_;
    uint pin_cs_;

    uint8_t x_offset_ = 1;
    uint8_t y_offset_ = 1;

    void reset();
    void select();
    void deselect();
    void writeCommand(uint8_t command);
    void writeData(const uint8_t* data, size_t len);
    void writeDataByte(uint8_t byte);
    void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void writeRepeatColor(uint16_t color, uint32_t pixels);
};
