#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#define BACKGROUND "bg.jpg"

Bitmap background;
int brightness = 255;

void setup()
{
#ifndef BACKGROUND
  GD.begin(~GD_STORAGE);
#else
  GD.begin();
  background.fromfile(BACKGROUND);
#endif
  bitmap();
}

// Make a 32x32 "soft glow" bitmap

void bitmap()
{
  GD.cmd_setbitmap(0, L8, 32, 32);
  for (int i = 0; i < 32; i++)
    for (int j = 0; j < 32; j++) {
      int x = i - 16, y = j - 16;
      float d = sqrt((x * x) + (y * y));
      float t = constrain(1.0 - d / 16, 0, 1);
      GD.wr(32 * i + j, 255 * t * t);
    }
}

// Catmull-Rom spline

float spline(float t, float p_1, float p0, float p1, float p2)
{
  return (
        t*((2-t)*t - 1)   * p_1
      + (t*t*(3*t - 5) + 2) * p0
      + t*((4 - 3*t)*t + 1) * p1
      + (t-1)*t*t         * p2 ) / 2;
}

// Return a point on path p

xy &path(float t, xy p[])
{
  static xy pos;
  pos.set(spline(t, p[0].x, p[1].x, p[2].x, p[3].x),
          spline(t, p[0].y, p[1].y, p[2].y, p[3].y));
  return pos;
}

void loop()
{
  GD.Clear();
  GD.ColorRGB(brightness, brightness, brightness);
  background.wallpaper();
  GD.ColorRGB(255, 255, 255);

#if 0
  GD.Begin(BITMAPS);

  GD.ColorRGB(0x335020);          // Greenish
  GD.BlendFunc(SRC_ALPHA, ONE);   // Additive blending

  xy p[4];                        // random control points
  int allon;                      // true if the whole path is onscreen

  do {
    p[1].set(GD.random(PIXELS(GD.w - 32)), GD.random(PIXELS(GD.h - 32)));
    p[2].set(GD.random(PIXELS(GD.w - 32)), GD.random(PIXELS(GD.h - 32)));
    p[0] = p[1];
    p[0].rmove(PIXELS(300), GD.random());
    p[3] = p[2];
    p[3].rmove(PIXELS(300), GD.random());

    allon = 1;
    for (int i = 0; i < 100; i++)
      allon &= path(i * .01, p).onscreen();
  } while (!allon);

  for (int i = 0; i < 100; i++)
    path(i * .01, p).draw();
#endif
  GD.swap();

  // delay(100);
  // GD.wr(REG_PWM_DUTY, 128);
  // delay(GD.random(20, 50));       // range of times to light up, in ms
  // GD.wr(REG_PWM_DUTY, 0);
  // delay(GD.random(300, 500));     // range of times to remain dark, in ms

  GD.get_inputs();
  if (GD.inputs.touching)
    brightness = map(GD.inputs.x, 0, GD.w, 0, 255);
}
