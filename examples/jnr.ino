#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

class scroller {
public:
  signed short dragprev;
  int vel;      // velocity
  long base;    // screen x coordinate, in 1/16ths pixel
  long limit;
  void init(uint32_t lim) {
    dragprev = -32768;
    vel = 0;      // velocity
    base = 0;     // screen x coordinate, in 1/16ths pixel
    limit = lim;
  }
  void run(bool touching, int16_t sx) {
    if (touching & (dragprev != -32768)) {
      vel = (dragprev - sx) << 4;
    } else {
      int change = max(1, abs(vel) >> 5);
      if (vel < 0)
        vel += change;
      if (vel > 0)
        vel -= change;
    }
    dragprev = touching ? sx : -32768;
    base += vel;
    base = max(0, min(base, limit));
  }
  uint16_t getval() {
    return base >> 4;
  }
};

scroller xscroll;

#include "jnr_assets.h"

static byte level[240] = {
  0xff,
  0xe2,
  0xe2,
  0xc0,
  0x44,
  0x40,
  0x04,
  0x40,
  0x44,
  0x00,
  0x04,
  0x40,
  0x5c,
  0x08,
};

int mapxy(int x, int y)
{
  if (x < 0)
    return 0;
  if (y < 0)
    return 0;
  if (8 <= y)
    return 0;
  if (240 <= y)
    return 0;
  return 1 & (level[x] >> y);
}

static void parallax(int x, int y)
{
  x %= 400;
  for (int i = 0; i < GD.w; i += 400) {
    GD.Vertex2f(8 * (-x + i), 8 * y);
  }
}

static void draw(int xx)
{
  GD.VertexFormat(3);
  GD.SaveContext();
  GD.Clear();
  GD.ScissorSize(GD.w, GD.h - 32);

  GD.ClearColorRGB(0x2578c5);
  GD.Clear();
  GD.Begin(BITMAPS);

  GD.BitmapHandle(LAYER1_HANDLE);
  GD.ColorRGB(0x9ae8ff);
  parallax(xx >> 4, 0);

  GD.BitmapHandle(LAYER2_HANDLE);
  GD.ColorRGB(0x85d2e9);
  parallax(xx >> 3, 240 - LAYER2_HEIGHT);

  GD.BitmapHandle(LAYER3_HANDLE);
  GD.ColorRGB(0x67b0c5);
  parallax(xx >> 2, 240 - LAYER3_HEIGHT);

  GD.BitmapHandle(LAYER4_HANDLE);
  GD.ColorRGB(0x549faa);
  parallax(xx >> 1, 240 - LAYER4_HEIGHT);

  GD.ColorRGB(0xffffff);
  GD.BitmapHandle(TILES_HANDLE);
  int bx = xx / 32;
  for (int x = 0; x < 16; x++)
    for (int y = 0; y < 8; y++) {
      byte index = 0;
      if (mapxy(bx + x, y)) {
        if (mapxy(bx + x, y - 1))
          index += 1;
        if (mapxy(bx + x + 1, y))
          index += 2;
        if (mapxy(bx + x, y + 1))
          index += 4;
        if (mapxy(bx + x - 1, y))
          index += 8;
      } else {
        index = 17;
      }
      GD.Tag(128 + 8 * x + y);
      GD.Cell(index);
      GD.Vertex2f(8 * (-(xx & 31) + 32 * x), 8 * 32 * y);
    }

  GD.RestoreContext();
  GD.cmd_scale(F16(16), F16(16));
  GD.cmd_setmatrix();
  GD.BitmapHandle(CHECKER_HANDLE);
  GD.BitmapSize(NEAREST, REPEAT, REPEAT, GD.w, 32);
  GD.Cell(0);
  GD.Vertex2f(8 * -(xx & 31), 8 * 240);
}

void setup()
{
  GD.begin(~GD_STORAGE);

  LOAD_ASSETS();

  xscroll.init((240UL * 32) << 4);
  Serial.begin(1000000);    // JCB
}

void loop()
{
  static int prevtag;
  uint16_t bx = xscroll.base >> 4;

  GD.get_inputs();
  int touching = (GD.inputs.x != -32768);
  byte tag = GD.inputs.tag;

  if (prevtag != tag && (128 <= tag)) {
    level[(bx >> 5) + (tag - 128) / 8] |= 1 << (tag & 7);
  }
  prevtag = tag;

  xscroll.run(GD.inputs.y > 240 && touching, GD.inputs.x);
  draw(bx);
#ifdef DUMPDEV  // JCB{
  GD.RestoreContext();
  GD.ColorA(128);
  GD.Begin(POINTS);
  if (touching) {
    int size = 512 - GD.inputs.rz / 3;
    GD.PointSize(min(512, max(size, 0)));
    GD.Vertex2ii(GD.inputs.x, GD.inputs.y, 0, 0);
  }
#endif  // }JCB
  int t;
  GD.swap();
}
