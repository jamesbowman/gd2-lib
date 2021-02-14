#ifndef __TERMINAL_H__
#define __TERMINAL_H__

#include <Arduino.h>
#include <SPI.h>
#include <GD2.h>

// DB32 Pallete
// https://github.com/geoffb/dawnbringer-palettes

#define COLOR_BLACK            0x000000  // #000000
#define COLOR_VALHALLA         0x222034  // #222034
#define COLOR_LOULOU           0x45283c  // #45283c
#define COLOR_OILED_CEDAR      0x663931  // #663931
#define COLOR_ROPE             0x8f563b  // #8f563b
#define COLOR_TAHITI_GOLD      0xdf7126  // #df7126
#define COLOR_TWINE            0xd9a066  // #d9a066
#define COLOR_PANCHO           0xeec39a  // #eec39a

#define COLOR_GOLDEN_FIZZ      0xfbf236  // #fbf236
#define COLOR_ATLANTIS         0x99e550  // #99e550
#define COLOR_RAINFOREST       0x6abe30  // #6abe30
#define COLOR_ELF_GREEN        0x37946e  // #37946e
#define COLOR_DELL             0x4b692f  // #4b692f
#define COLOR_VERDIGRIS        0x524b24  // #524b24
#define COLOR_OPAL             0x323c39  // #323c39
#define COLOR_DEEP_KOAMARU     0x3f3f74  // #3f3f74

#define COLOR_VENICE_BLUE      0x306082  // #306082
#define COLOR_ROYAL_BLUE       0x5b6ee1  // #5b6ee1
#define COLOR_CORNFLOWER       0x639bff  // #639bff
#define COLOR_VIKING           0x5fcde4  // #5fcde4
#define COLOR_LIGHT_STEEL_BLUE 0xcbdbfc  // #cbdbfc
#define COLOR_WHITE            0xffffff  // #ffffff
#define COLOR_HEATHER          0x9badb7  // #9badb7
#define COLOR_TOPAZ            0x847e87  // #847e87

#define COLOR_DIM_GRAY         0x696a6a  // #696a6a
#define COLOR_SMOKEY_ASH       0x595652  // #595652
#define COLOR_CLAIRVOYANT      0x76428a  // #76428a
#define COLOR_RED              0xac3232  // #ac3232
#define COLOR_MANDY            0xd95763  // #d95763
#define COLOR_PLUM             0xd77bba  // #d77bba
#define COLOR_STINGER          0x8f974a  // #8f974a
#define COLOR_BROWN            0x8a6f30  // #8a6f30

#define TERMINAL_KEY_BELL 7
#define TERMINAL_KEY_BACKSPACE 8
#define TERMINAL_KEY_CR 13
#define TERMINAL_KEY_LF 10
#define TERMINAL_KEY_SPACE 32
#define TERMINAL_KEY_ESC 27
#define TERMINAL_KEY_DEL 127

#define SCROLLBAR_WIDTH 20
#define SCROLLBAR_HALF_WIDTH 10

#define TAG_SCROLLBAR 201

#define LINE_FULL 0
#define CHAR_READ 1

#define TERMINAL_VGA_BLACK   0
#define TERMINAL_VGA_BLUE    1
#define TERMINAL_VGA_GREEN   2
#define TERMINAL_VGA_CYAN    3
#define TERMINAL_VGA_RED     4
#define TERMINAL_VGA_MAGENTA 5
#define TERMINAL_VGA_YELLOW  6
#define TERMINAL_VGA_WHITE   7
#define TERMINAL_VGA_BRIGHT_BLACK   8
#define TERMINAL_VGA_BRIGHT_BLUE    9
#define TERMINAL_VGA_BRIGHT_GREEN   10
#define TERMINAL_VGA_BRIGHT_CYAN    11
#define TERMINAL_VGA_BRIGHT_RED     12
#define TERMINAL_VGA_BRIGHT_MAGENTA 13
#define TERMINAL_VGA_BRIGHT_YELLOW  14
#define TERMINAL_VGA_BRIGHT_WHITE   15

#define TERMINAL_BITMAP_HANDLE_TEXT 14
#define TERMINAL_BITMAP_HANDLE_BACKGROUND 13

class GD2Terminal {
public:
  GD2Terminal();

  uint16_t cursor_index;
  uint16_t line_count;
  uint16_t last_line_address;
  uint16_t lines_per_screen;
  uint8_t line_pixel_height;
  uint8_t characters_per_line;
  uint8_t bytes_per_line;
  uint8_t current_font;
  bool background_colors_enabled;
  uint8_t foreground_color;
  uint8_t background_color;
  uint16_t scrollback_length;
  float lines_per_screen_percent;
  uint16_t scrollbar_size;
  uint16_t scrollbar_size_half;
  uint16_t scrollbar_position;
  float scrollbar_position_percent;
  uint16_t scroll_offset;
  int draw_x_coord;
  int draw_y_coord;
  uint16_t draw_width;
  uint16_t draw_height;
  uint32_t terminal_window_bg_color;
  uint8_t terminal_window_opacity;

  uint16_t bell;

  void begin(uint8_t initial_font_mode);
  void reset();
  void set_font_8x8();
  void set_font_vga();
  uint32_t bitmap_byte_size();
  uint32_t ram_end_address();
  void change_size(uint16_t rows, uint16_t columns);
  void enable_vga_background_colors();
  void disable_vga_background_colors();
  void ring_bell();
  uint8_t append_character(char newchar);
  void append_string(const char* str);
  void new_line();
  void upload_to_graphics_ram();
  void update_scrollbar_position(uint16_t new_position);
  void update_scrollbar();
  void set_window_bg_color(uint32_t color);
  void set_window_opacity(uint8_t opacity);
  void draw();
  void draw(int startx, int starty);
  void set_position(int x, int y);
  void set_size_fullscreen();
 private:
  void erase_line_buffer();
  void put_char(char newchar);
  void set_scrollbar_handle_size();
};

#endif
