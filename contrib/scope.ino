#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

// Make a diam x diam "soft glow" bitmap

void bitmap(uint32_t addr, int diam, float p)
{
  int r = diam / 2;
  GD.cmd_setbitmap(addr, L8, diam, diam);
  for (int i = 0; i < diam; i++)
    for (int j = 0; j < diam; j++) {
      int x = i - r, y = j - r;
      float d = pow((x * x) + (y * y), p);
      float t = constrain(1.0 - d / r, 0, 1);
      GD.wr(addr + diam * i + j, 255 * t * t);
    }
}

static xy points[1024];
int oldest;

void add(xy &p)
{
  points[oldest] = p;
  oldest = (oldest + 1) & 1023;
}

static xy pos, vel;

void simulate()
{
  pos += vel;
  int f = 1;
  if (pos.x < PIXELS(400))
    vel.x += f;
  else
    vel.x -= f;
  if (pos.y < PIXELS(240))
    vel.y += f;
  else
    vel.y -= f;
  // The "swerve" - every once in a while
  if (GD.random(200) == 0) {
    vel.set(vel.y, -vel.x);
  }
  if (GD.random(60) == 0) {
    vel.x -= vel.x >> 5;
    vel.y -= vel.y >> 5;
  }
}

void setup()
{
  GD.begin(~GD_STORAGE);
  pos.set(PIXELS(100), PIXELS(100));
  vel.set(1, 1);
  GD.BitmapHandle(0);
  bitmap(0, 32, 0.9);
  GD.BitmapHandle(1);
  bitmap(1024, 64, 0.5);

  for (int i = 0; i < 1024; i++) {
    add(pos);
    simulate();
  }
}

void loop()
{
  GD.Clear();

  GD.ColorRGB(0x80c950);          // Greenish
  GD.BlendFunc(SRC_ALPHA, ONE);   // Additive blending
  GD.Begin(BITMAPS);
  GD.BitmapHandle(0);
  GD.VertexTranslateX(PIXELS(-16));
  GD.VertexTranslateY(PIXELS(-16));
  for (int i = 0; i < 1020; i++) {
    if ((i & 3) == 0)
      GD.ColorA(i >> 2);
    points[(oldest + i) & 1023].draw();
  }

  // Draw the last 4 points with a large, bright cursor
  GD.VertexTranslateX(PIXELS(-32));
  GD.VertexTranslateY(PIXELS(-32));
  GD.ColorRGB(0xffff88);          //  bright yellow
  GD.BitmapHandle(1);
  for (int i = 1020; i < 1024; i++)
    points[(oldest + i) & 1023].draw();
  
  GD.swap();

  for (int i = 0; i < 3; i++) {
    add(pos);
    simulate();
  }
}
