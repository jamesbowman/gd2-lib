#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include "logo_assets.h"

void setup()
{
  Serial.begin(1000000);    // JCB
  GD.begin();
  LOAD_ASSETS();
}

byte clamp255(int x)
{
  if (x < 0)
    return 0;
  if (255 < x)
    return 255;
  return x;
}

#define NSTARS 256

void loop()
{
  byte fade;
  xy stars[NSTARS];
  for (int i = 0; i < NSTARS; i++)
    stars[i].set(GD.random(PIXELS(GD.w)), GD.random(PIXELS(GD.h)));

  for (int t = 0; t < 464; t++) {
    GD.cmd_gradient(0, 0, 0x120000, 0, GD.h, 0x480000);
    GD.BlendFunc(SRC_ALPHA, ONE);
    GD.Begin(POINTS);
    for (int i = 0; i < NSTARS; i++) {
      GD.ColorA(64 + (i >> 2));
      GD.PointSize(8 + (i >> 5));
      stars[i].draw();
      // stars drift left, then wrap around
      stars[i].x -= 1 + (i >> 5);
      if (stars[i].x < -256) {
        stars[i].x += PIXELS(GD.w + 20);
        stars[i].y = GD.random(PIXELS(GD.h));
      }
    }
    GD.RestoreContext();
    GD.Begin(BITMAPS);

    // Main logo fades up from black
    fade = clamp255(5 * (t - 15));
    GD.ColorRGB(fade, fade, fade);
    GD.Vertex2ii(240 - GAMEDUINO_WIDTH/2, 65, GAMEDUINO_HANDLE, 0);
    GD.RestoreContext();

    // The '2' and its glow
    fade = clamp255(8 * (t - 120));
    GD.ColorA(fade);
    GD.Vertex2ii(270, 115, TWO_HANDLE, 0);
    fade = clamp255(5 * (t - 144));

    GD.BlendFunc(SRC_ALPHA, ONE);
    GD.ColorA(fade);
    GD.ColorRGB(85,85,85);
    GD.Vertex2ii(270, 115, TWO_HANDLE, 1);

    GD.RestoreContext();

    // The text fades up. Its glow is a full-screen bitmap
    fade = clamp255(8 * (t - 160));
    GD.ColorA(fade);
    GD.cmd_text(140, 200, 29, OPT_CENTER, "This time");
    GD.cmd_text(140, 225, 29, OPT_CENTER, "it's personal");
    fade = clamp255(5 * (t - 184));
    GD.BlendFunc(SRC_ALPHA, ONE);
    GD.ColorA(fade);
    GD.ColorRGB(85,85,85);
    GD.Vertex2ii(0, 0, PERSONAL_HANDLE, 0);

    // OSHW logo fades in
    GD.ColorRGB(0, 153 * 160 / 255, 176 * 160 / 255);
    GD.Vertex2ii(2, 2, OSHW_HANDLE, 0);
    GD.RestoreContext();

    // Fade to white at the end by drawing a white rectangle on top
    fade = clamp255(5 * (t - 400));
    GD.ColorA(fade);
    GD.Begin(EDGE_STRIP_R);
    GD.Vertex2f(0, 0);
    GD.Vertex2f(0, PIXELS(GD.h));

    GD.swap();
  }
}
