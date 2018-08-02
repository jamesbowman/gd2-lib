#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include "life_assets.h"

const PROGMEM uint8_t ones[] = { ONES };

#define WIDTH 120
#define HEIGHT 68

#define BW    (WIDTH / 8)

uint8_t a[BW * HEIGHT] = {PATTERN};

void setup()
{
  GD.begin(~GD_STORAGE);

  LOAD_ASSETS();

  // for (int i = 0; i < sizeof(a); i++) a[i] = GD.random();

  GD.BitmapHandle(10);
  GD.BitmapSource(ASSETS_END);
  GD.BitmapSize(NEAREST, BORDER, BORDER, GD.w, GD.h);
  GD.BitmapLayout(L1, BW, HEIGHT);
}

// Taken from:
// http://dotat.at/cgi/cvsweb/things/life.c?rev=1.15

#define ADD2(s0,s1,a0,a1) do {          \
            s1 = (a0) & (a1);           \
            s0 = (a0) ^ (a1);           \
        } while (0)
/*!
 * \brief A parallel full-adder.
 * \param s0 Bit 0 of the sum
 * \param s1 Bit 1 of the sum
 * \param a0 First argument
 * \param a1 Second argument
 * \param a2 Third argument
 * \note s0 must not be the same as a2
 */
#define ADD3(s0,s1,a0,a1,a2) do {       \
            unsigned c0, c1;            \
            ADD2(s0,c0,a0,a1);          \
            ADD2(s0,c1,s0,a2);          \
            s1 = c0 | c1;               \
        } while (0)

#define LIFE(NW, N, NE, W, C, E, SW, S, SE) do {    \
      if (y == 0)                                   \
        prevone = 0, prevtwo = 0;                   \
      else {                                        \
        prevC = N;                                  \
        prevL = (N << 1) | (NE >> 7);               \
        prevR = (N >> 1) | (NW << 7);               \
        ADD3(prevone,prevtwo,prevL,prevC,prevR);    \
      }                                             \
      if (y == (HEIGHT - 1))                        \
        nextone = 0, nexttwo = 0;                   \
      else {                                        \
        nextC = S;                                  \
        nextL = (S << 1) | (SE >> 7);               \
        nextR = (S >> 1) | (SW << 7);               \
        ADD3(nextone,nexttwo,nextL,nextC,nextR);    \
      }                                             \
      thisC = C;                                    \
      thisL = (C << 1) | (E >> 7);                  \
      thisR = (C >> 1) | (W << 7);                  \
      ADD2(thisone,thistwo,thisL,        thisR);    \
      ADD3(newone,newtwo,prevone,thisone,nextone);  \
      ADD3(newtwo,new4a, prevtwo,newtwo,nexttwo);   \
      ADD2(newtwo,new4b, thistwo,newtwo);           \
      newone = newone | thisC;                      \
      newbmp = newone & newtwo & ~new4a & ~new4b;   \
      pop += pgm_read_byte_near(ones + newbmp);     \
      *dst++ = newbmp;                              \
      } while (0)

long gen;
long pop;

void loop()
{
  uint8_t prevL, prevC, prevR;
  uint8_t thisL, thisC, thisR;
  uint8_t nextL, nextC, nextR;

  uint8_t thisone, thistwo;
  uint8_t prevone, prevtwo;
  uint8_t nextone, nexttwo;
  uint8_t newbmp, newone, newtwo, new4a, new4b;

  uint32_t t0 = micros();
  uint8_t *p = a, *dst;
  uint8_t scratch[2 * BW];
  if (1) {
    dst = scratch;
    pop = 0;
    for (int y = 0; y < HEIGHT; y++) {
      LIFE(0, p[-BW], p[-BW+1], 0, p[0], p[1], 0, p[BW+0], p[BW+1]);
      p++;
      for (int x = 1; x < (BW - 1); x++) {
        LIFE(p[-BW-1], p[-BW], p[-BW+1], p[-1], p[0], p[1], p[BW-1], p[BW+0], p[BW+1]);
        p++;
      }
      LIFE(p[-BW-1], p[-BW], 0, p[-1], p[0], 0, p[BW-1], p[BW+0], 0);
      p++;
      if (y > 0)
        memcpy(p - 2 * BW, &scratch[BW * (~y & 1)], BW);
      if (y & 1)
        dst = scratch;
    }
    memcpy(p - 2 * BW, scratch, 2 * BW);
  }
  GD.wr_n(ASSETS_END, a, sizeof(a));

  int scale = 256L * WIDTH / GD.w;
  GD.BitmapTransformA(scale);
  GD.BitmapTransformE(scale);
  GD.cmd_gradient(0, 0, 0x003333, 0, GD.h, 0x202020);
  GD.Begin(BITMAPS);
  GD.ColorRGB(0x304030);
  GD.BlendFunc(SRC_ALPHA, 1);
  GD.Vertex2ii(0, 0, 10);
  GD.Vertex2ii(1, 0, 10);
  GD.Vertex2ii(0, 1, 10);
  GD.Vertex2ii(1, 1, 10);

  int x0 = PIXELS(GD.w) / 32;
  int x1 = PIXELS(GD.w) - x0;
  int y0 = PIXELS(16);
  int y1 = PIXELS(16 + JURA_HEIGHT + 2);

  // A transparent black rectangle
  GD.RestoreContext();
  GD.ColorRGB(0,0,0);
  GD.ColorA(144);
  GD.Begin(RECTS);
  GD.Vertex2f(x0, y0);
  GD.Vertex2f(x1, y1);

  // Thin yellow outline
  GD.RestoreContext();
  GD.ColorRGB(0xffa500U);

  GD.LineWidth(PIXELS(0.375));
  GD.Begin(LINE_STRIP);
  GD.Vertex2f(x0, y0);
  GD.Vertex2f(x0, y1);
  GD.Vertex2f(x1, y1);
  GD.Vertex2f(x1, y0);
  GD.Vertex2f(x0, y0);

  // The text
  int x = 1 * GD.w / 4;
  GD.cmd_text(x, 16, JURA_HANDLE, OPT_RIGHTX, "Gen: ");
  GD.cmd_number(x, 16, JURA_HANDLE, 0, gen++);
  GD.cmd_text(GD.w - x, 16, JURA_HANDLE, OPT_RIGHTX, "Pop: ");
  GD.cmd_number(GD.w - x, 16, JURA_HANDLE, 0, pop);
  GD.swap(); // GD.dumpscreen();

  GD.get_inputs();
  if (GD.inputs.x != -32768) {
    gen = 0;
    for (int i = 0; i < sizeof(a); i++) a[i] = GD.random();
  }
}
