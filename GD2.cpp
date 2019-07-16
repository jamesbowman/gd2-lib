/*
}
 * Copyright (C) 2013-2016 by James Bowman <jamesb@excamera.com>
 * Gameduino 2/3 library for Arduino, Arduino Due, Raspberry Pi,
 * Teensy 3.2, ESP8266 and ESP32.
 *
 */

#include <Arduino.h>
#include "SPI.h"
#if !defined(__SAM3X8E__)
#include "EEPROM.h"
#endif
#define VERBOSE       0
#include <GD2.h>

#ifdef DUMPDEV
#include <assert.h>
#include "transports/dump.h"
#endif

#ifdef RASPBERRY_PI
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "transports/spidev.h"
#endif

byte ft8xx_model;

#if defined(ARDUINO)
#include "transports/wiring.h"
#endif

#if defined(SPIDRIVER)
#include "transports/tr-spidriver.h"
#endif

////////////////////////////////////////////////////////////////////////

void xy::set(int _x, int _y)
{
  x = _x;
  y = _y;
}

void xy::rmove(int distance, int angle)
{
  x -= GD.rsin(distance, angle);
  y += GD.rcos(distance, angle);
}

int xy::angleto(class xy &other)
{
  int dx = other.x - x, dy = other.y - y;
  return GD.atan2(dy, dx);
}

void xy::draw(byte offset)
{
  GD.Vertex2f(x - PIXELS(offset), y - PIXELS(offset));
}

int xy::onscreen(void)
{
  return (0 <= x) &&
         (x < PIXELS(GD.w)) &&
         (0 <= y) &&
         (y < PIXELS(GD.h));
}

class xy xy::operator+=(class xy &other)
{
  x += other.x;
  y += other.y;
  return *this;
}

class xy xy::operator-=(class xy &other)
{
  x -= other.x;
  y -= other.y;
  return *this;
}

class xy xy::operator<<=(int d)
{
  x <<= d;
  y <<= d;
  return *this;
}

long xy::operator*(class xy &other)
{
  return (long(x) * other.x) + (long(y) * other.y);
}

class xy xy::operator*=(int s)
{
  x *= s;
  y *= s;
  return *this;
}

int xy::nearer_than(int distance, xy &other)
{
  int lx = abs(x - other.x);
  if (lx > distance)
    return 0;
  int ly = abs(y - other.y);
  if (ly > distance)
    return 0;

  // trivial accept: 5/8 is smaller than 1/sqrt(2)
  int d2 = (5 * distance) >> 3;
  if ((lx < d2) && (ly < d2))
    return 1;

#define SQ(c) (long(c) * (c))
  return (SQ(lx) + SQ(ly)) < SQ(distance);
#undef SQ
}

void xy::rotate(int angle)
{
  // the hardware's convention that rotation is clockwise
  int32_t s = GD.rsin(32767, angle);
  int32_t c = GD.rcos(32767, angle);

  int xr = ((x * c) - (y * s)) >> 15;
  int yr = ((x * s) + (y * c)) >> 15;
  x = xr;
  y = yr;
}

////////////////////////////////////////////////////////////////////////

void Bitmap::fromtext(int font, const char* s)
{
  GD.textsize(size.x, size.y, font, s);
  int pclk = GD.rd16(REG_PCLK);
  int vsize = GD.rd16(REG_VSIZE);
  int hsize = GD.rd16(REG_HSIZE);

  GD.finish();
  GD.wr(REG_PCLK, 0);
  delay(1);
  GD.wr16(REG_HSIZE, size.x);
  GD.wr16(REG_VSIZE, size.y);

  GD.cmd_dlstart();
  GD.Clear();
  GD.BlendFunc(1,1);
  GD.cmd_text(0, 0, font, 0, s);
  GD.swap();

  GD.loadptr = (GD.loadptr + 1) & ~1;
  GD.cmd_snapshot(GD.loadptr);
  GD.finish();

  GD.wr16(REG_HSIZE, hsize);
  GD.wr16(REG_VSIZE, vsize);
  GD.wr16(REG_PCLK, pclk);
  defaults(ARGB4);
}

void Bitmap::fromfile(const char* filename, int format)
{
  GD.loadptr = (GD.loadptr + 1) & ~1;
  GD.cmd_loadimage(GD.loadptr, OPT_NODL);
  GD.load(filename);
  uint32_t ptr, w, h;
  GD.cmd_getprops(ptr, w, h);
  GD.finish();
  size.x = GD.rd16(w);
  size.y = GD.rd16(h);
  defaults(format);
}

static const PROGMEM uint8_t bpltab[] = {
/* 0  ARGB1555  */ 0xff,
/* 1  L1        */ 3,
/* 2  L4        */ 1,
/* 3  L8        */ 0,
/* 4  RGB332    */ 0,
/* 5  ARGB2     */ 0,
/* 6  ARGB4     */ 0xff,
/* 7  RGB565    */ 0xff,
/* 8  PALETTED  */ 0,
/* 9  TEXT8X8   */ 0xff,
/* 10 TEXTVGA   */ 0xff,
/* 11 BARGRAPH  */ 0,
/* 12           */ 0xff,
/* 13           */ 0xff,
/* 14           */ 0xff,
/* 15           */ 0xff,
/* 16           */ 0xff,
/* 17 L2        */ 2
};

static uint16_t stride(uint8_t format, uint16_t w)
{
  uint8_t k = pgm_read_byte_near(bpltab + format);
  if (k == 0xff)
    return w << 1;
  uint16_t r = (1 << k) - 1;
  return (w + r) >> k;
}

void Bitmap::defaults(uint8_t f)
{
  source = GD.loadptr;
  format = f;
  handle = -1;
  center.x = size.x / 2;
  center.y = size.y / 2;
  GD.loadptr += stride(format, size.x) * size.y;
}

void Bitmap::setup(void)
{
  GD.BitmapSource(source);
  GD.BitmapLayout(format, stride(format, size.x), size.y);
  GD.BitmapSize(NEAREST, BORDER, BORDER, size.x, size.y);
}

void Bitmap::bind(uint8_t h)
{
  handle = h;

  GD.BitmapHandle(handle);
  setup();
}

#define IS_POWER_2(x) (((x) & ((x) - 1)) == 0)

void Bitmap::wallpaper()
{
  if (handle == -1) {
    GD.BitmapHandle(15);
    setup();
  } else {
    GD.BitmapHandle(handle);
  }
  GD.Begin(BITMAPS);
  // if power-of-2, can just use REPEAT,REPEAT
  // otherwise must draw it across whole screen
  if (IS_POWER_2(size.x) && IS_POWER_2(size.y)) {
    GD.BitmapSize(NEAREST, REPEAT, REPEAT, GD.w, GD.h);
    GD.Vertex2f(0, 0);
  } else {
    for (int x = 0; x < GD.w; x += size.x)
      for (int y = 0; y < GD.h; y += size.y)
        GD.Vertex2f(x << 4, y << 4);
  }
}

void Bitmap::draw(int x, int y, int16_t angle)
{
  xy pos;
  pos.set(x, y);
  pos <<= GD.vxf;
  draw(pos, angle);
}

void Bitmap::draw(const xy &p, int16_t angle)
{
  xy pos = p;
  if (handle == -1) {
    GD.BitmapHandle(15);
    setup();
  } else {
    GD.BitmapHandle(handle);
  }
  GD.Begin(BITMAPS);

  if (angle == 0) {
    xy c4 = center;
    c4 <<= GD.vxf;
    pos -= c4;
    GD.BitmapSize(NEAREST, BORDER, BORDER, size.x, size.y);
    GD.Vertex2f(pos.x, pos.y);
  } else {
    // Compute the screen positions of 4 corners of the bitmap
    xy corners[4] = {
      {0,0              },
      {size.x,  0       },
      {0,       size.y  },
      {size.x,  size.y  },
    };
    for (int i = 0; i < 4; i++) {
      xy &c = corners[i];
      c -= center;
      c <<= GD.vxf;
      c.rotate(angle);
      c += pos;
    }

    // Find top-left and bottom-right boundaries
    xy topleft, bottomright;
    topleft.set(
      min(min(corners[0].x, corners[1].x), min(corners[2].x, corners[3].x)),
      min(min(corners[0].y, corners[1].y), min(corners[2].y, corners[3].y)));
    bottomright.set(
      max(max(corners[0].x, corners[1].x), max(corners[2].x, corners[3].x)),
      max(max(corners[0].y, corners[1].y), max(corners[2].y, corners[3].y)));

    // span is the total size of this region
    xy span = bottomright;
    span -= topleft;
    GD.BitmapSize(BILINEAR, BORDER, BORDER,
                  (span.x + 15) >> GD.vxf, (span.y + 15) >> GD.vxf);

    // Set up the transform and draw the bitmap
    pos -= topleft;
    GD.SaveContext();
    GD.cmd_loadidentity();
    int s = 16 - GD.vxf;
    GD.cmd_translate((int32_t)pos.x << s, (int32_t)pos.y << s);
    GD.cmd_rotate(angle);
    GD.cmd_translate(F16(-center.x), F16(-center.y));
    GD.cmd_setmatrix();
    GD.Vertex2f(topleft.x, topleft.y);
    GD.RestoreContext();
  }
}

class Bitmap __fromatlas(uint32_t a)
{
  Bitmap r;
  r.size.x    = GD.rd16(a);
  r.size.y    = GD.rd16(a + 2);
  r.center.x  = GD.rd16(a + 4);
  r.center.y  = GD.rd16(a + 6);
  r.source    = GD.rd32(a + 8);
  r.format    = GD.rd(a + 12);
  r.handle    = -1;
  return r;
}

////////////////////////////////////////////////////////////////////////

static GDTransport GDTR;

GDClass GD;

////////////////////////////////////////////////////////////////////////

// The GD3 has a tiny configuration EEPROM - AT24C01D
// It is programmed at manufacturing time with the setup
// commands for the connected panel. The SCL,SDA lines
// are connected to the FT81x GPIO0, GPIO1 signals.
// This is a read-only driver for it.  A single method
// 'read()' initializes the RAM and reads all 128 bytes
// into an array.

class ConfigRam {
private:
  uint8_t gpio, gpio_dir, sda;
  void set_SDA(byte n)
  {
    if (sda != n) {
      GDTR.__wr16(REG_GPIO_DIR, gpio_dir | (0x03 - n));    // Drive SCL, SDA low
      sda = n;
    }
  }

  void set_SCL(byte n)
  {
    GDTR.__wr16(REG_GPIO, gpio | (n << 1));
  }

  int get_SDA(void)
  {
    return GDTR.__rd16(REG_GPIO) & 1;
  }

  void i2c_start(void)
  {
    set_SDA(1);
    set_SCL(1);
    set_SDA(0);
    set_SCL(0);
  }

  void i2c_stop(void)
  {
    set_SDA(0);
    set_SCL(1);
    set_SDA(1);
    set_SCL(1);
  }

  int i2c_rx1()
  {
    set_SDA(1);
    set_SCL(1);
    byte r = get_SDA();
    set_SCL(0);
    return r;
  }

  void i2c_tx1(byte b)
  {
    set_SDA(b);
    set_SCL(1);
    set_SCL(0);
  }

  int i2c_tx(byte x)
  {
    for (byte i = 0; i < 8; i++, x <<= 1)
      i2c_tx1(x >> 7);
    return i2c_rx1();
  }

  int i2c_rx(int nak)
  {
    byte r = 0;
    for (byte i = 0; i < 8; i++)
      r = (r << 1) | i2c_rx1();
    i2c_tx1(nak);
    return r;
  }

public:
  void read(byte *v)
  {
    GDTR.__end();
    gpio = GDTR.__rd16(REG_GPIO) & ~3;
    gpio_dir = GDTR.__rd16(REG_GPIO_DIR) & ~3;
    sda = 2;

    // 2-wire software reset
    i2c_start();
    i2c_rx(1);
    i2c_start();
    i2c_stop();

    int ADDR = 0xa0;

    i2c_start();
    if (i2c_tx(ADDR))
      return;
    if (i2c_tx(0))
      return;

    i2c_start();
    if (i2c_tx(ADDR | 1))
      return;
    for (int i = 0; i < 128; i++) {
      *v++ = i2c_rx(i == 127);
      // Serial.println(v[-1], DEC);
    }
    i2c_stop();
    GDTR.resume();
  }
};

void GDClass::flush(void)
{
  GDTR.flush();
}

void GDClass::swap(void) {
  Display();
  cmd_swap();
  cmd_loadidentity();
  cmd_dlstart();
  GDTR.flush();
#ifdef DUMPDEV
  GDTR.swap();
#endif
}

uint32_t GDClass::measure_freq(void)
{
  unsigned long t0 = GDTR.rd32(REG_CLOCK);
  delayMicroseconds(15625);
  unsigned long t1 = GDTR.rd32(REG_CLOCK);
  // Serial.println((t1 - t0) << 6);
  return (t1 - t0) << 6;
}

#define LOW_FREQ_BOUND  47040000UL
// #define LOW_FREQ_BOUND  32040000UL

void GDClass::tune(void)
{
  uint32_t f;
  for (byte i = 0; (i < 31) && ((f = measure_freq()) < LOW_FREQ_BOUND); i++) {
    GDTR.wr(REG_TRIM, i);
  }
  GDTR.wr32(REG_FREQUENCY, f);
}

void GDClass::begin(uint8_t options, int cs, int sdcs) {
#if defined(ARDUINO) || defined(ESP8266) || defined(ESP32) || defined(SPIDRIVER)
  GDTR.begin0(cs);
  if (STORAGE && (options & GD_STORAGE))
    SD.begin(sdcs);
#endif

  byte external_crystal = 0;
begin1:
  GDTR.begin1();

  Clear();
  swap();
  finish();

#if 0
  Serial.print("GD options:");
  Serial.println(options, DEC);
  Serial.print("cs:");
  Serial.println(cs, DEC);
  Serial.print("sdcs:");
  Serial.println(sdcs, DEC);
  Serial.print("BOARD");
  Serial.println(BOARD, DEC);

  Serial.println("ID REGISTER:");
  Serial.println(GDTR.rd32(REG_ID), HEX);
  Serial.println("READ PTR:");
  Serial.println(GDTR.rd32(REG_CMD_READ), HEX);
  Serial.println("WRITE PTR:");
  Serial.println(GDTR.rd32(REG_CMD_WRITE), HEX);
  Serial.println("CMDB SPACE:");
  Serial.println(GDTR.rd32(REG_CMDB_SPACE), HEX);
#endif

#if (BOARD == BOARD_FTDI_80x)
  GDTR.wr(REG_PCLK_POL, 1);
  GDTR.wr(REG_PCLK, 5);
#endif

#if (BOARD == BOARD_SUNFLOWER)
  GDTR.wr32(REG_HSIZE, 320);
  GDTR.wr32(REG_VSIZE, 240);
  GDTR.wr32(REG_HCYCLE, 408);
  GDTR.wr32(REG_HOFFSET, 70);
  GDTR.wr32(REG_HSYNC0, 0);
  GDTR.wr32(REG_HSYNC1, 10);
  GDTR.wr32(REG_VCYCLE, 263);
  GDTR.wr32(REG_VOFFSET, 13);
  GDTR.wr32(REG_VSYNC0, 0);
  GDTR.wr32(REG_VSYNC1, 2);
  GDTR.wr32(REG_PCLK, 8);
  GDTR.wr32(REG_PCLK_POL, 0);
  GDTR.wr32(REG_CSPREAD, 1);
  GDTR.wr32(REG_DITHER, 1);
  GDTR.wr32(REG_ROTATE, 0);
  GDTR.wr(REG_SWIZZLE, 2);
#endif

  GDTR.wr(REG_PWM_DUTY, 0);
#if BOARD != BOARD_OTHER
  GDTR.wr(REG_GPIO_DIR, 0x83);
  GDTR.wr(REG_GPIO, GDTR.rd(REG_GPIO) | 0x80);
#endif

#if (BOARD == BOARD_GAMEDUINO23)
  Clear(); swap();
  switch (ft8xx_model) {

  case 1: { // GD3 (810-series) have config in attached I2C "ConfigRam"
    ConfigRam cr;
    byte v8[128] = {0};
    cr.read(v8);
    if ((v8[1] == 0xff) && (v8[2] == 0x01)) {
      options &= ~(GD_TRIM | GD_CALIBRATE);
      if (!external_crystal && (v8[3] & 2)) {
        GDTR.external_crystal();
        external_crystal = 1;
        goto begin1;
      }
      copyram(v8 + 4, 124);
      break;
    } else
      goto fallback;
  }

  case 2: { // GD3X (815-series) have config in flash
    uint32_t fr;
    cmd_flashfast(fr);
    finish();
    if (rd32(fr) == 0) {
      uint32_t a = 0xff000UL;
      cmd_flashread(a, 0x1000, 0x1000);
      GD.finish();
      if (rd32(a + 0xffc) == 0x7C6A0100UL) {
        for (int i = 0; i < 128; i++)
          cI(rd32(a + 4 * i));
        options &= ~(GD_TRIM | GD_CALIBRATE);
        break;
      }
    }
  }

  default:
  fallback:
    cmd_regwrite(REG_OUTBITS, 0666);
    cmd_regwrite(REG_DITHER, 1);
    cmd_regwrite(REG_ROTATE, 1);
    cmd_regwrite(REG_SWIZZLE, 3);
    cmd_regwrite(REG_PCLK_POL, 1);
    cmd_regwrite(REG_PCLK, 5);
  }
#endif

  GD.finish();
  w = GDTR.rd16(REG_HSIZE);
  h = GDTR.rd16(REG_VSIZE);
  loadptr = 0;

  // Work-around issue with bitmap sizes not being reset
  for (byte i = 0; i < 32; i++) {
    BitmapHandle(i);
    cI(0x28000000UL);
    cI(0x29000000UL);
  }

  Clear(); swap();
  Clear(); swap();
  Clear(); swap();
  cmd_regwrite(REG_PWM_DUTY, 128);
  flush();

  if (CALIBRATION & (options & GD_CALIBRATE)) {

#if defined(ARDUINO) && !defined(__DUE__)
    if ((EEPROM.read(0) != 0x7c)) {
      self_calibrate();
      // for (int i = 0; i < 24; i++) Serial.println(GDTR.rd(REG_TOUCH_TRANSFORM_A + i), HEX);
      for (int i = 0; i < 24; i++)
        EEPROM.write(1 + i, GDTR.rd(REG_TOUCH_TRANSFORM_A + i));
      EEPROM.write(0, 0x7c);  // is written!
    } else {
      for (int i = 0; i < 24; i++)
        GDTR.wr(REG_TOUCH_TRANSFORM_A + i, EEPROM.read(1 + i));
    }
#endif

#ifdef __DUE__
    // The Due has no persistent storage. So instead use a "canned"
    // calibration.

    // self_calibrate();
    // for (int i = 0; i < 24; i++)
    //   Serial.println(GDTR.rd(REG_TOUCH_TRANSFORM_A + i), HEX);
    static const byte canned_calibration[24] = {
      0xCC, 0x7C, 0xFF, 0xFF, 0x57, 0xFE, 0xFF, 0xFF,
      0xA1, 0x04, 0xF9, 0x01, 0x93, 0x00, 0x00, 0x00,
      0x5E, 0x4B, 0x00, 0x00, 0x08, 0x8B, 0xF1, 0xFF };
    for (int i = 0; i < 24; i++)
      GDTR.wr(REG_TOUCH_TRANSFORM_A + i, canned_calibration[i]);
#endif

#if defined(RASPBERRY_PI)
    {
      uint8_t cal[24];
      FILE *calfile = fopen(".calibration", "r");
      if (calfile == NULL) {
        calfile = fopen(".calibration", "w");
        if (calfile != NULL) {
          self_calibrate();
          for (int i = 0; i < 24; i++)
            cal[i] = GDTR.rd(REG_TOUCH_TRANSFORM_A + i);
          fwrite(cal, 1, sizeof(cal), calfile);
          fclose(calfile);
        }
      } else {
        fread(cal, 1, sizeof(cal), calfile);
        for (int i = 0; i < 24; i++)
          GDTR.wr(REG_TOUCH_TRANSFORM_A + i, cal[i]);
        fclose(calfile);
      }
    }
#endif

  }

  GDTR.wr16(REG_TOUCH_RZTHRESH, 1200);

  rseed = 0x77777777;
  cprim = 0xff;
  vxf = 4;

  if ((BOARD == BOARD_GAMEDUINO23) && (options & GD_TRIM)) {
    tune();
  }
  finish();
}

void GDClass::storage(void) {
  GDTR.__end();
  SD.begin(SD_PIN);
  GDTR.resume();
}

void GDClass::self_calibrate(void) {
  cmd_dlstart();
  Clear();
  cmd_text(w / 2, h / 2, 30, OPT_CENTER, "please tap on the dot");
  cmd_calibrate();
  finish();
  cmd_loadidentity();
  cmd_dlstart();
  GDTR.flush();
}

void GDClass::seed(uint16_t n) {
  rseed = n ? n : 7;
}

uint16_t GDClass::random() {
  rseed ^= rseed << 2;
  rseed ^= rseed >> 5;
  rseed ^= rseed << 1;
  return rseed;
}

uint16_t GDClass::random(uint16_t n) {
  uint16_t p = random();
  if (n == (n & -n))
    return p & (n - 1);
  return (uint32_t(p) * n) >> 16;
}

uint16_t GDClass::random(uint16_t n0, uint16_t n1) {
  return n0 + random(n1 - n0);
}

// >>> [int(65535*math.sin(math.pi * 2 * i / 1024)) for i in range(257)]
static const PROGMEM uint16_t sintab[257] = {
0, 402, 804, 1206, 1608, 2010, 2412, 2813, 3215, 3617, 4018, 4419, 4821, 5221, 5622, 6023, 6423, 6823, 7223, 7622, 8022, 8421, 8819, 9218, 9615, 10013, 10410, 10807, 11203, 11599, 11995, 12390, 12785, 13179, 13573, 13966, 14358, 14750, 15142, 15533, 15923, 16313, 16702, 17091, 17479, 17866, 18252, 18638, 19023, 19408, 19791, 20174, 20557, 20938, 21319, 21699, 22078, 22456, 22833, 23210, 23585, 23960, 24334, 24707, 25079, 25450, 25820, 26189, 26557, 26924, 27290, 27655, 28019, 28382, 28744, 29105, 29465, 29823, 30181, 30537, 30892, 31247, 31599, 31951, 32302, 32651, 32999, 33346, 33691, 34035, 34378, 34720, 35061, 35400, 35737, 36074, 36409, 36742, 37075, 37406, 37735, 38063, 38390, 38715, 39039, 39361, 39682, 40001, 40319, 40635, 40950, 41263, 41574, 41885, 42193, 42500, 42805, 43109, 43411, 43711, 44010, 44307, 44603, 44896, 45189, 45479, 45768, 46055, 46340, 46623, 46905, 47185, 47463, 47739, 48014, 48287, 48558, 48827, 49094, 49360, 49623, 49885, 50145, 50403, 50659, 50913, 51165, 51415, 51664, 51910, 52155, 52397, 52638, 52876, 53113, 53347, 53580, 53810, 54039, 54265, 54490, 54712, 54933, 55151, 55367, 55581, 55793, 56003, 56211, 56416, 56620, 56821, 57021, 57218, 57413, 57606, 57796, 57985, 58171, 58355, 58537, 58717, 58894, 59069, 59242, 59413, 59582, 59748, 59912, 60074, 60234, 60391, 60546, 60699, 60849, 60997, 61143, 61287, 61428, 61567, 61704, 61838, 61970, 62100, 62227, 62352, 62474, 62595, 62713, 62828, 62941, 63052, 63161, 63267, 63370, 63472, 63570, 63667, 63761, 63853, 63942, 64029, 64114, 64196, 64275, 64353, 64427, 64500, 64570, 64637, 64702, 64765, 64825, 64883, 64938, 64991, 65042, 65090, 65135, 65178, 65219, 65257, 65293, 65326, 65357, 65385, 65411, 65435, 65456, 65474, 65490, 65504, 65515, 65523, 65530, 65533, 65535
};

int16_t GDClass::rsin(int16_t r, uint16_t th) {
  th >>= 6; // angle 0-1023
  // return int(r * sin((2 * M_PI) * th / 1024.));
  int th4 = th & 511;
  if (th4 & 256)
    th4 = 512 - th4; // 256->256 257->255, etc
  uint16_t s = pgm_read_word_near(sintab + th4);
  int16_t p = ((uint32_t)s * r) >> 16;
  if (th & 512)
    p = -p;
  return p;
}

int16_t GDClass::rcos(int16_t r, uint16_t th) {
  return rsin(r, th + 0x4000);
}

void GDClass::polar(int &x, int &y, int16_t r, uint16_t th) {
  x = (int)(-GD.rsin(r, th));
  y = (int)( GD.rcos(r, th));
}

// >>> [int(round(1024 * math.atan(i / 256.) / math.pi)) for i in range(256)]
static const PROGMEM uint8_t atan8[] = {
0,1,3,4,5,6,8,9,10,11,13,14,15,17,18,19,20,22,23,24,25,27,28,29,30,32,33,34,36,37,38,39,41,42,43,44,46,47,48,49,51,52,53,54,55,57,58,59,60,62,63,64,65,67,68,69,70,71,73,74,75,76,77,79,80,81,82,83,85,86,87,88,89,91,92,93,94,95,96,98,99,100,101,102,103,104,106,107,108,109,110,111,112,114,115,116,117,118,119,120,121,122,124,125,126,127,128,129,130,131,132,133,134,135,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,177,178,179,180,181,182,183,184,185,186,187,188,188,189,190,191,192,193,194,195,195,196,197,198,199,200,201,201,202,203,204,205,206,206,207,208,209,210,211,211,212,213,214,215,215,216,217,218,219,219,220,221,222,222,223,224,225,225,226,227,228,228,229,230,231,231,232,233,234,234,235,236,236,237,238,239,239,240,241,241,242,243,243,244,245,245,246,247,248,248,249,250,250,251,251,252,253,253,254,255,255
};

uint16_t GDClass::atan2(int16_t y, int16_t x)
{
  uint16_t a;
  uint16_t xx = 0;

  /* These values are tricky. So pretend they are not */
  if (x == -32768)
    x++;
  if (y == -32768)
    y++;

  if ((x <= 0) ^ (y > 0)) {
    int16_t t; t = x; x = y; y = t;
    xx ^= 0x4000;
  }
  if (x <= 0) {
    x = -x;
  } else {
    xx ^= 0x8000;
  }
  y = abs(y);
  if (x > y) {
    int16_t t; t = x; x = y; y = t;
    xx ^= 0x3fff;
  }
  while ((x | y) & 0xff80) {
    x >>= 1;
    y >>= 1;
  }
  if (y == 0) {
    a = 0;
  } else if (x == y) {
    a = 0x2000;
  } else {
    // assert(x <= y);
    int r = ((x << 8) / y);
    // assert(0 <= r);
    // assert(r < 256);
    a = pgm_read_byte(atan8 + r) << 5;
  }
  a ^= xx;
  return a;
}

void GDClass::align(byte n) {
  while ((n++) & 3)
    GDTR.cmdbyte(0);
}

void GDClass::cH(uint16_t v) {
  GDTR.cmdbyte(v & 0xff);
  GDTR.cmdbyte((v >> 8) & 0xff);
}

void GDClass::ch(int16_t v) {
  cH((uint16_t)v);
}

void GDClass::cI(uint32_t v) {
  GDTR.cmd32(v);
}

void GDClass::cFFFFFF(byte v) {
  union {
    uint32_t c;
    uint8_t b[4];
  };
  b[0] = v;
  b[1] = 0xff;
  b[2] = 0xff;
  b[3] = 0xff;
  GDTR.cmd32(c);
}

void GDClass::ci(int32_t v) {
  cI((uint32_t) v);
}

void GDClass::cs(const char *s) {
  int count = 0;
  while (*s) {
    char c = *s++;
    GDTR.cmdbyte(c);
    count++;
  }
  GDTR.cmdbyte(0);
  align(count + 1);
}

#if !defined(ESP8266) && !defined(ESP32)
void GDClass::copy(const PROGMEM uint8_t *src, int count) {
#else
void GDClass::copy(const uint8_t *src, int count) {
#endif
  byte a = count & 3;
  while (count--) {
    GDTR.cmdbyte(pgm_read_byte_near(src));
    src++;
  }
  align(a);
}

void GDClass::copyram(byte *src, int count) {
  byte a = count & 3;
  GDTR.cmd_n(src, count);
  align(a);
}

void GDClass::AlphaFunc(byte func, byte ref) {
  cI((9UL << 24) | ((func & 7L) << 8) | ((ref & 255L) << 0));
}
void GDClass::Begin(byte prim) {
  // if (prim != cprim) {
    cI((31UL << 24) | prim);
    cprim = prim;
  // }
}
void GDClass::BitmapHandle(byte handle) {
  cI((5UL << 24) | handle);
}
void GDClass::BitmapLayout(byte format, uint16_t linestride, uint16_t height) {
  // cI((7UL << 24) | ((format & 31L) << 19) | ((linestride & 1023L) << 9) | ((height & 511L) << 0));
  union {
    uint32_t c;
    uint8_t b[4];
  };
  b[0] = height;
  b[1] = (1 & (height >> 8)) | (linestride << 1);
  b[2] = (7 & (linestride >> 7)) | (format << 3);
  b[3] = 7;
  cI(c);
  if (ft8xx_model) {
    b[0] = (((linestride >> 10) & 3) << 2) | ((height >> 9) & 3);
    b[3] = 0x28;
    cI(c);
  }
}
void GDClass::BitmapSize(byte filter, byte wrapx, byte wrapy, uint16_t width, uint16_t height) {
  byte fxy = (filter << 2) | (wrapx << 1) | (wrapy);
  // cI((8UL << 24) | ((uint32_t)fxy << 18) | ((width & 511L) << 9) | ((height & 511L) << 0));
  union {
    uint32_t c;
    uint8_t b[4];
  };
  b[0] = height;
  b[1] = (1 & (height >> 8)) | (width << 1);
  b[2] = (3 & (width >> 7)) | (fxy << 2);
  b[3] = 8;
  cI(c);
  if (ft8xx_model) {
    b[0] = ((width >> 9) << 2) | (3 & (height >> 9));
    b[3] = 0x29;
    cI(c);
  }
}
void GDClass::BitmapSource(uint32_t addr) {
  cI((1UL << 24) | ((addr & 0xffffffL) << 0));
}
void GDClass::BitmapTransformA(int32_t a) {
  cI((21UL << 24) | ((a & 131071L) << 0));
}
void GDClass::BitmapTransformB(int32_t b) {
  cI((22UL << 24) | ((b & 131071L) << 0));
}
void GDClass::BitmapTransformC(int32_t c) {
  cI((23UL << 24) | ((c & 16777215L) << 0));
}
void GDClass::BitmapTransformD(int32_t d) {
  cI((24UL << 24) | ((d & 131071L) << 0));
}
void GDClass::BitmapTransformE(int32_t e) {
  cI((25UL << 24) | ((e & 131071L) << 0));
}
void GDClass::BitmapTransformF(int32_t f) {
  cI((26UL << 24) | ((f & 16777215L) << 0));
}
void GDClass::BlendFunc(byte src, byte dst) {
  cI((11UL << 24) | ((src & 7L) << 3) | ((dst & 7L) << 0));
}
void GDClass::Call(uint16_t dest) {
  cI((29UL << 24) | ((dest & 2047L) << 0));
}
void GDClass::Cell(byte cell) {
  cI((6UL << 24) | ((cell & 127L) << 0));
}
void GDClass::ClearColorA(byte alpha) {
  cI((15UL << 24) | ((alpha & 255L) << 0));
}
void GDClass::ClearColorRGB(byte red, byte green, byte blue) {
  cI((2UL << 24) | ((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
}
void GDClass::ClearColorRGB(uint32_t rgb) {
  cI((2UL << 24) | (rgb & 0xffffffL));
}
void GDClass::Clear(byte c, byte s, byte t) {
  byte m = (c << 2) | (s << 1) | t;
  cI((38UL << 24) | m);
}
void GDClass::Clear(void) {
  cI((38UL << 24) | 7);
}
void GDClass::ClearStencil(byte s) {
  cI((17UL << 24) | ((s & 255L) << 0));
}
void GDClass::ClearTag(byte s) {
  cI((18UL << 24) | ((s & 255L) << 0));
}
void GDClass::ColorA(byte alpha) {
  cI((16UL << 24) | ((alpha & 255L) << 0));
}
void GDClass::ColorMask(byte r, byte g, byte b, byte a) {
  cI((32UL << 24) | ((r & 1L) << 3) | ((g & 1L) << 2) | ((b & 1L) << 1) | ((a & 1L) << 0));
}
void GDClass::ColorRGB(byte red, byte green, byte blue) {
  // cI((4UL << 24) | ((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
  union {
    uint32_t c;
    uint8_t b[4];
  };
  b[0] = blue;
  b[1] = green;
  b[2] = red;
  b[3] = 4;
  cI(c);
}
void GDClass::ColorRGB(uint32_t rgb) {
  cI((4UL << 24) | (rgb & 0xffffffL));
}
void GDClass::Display(void) {
  cI((0UL << 24));
}
void GDClass::End(void) {
  cI((33UL << 24));
}
void GDClass::Jump(uint16_t dest) {
  cI((30UL << 24) | ((dest & 2047L) << 0));
}
void GDClass::LineWidth(uint16_t width) {
  cI((14UL << 24) | ((width & 4095L) << 0));
}
void GDClass::Macro(byte m) {
  cI((37UL << 24) | ((m & 1L) << 0));
}
void GDClass::PointSize(uint16_t size) {
  cI((13UL << 24) | ((size & 8191L) << 0));
}
void GDClass::RestoreContext(void) {
  cI((35UL << 24));
}
void GDClass::Return(void) {
  cI((36UL << 24));
}
void GDClass::SaveContext(void) {
  cI((34UL << 24));
}
void GDClass::ScissorSize(uint16_t width, uint16_t height) {
  if (ft8xx_model == 0)
    cI((28UL << 24) | ((width & 1023L) << 10) | ((height & 1023L) << 0));
  else
    cI((28UL << 24) | ((width & 4095L) << 12) | ((height & 4095L) << 0));
}
void GDClass::ScissorXY(uint16_t x, uint16_t y) {
  if (ft8xx_model == 0)
    cI((27UL << 24) | ((x & 511L) << 9) | ((y & 511L) << 0));
  else
    cI((27UL << 24) | ((x & 2047L) << 11) | ((y & 2047L) << 0));
}
void GDClass::StencilFunc(byte func, byte ref, byte mask) {
  cI((10UL << 24) | ((func & 7L) << 16) | ((ref & 255L) << 8) | ((mask & 255L) << 0));
}
void GDClass::StencilMask(byte mask) {
  cI((19UL << 24) | ((mask & 255L) << 0));
}
void GDClass::StencilOp(byte sfail, byte spass) {
  cI((12UL << 24) | ((sfail & 7L) << 3) | ((spass & 7L) << 0));
}
void GDClass::TagMask(byte mask) {
  cI((20UL << 24) | ((mask & 1L) << 0));
}
void GDClass::Tag(byte s) {
  cI((3UL << 24) | ((s & 255L) << 0));
}
void GDClass::Vertex2f(int16_t x, int16_t y) {
  // x = int(16 * x);
  // y = int(16 * y);
  cI((1UL << 30) | ((x & 32767L) << 15) | ((y & 32767L) << 0));
}
void GDClass::Vertex2ii(uint16_t x, uint16_t y, byte handle, byte cell) {
  // cI((2UL << 30) | ((x & 511L) << 21) | ((y & 511L) << 12) | ((handle & 31L) << 7) | ((cell & 127L) << 0));
  union {
    uint32_t c;
    uint8_t b[4];
  };
  b[0] = (cell & 127) | ((handle & 1) << 7);
  b[1] = (handle >> 1) | (y << 4);
  b[2] = (y >> 4) | (x << 5);
  b[3] = (2 << 6) | (x >> 3);
  cI(c);
}
void GDClass::VertexFormat(byte frac) {
  cI((39UL << 24) | (((frac) & 7) << 0));
}
void GDClass::BitmapLayoutH(byte linestride, byte height) {
  cI((40UL << 24) | (((linestride) & 3) << 2) | (((height) & 3) << 0));
}
void GDClass::BitmapSizeH(byte width, byte height) {
  cI((41UL << 24) | (((width) & 3) << 2) | (((height) & 3) << 0));
}
void GDClass::PaletteSource(uint32_t addr) {
  cI((42UL << 24) | (((addr) & 4194303UL) << 0));
}
void GDClass::VertexTranslateX(uint32_t x) {
  cI((43UL << 24) | (((x) & 131071UL) << 0));
}
void GDClass::VertexTranslateY(uint32_t y) {
  cI((44UL << 24) | (((y) & 131071UL) << 0));
}
void GDClass::Nop(void) {
  cI((45UL << 24));
}

void GDClass::cmd_append(uint32_t ptr, uint32_t num) {
  cFFFFFF(0x1e);
  cI(ptr);
  cI(num);
  cprim = 0xff;
}
void GDClass::cmd_bgcolor(uint32_t c) {
  cFFFFFF(0x09);
  cI(c);
}
void GDClass::cmd_button(int16_t x, int16_t y, uint16_t w, uint16_t h, byte font, uint16_t options, const char *s) {
  cFFFFFF(0x0d);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
  ch(font);
  cH(options);
  cs(s);
  cprim = 0xff;
}
void GDClass::cmd_calibrate(void) {
  cFFFFFF(0x15);
  cFFFFFF(0xff);
}
void GDClass::cmd_clock(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t h, uint16_t m, uint16_t s, uint16_t ms) {
  cFFFFFF(0x14);
  ch(x);
  ch(y);
  ch(r);
  cH(options);
  cH(h);
  cH(m);
  cH(s);
  cH(ms);
  cprim = 0xff;
}
void GDClass::cmd_coldstart(void) {
  cFFFFFF(0x32);
}
void GDClass::cmd_dial(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t val) {
  cFFFFFF(0x2d);
  ch(x);
  ch(y);
  ch(r);
  cH(options);
  cH(val);
  cH(0);
  cprim = 0xff;
}
void GDClass::cmd_dlstart(void) {
  cFFFFFF(0x00);
  cprim = 0xff;
}
void GDClass::cmd_fgcolor(uint32_t c) {
  cFFFFFF(0x0a);
  cI(c);
}
void GDClass::cmd_gauge(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t major, uint16_t minor, uint16_t val, uint16_t range) {
  cFFFFFF(0x13);
  ch(x);
  ch(y);
  ch(r);
  cH(options);
  cH(major);
  cH(minor);
  cH(val);
  cH(range);
  cprim = 0xff;
}
void GDClass::cmd_getmatrix(void) {
  cFFFFFF(0x33);
  ci(0);
  ci(0);
  ci(0);
  ci(0);
  ci(0);
  ci(0);
}
void GDClass::cmd_getprops(uint32_t &ptr, uint32_t &w, uint32_t &h) {
  cFFFFFF(0x25);
  ptr = GDTR.getwp();
  cI(0);
  w = GDTR.getwp();
  cI(0);
  h = GDTR.getwp();
  cI(0);
}
void GDClass::cmd_getptr(void) {
  cFFFFFF(0x23);
  cI(0);
}
void GDClass::cmd_gradcolor(uint32_t c) {
  cFFFFFF(0x34);
  cI(c);
}
void GDClass::cmd_gradient(int16_t x0, int16_t y0, uint32_t rgb0, int16_t x1, int16_t y1, uint32_t rgb1) {
  cFFFFFF(0x0b);
  ch(x0);
  ch(y0);
  cI(rgb0);
  ch(x1);
  ch(y1);
  cI(rgb1);
  cprim = 0xff;
}
void GDClass::cmd_inflate(uint32_t ptr) {
  cFFFFFF(0x22);
  cI(ptr);
}
void GDClass::cmd_interrupt(uint32_t ms) {
  cFFFFFF(0x02);
  cI(ms);
}
void GDClass::cmd_keys(int16_t x, int16_t y, int16_t w, int16_t h, byte font, uint16_t options, const char*s) {
  cFFFFFF(0x0e);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
  ch(font);
  cH(options);
  cs(s);
  cprim = 0xff;
}
void GDClass::cmd_loadidentity(void) {
  cFFFFFF(0x26);
}
void GDClass::cmd_loadimage(uint32_t ptr, int32_t options) {
  cFFFFFF(0x24);
  cI(ptr);
  cI(options);
}
void GDClass::cmd_memcpy(uint32_t dest, uint32_t src, uint32_t num) {
  cFFFFFF(0x1d);
  cI(dest);
  cI(src);
  cI(num);
}
void GDClass::cmd_memset(uint32_t ptr, byte value, uint32_t num) {
  cFFFFFF(0x1b);
  cI(ptr);
  cI((uint32_t)value);
  cI(num);
}
uint32_t GDClass::cmd_memcrc(uint32_t ptr, uint32_t num) {
  cFFFFFF(0x18);
  cI(ptr);
  cI(num);
  uint32_t r = GDTR.getwp();
  cI(0xFFFFFFFF);
  return r;
}
void GDClass::cmd_memwrite(uint32_t ptr, uint32_t num) {
  cFFFFFF(0x1a);
  cI(ptr);
  cI(num);
}
void GDClass::cmd_regwrite(uint32_t ptr, uint32_t val) {
  cFFFFFF(0x1a);
  cI(ptr);
  cI(4UL);
  cI(val);
}
void GDClass::cmd_number(int16_t x, int16_t y, byte font, uint16_t options, uint32_t n) {
  cFFFFFF(0x2e);
  ch(x);
  ch(y);
  ch(font);
  cH(options);
  ci(n);
  cprim = BITMAPS;
}
void GDClass::cmd_progress(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t range) {
  cFFFFFF(0x0f);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
  cH(options);
  cH(val);
  cH(range);
  cH(0);
  cprim = 0xff;
}
void GDClass::cmd_regread(uint32_t ptr) {
  cFFFFFF(0x19);
  cI(ptr);
  cI(0);
}
void GDClass::cmd_rotate(int32_t a) {
  cFFFFFF(0x29);
  ci(a);
}
void GDClass::cmd_scale(int32_t sx, int32_t sy) {
  cFFFFFF(0x28);
  ci(sx);
  ci(sy);
}
void GDClass::cmd_screensaver(void) {
  cFFFFFF(0x2f);
}
void GDClass::cmd_scrollbar(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t size, uint16_t range) {
  cFFFFFF(0x11);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
  cH(options);
  cH(val);
  cH(size);
  cH(range);
  cprim = 0xff;
}
void GDClass::cmd_setfont(byte font, uint32_t ptr) {
  cFFFFFF(0x2b);
  cI(font);
  cI(ptr);
}
void GDClass::cmd_setmatrix(void) {
  cFFFFFF(0x2a);
}
void GDClass::cmd_sketch(int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t ptr, uint16_t format) {
  cFFFFFF(0x30);
  ch(x);
  ch(y);
  cH(w);
  cH(h);
  cI(ptr);
  cI(format);
}
void GDClass::cmd_slider(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t options, uint16_t val, uint16_t range) {
  cFFFFFF(0x10);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
  cH(options);
  cH(val);
  cH(range);
  cH(0);
  cprim = 0xff;
}
void GDClass::cmd_snapshot(uint32_t ptr) {
  cFFFFFF(0x1f);
  cI(ptr);
}
void GDClass::cmd_spinner(int16_t x, int16_t y, byte style, byte scale) {
  cFFFFFF(0x16);
  ch(x);
  ch(y);
  cH(style);
  cH(scale);
}
void GDClass::cmd_stop(void) {
  cFFFFFF(0x17);
}
void GDClass::cmd_swap(void) {
  cFFFFFF(0x01);
}
void GDClass::cmd_text(int16_t x, int16_t y, byte font, uint16_t options, const char *s) {
  cFFFFFF(0x0c);
  ch(x);
  ch(y);
  ch(font);
  cH(options);
  cs(s);
  cprim = BITMAPS;
}
void GDClass::cmd_toggle(int16_t x, int16_t y, int16_t w, byte font, uint16_t options, uint16_t state, const char *s) {
  cFFFFFF(0x12);
  ch(x);
  ch(y);
  ch(w);
  ch(font);
  cH(options);
  cH(state);
  cs(s);
}
void GDClass::cmd_track(int16_t x, int16_t y, uint16_t w, uint16_t h, byte tag) {
  cFFFFFF(0x2c);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
  ch(tag);
  ch(0);
}
void GDClass::cmd_translate(int32_t tx, int32_t ty) {
  cFFFFFF(0x27);
  ci(tx);
  ci(ty);
}
void GDClass::cmd_playvideo(int32_t options) {
  cFFFFFF(0x3a);
  cI(options);
}
void GDClass::cmd_romfont(uint32_t font, uint32_t romslot) {
  cFFFFFF(0x3f);
  cI(font);
  cI(romslot);
}
void GDClass::cmd_mediafifo(uint32_t ptr, uint32_t size) {
  cFFFFFF(0x39);
  cI(ptr);
  cI(size);
}
void GDClass::cmd_setbase(uint32_t b) {
  cFFFFFF(0x38);
  cI(b);
}
void GDClass::cmd_videoframe(uint32_t dst, uint32_t ptr) {
  cFFFFFF(0x41);
  cI(dst);
  cI(ptr);
}
void GDClass::cmd_snapshot2(uint32_t fmt, uint32_t ptr, int16_t x, int16_t y, int16_t w, int16_t h) {
  cFFFFFF(0x37);
  cI(fmt);
  cI(ptr);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
}
void GDClass::cmd_setfont2(uint32_t font, uint32_t ptr, uint32_t firstchar) {
  cFFFFFF(0x3b);
  cI(font);
  cI(ptr);
  cI(firstchar);
}
void GDClass::cmd_setbitmap(uint32_t source, uint16_t fmt, uint16_t w, uint16_t h) {
  cFFFFFF(0x43);
  cI(source);
  ch(fmt);
  ch(w);
  ch(h);
  ch(0);
}

void GDClass::cmd_flasherase() {
  cFFFFFF(0x44);
}

void GDClass::cmd_flashwrite(uint32_t dest, uint32_t num) {
  cFFFFFF(0x45);
  cI(dest);
  cI(num);
}

void GDClass::cmd_flashupdate(uint32_t dst, uint32_t src, uint32_t n) {
  cFFFFFF(0x47);
  cI(dst);
  cI(src);
  cI(n);
}

void GDClass::cmd_flashread(uint32_t dst, uint32_t src, uint32_t n) {
  cFFFFFF(0x46);
  cI(dst);
  cI(src);
  cI(n);
}

void GDClass::cmd_flashdetach() {
  cFFFFFF(0x48);
}

void GDClass::cmd_flashattach() {
  cFFFFFF(0x49);
}

uint32_t GDClass::cmd_flashfast(uint32_t &r) {
  cFFFFFF(0x4a);
  r = GDTR.getwp();
  cI(0xdeadbeef);
}

// XXX to do: cmd_rotate_around cmd_inflate2

void GDClass::cmd_setrotate(uint32_t r) {
  cFFFFFF(0x36);
  cI(r);
  // As a special favor, update variables w and h according to this
  // rotation
  w = GDTR.rd16(REG_HSIZE);
  h = GDTR.rd16(REG_VSIZE);
  if (r & 2) {
    int t = h;
    h = w;
    w = t;
  }
}

void GDClass::cmd_videostart() {
  cFFFFFF(0x40);
}

void GDClass::cmd_sync() {
  cFFFFFF(0x42);
}

byte GDClass::rd(uint32_t addr) {
  return GDTR.rd(addr);
}
void GDClass::wr(uint32_t addr, uint8_t v) {
  GDTR.wr(addr, v);
}
uint16_t GDClass::rd16(uint32_t addr) {
  return GDTR.rd16(addr);
}
void GDClass::wr16(uint32_t addr, uint16_t v) {
  GDTR.wr16(addr, v);
}
uint32_t GDClass::rd32(uint32_t addr) {
  return GDTR.rd32(addr);
}
void GDClass::wr32(uint32_t addr, uint32_t v) {
  GDTR.wr32(addr, v);
}
void GDClass::wr_n(uint32_t addr, byte *src, uint32_t n) {
  GDTR.wr_n(addr, src, n);
}
void GDClass::rd_n(byte *dst, uint32_t addr, uint32_t n) {
  GDTR.rd_n(dst, addr, n);
}

void GDClass::cmdbyte(uint8_t b) {
  GDTR.cmdbyte(b);
}
void GDClass::cmd32(uint32_t b) {
  GDTR.cmd32(b);
}
void GDClass::finish(void) {
  GDTR.finish();
}
#if defined(ARDUINO_ARCH_STM32)
void GDClass::get_accel(int &x, int &y, int &z) {
  x = 0;
  y = 0;
  z = 0;
}
#else
void GDClass::get_accel(int &x, int &y, int &z) {
  static int f[3];

  for (byte i = 0; i < 3; i++) {
    int a = analogRead(A0 + i);
    int s = (-160 * (a - 376)) >> 6;
    f[i] = ((3 * f[i]) >> 2) + (s >> 2);
  }
  x = f[2];
  y = f[1];
  z = f[0];
}
#endif
void GDClass::get_inputs(void) {
  GDTR.finish();
  byte *bi = (byte*)&inputs;
#if defined(DUMPDEV)
  extern FILE* stimfile;
  if (stimfile) {
    byte tag;
    fscanf(stimfile, "%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx",
        &bi[0],
        &bi[1],
        &bi[2],
        &bi[3],
        &bi[4],
        &bi[5],
        &bi[6],
        &bi[7],
        &bi[8],
        &bi[9],
        &bi[10],
        &bi[11],
        &bi[12],
        &bi[13],
        &bi[14],
        &bi[15],
        &bi[16],
        &bi[17]);
    GDTR.wr(REG_TAG, tag);
  } else {
    inputs.x = inputs.y = -32768;
  }
#else
  GDTR.rd_n(bi, REG_TRACKER, 4);
  GDTR.rd_n(bi + 4, REG_TOUCH_RZ, 13);
  GDTR.rd_n(bi + 17, REG_TAG, 1);
  inputs.touching = (inputs.x != -32768);
  inputs.xytouch.set(PIXELS(inputs.x), PIXELS(inputs.y));
#ifdef DUMP_INPUTS
  for (size_t i = 0; i < sizeof(inputs); i++) {
    Serial.print(bi[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
#endif
#endif
}
void GDClass::bulkrd(uint32_t a) {
  GDTR.bulk(a);
}
void GDClass::resume(void) {
  GDTR.resume();
}
void GDClass::__end(void) {
#if !defined(DUMPDEV) && !defined(RASPBERRY_PI)
  GDTR.__end();
#endif
}
void GDClass::play(uint8_t instrument, uint8_t note) {
  wr16(REG_SOUND, (note << 8) | instrument);
  wr(REG_PLAY, 1);
}
void GDClass::sample(uint32_t start, uint32_t len, uint16_t freq, uint16_t format, int loop) {
  GD.wr32(REG_PLAYBACK_START, start);
  GD.wr32(REG_PLAYBACK_LENGTH, len);
  GD.wr16(REG_PLAYBACK_FREQ, freq);
  GD.wr(REG_PLAYBACK_FORMAT, format);
  GD.wr(REG_PLAYBACK_LOOP, loop);
  GD.wr(REG_PLAYBACK_PLAY, 1);
}
void GDClass::reset() {
  GDTR.__end();
  GDTR.wr(REG_CPURESET, 1);
  GDTR.wr(REG_CPURESET, 0);
  GDTR.resume();
}

// Load named file from storage
// returns 0 on failure (e.g. file not found), 1 on success

byte GDClass::load(const char *filename, void (*progress)(long, long))
{
#if defined(RASPBERRY_PI) || defined(DUMPDEV) || defined(SPIDRIVER)
  char full_name[2048]  = "sdcard/";
  strcat(full_name, filename);
  FILE *f = fopen(full_name, "rb");
  if (!f) {
    perror(full_name);
    exit(1);
  }
  byte buf[512];
  int n;
  while ((n = fread(buf, 1, 512, f)) > 0) {
    GDTR.cmd_n(buf, (n + 3) & ~3);
  }
  fclose(f);
  return 1;
#else
  GD.__end();
  Reader r;
  if (r.openfile(filename)) {
    byte buf[512];
    while (r.offset < r.size) {
      uint16_t n = min(512U, r.size - r.offset);
      n = (n + 3) & ~3;   // force 32-bit alignment
      r.readsector(buf);

      GD.resume();
      if (progress)
        (*progress)(r.offset, r.size);
      GD.copyram(buf, n);
      GDTR.stop();
    }
    GD.resume();
    return 1;
  }
  GD.resume();
  return 0;
#endif
}

// Generated by mk_bsod.py. Blue screen with 'ERROR' text
static const PROGMEM uint8_t __bsod[48] = {
0, 255, 255, 255, 96, 0, 0, 2, 7, 0, 0, 38, 12, 255, 255, 255, 240, 0,
90, 0, 31, 0, 0, 6, 67, 111, 112, 114, 111, 99, 101, 115, 115, 111,
114, 32, 101, 120, 99, 101, 112, 116, 105, 111, 110, 0, 0, 0
};
static const PROGMEM uint8_t __bsod_815[56] = {
0, 255, 255, 255, 96, 0, 0, 2, 7, 0, 0, 38, 12, 255, 255, 255, 240, 0,
136, 0, 28, 0, 0, 22, 67, 111, 112, 114, 111, 99, 101, 115, 115, 111,
114, 32, 101, 120, 99, 101, 112, 116, 105, 111, 110, 58, 10, 10, 37,
115, 0, 0, 0, 0, 0, 0
};
static const PROGMEM uint8_t __bsod_badfile[44] = {
0, 255, 255, 255, 96, 0, 0, 2, 7, 0, 0, 38, 12, 255, 255, 255, 240, 0,
90, 0, 29, 0, 0, 6, 67, 97, 110, 110, 111, 116, 32, 111, 112, 101,
110, 32, 102, 105, 108, 101, 58, 0, 0, 0
};

// Fatal error alert.
// Show a blue screen with message.
// This method never returns.

void GDClass::alert()
{
  GDTR.coprocsssor_recovery();
  if (ft8xx_model < 2)
    copy(__bsod, sizeof(__bsod));
  else
    copy(__bsod_815, sizeof(__bsod_815));
  swap();
  GD.finish();
  for (;;) ;
}

void GDClass::safeload(const char *filename)
{
  if (!load(filename)) {
    __end();
    begin(0);
    copy(__bsod_badfile, sizeof(__bsod_badfile));
    cmd_text(240, 190, 29, OPT_CENTER, filename);
    swap();
    for (;;) ;
  }
}

void GDClass::textsize(int &w, int &h, int font, const char *s)
{
  uint32_t font_addr = rd32(0x309074 + 4 * font);
  w = 0;
  while (*s)
    w += GD.rd(font_addr + *s++);
  h = GD.rd(font_addr + 140);
}

#define REG_SCREENSHOT_EN    (ft8xx_model ? 0x302010UL : 0x102410UL) // Set to enable screenshot mode
#define REG_SCREENSHOT_Y     (ft8xx_model ? 0x302014UL : 0x102414UL) // Y line register
#define REG_SCREENSHOT_START (ft8xx_model ? 0x302018UL : 0x102418UL) // Screenshot start trigger
#define REG_SCREENSHOT_BUSY  (ft8xx_model ? 0x3020e8UL : 0x1024d8UL) // Screenshot ready flags
#define REG_SCREENSHOT_READ  (ft8xx_model ? 0x302174UL : 0x102554UL) // Set to enable readout
#define RAM_SCREENSHOT       (ft8xx_model ? 0x3c2000UL : 0x1C2000UL) // Screenshot readout buffer

#ifndef DUMPDEV
void GDClass::dumpscreen(void)
{
  {
    finish();
    int w = GD.rd16(REG_HSIZE), h = GD.rd16(REG_VSIZE);

    wr(REG_SCREENSHOT_EN, 1);
    if (ft8xx_model)
      wr(0x0030201c, 32);
    Serial.write(0xa5);
    Serial.write(w & 0xff);
    Serial.write((w >> 8) & 0xff);
    Serial.write(h & 0xff);
    Serial.write((h >> 8) & 0xff);
    for (int ly = 0; ly < h; ly++) {
      wr16(REG_SCREENSHOT_Y, ly);
      wr(REG_SCREENSHOT_START, 1);
      delay(2);
      while (rd32(REG_SCREENSHOT_BUSY) | rd32(REG_SCREENSHOT_BUSY + 4))
        ;
      wr(REG_SCREENSHOT_READ, 1);
      bulkrd(RAM_SCREENSHOT);
      SPI.transfer(0xff);
      for (int x = 0; x < w; x += 8) {
        union {
          uint32_t v;
          struct {
            uint8_t b, g, r, a;
          };
        } block[8];
        for (int i = 0; i < 8; i++) {
          block[i].b = SPI.transfer(0xff);
          block[i].g = SPI.transfer(0xff);
          block[i].r = SPI.transfer(0xff);
          block[i].a = SPI.transfer(0xff);
        }
        // if (x == 0) block[0].r = 0xff;
        byte difference = 1;
        for (int i = 1, mask = 2; i < 8; i++, mask <<= 1)
          if (block[i].v != block[i-1].v)
            difference |= mask;
        Serial.write(difference);

        for (int i = 0; i < 8; i++)
          if (1 & (difference >> i)) {
            Serial.write(block[i].b);
            Serial.write(block[i].g);
            Serial.write(block[i].r);
          }
      }
      resume();
      wr(REG_SCREENSHOT_READ, 0);
    }
    wr16(REG_SCREENSHOT_EN, 0);
  }
}
#endif
