#include <SPI.h>
#include <GD2.h>
#include <GD2Terminal.h>
#include <math.h>

GD2Terminal terminal;

// Standard GD3 Chip Select Pins
#define GD3_CS 8
#define GD3_SDCS 9

// Some nice colors to work with
const uint32_t colors_endesga32[] = {
  0xbe4a2f,
  0xd77643,
  0xead4aa,
  0xe4a672,
  0xb86f50,
  0x733e39,
  0x3e2731,
  0xa22633,
  0xe43b44,
  0xf77622,
  0xfeae34,
  0xfee761,
  0x63c74d,
  0x3e8948,
  0x265c42,
  0x193c3e,
  0x124e89,
  0x0099db,
  0x2ce8f5,
  0xffffff,
  0xc0cbdc,
  0x8b9bb4,
  0x5a6988,
  0x3a4466,
  0x262b44,
  0x181425,
  0xff0044,
  0x68386c,
  0xb55088,
  0xf6757a,
  0xe8b796,
  0xc28569,
};

void setup() {
  // Serial.begin(115200);
  // while (!Serial) {}

  SPI.begin();

  // Gameduino3 Setup
  // Serial.println("GD.begin()");
  GD.begin(GD_CALIBRATE | GD_TRIM | GD_STORAGE, GD3_CS, GD3_SDCS);
  // Serial.println("GD.begin() Complete");

  // Setup the terminal. Must come after GD.begin -------------------
  terminal.begin(TEXTVGA);
  // terminal.begin(TEXT8X8);
  terminal.set_window_bg_color(0x000000);  // Background window color
  terminal.set_window_opacity(200);  // 0-255

  // Changing Fonts -------------------------------------------------
  // 8X8 Monochrome
  // terminal.set_font_8x8();
  // VGA 16-color
  terminal.set_font_vga();
  // VGA background colors are disabled by default.
  terminal.disable_vga_background_colors();
  // Enable character backgound color. Disables window color&opacity
  // terminal.enable_vga_background_colors();

  // Font changes must be followed by setting a size ---------------
  // Fullscreen
  terminal.set_size_fullscreen();
  // Custom Size
  // terminal.change_size(row_count, column_count);

  // Get FT81X first available ram address (after terminal data) ---
  // terminal.ram_end_address();

  GD.ClearColorRGB(0x000000);
}


#define TWO_PI 6.283185307179586476925286766559
float start_offset = 0;

void draw_circles() {
  GD.SaveContext();
  GD.Clear();

  // Draw some moving circles
  GD.PointSize(16 * 30); // means 30 pixels
  GD.Begin(POINTS);
  int start_x = floor(GD.w / 34);
  int start_y = floor(GD.h / 2);
  start_offset += TWO_PI/1024;
  if (start_offset > TWO_PI) {
    start_offset = 0;
  }
  int x;
  for (int i = 0; i < 32; i++) {
    GD.ColorRGB(colors_endesga32[i]);
    x = (i + 2) * start_x;
    GD.Vertex2ii(x, start_y + ((GD.h/3) * sin(x + start_offset)));
  }
  GD.RestoreContext();
}

// FPS and time per game loop
uint32_t delta_time_start = 0;
uint32_t delta_time_micros = 30000; // initial guess
// Display List size
uint16_t display_list_offset = 0;
char line_buffer[160];
uint32_t last_terminal_refresh = 0;

void loop() {
  // Frame time start
  delta_time_start = micros();

  // Get touchscreen inputs
  GD.get_inputs();
  terminal.update_scrollbar();

  // Your app update code here

  // Your app draw code here
  draw_circles();

  // Every 2 seconds print something
  if (millis() > last_terminal_refresh + 2000) {
    terminal.append_character('\n');

    // Print time
    terminal.foreground_color = TERMINAL_VGA_BRIGHT_MAGENTA;
    terminal.background_color = TERMINAL_VGA_BRIGHT_CYAN;
    terminal.append_string("[Time] ");

    terminal.foreground_color = TERMINAL_VGA_WHITE;
    terminal.background_color = TERMINAL_VGA_BLACK;
    sprintf(line_buffer, "%02d:%02d:%02d ", 16, 41, GD.random(60));
    terminal.append_string(line_buffer);

    terminal.foreground_color = TERMINAL_VGA_BRIGHT_WHITE;
    sprintf(line_buffer, "%04d-%02d-%02d\n", 2021, 2, 14);
    terminal.append_string(line_buffer);

    terminal.foreground_color = TERMINAL_VGA_BRIGHT_GREEN;
    sprintf(line_buffer, "[Battery] %1.2f V\n", 2.5+2*((float)GD.random(1024)/1024.0));
    terminal.append_string(line_buffer);

    terminal.foreground_color = TERMINAL_VGA_BRIGHT_CYAN;
    terminal.append_string("\nAll Characters:");

    // Print all characters
    char c;
    for (int i=0; i<256; i++) {
      terminal.foreground_color = i%16;
      c = i;
      if (c > 255 || c == '\n' || c == '\r')
        c = ' ';
      terminal.append_character(c);
      if (i%terminal.characters_per_line == 0)
        terminal.append_character('\n');
    }
    terminal.append_character('\n');

    last_terminal_refresh = millis();
  } // end last_terminal_refresh check

  // Draw terminal.
  terminal.draw(0, 0);

  // Print extra debug info;
  // FPS and Display List Usage for the previous frame.
  float delta_time = (float)delta_time_micros * 1e-6;
  sprintf(line_buffer,
          "FPS: %.2f --- DisplayList: %3.2f%% (%u / 8192)",
          1.0 / delta_time,
          100.0 * ((float) display_list_offset/8192),
          display_list_offset);
  // Draw white text with a black outline.
  GD.ColorRGB(COLOR_LIGHT_STEEL_BLUE);
  GD.cmd_text(1, GD.h-16,   18, 0, line_buffer);

  // Get the size (current position) of the display list.
  GD.finish();
  display_list_offset = GD.rd16(REG_CMD_DL);

  GD.swap();

  // Frame time end.
  delta_time_micros = micros() - delta_time_start;
}

