#include "ShelbyMenu.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace {

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Colors (from src/menu.py)
constexpr uint16_t BG = 0x0000;
constexpr uint16_t TITLE_C = 0xFFFF;
constexpr uint16_t HINT_C = rgb565(80, 80, 80);
constexpr uint16_t TITLE_BG = rgb565(10, 10, 30);
constexpr uint16_t CARD_BG = rgb565(20, 20, 40);
constexpr uint16_t CARD_SEL = rgb565(40, 80, 120);
constexpr uint16_t ICON_C = 0xFFFF;
constexpr uint16_t ICON_SEL_C = rgb565(0, 255, 255);
constexpr uint16_t LABEL_C = rgb565(180, 180, 180);
constexpr uint16_t LABEL_SEL_C = 0xFFFF;
constexpr uint16_t BORDER_C = rgb565(60, 60, 100);
constexpr uint16_t BORDER_SEL_C = rgb565(0, 255, 255);

// Layout constants (from src/menu.py)
constexpr int TITLE_H = 14;
constexpr int FOOTER_H = 12;
constexpr int COLS = 3;
constexpr int ROWS = 2;
constexpr int PADDING = 4;
constexpr int GRID_X = PADDING;
constexpr int GRID_Y = TITLE_H + PADDING;
constexpr int GRID_W = 160 - PADDING * 2;
constexpr int GRID_H = 128 - TITLE_H - FOOTER_H - PADDING * 2;
constexpr int CARD_W = (GRID_W - PADDING * (COLS - 1)) / COLS;
constexpr int CARD_H = (GRID_H - PADDING * (ROWS - 1)) / ROWS;

constexpr uint8_t kFontWidth = 5;
constexpr uint8_t kFontHeight = 8;
constexpr uint8_t kFontStart = 32;
constexpr uint8_t kFontEnd = 127;

// From src/font.py (5x8, ASCII 32..127, columns bit-packed bit0=top)
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

inline bool in_bounds(const Framebuffer16& fb, int x, int y) {
  return x >= 0 && y >= 0 && x < fb.width && y < fb.height;
}

inline void set_px(Framebuffer16 fb, int x, int y, uint16_t c) {
  if (!fb.pixels) return;
  if (!in_bounds(fb, x, y)) return;
  fb.pixels[y * fb.stride + x] = c;
}

inline void fill_rect(Framebuffer16 fb, int x, int y, int w, int h, uint16_t c) {
  if (!fb.pixels) return;
  if (w <= 0 || h <= 0) return;
  const int x0 = std::max(0, x);
  const int y0 = std::max(0, y);
  const int x1 = std::min(fb.width - 1, x + w - 1);
  const int y1 = std::min(fb.height - 1, y + h - 1);
  if (x1 < x0 || y1 < y0) return;
  for (int yy = y0; yy <= y1; ++yy) {
    uint16_t* row = fb.pixels + yy * fb.stride + x0;
    std::fill(row, row + (x1 - x0 + 1), c);
  }
}

inline void line(Framebuffer16 fb, int x0, int y0, int x1, int y1, uint16_t c) {
  const int dx = std::abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;

  while (true) {
    set_px(fb, x0, y0, c);
    if (x0 == x1 && y0 == y1) break;
    const int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

inline const uint8_t* glyph_cols(char ch) {
  const uint8_t ci = static_cast<uint8_t>(ch);
  if (ci < kFontStart || ci > kFontEnd) return nullptr;
  return &kFont5x8[static_cast<size_t>(ci - kFontStart) * kFontWidth];
}

inline void draw_char_on_bg(Framebuffer16 fb, char ch, int x, int y, uint16_t fg, uint16_t bg) {
  const uint8_t* cols = glyph_cols(ch);
  if (!cols) return;
  for (uint8_t col = 0; col < kFontWidth; ++col) {
    uint8_t bits = cols[col];
    for (uint8_t row = 0; row < kFontHeight; ++row) {
      set_px(fb, x + col, y + row, (bits & 0x01) ? fg : bg);
      bits >>= 1;
    }
  }
}

inline int str_len(const char* s) {
  return s ? static_cast<int>(std::strlen(s)) : 0;
}

inline void draw_text_on_bg(Framebuffer16 fb, const char* text, int x, int y, uint16_t fg, uint16_t bg) {
  if (!text) return;
  int px = x;
  for (const char* p = text; *p; ++p) {
    draw_char_on_bg(fb, *p, px, y, fg, bg);
    px += static_cast<int>(kFontWidth) + 1;
  }
}

struct AppDef {
  ShelbyMenu::Action action;
  const char* label;
  enum class Icon : uint8_t { GitHub, System, Tasks, Hackatime, Settings, Music, Games } icon;
};

constexpr AppDef kApps[6] = {
    {ShelbyMenu::Action::OpenGitHub,    "GitHub",    AppDef::Icon::GitHub},
    {ShelbyMenu::Action::OpenSystem,    "System",    AppDef::Icon::System},
    {ShelbyMenu::Action::OpenTasks,     "Tasks",     AppDef::Icon::Tasks},
    {ShelbyMenu::Action::OpenHackatime, "Hackatime", AppDef::Icon::Hackatime},
    {ShelbyMenu::Action::OpenSettings,  "Settings",  AppDef::Icon::Settings},
    {ShelbyMenu::Action::ExitToGames,   "Games",     AppDef::Icon::Games},
};

inline void card_rect(int index, int* x, int* y, int* w, int* h) {
  const int row = index / COLS;
  const int col = index % COLS;
  *x = GRID_X + col * (CARD_W + PADDING);
  *y = GRID_Y + row * (CARD_H + PADDING);
  *w = CARD_W;
  *h = CARD_H;
}

}  // namespace

ShelbyMenu::ShelbyMenu() = default;

void ShelbyMenu::render(Framebuffer16 fb) {
  if (!drawn_) {
    full_draw_(fb);
    drawn_ = true;
    prev_cursor_ = cursor_;
    return;
  }

  if (prev_cursor_ != cursor_) {
    draw_card_(fb, prev_cursor_);
    draw_card_(fb, cursor_);
    prev_cursor_ = cursor_;
  }
}

void ShelbyMenu::on_button(ShelbyButton b) {
  if (b == ShelbyButton::J) {
    pending_action_ = Action::ExitToClock;
    return;
  }

  if (b == ShelbyButton::W) {
    if (cursor_ >= COLS) cursor_ -= COLS;
  } else if (b == ShelbyButton::S) {
    if (cursor_ + COLS < static_cast<int>(std::size(kApps))) cursor_ += COLS;
  } else if (b == ShelbyButton::A) {
    if ((cursor_ % COLS) != 0) cursor_ -= 1;
  } else if (b == ShelbyButton::D) {
    if ((cursor_ % COLS) != (COLS - 1) && (cursor_ + 1) < static_cast<int>(std::size(kApps))) cursor_ += 1;
  } else if (b == ShelbyButton::I) {
    pending_action_ = kApps[cursor_].action;
  }
}

ShelbyMenu::Action ShelbyMenu::take_action() {
  Action a = pending_action_;
  pending_action_ = Action::None;
  return a;
}

void ShelbyMenu::full_draw_(Framebuffer16 fb) {
  fill_rect(fb, 0, 0, 160, 128, BG);

  fill_rect(fb, 0, 0, 160, TITLE_H, TITLE_BG);
  {
    const char* title = "SHELBY OS";
    const int tx = (160 - str_len(title) * 6) / 2;
    draw_text_on_bg(fb, title, tx, 3, TITLE_C, TITLE_BG);
  }

  const int fy = 128 - FOOTER_H;
  fill_rect(fb, 0, fy, 160, FOOTER_H, TITLE_BG);
  {
    const char* hint = "WASD:nav  I:open  J:back";
    const int hx = (160 - str_len(hint) * 6) / 2;
    draw_text_on_bg(fb, hint, hx, fy + 2, HINT_C, TITLE_BG);
  }

  for (int i = 0; i < static_cast<int>(std::size(kApps)); ++i) draw_card_(fb, i);
}

void ShelbyMenu::draw_card_(Framebuffer16 fb, int index) {
  if (index < 0 || index >= static_cast<int>(std::size(kApps))) return;

  const bool sel = (index == cursor_);
  const uint16_t card_bg = sel ? CARD_SEL : CARD_BG;

  int x, y, w, h;
  card_rect(index, &x, &y, &w, &h);

  fill_rect(fb, x, y, w, h, card_bg);

  const uint16_t bc = sel ? BORDER_SEL_C : BORDER_C;
  line(fb, x, y, x + w - 1, y, bc);
  line(fb, x, y + h - 1, x + w - 1, y + h - 1, bc);
  line(fb, x, y, x, y + h - 1, bc);
  line(fb, x + w - 1, y, x + w - 1, y + h - 1, bc);

  const uint16_t ic = sel ? ICON_SEL_C : ICON_C;
  const int icon_x = x + (w - 16) / 2;
  const int icon_y = y + 6;

  switch (kApps[index].icon) {
    case AppDef::Icon::GitHub:    draw_github_(fb, icon_x, icon_y, ic); break;
    case AppDef::Icon::System:    draw_system_(fb, icon_x, icon_y, ic); break;
    case AppDef::Icon::Tasks:     draw_tasks_(fb, icon_x, icon_y, ic); break;
    case AppDef::Icon::Hackatime: draw_hackatime_(fb, icon_x, icon_y, ic); break;
    case AppDef::Icon::Settings:  draw_settings_(fb, icon_x, icon_y, ic); break;
    case AppDef::Icon::Music:     draw_music_(fb, icon_x, icon_y, ic); break;
    case AppDef::Icon::Games:     draw_games_(fb, icon_x, icon_y, ic); break;
  }

  const uint16_t lc = sel ? LABEL_SEL_C : LABEL_C;
  const char* label = kApps[index].label;
  const int lw = str_len(label) * 6;
  const int lx = x + (w - lw) / 2;
  const int ly = y + h - 10;
  draw_text_on_bg(fb, label, lx, ly, lc, card_bg);
}

// Icons (translated from src/icons.py). NOTE: functions assume ~24x24 draw space.
void ShelbyMenu::draw_github_(Framebuffer16 fb, int x, int y, uint16_t color) {
  line(fb, x + 4, y + 8, x + 20, y + 8, color);
  line(fb, x + 4, y + 20, x + 20, y + 20, color);
  line(fb, x + 4, y + 8, x + 4, y + 20, color);
  line(fb, x + 20, y + 8, x + 20, y + 20, color);
  line(fb, x + 4, y + 8, x + 4, y + 2, color);
  line(fb, x + 4, y + 2, x + 8, y + 8, color);
  line(fb, x + 20, y + 8, x + 20, y + 2, color);
  line(fb, x + 20, y + 2, x + 16, y + 8, color);
  line(fb, x + 0, y + 12, x + 3, y + 12, color);
  line(fb, x + 21, y + 12, x + 24, y + 12, color);
  line(fb, x + 0, y + 16, x + 3, y + 16, color);
  line(fb, x + 21, y + 16, x + 24, y + 16, color);
  fill_rect(fb, x + 8, y + 12, 2, 2, color);
  fill_rect(fb, x + 14, y + 12, 2, 2, color);
}

void ShelbyMenu::draw_tasks_(Framebuffer16 fb, int x, int y, uint16_t color) {
  line(fb, x + 4, y + 2, x + 18, y + 2, color);
  line(fb, x + 4, y + 22, x + 18, y + 22, color);
  line(fb, x + 4, y + 2, x + 4, y + 22, color);
  line(fb, x + 18, y + 2, x + 18, y + 22, color);
  fill_rect(fb, x + 8, y + 0, 6, 4, color);
  line(fb, x + 7, y + 12, x + 11, y + 16, color);
  line(fb, x + 7, y + 13, x + 11, y + 17, color);
  line(fb, x + 11, y + 16, x + 17, y + 8, color);
  line(fb, x + 11, y + 17, x + 17, y + 9, color);
}

void ShelbyMenu::draw_settings_(Framebuffer16 fb, int x, int y, uint16_t color) {
  line(fb, x + 10, y + 10, x + 14, y + 10, color);
  line(fb, x + 10, y + 14, x + 14, y + 14, color);
  line(fb, x + 10, y + 10, x + 10, y + 14, color);
  line(fb, x + 14, y + 10, x + 14, y + 14, color);
  line(fb, x + 8, y + 8, x + 16, y + 8, color);
  line(fb, x + 8, y + 16, x + 16, y + 16, color);
  line(fb, x + 8, y + 8, x + 8, y + 16, color);
  line(fb, x + 16, y + 8, x + 16, y + 16, color);
  fill_rect(fb, x + 10, y + 4, 4, 4, color);
  fill_rect(fb, x + 10, y + 16, 4, 4, color);
  fill_rect(fb, x + 4, y + 10, 4, 4, color);
  fill_rect(fb, x + 16, y + 10, 4, 4, color);
  fill_rect(fb, x + 6, y + 6, 2, 2, color);
  fill_rect(fb, x + 16, y + 6, 2, 2, color);
  fill_rect(fb, x + 6, y + 16, 2, 2, color);
  fill_rect(fb, x + 16, y + 16, 2, 2, color);
}

void ShelbyMenu::draw_hackatime_(Framebuffer16 fb, int x, int y, uint16_t color) {
  line(fb, x + 4, y + 0, x + 20, y + 0, color);
  line(fb, x + 4, y + 20, x + 20, y + 20, color);
  line(fb, x + 0, y + 4, x + 0, y + 16, color);
  line(fb, x + 24, y + 4, x + 24, y + 16, color);
  line(fb, x + 12, y + 4, x + 12, y + 12, color);
  line(fb, x + 12, y + 12, x + 18, y + 12, color);
  line(fb, x + 14, y + 14, x + 10, y + 20, color);
  line(fb, x + 10, y + 20, x + 13, y + 20, color);
  line(fb, x + 13, y + 20, x + 9, y + 24, color);
}

void ShelbyMenu::draw_music_(Framebuffer16 fb, int x, int y, uint16_t color) {
  fill_rect(fb, x + 4, y + 16, 6, 4, color);
  fill_rect(fb, x + 3, y + 17, 8, 2, color);
  line(fb, x + 10, y + 4, x + 10, y + 18, color);
  line(fb, x + 10, y + 4, x + 16, y + 7, color);
  line(fb, x + 10, y + 8, x + 16, y + 11, color);
  line(fb, x + 16, y + 7, x + 16, y + 11, color);
}

void ShelbyMenu::draw_system_(Framebuffer16 fb, int x, int y, uint16_t color) {
  line(fb, x + 4, y + 4, x + 20, y + 4, color);
  line(fb, x + 4, y + 20, x + 20, y + 20, color);
  line(fb, x + 4, y + 4, x + 4, y + 20, color);
  line(fb, x + 20, y + 4, x + 20, y + 20, color);
  line(fb, x + 9, y + 9, x + 15, y + 9, color);
  line(fb, x + 9, y + 15, x + 15, y + 15, color);
  line(fb, x + 9, y + 9, x + 9, y + 15, color);
  line(fb, x + 15, y + 9, x + 15, y + 15, color);
  line(fb, x + 7, y + 2, x + 7, y + 4, color);
  line(fb, x + 12, y + 2, x + 12, y + 4, color);
  line(fb, x + 17, y + 2, x + 17, y + 4, color);
  line(fb, x + 7, y + 20, x + 7, y + 22, color);
  line(fb, x + 12, y + 20, x + 12, y + 22, color);
  line(fb, x + 17, y + 20, x + 17, y + 22, color);
  line(fb, x + 2, y + 7, x + 4, y + 7, color);
  line(fb, x + 2, y + 12, x + 4, y + 12, color);
  line(fb, x + 2, y + 17, x + 4, y + 17, color);
  line(fb, x + 20, y + 7, x + 22, y + 7, color);
  line(fb, x + 20, y + 12, x + 22, y + 12, color);
  line(fb, x + 20, y + 17, x + 22, y + 17, color);
}

void ShelbyMenu::draw_games_(Framebuffer16 fb, int x, int y, uint16_t color) {
  // Simple gamepad icon (new, to represent "Games")
  // Body
  line(fb, x + 4, y + 10, x + 20, y + 10, color);
  line(fb, x + 4, y + 18, x + 20, y + 18, color);
  line(fb, x + 4, y + 10, x + 2, y + 14, color);
  line(fb, x + 2, y + 14, x + 4, y + 18, color);
  line(fb, x + 20, y + 10, x + 22, y + 14, color);
  line(fb, x + 22, y + 14, x + 20, y + 18, color);
  // D-pad
  line(fb, x + 7, y + 14, x + 11, y + 14, color);
  line(fb, x + 9, y + 12, x + 9, y + 16, color);
  // Buttons
  fill_rect(fb, x + 15, y + 13, 2, 2, color);
  fill_rect(fb, x + 18, y + 15, 2, 2, color);
}

