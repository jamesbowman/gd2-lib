#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include "kenney_assets.h"

#undef PLAYER1_SIZE
#define PLAYER1_SIZE  (PLAYER1_HEIGHT + 8)
#define HEART_SZ    (HEART_HEIGHT + 14)

static void rotate_player(int a)
{
    GD.cmd_loadidentity();
    GD.cmd_translate(F16(PLAYER1_SIZE / 2),F16(PLAYER1_SIZE / 2));
    GD.cmd_rotate(a);
    GD.cmd_translate(F16(-PLAYER1_WIDTH / 2),F16(-PLAYER1_HEIGHT / 2));
    GD.cmd_setmatrix();
}

static void rotate_heart(int a)
{
    GD.cmd_loadidentity();
    GD.cmd_translate(F16(HEART_SZ / 2),F16(HEART_SZ / 2));
    GD.cmd_rotate(a);
    GD.cmd_translate(F16(-HEART_WIDTH / 2),F16(-HEART_HEIGHT / 2));
    GD.cmd_setmatrix();
}

#define BLOBS 32
class Trail {
  public:
    int idx;
    int x[BLOBS];
    int y[BLOBS];
    int sz[BLOBS];
    void add(int nx, int ny) {
      x[idx] = nx;
      y[idx] = ny;
      sz[idx] = 192;
      idx = (idx + 1) % BLOBS;
    }
    void draw(int sy) {
      for (int i = 0; i < BLOBS; i++) {
        if (sz[i]) {
          GD.PointSize(sz[i]);
          GD.Vertex2f(x[i], -(sy << 4) + y[i]);
          sz[i] -= 1;
          y[i] += 12;
        }
      }
    }
};

struct state_t {
  xy p[3];
  Trail trail[3];
  xy hearts[20];
  xy clouds[20];
} state;

static void polar(uint16_t th, int r)
{
  int x, y;
  GD.polar(x, y, r, th);
  GD.Vertex2f(16 * 240 + x, 16 * 136 + y);
}

#define RAYS    7
#define RAYSIZE (65536 / RAYS)
#define RAYEDGE (4 * 16)

static void ray(uint16_t th, int r0, int r1)
{
  polar(th, r0);
  polar(th, r1);
  polar(th + RAYSIZE / 2, r1);
  polar(th + RAYSIZE / 2, r0);
  polar(th, r0);
}

// c1 is edge color, c2 is interior color

static void burst(uint16_t th, int r, uint32_t c1, uint32_t c2)
{
  GD.Clear(0,1,0);
  GD.ColorMask(0,0,0,0);
  GD.StencilOp(KEEP, INVERT);
  GD.StencilFunc(ALWAYS, 255, 255);
  for (int i = 0; i < RAYS; i++) {
    GD.Begin(EDGE_STRIP_A);
    ray(th + (i * RAYSIZE), r / 4, r + (r >> 4));
  }
  GD.ColorMask(1,1,1,1);
  GD.StencilFunc(EQUAL, 255, 255);
  GD.StencilOp(KEEP, KEEP);

  GD.Begin(POINTS);
  GD.ColorRGB(c1);
  GD.PointSize(r);
  GD.Vertex2ii(240, 136);
  GD.ColorRGB(c2);
  GD.PointSize(r - RAYEDGE);
  GD.Vertex2ii(240, 136);
  GD.ColorRGB(c1);
  GD.PointSize((r / 4) + RAYEDGE);
  GD.Vertex2ii(240, 136);

  GD.StencilFunc(ALWAYS, 255, 255);
  GD.LineWidth(RAYEDGE / 2);
  for (int i = 0; i < RAYS; i++) {
    GD.Begin(LINES);
    ray(th + (i * RAYSIZE), r / 4 + (RAYEDGE / 2), r - (RAYEDGE / 2));
  }
}

static void sunrise(int t)
{
  static int r = 0, v = 0;
  // yellow   0xffcc00
  // reddish  0xe86a17
  // blue     0x1ea7e1
  // green    0x73cd4b

  GD.ColorRGB(C1B);
  GD.PointSize((r * 3) >> 4);
  GD.Begin(POINTS);
  GD.Vertex2f(16 * 240, 16 * 136);

  // GD.ColorA(min(255, t * 6));
  if (t == 0) {
    r = 0;
    v = 0;
  }
  burst(t * -261, r + (r >> 1), C2B, C2);
  burst(t * 335, r, C1B, C1);
  int f = ((16L * 130) - r) / 29;
  v = ((v + f) * 243L) >> 8;
  r += v;
}

static void draw(int t)
{
  int y = 1648 - t;
  // GD.finish(); long t0 = micros();

  // GD.ClearColorRGB(0xd0f4f7); GD.Clear();
  GD.cmd_gradient(0,   0, 0xa0a4f7,     //' gradient{
                  0, 272, 0xd0f4f7);  //' }gradient

  if (360 <= t) {
    GD.RestoreContext();
    sunrise(t - 360);
  }

  GD.Begin(BITMAPS);  //' clouds{
  GD.BlendFunc(ONE, ONE_MINUS_SRC_ALPHA);
  GD.BitmapHandle(CLOUD_HANDLE);
  GD.Cell(0);
  for (int i = 0; i < 20; i++) {
    byte lum = 128 + 5 * i;
    GD.ColorA(lum);
    GD.ColorRGB(lum, lum, lum);
    GD.Vertex2f(state.clouds[i].x, state.clouds[i].y);
    state.clouds[i].y += (4 + (i >> 3));
    if (state.clouds[i].y > (16 * 272))
      state.clouds[i].y -= 16 * (272 + CLOUD_HEIGHT);
  } //' }clouds

  GD.RestoreContext();
  GD.BlendFunc(ONE, ONE_MINUS_SRC_ALPHA);
  GD.Begin(BITMAPS); //' tiles{
  GD.BitmapHandle(TILES_HANDLE);
  const PROGMEM uint8_t *src = layer1_map + (y >> 5) * 15;
  byte yo = y & 31;
  for (byte j = 0; j < 10; j++)
    for (byte i = 0; i < 15; i++) {
      byte t = pgm_read_byte_near(src++);
      if (t != 0) {
        GD.Cell(t - 1);
        GD.Vertex2f(16 * 32 * i, 16 * ((32 * j) - yo));
      }
    } //' }tiles
  // GD.BlendFunc(SRC_ALPHA, ZERO);

  uint16_t a = t * 100;

  uint16_t a2 = a << 1;
  uint16_t a3 = a2 << 1;

  state.p[0].x = 16 * 240 + GD.rsin(16 * 120, a) + GD.rsin(16 * 120, a2);
  state.p[0].y = 16 * 136 + GD.rsin(16 * 70, a3);

  state.p[1].x = 16 * 240 + GD.rsin(16 * 240, a);
  state.p[1].y = 16 * 100 + GD.rsin(16 * 36, a2);

  state.p[2].x = 16 * 240 + GD.rsin(16 * 240, a2);
  state.p[2].y = 16 * 135 + GD.rsin(16 * 10, a) + GD.rsin(16 * 18, a3);

  for (int i = 0; i < 3; i++) {
    if ((t & 3) == 0) {
      state.trail[i].add(state.p[i].x, (y << 4) + state.p[i].y);
    }

    GD.BlendFunc(SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
    GD.Begin(POINTS);
    GD.ColorA(0x90);
    uint32_t colors[3] = {0x8bcfba, 0x8db5e7, 0xf19cb7};
    GD.ColorRGB(colors[i]);
    state.trail[i].draw(y);
  }

  GD.ColorRGB(0xffffff);
  GD.ColorA(0xff);
  GD.BlendFunc(ONE, ONE_MINUS_SRC_ALPHA);
  GD.BitmapHandle(PLAYER1_HANDLE);  //' player{
  GD.Begin(BITMAPS);
  for (int i = 0; i < 3; i++) {
    rotate_player(a + i * 0x7000);
    GD.Cell(i);
    GD.Vertex2f(state.p[i].x - (16 * PLAYER1_SIZE / 2),
                state.p[i].y - (16 * PLAYER1_SIZE / 2));
  } //' }player

  if (t > 480) {
    GD.BlendFunc(SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
    GD.ColorA(min((t - 480) * 4, 255));
    GD.Begin(BITMAPS);
    GD.BitmapHandle(HEART_HANDLE);
    GD.Cell(0);

    for (int i = 0; i < 20; i++) {
      if ((i & 3) == 0)
        rotate_heart(a + (i << 12));
      GD.Vertex2f(state.hearts[i].x - (16 * HEART_SZ / 2), state.hearts[i].y);
      state.hearts[i].y += 30 + (i << 2);
      if (state.hearts[i].y > (16 * 272))
        state.hearts[i].y -= 16 * (272 + HEART_SZ);
    }
  }

  // GD.RestoreContext(); GD.cmd_number(0, 0, 26, 6, micros() - t0);

  GD.swap();
}

void setup()
{
  GD.begin(~GD_STORAGE);
  Serial.begin(1000000);
  // GD.wr32(REG_PWM_HZ, 18000); GD.wr(REG_PWM_DUTY, 64);

  GD.Clear();
  LOAD_ASSETS();

  // Handle     Graphic
  //   0        Tiles

  byte hy[] = {36, 12, 144, 204, 216, 120, 48, 168, 192, 84, 72, 60, 24, 180, 0, 108, 228, 132, 156, 96};
  for (int i = 0; i < 20; i++) {
    state.hearts[i].x = random(16 * 480);
    state.hearts[i].y = 16 * hy[i];
    state.clouds[i].x = 16 * (random(480) - (CLOUD_WIDTH / 2));
    state.clouds[i].y = 16 * hy[i];
  }
}

void loop()
{
  for (int t = 0; t < 720; t++) {
    draw(t);
  }
}
