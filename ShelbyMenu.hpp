#pragma once

#include <cstddef>
#include <cstdint>

struct Framebuffer16 {
  uint16_t* pixels = nullptr;  // RGB565
  int width = 160;
  int height = 128;
  int stride = 160;            // pixels per row
};

enum class ShelbyButton : uint8_t { W, A, S, D, I, J, K, L, None };

class ShelbyApp {
 public:
  virtual ~ShelbyApp() = default;
  virtual void update(uint32_t /*dt_ms*/) {}
  virtual void render(Framebuffer16 fb) = 0;
  virtual void on_button(ShelbyButton b) = 0;
};

class ShelbyMenu final : public ShelbyApp {
 public:
  enum class Action : uint8_t {
    None = 0,
    OpenGitHub,
    OpenSystem,
    OpenTasks,
    OpenHackatime,
    OpenSettings,
    OpenMusic,
    ExitToGames,
    ExitToClock,
  };

  ShelbyMenu();

  void render(Framebuffer16 fb) override;
  void on_button(ShelbyButton b) override;

  Action take_action();

 private:
  void full_draw_(Framebuffer16 fb);
  void draw_card_(Framebuffer16 fb, int index);

  void draw_github_(Framebuffer16 fb, int x, int y, uint16_t color);
  void draw_tasks_(Framebuffer16 fb, int x, int y, uint16_t color);
  void draw_settings_(Framebuffer16 fb, int x, int y, uint16_t color);
  void draw_hackatime_(Framebuffer16 fb, int x, int y, uint16_t color);
  void draw_music_(Framebuffer16 fb, int x, int y, uint16_t color);
  void draw_system_(Framebuffer16 fb, int x, int y, uint16_t color);
  void draw_games_(Framebuffer16 fb, int x, int y, uint16_t color);

  int cursor_ = 0;
  bool drawn_ = false;
  int prev_cursor_ = -1;
  Action pending_action_ = Action::None;
};

