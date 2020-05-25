/*
 * Copyright (C) 2013-2018 by James Bowman <jamesb@excamera.com>
 * Gameduino 2/3 library for Arduino, Arduino Due, Teensy 3.2,
 * ESP8266 and ESP32.
 */

#ifndef _GD2_H_INCLUDED
#define _GD2_H_INCLUDED

#define GD2_VERSION "%VERSION"

#if defined(RASPBERRY_PI) || defined(DUMPDEV)
#include "wiring.h"
#endif

#include "Arduino.h"
#include <stdarg.h>

#define RGB(r, g, b)    ((uint32_t)((((r) & 0xffL) << 16) | (((g) & 0xffL) << 8) | ((b) & 0xffL)))
#define F8(x)           (int((x) * 256L))
#define F16(x)          ((int32_t)((x) * 65536L))

                            // Options for GD.begin():
#define GD_CALIBRATE    1   // enable touchscreen calibration at startup
#define GD_TRIM         2   // trim the built-in oscillator
#define GD_STORAGE      4   // initialize attached microSD
#define GD_NOBACKLIGHT  8   // don't turn on the backlight

#ifdef __SAM3X8E__
#define __DUE__ 1
#endif

#if defined(ESP8266)
#define SD_PIN        D9    // pin used for the microSD enable signal
#elif defined(ARDUINO_ARCH_STM32)
#define SD_PIN        PB1
#else
#define SD_PIN        9     // pin used for the microSD enable signal
#endif

#define BOARD_FTDI_80x    0
#define BOARD_GAMEDUINO23 1
#define BOARD_SUNFLOWER   2
#define BOARD_OTHER       3

#ifndef BOARD
#define BOARD         BOARD_GAMEDUINO23 // board, from above
#endif

#define STORAGE       1                 // Want SD storage?
#define CALIBRATION   1                 // Want touchscreen calibration?

// FTDI boards do not have storage
#if (BOARD == BOARD_FTDI_80x) || defined(RASPBERRY_PI) || defined(DUMPDEV) || defined(SPIDRIVER)
#undef STORAGE
#define STORAGE 0
#endif

#ifndef DEFAULT_CS
#if defined(ESP8266)
#define DEFAULT_CS D8
#elif defined(ARDUINO_ARCH_STM32)
#define DEFAULT_CS PB0
#elif (BOARD == BOARD_SUNFLOWER)
#define DEFAULT_CS 6
#else
#define DEFAULT_CS 8
#endif
#endif


////////////////////////////////////////////////////////////////////////
// Decide if we want to compile in SDcard support
//
// For stock Arduino models: yes
// Raspberry PI: no
// Arduino Due: no
//

#if !defined(RASPBERRY_PI) && !defined(DUMPDEV)
#define SDCARD 1
#else
#define SDCARD 0
#endif

#if defined(__DUE__)
#define MOSI  11
#define MISO  12
#define SCK   13    // B.27

class ASPI_t {
public:
  void begin(void) {
    pinMode(MOSI, OUTPUT);
    pinMode(MISO, INPUT);
    pinMode(SCK, OUTPUT);
    digitalWrite(SCK, 0);

    // PIOB->PIO_PER = PIO_PB27;
    // PIOB->PIO_CODR = PIO_PB27;
    // PIOB->PIO_PUDR = PIO_PB27;
  }
  byte transfer(byte x ) {
    byte r = 0;
    for (byte i = 8; i; i--) {
      if (x & 0x80)
        PIOD->PIO_SODR = PIO_PD7;
      else
        PIOD->PIO_CODR = PIO_PD7;
      // digitalWrite(MOSI, (x >> 7) & 1);
      x <<= 1;
      // digitalWrite(SCK, 1);
      PIOB->PIO_SODR = PIO_PB27;
      r <<= 1;
      r |= digitalRead(MISO);
      // digitalWrite(SCK, 0);
      PIOB->PIO_CODR = PIO_PB27;
    }
    return r;
  }
  void transfer(byte*m, int s) {
    while (s--) {
      *m = transfer(*m);
      m++;
    }
  }
};
static class ASPI_t ASPI;
#define SPI ASPI

#endif

#if defined(ARDUINO_STM32L4_BLACKICE)
// BlackIce Board uses SPI1 on the Arduino header.
#define SPI SPI1
// Board Support:
//   JSON: http://www.hamnavoe.com/package_millerresearch_mystorm_index.json
//   Source: https://github.com/millerresearch/arduino-mystorm
#endif

#if SDCARD

#if defined(VERBOSE) && (VERBOSE > 0)
#define INFO(X) Serial.println((X))
#if defined(RASPBERRY_PI)
#define REPORT(VAR) fprintf(stderr, #VAR "=%d\n", (VAR))
#else
#define REPORT(VAR) (Serial.print(#VAR "="), Serial.print(VAR, DEC), Serial.print(' '), Serial.println(VAR, HEX))
#endif
#else
#define INFO(X)
#define REPORT(X)
#endif

struct dirent {
  char name[8];
  char ext[3];
  byte attribute;
  byte reserved[8];
  uint16_t cluster_hi;  // FAT32 only
  uint16_t time;
  uint16_t date;
  uint16_t cluster;
  uint32_t size;
};

// https://www.sdcard.org/downloads/pls/simplified_specs/Part_1_Physical_Layer_Simplified_Specification_Ver_3.01_Final_100518.pdf
// page 22
// http://mac6.ma.psu.edu/space2008/RockSat/microController/sdcard_appnote_foust.pdf
// http://elm-chan.org/docs/mmc/mmc_e.html
// http://www.pjrc.com/tech/8051/ide/fat32.html

#define FAT16 0
#define FAT32 1

#define DD

class sdcard {
  public:
  void sel() {
    digitalWrite(pin, LOW);
    delay(1);
  }
  void desel() {
    digitalWrite(pin, HIGH);
    SPI.transfer(0xff); // force DO release
  }
  void sd_delay(byte n) {
    while (n--) {
      DD SPI.transfer(0xff);
    }
  }

  void cmd(byte cmd, uint32_t lba = 0, uint8_t crc = 0x95) {
#if VERBOSE > 1
    Serial.print("cmd ");
    Serial.print(cmd, DEC);
    Serial.print(" ");
    Serial.print(lba, HEX);
    Serial.println();
#endif

    sel();
    // DD SPI.transfer(0xff);
    DD SPI.transfer(0x40 | cmd);
    DD SPI.transfer(0xff & (lba >> 24));
    DD SPI.transfer(0xff & (lba >> 16));
    DD SPI.transfer(0xff & (lba >> 8));
    DD SPI.transfer(0xff & (lba));
    DD SPI.transfer(crc);
    // DD SPI.transfer(0xff);
  }

  byte response() {
    byte r;
    DD
    r = SPI.transfer(0xff);
    while (r & 0x80) {
      DD
      r = SPI.transfer(0xff);
    }
    return r;
  }

  byte R1() {   // read response R1
    byte r = response();
    desel();
    SPI.transfer(0xff);   // trailing byte
    return r;
  }

  byte sdR3(uint32_t &ocr) {  // read response R3
    byte r = response();
    for (byte i = 4; i; i--)
      ocr = (ocr << 8) | SPI.transfer(0xff);
    SPI.transfer(0xff);   // trailing byte

    desel();
    return r;
  }

  byte sdR7() {  // read response R3
    byte r = response();
    for (byte i = 4; i; i--)
      // Serial.println(SPI.transfer(0xff), HEX);
      SPI.transfer(0xff);
    desel();

    return r;
  }

  void appcmd(byte cc, uint32_t lba = 0) {
    cmd(55); R1();
    cmd(cc, lba);
  }

  void begin(byte p) {
    byte type_code;
    byte sdhc;
    pin = p;

    pinMode(pin, OUTPUT);
#if !defined(__DUE__) && !defined(TEENSYDUINO) && !defined(ARDUINO_ARCH_STM32L4)
    SPI.setClockDivider(SPI_CLOCK_DIV64);
#endif
    desel();

  // for (;;) SPI.transfer(0xff);
    delay(50);      // wait for boot
    sd_delay(10);   // deselected, 80 pulses

    INFO("Attempting card reset... ");
    byte r1;
    static int attempts;
    attempts = 0;
    do {       // reset, enter idle
      cmd(0);
      while ((r1 = SPI.transfer(0xff)) & 0x80)
        if (++attempts == 1000)
          goto finished;
      desel();
      SPI.transfer(0xff);   // trailing byte
      REPORT(r1);
    } while (r1 != 1);
    INFO("reset ok\n");

    sdhc = 0;
    cmd(8, 0x1aa, 0x87);
    r1 = sdR7();
    sdhc = (r1 == 1);

    REPORT(sdhc);

    INFO("Sending card init command");
    attempts = 0;
    while (1) {
      appcmd(41, sdhc ? (1UL << 30) : 0); // card init
      r1 = R1();
#if VERBOSE
      Serial.println(r1, HEX);
#endif
      if ((r1 & 1) == 0)
        break;
      if (++attempts == 300)
        goto finished;
      delay(1);
    }
    INFO("OK");

    if (sdhc) {
      uint32_t OCR = 0;
      for (int i = 10; i; i--) {
        cmd(58);
        sdR3(OCR);
        REPORT(OCR);
      }
      ccs = 1UL & (OCR >> 30);
    } else {
      ccs = 0;
    }
    REPORT(ccs);

    // Test point: dump sector 0 to serial.
    // should see first 512 bytes of card, ending 55 AA.
#if 0
    cmd17(0);
    for (int i = 0; i < 512; i++) {
      delay(10);
      byte b = SPI.transfer(0xff);
      Serial.print(b, HEX);
      Serial.print(' ');
      if ((i & 15) == 15)
        Serial.println();
    }
    desel();
    for (;;);
#endif

#if !defined(__DUE__) && !defined(ESP8266) && !defined(ESP32) && !defined(ARDUINO_ARCH_STM32L4) && !defined(ARDUINO_ARCH_STM32)
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    SPSR = (1 << SPI2X);
#endif

#if defined(ESP8266)
    SPI.setFrequency(40000000L);
#elif defined(ESP32)
    SPI.setFrequency(25000000L);
#elif defined(ARDUINO_ARCH_STM32)
    SPI.beginTransaction(SPISettings(18000000, MSBFIRST, SPI_MODE0));
#endif

    type_code = rd(0x1be + 0x4);
    switch (type_code) {
      default:
        type = FAT16;
        break;
      case 0x0b:
      case 0x0c:
        type = FAT32;
        break;
    }
    REPORT(type_code);

    o_partition = 512L * rd4(0x1be + 0x8);
    sectors_per_cluster = rd(o_partition + 0xd);
    reserved_sectors = rd2(o_partition + 0xe);
    cluster_size = 512L * sectors_per_cluster;
    REPORT(sectors_per_cluster);

    // Serial.println("Bytes per sector:    %d\n", rd2(o_partition + 0xb));
    // Serial.println("Sectors per cluster: %d\n", sectors_per_cluster);

    if (type == FAT16) {
      max_root_dir_entries = rd2(o_partition + 0x11);
      sectors_per_fat = rd2(o_partition + 0x16);
      o_fat = o_partition + 512L * reserved_sectors;
      o_root = o_fat + (2 * 512L * sectors_per_fat);
      // data area starts with cluster 2, so offset it here
      o_data = o_root + (max_root_dir_entries * 32L) - (2L * cluster_size);
    } else {
      uint32_t sectors_per_fat = rd4(o_partition + 0x24);
      root_dir_first_cluster = rd4(o_partition + 0x2c);
      uint32_t fat_begin_lba = (o_partition >> 9) + reserved_sectors;
      uint32_t cluster_begin_lba = (o_partition >> 9) + reserved_sectors + (2 * sectors_per_fat);

      o_fat = 512L * fat_begin_lba;
      o_root = (512L * (cluster_begin_lba + (root_dir_first_cluster - 2) * sectors_per_cluster));
      o_data = (512L * (cluster_begin_lba - 2 * sectors_per_cluster));
    }
  finished:
    INFO("finished");
    ;
  }
  void cmd17(uint32_t off) {
    REPORT(off);
    if (ccs)
      cmd(17, off >> 9);
    else
      cmd(17, off & ~511L);
    R1();
    sel();
    while (SPI.transfer(0xff) != 0xfe)
      ;
  }
  void rdn(byte *d, uint32_t off, uint16_t n) {
    cmd17(off);
    uint16_t i;
    uint16_t bo = (off & 511);
    for (i = 0; i < bo; i++)
      SPI.transfer(0xff);
    for (i = 0; i < n; i++)
      *d++ = SPI.transfer(0xff);
    for (i = 0; i < (514 - bo - n); i++)
      SPI.transfer(0xff);
    desel();
  }

  uint32_t rd4(uint32_t off) {
    uint32_t r;
    rdn((byte*)&r, off, sizeof(r));
    return r;
  }

  uint16_t rd2(uint32_t off) {
    uint16_t r;
    rdn((byte*)&r, off, sizeof(r));
    return r;
  }

  byte rd(uint32_t off) {
    byte r;
    rdn((byte*)&r, off, sizeof(r));
    return r;
  }
  byte pin;
  byte ccs;

  byte type;
  uint16_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint16_t max_root_dir_entries;
  uint16_t sectors_per_fat;
  uint16_t cluster_size;
  uint32_t root_dir_first_cluster;

  // These are all linear addresses, hence the o_ prefix
  uint32_t o_partition;
  uint32_t o_fat;
  uint32_t o_root;
  uint32_t o_data;
};

static void dos83(byte dst[11], const char *ps)
{
  byte i = 0;
  while (*ps) {
    if (*ps != '.')
      dst[i++] = toupper(*ps);
    else {
      while (i < 8)
        dst[i++] = ' ';
    }
    ps++;
  }
  while (i < 11)
    dst[i++] = ' ';
}

#else
class sdcard {
public:
  void begin(int p) {};
};
#endif

////////////////////////////////////////////////////////////////////////

class xy {
public:
  int x, y;
  void set(int _x, int _y);
  void rmove(int distance, int angle);
  int angleto(class xy &other);
  void draw(byte offset = 0);
  void rotate(int angle);
  int onscreen(void);
  class xy operator<<=(int d);
  class xy operator+=(class xy &other);
  class xy operator-=(class xy &other);
  long operator*(class xy &other);
  class xy operator*=(int);
  int nearer_than(int distance, xy &other);
};

class Bitmap {
public:
  xy size, center;
  uint32_t source;
  uint16_t format;
  int8_t handle;

  void fromtext(int font, const char* s);
  void fromfile(const char *filename, int format = 7);

  void bind(uint8_t handle);

  void wallpaper();
  void draw(int x, int y, int16_t angle = 0);
  void draw(const xy &pos, int16_t angle = 0);

private:
  void defaults(uint8_t f);
  void setup(void);
};

class Bitmap __fromatlas(uint32_t addr);

////////////////////////////////////////////////////////////////////////

class GDClass {
public:
  int w, h;
  uint32_t loadptr;
  byte vxf;   // Vertex Format

  void begin(uint8_t options = (GD_CALIBRATE | GD_TRIM | GD_STORAGE), int cs = DEFAULT_CS, int sdcs = SD_PIN);

  uint16_t random();
  uint16_t random(uint16_t n);
  uint16_t random(uint16_t n0, uint16_t n1);
  void seed(uint16_t n);
  int16_t rsin(int16_t r, uint16_t th);
  int16_t rcos(int16_t r, uint16_t th);
  void polar(int &x, int &y, int16_t r, uint16_t th);
  uint16_t atan2(int16_t y, int16_t x);

#if !defined(ESP8266) && !defined(ESP32) && !defined(TEENSYDUINO)
  void copy(const PROGMEM uint8_t *src, int count);
#else
  void copy(const uint8_t *src, int count);
#endif
  void copyram(byte *src, int count);

  void self_calibrate(void);

  void swap(void);
  void flush(void);
  void finish(void);

  void play(uint8_t instrument, uint8_t note = 0);
  void sample(uint32_t start, uint32_t len, uint16_t freq, uint16_t format, int loop = 0);

  void get_inputs(void);
  void get_accel(int &x, int &y, int &z);
  struct {
    uint16_t track_tag;
    uint16_t track_val;
    uint16_t rz;
    uint16_t __dummy_1;
    int16_t y;
    int16_t x;
    int16_t tag_y;
    int16_t tag_x;
    uint8_t tag;
    uint8_t ptag;
    uint8_t touching;
    xy xytouch;
  } inputs;

  void AlphaFunc(byte func, byte ref);
  void Begin(byte prim);
  void BitmapHandle(byte handle);
  void BitmapLayout(byte format, uint16_t linestride, uint16_t height);
  void BitmapSize(byte filter, byte wrapx, byte wrapy, uint16_t width, uint16_t height);
  void BitmapSource(uint32_t addr);
  void BitmapTransformA(int32_t a);
  void BitmapTransformB(int32_t b);
  void BitmapTransformC(int32_t c);
  void BitmapTransformD(int32_t d);
  void BitmapTransformE(int32_t e);
  void BitmapTransformF(int32_t f);
  void BlendFunc(byte src, byte dst);
  void Call(uint16_t dest);
  void Cell(byte cell);
  void ClearColorA(byte alpha);
  void ClearColorRGB(byte red, byte green, byte blue);
  void ClearColorRGB(uint32_t rgb);
  void Clear(byte c, byte s, byte t);
  void Clear(void);
  void ClearStencil(byte s);
  void ClearTag(byte s);
  void ColorA(byte alpha);
  void ColorMask(byte r, byte g, byte b, byte a);
  void ColorRGB(byte red, byte green, byte blue);
  void ColorRGB(uint32_t rgb);
  void Display(void);
  void End(void);
  void Jump(uint16_t dest);
  void LineWidth(uint16_t width);
  void Macro(byte m);
  void PointSize(uint16_t size);
  void RestoreContext(void);
  void Return(void);
  void SaveContext(void);
  void ScissorSize(uint16_t width, uint16_t height);
  void ScissorXY(uint16_t x, uint16_t y);
  void StencilFunc(byte func, byte ref, byte mask);
  void StencilMask(byte mask);
  void StencilOp(byte sfail, byte spass);
  void TagMask(byte mask);
  void Tag(byte s);
  void Vertex2f(int16_t x, int16_t y);
  void Vertex2ii(uint16_t x, uint16_t y, byte handle = 0, byte cell = 0);

  void VertexFormat(byte frac);
  void BitmapLayoutH(byte linestride, byte height);
  void BitmapSizeH(byte width, byte height);
  void PaletteSource(uint32_t addr);
  void VertexTranslateX(uint32_t x);
  void VertexTranslateY(uint32_t y);
  void Nop(void);

  // Higher-level graphics commands

  void cmd_append(uint32_t ptr, uint32_t num);
  void cmd_bgcolor(uint32_t c);
  void cmd_button(int16_t x, int16_t y, uint16_t w, uint16_t h, byte font, uint16_t options, const char *s);
  void cmd_calibrate(void);
  void cmd_clock(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t h, uint16_t m, uint16_t s, uint16_t ms);
  void cmd_coldstart(void);
  void cmd_dial(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t val);
  void cmd_dlstart(void);
  void cmd_fgcolor(uint32_t c);
  void cmd_gauge(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t major, uint16_t minor, uint16_t val, uint16_t range);
  void cmd_getmatrix(void);
  void cmd_getprops(uint32_t &ptr, uint32_t &w, uint32_t &h);
  void cmd_getptr(void);
  void cmd_gradcolor(uint32_t c);
  void cmd_gradient(int16_t x0, int16_t y0, uint32_t rgb0, int16_t x1, int16_t y1, uint32_t rgb1);
  void cmd_inflate(uint32_t ptr);
  void cmd_interrupt(uint32_t ms);
  void cmd_keys(int16_t x, int16_t y, int16_t w, int16_t h, byte font, uint16_t options, const char*s);
  void cmd_loadidentity(void);
  void cmd_loadimage(uint32_t ptr, int32_t options);
  void cmd_memcpy(uint32_t dest, uint32_t src, uint32_t num);
  void cmd_memset(uint32_t ptr, byte value, uint32_t num);
  uint32_t cmd_memcrc(uint32_t ptr, uint32_t num);
  void cmd_memwrite(uint32_t ptr, uint32_t num);
  void cmd_regwrite(uint32_t ptr, uint32_t val);
  void cmd_number(int16_t x, int16_t y, byte font, uint16_t options, uint32_t n);
  void cmd_progress(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t range);
  void cmd_regread(uint32_t ptr);
  void cmd_rotate(int32_t a);
  void cmd_scale(int32_t sx, int32_t sy);
  void cmd_screensaver(void);
  void cmd_scrollbar(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t size, uint16_t range);
  void cmd_setfont(byte font, uint32_t ptr);
  void cmd_setmatrix(void);
  void cmd_sketch(int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t ptr, uint16_t format);
  void cmd_slider(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t options, uint16_t val, uint16_t range);
  void cmd_snapshot(uint32_t ptr);
  void cmd_spinner(int16_t x, int16_t y, byte style, byte scale);
  void cmd_stop(void);
  void cmd_swap(void);
  void cmd_text(int16_t x, int16_t y, byte font, uint16_t options, const char *s);
  void cmd_toggle(int16_t x, int16_t y, int16_t w, byte font, uint16_t options, uint16_t state, const char *s);
  void cmd_track(int16_t x, int16_t y, uint16_t w, uint16_t h, byte tag);
  void cmd_translate(int32_t tx, int32_t ty);

  void cmd_playvideo(int32_t options);
  void cmd_romfont(uint32_t font, uint32_t romslot);
  void cmd_mediafifo(uint32_t ptr, uint32_t size);
  void cmd_setbase(uint32_t b);
  void cmd_videoframe(uint32_t dst, uint32_t ptr);
  void cmd_snapshot2(uint32_t fmt, uint32_t ptr, int16_t x, int16_t y, int16_t w, int16_t h);
  void cmd_setfont2(uint32_t font, uint32_t ptr, uint32_t firstchar);
  void cmd_setrotate(uint32_t r);
  void cmd_videostart();
  void cmd_setbitmap(uint32_t source, uint16_t fmt, uint16_t w, uint16_t h);
  void cmd_sync();
  void cmd_flasherase();
  void cmd_flashwrite(uint32_t dest, uint32_t num);
  void cmd_flashupdate(uint32_t dst, uint32_t src, uint32_t n);
  void cmd_flashread(uint32_t dst, uint32_t src, uint32_t n);
  void cmd_flashdetach();
  void cmd_flashattach();
  uint32_t cmd_flashfast(uint32_t &r);

  byte rd(uint32_t addr);
  void wr(uint32_t addr, uint8_t v);
  uint16_t rd16(uint32_t addr);
  void wr16(uint32_t addr, uint16_t v);
  uint32_t rd32(uint32_t addr);
  void wr32(uint32_t addr, uint32_t v);
  void wr_n(uint32_t addr, byte *src, uint32_t n);
  void rd_n(byte *dst, uint32_t addr, uint32_t n);

  void cmd32(uint32_t b);

  void bulkrd(uint32_t a);
  void resume(void);
  void __end(void);
  void reset(void);

  void dumpscreen(void);
  byte load(const char *filename, void (*progress)(long, long) = NULL);
  void safeload(const char *filename);
  void alert();
  void textsize(int &w, int &h, int font, const char *s);

  sdcard SD;

  void storage(void);
  void tune(void);

private:
  static void cFFFFFF(byte v);
  static void cI(uint32_t);
  static void ci(int32_t);
  static void cH(uint16_t);
  static void ch(int16_t);
  static void cs(const char *);
  static void fmtcmd(const char *fmt, ...);

  static void align(byte n);
  void cmdbyte(uint8_t b);

  uint32_t measure_freq(void);

  uint32_t rseed;
  byte cprim;       // current primitive
};

extern GDClass GD;
extern byte ft8xx_model;

#if SDCARD
class Reader {
public:
  int openfile(const char *filename) {
    int i = 0;
    byte dosname[11];
    dirent de;

    dos83(dosname, filename);
    do {
      GD.SD.rdn((byte*)&de, GD.SD.o_root + i * 32, sizeof(de));
      if (0 == memcmp(de.name, dosname, 11)) {
        begin(de);
        return 1;
      }
      i++;
    } while (de.name[0]);
    return 0;
  }

  void begin(dirent &de) {
    nseq = 0;
    size = de.size;
    cluster0 = de.cluster;
    if (GD.SD.type == FAT32)
      cluster0 |= ((long)de.cluster_hi << 16);
    rewind();
  }

  void rewind(void) {
    cluster = cluster0;
    sector = 0;
    offset = 0;
  }

  void nextcluster() {
    if (GD.SD.type == FAT16)
      cluster = GD.SD.rd2(GD.SD.o_fat + 2 * cluster);
    else
      cluster = GD.SD.rd4(GD.SD.o_fat + 4 * cluster);
#if VERBOSE
    Serial.print("nextcluster=");
    Serial.println(cluster, DEC);
#endif
  }

  void fetch512(byte *dst) {
#if defined(__DUE__) || defined(TEENSYDUINO) || defined(ESP8266) || defined(ESP32) || 1

#if defined(ARDUINO_ARCH_STM32)
    SPI.read(dst, 512);
#elif defined(ESP8266) || defined(ESP32)
    SPI.transferBytes(NULL, dst, 512);
#else
    // for (int i = 0; i < 512; i++) *dst++ = SPI.transfer(0xff);
    memset(dst, 0xff, 512); SPI.transfer(dst, 512);
#endif

    SPI.transfer(0xff);   // consume CRC
    SPI.transfer(0xff);
#else
    SPDR = 0xff;
    asm volatile("nop"); while (!(SPSR & _BV(SPIF))) ;
    for (int i = 0; i < 512; i++) {
      while (!(SPSR & _BV(SPIF))) ;
      asm volatile("nop");
      asm volatile("nop");
      asm volatile("nop");
      asm volatile("nop");
      asm volatile("nop");
      asm volatile("nop");
      asm volatile("nop");
      asm volatile("nop");
      asm volatile("nop");
      asm volatile("nop");
      asm volatile("nop");

      *dst++ = SPDR;
      SPDR = 0xff;
    }
    asm volatile("nop"); while (!(SPSR & _BV(SPIF))) ;
    SPI.transfer(0xff);
#endif
    GD.SD.desel();
  }

  void nextcluster2(byte *dst) {
    if (nseq) {
      nseq--;
      cluster++;
      return;
    }
    uint32_t off = GD.SD.o_fat + 4 * cluster;
    GD.SD.cmd17(off & ~511L);
    fetch512(dst);
    int i = off & 511;
    cluster = *(uint32_t*)&dst[i];
    nseq = 0;
    for (uint32_t c = cluster;
         (i < 512) && *(uint32_t*)&dst[i] == c;
         i += 4, c++)
      nseq++;
  }

  void skipcluster() {
    nextcluster();
    offset += GD.SD.cluster_size;
  }

  void skipsector() {
    if (sector == GD.SD.sectors_per_cluster) {
      sector = 0;
      nextcluster();
    }
    sector++;
    offset += 512;
  }

  void seek(uint32_t o) {
    union {
      uint8_t buf[512];
      uint32_t fat32[128];
      uint16_t fat16[256];
    };
    uint32_t co = ~0;

    if (o < offset)
      rewind();
    while (offset < o) {
      if ((sector == GD.SD.sectors_per_cluster) && ((o - offset) > (long)GD.SD.cluster_size)) {
        uint32_t o;
        if (GD.SD.type == FAT16)
          o = (GD.SD.o_fat + 2 * cluster) & ~511;
        else
          o = (GD.SD.o_fat + 4 * cluster) & ~511;
        if (o != co) {
          GD.SD.rdn(buf, o, 512);
          co = o;
        }
        cluster = fat32[cluster & 127];
        offset += GD.SD.cluster_size;
      } else
        skipsector();
    }
  }

  void readsector(byte *dst) {
    if (sector == GD.SD.sectors_per_cluster) {
      sector = 0;
      nextcluster2(dst);
    }
    REPORT(cluster);
    uint32_t off = GD.SD.o_data + ((long)GD.SD.cluster_size * cluster) + (512L * sector);
    REPORT(off);
    GD.SD.cmd17(off & ~511L);
    REPORT(off);
    sector++;
    offset += 512;
    fetch512(dst);
  }

  int eof(void) {
    return size <= offset;
  }

  uint32_t cluster, cluster0;
  uint32_t offset;
  uint32_t size;
  byte sector;
  byte nseq;
};
#endif

typedef struct {
  byte handle;
  uint16_t w, h;
  uint16_t size;
} shape_t;

// convert integer pixels to subpixels
#define PIXELS(x)  int((x) * 16)

// Convert degrees to Furmans
#define DEGREES(n) ((65536L * (n)) / 360)

#define NEVER                0
#define LESS                 1
#define LEQUAL               2
#define GREATER              3
#define GEQUAL               4
#define EQUAL                5
#define NOTEQUAL             6
#define ALWAYS               7

#define ARGB1555             0
#define L1                   1
#define L4                   2
#define L8                   3
#define RGB332               4
#define ARGB2                5
#define ARGB4                6
#define RGB565               7
#define PALETTED             8
#define TEXT8X8              9
#define TEXTVGA              10
#define BARGRAPH             11
#define PALETTED565          14
#define PALETTED4444         15
#define PALETTED8            16
#define L2                   17

#define NEAREST              0
#define BILINEAR             1

#define BORDER               0
#define REPEAT               1

#define KEEP                 1
#define REPLACE              2
#define INCR                 3
#define DECR                 4
#define INVERT               5

#define DLSWAP_DONE          0
#define DLSWAP_LINE          1
#define DLSWAP_FRAME         2

#define INT_SWAP             1
#define INT_TOUCH            2
#define INT_TAG              4
#define INT_SOUND            8
#define INT_PLAYBACK         16
#define INT_CMDEMPTY         32
#define INT_CMDFLAG          64
#define INT_CONVCOMPLETE     128

#define TOUCHMODE_OFF        0
#define TOUCHMODE_ONESHOT    1
#define TOUCHMODE_FRAME      2
#define TOUCHMODE_CONTINUOUS 3

#define ZERO                 0
#define ONE                  1
#define SRC_ALPHA            2
#define DST_ALPHA            3
#define ONE_MINUS_SRC_ALPHA  4
#define ONE_MINUS_DST_ALPHA  5

#define BITMAPS              1
#define POINTS               2
#define LINES                3
#define LINE_STRIP           4
#define EDGE_STRIP_R         5
#define EDGE_STRIP_L         6
#define EDGE_STRIP_A         7
#define EDGE_STRIP_B         8
#define RECTS                9

#define OPT_MONO             1
#define OPT_NODL             2
#define OPT_FLAT             256
#define OPT_CENTERX          512
#define OPT_CENTERY          1024
#define OPT_CENTER           (OPT_CENTERX | OPT_CENTERY)
#define OPT_NOBACK           4096
#define OPT_NOTICKS          8192
#define OPT_NOHM             16384
#define OPT_NOPOINTER        16384
#define OPT_NOSECS           32768
#define OPT_NOHANDS          49152
#define OPT_RIGHTX           2048
#define OPT_SIGNED           256
#define OPT_SOUND            32

#define OPT_NOTEAR           4
#define OPT_FULLSCREEN       8
#define OPT_MEDIAFIFO        16

#define LINEAR_SAMPLES       0
#define ULAW_SAMPLES         1
#define ADPCM_SAMPLES        2

// 'instrument' argument to GD.play()

#define SILENCE              0x00

#define SQUAREWAVE           0x01
#define SINEWAVE             0x02
#define SAWTOOTH             0x03
#define TRIANGLE             0x04

#define BEEPING              0x05
#define ALARM                0x06
#define WARBLE               0x07
#define CAROUSEL             0x08

#define PIPS(n)              (0x0f + (n))

#define HARP                 0x40
#define XYLOPHONE            0x41
#define TUBA                 0x42
#define GLOCKENSPIEL         0x43
#define ORGAN                0x44
#define TRUMPET              0x45
#define PIANO                0x46
#define CHIMES               0x47
#define MUSICBOX             0x48
#define BELL                 0x49

#define CLICK                0x50
#define SWITCH               0x51
#define COWBELL              0x52
#define NOTCH                0x53
#define HIHAT                0x54
#define KICKDRUM             0x55
#define POP                  0x56
#define CLACK                0x57
#define CHACK                0x58

#define MUTE                 0x60
#define UNMUTE               0x61

#define RAM_PAL              1056768UL

#define RAM_CMD               (ft8xx_model ? 0x308000UL : 0x108000UL)
#define RAM_DL                (ft8xx_model ? 0x300000UL : 0x100000UL)
#define REG_CLOCK             (ft8xx_model ? 0x302008UL : 0x102408UL)
#define REG_CMD_DL            (ft8xx_model ? 0x302100UL : 0x1024ecUL)
#define REG_CMD_READ          (ft8xx_model ? 0x3020f8UL : 0x1024e4UL)
#define REG_CMD_WRITE         (ft8xx_model ? 0x3020fcUL : 0x1024e8UL)
#define REG_CPURESET          (ft8xx_model ? 0x302020UL : 0x10241cUL)
#define REG_CSPREAD           (ft8xx_model ? 0x302068UL : 0x102464UL)
#define REG_DITHER            (ft8xx_model ? 0x302060UL : 0x10245cUL)
#define REG_DLSWAP            (ft8xx_model ? 0x302054UL : 0x102450UL)
#define REG_FRAMES            (ft8xx_model ? 0x302004UL : 0x102404UL)
#define REG_FREQUENCY         (ft8xx_model ? 0x30200cUL : 0x10240cUL)
#define REG_GPIO              (ft8xx_model ? 0x302094UL : 0x102490UL)
#define REG_GPIO_DIR          (ft8xx_model ? 0x302090UL : 0x10248cUL)
#define REG_HCYCLE            (ft8xx_model ? 0x30202cUL : 0x102428UL)
#define REG_HOFFSET           (ft8xx_model ? 0x302030UL : 0x10242cUL)
#define REG_HSIZE             (ft8xx_model ? 0x302034UL : 0x102430UL)
#define REG_HSYNC0            (ft8xx_model ? 0x302038UL : 0x102434UL)
#define REG_HSYNC1            (ft8xx_model ? 0x30203cUL : 0x102438UL)
#define REG_ID                (ft8xx_model ? 0x302000UL : 0x102400UL)
#define REG_INT_EN            (ft8xx_model ? 0x3020acUL : 0x10249cUL)
#define REG_INT_FLAGS         (ft8xx_model ? 0x3020a8UL : 0x102498UL)
#define REG_INT_MASK          (ft8xx_model ? 0x3020b0UL : 0x1024a0UL)
#define REG_MACRO_0           (ft8xx_model ? 0x3020d8UL : 0x1024c8UL)
#define REG_MACRO_1           (ft8xx_model ? 0x3020dcUL : 0x1024ccUL)
#define REG_OUTBITS           (ft8xx_model ? 0x30205cUL : 0x102458UL)
#define REG_PCLK              (ft8xx_model ? 0x302070UL : 0x10246cUL)
#define REG_PCLK_POL          (ft8xx_model ? 0x30206cUL : 0x102468UL)
#define REG_PLAY              (ft8xx_model ? 0x30208cUL : 0x102488UL)
#define REG_PLAYBACK_FORMAT   (ft8xx_model ? 0x3020c4UL : 0x1024b4UL)
#define REG_PLAYBACK_FREQ     (ft8xx_model ? 0x3020c0UL : 0x1024b0UL)
#define REG_PLAYBACK_LENGTH   (ft8xx_model ? 0x3020b8UL : 0x1024a8UL)
#define REG_PLAYBACK_LOOP     (ft8xx_model ? 0x3020c8UL : 0x1024b8UL)
#define REG_PLAYBACK_PLAY     (ft8xx_model ? 0x3020ccUL : 0x1024bcUL)
#define REG_PLAYBACK_READPTR  (ft8xx_model ? 0x3020bcUL : 0x1024acUL)
#define REG_PLAYBACK_START    (ft8xx_model ? 0x3020b4UL : 0x1024a4UL)
#define REG_PWM_DUTY          (ft8xx_model ? 0x3020d4UL : 0x1024c4UL)
#define REG_PWM_HZ            (ft8xx_model ? 0x3020d0UL : 0x1024c0UL)
#define REG_ROTATE            (ft8xx_model ? 0x302058UL : 0x102454UL)
#define REG_SOUND             (ft8xx_model ? 0x302088UL : 0x102484UL)
#define REG_SWIZZLE           (ft8xx_model ? 0x302064UL : 0x102460UL)
#define REG_TAG               (ft8xx_model ? 0x30207cUL : 0x102478UL)
#define REG_TAG_X             (ft8xx_model ? 0x302074UL : 0x102470UL)
#define REG_TAG_Y             (ft8xx_model ? 0x302078UL : 0x102474UL)
#define REG_TOUCH_ADC_MODE    (ft8xx_model ? 0x302108UL : 0x1024f4UL)
#define REG_TOUCH_CHARGE      (ft8xx_model ? 0x30210cUL : 0x1024f8UL)
#define REG_TOUCH_DIRECT_XY   (ft8xx_model ? 0x30218cUL : 0x102574UL)
#define REG_TOUCH_DIRECT_Z1Z2 (ft8xx_model ? 0x302190UL : 0x102578UL)
#define REG_TOUCH_MODE        (ft8xx_model ? 0x302104UL : 0x1024f0UL)
#define REG_TOUCH_OVERSAMPLE  (ft8xx_model ? 0x302114UL : 0x102500UL)
#define REG_TOUCH_RAW_XY      (ft8xx_model ? 0x30211cUL : 0x102508UL)
#define REG_TOUCH_RZ          (ft8xx_model ? 0x302120UL : 0x10250cUL)
#define REG_TOUCH_RZTHRESH    (ft8xx_model ? 0x302118UL : 0x102504UL)
#define REG_TOUCH_SCREEN_XY   (ft8xx_model ? 0x302124UL : 0x102510UL)
#define REG_TOUCH_SETTLE      (ft8xx_model ? 0x302110UL : 0x1024fcUL)
#define REG_TOUCH_TAG         (ft8xx_model ? 0x30212cUL : 0x102518UL)
#define REG_TOUCH_TAG_XY      (ft8xx_model ? 0x302128UL : 0x102514UL)
#define REG_TOUCH_TRANSFORM_A (ft8xx_model ? 0x302150UL : 0x10251cUL)
#define REG_TOUCH_TRANSFORM_B (ft8xx_model ? 0x302154UL : 0x102520UL)
#define REG_TOUCH_TRANSFORM_C (ft8xx_model ? 0x302158UL : 0x102524UL)
#define REG_TOUCH_TRANSFORM_D (ft8xx_model ? 0x30215cUL : 0x102528UL)
#define REG_TOUCH_TRANSFORM_E (ft8xx_model ? 0x302160UL : 0x10252cUL)
#define REG_TOUCH_TRANSFORM_F (ft8xx_model ? 0x302164UL : 0x102530UL)
#define REG_TRACKER           (ft8xx_model ? 0x309000UL : 0x109000UL)
#define REG_TRIM              (ft8xx_model ? 0x302180UL : 0x10256cUL)
#define REG_VCYCLE            (ft8xx_model ? 0x302040UL : 0x10243cUL)
#define REG_VOFFSET           (ft8xx_model ? 0x302044UL : 0x102440UL)
#define REG_VOL_PB            (ft8xx_model ? 0x302080UL : 0x10247cUL)
#define REG_VOL_SOUND         (ft8xx_model ? 0x302084UL : 0x102480UL)
#define REG_VSIZE             (ft8xx_model ? 0x302048UL : 0x102444UL)
#define REG_VSYNC0            (ft8xx_model ? 0x30204cUL : 0x102448UL)
#define REG_VSYNC1            (ft8xx_model ? 0x302050UL : 0x10244cUL)
#define FONT_ROOT             (ft8xx_model ? 0x2ffffcUL : 0x0ffffcUL)

// FT81x only registers
#define REG_CMDB_SPACE                       0x302574UL
#define REG_CMDB_WRITE                       0x302578UL
#define REG_MEDIAFIFO_READ                   0x309014UL
#define REG_MEDIAFIFO_WRITE                  0x309018UL

// FT815 only registers
#define REG_FLASH_SIZE                       0x00309024 
#define REG_FLASH_STATUS                     0x003025f0 
#define REG_ADAPTIVE_FRAMERATE               0x0030257c

#define VERTEX2II(x, y, handle, cell) \
        ((2UL << 30) | (((x) & 511UL) << 21) | (((y) & 511UL) << 12) | (((handle) & 31) << 7) | (((cell) & 127) << 0))

#define ROM_PIXEL_FF        0xc0400UL

class Poly {
    int x0, y0, x1, y1;
    int x[8], y[8];
    byte n;
    void restart() {
      n = 0;
      x0 = (1 << GD.vxf) * GD.w;
      x1 = 0;
      y0 = (1 << GD.vxf) * GD.h;
      y1 = 0;
    }
    void perim() {
      for (byte i = 0; i < n; i++)
        GD.Vertex2f(x[i], y[i]);
      GD.Vertex2f(x[0], y[0]);
    }
  public:
    void begin() {
      restart();

      GD.ColorMask(0,0,0,0);
      GD.StencilOp(KEEP, INVERT);
      GD.StencilFunc(ALWAYS, 255, 255);
    }
    void v(int _x, int _y) {
      x0 = min(x0, _x >> GD.vxf);
      x1 = max(x1, _x >> GD.vxf);
      y0 = min(y0, _y >> GD.vxf);
      y1 = max(y1, _y >> GD.vxf);
      x[n] = _x;
      y[n] = _y;
      n++;
    }
    void paint() {
      x0 = max(0, x0);
      y0 = max(0, y0);
      x1 = min((1 << GD.vxf) * GD.w, x1);
      y1 = min((1 << GD.vxf) * GD.h, y1);
      GD.ScissorXY(x0, y0);
      GD.ScissorSize(x1 - x0 + 1, y1 - y0 + 1);
      GD.Begin(EDGE_STRIP_B);
      perim();
    }
    void finish() {
      GD.ColorMask(1,1,1,1);
      GD.StencilFunc(EQUAL, 255, 255);

      GD.Begin(EDGE_STRIP_R);
      GD.Vertex2f(0, 0);
      GD.Vertex2f(0, (1 << GD.vxf) * GD.h);
    }
    void draw() {
      paint();
      finish();
    }
    void outline() {
      GD.Begin(LINE_STRIP);
      perim();
    }
};

#if SDCARD
class Streamer {
public:
  void begin(const char *rawsamples,
             uint16_t freq = 44100,
             byte format = ADPCM_SAMPLES,
             uint32_t _base = (0x40000UL - 8192), uint16_t size = 8192) {
    GD.__end();
    r.openfile(rawsamples);
    GD.resume();

    base = _base;
    mask = size - 1;
    wp = 0;

    for (byte i = 10; i; i--)
      feed();

    GD.sample(base, size, freq, format, 1);
  }
  int feed() {
    uint16_t rp = GD.rd32(REG_PLAYBACK_READPTR) - base;
    uint16_t freespace = mask & ((rp - 1) - wp);
    if (freespace >= 512) {
      // REPORT(base);
      // REPORT(rp);
      // REPORT(wp);
      // REPORT(freespace);
      // Serial.println();
      byte buf[512];
      // uint16_t n = min(512, r.size - r.offset);
      // n = (n + 3) & ~3;   // force 32-bit alignment
      GD.__end();
      r.readsector(buf);
      GD.resume();
      GD.cmd_memwrite(base + wp, 512);
      GD.copyram(buf, 512);
      wp = (wp + 512) & mask;
    }
    return r.offset < r.size;
  }
  void progress(uint16_t &val, uint16_t &range) {
    uint32_t m = r.size;
    uint32_t p = min(r.offset, m);
    while (m > 0x10000) {
      m >>= 1;
      p >>= 1;
    }
    val = p;
    range = m;
  }
private:
  Reader r;
  uint32_t base;
  uint16_t mask;
  uint16_t wp;
};
#else
class Streamer {
public:
  void begin(const char *rawsamples,
             uint16_t freq = 44100,
             byte format = ADPCM_SAMPLES,
             uint32_t _base = (0x40000UL - 4096), uint16_t size = 4096) {}
  int feed() {}
  void progress(uint16_t &val, uint16_t &range) {}
};

#endif

////////////////////////////////////////////////////////////////////////
//  TileMap: maps made with the "tiled" map editor
////////////////////////////////////////////////////////////////////////

class TileMap {
  uint32_t chunkstart;
  int chunkw, chunkh;
  int stride;
  int bpc;
  byte layers;

public:
  uint16_t w, h;
  void begin(uint32_t loadpoint) {
    GD.finish();
    w      = GD.rd16(loadpoint +  0);
    h      = GD.rd16(loadpoint +  2);
    chunkw = GD.rd16(loadpoint +  4);
    chunkh = GD.rd16(loadpoint +  6);
    stride = GD.rd16(loadpoint +  8);
    layers = GD.rd16(loadpoint + 10);
    bpc = (4 * 16);
    chunkstart = loadpoint + 12;
  }
  void draw(uint16_t x, uint16_t y, uint16_t layermask = ~0) {
    int16_t chunk_x = (x / chunkw);
    int16_t ox0 = -(x % chunkw);
    int16_t chunk_y = (y / chunkh);
    int16_t oy = -(y % chunkh);

    GD.Begin(BITMAPS);
    GD.SaveContext();
    GD.BlendFunc(ONE, ONE_MINUS_SRC_ALPHA);
    while (oy < GD.h) {
      int16_t ox = ox0;
      GD.VertexTranslateY(oy << 4);
      uint32_t pos = chunkstart + (chunk_x + long(stride) * chunk_y) * layers * bpc;
      while (ox < GD.w) {
        GD.VertexTranslateX(ox << 4);
        for (byte layer = 0; layer < layers; layer++)
          if (layermask & (1 << layer))
            GD.cmd_append(pos + bpc * layer, bpc);
        pos += (layers * bpc);
        ox += chunkw;
      }
      oy += chunkh;
      chunk_y++;
    }
    GD.RestoreContext();
  }
  void draw(xy pos) {
    draw(pos.x >> 4, pos.y >> 4);
  }
  uint32_t addr(uint16_t x, uint16_t y, byte layer) {
    int16_t tx = (x / (chunkw >> 2));
    int16_t ty = (y / (chunkh >> 2));
    return
      chunkstart +
      ((tx >> 2) + long(stride) * (ty >> 2)) * layers * bpc +
      (tx & 3) * 4 +
      (ty & 3) * 16 +
      layer * 64;
  }
  int read(uint16_t x, uint16_t y, byte layer) {
    uint32_t op = GD.rd32(addr(x, y, layer));
    if ((op >> 24) == 0x2d)
      return 0;
    else
      return 1 + (op & 2047);
  }
  void write(uint16_t x, uint16_t y, byte layer, int tile) {
    uint32_t op;
    uint32_t a = addr(x, y, layer);
    if (tile == 0)
      op = 0x2d000000UL;
    else
      op = (GD.rd32(a) & ~2047) | ((tile - 1) & 2047);
    GD.wr32(a, op);
  }
  int read(xy pos, byte layer) {
    return read(pos.x >> 4, pos.y >> 4, layer);
  }
  void write(xy pos, byte layer, int tile) {
    write(pos.x >> 4, pos.y >> 4, layer, tile);
  }
};

class MoviePlayer
{
  uint32_t mf_size, mf_base, wp;
  Reader r;
  void loadsector() {
    byte buf[512];
    GD.__end();
    r.readsector(buf);
    GD.resume();
    GD.wr_n(mf_base + wp, buf, 512);
    wp = (wp + 512) & (mf_size - 1);
  }

public:
  int begin(const char *filename) {
    mf_size = 0x40000UL;
    mf_base = 0x100000UL - mf_size;
    GD.__end();
    if (!r.openfile(filename)) {
      // Serial.println("Open failed");
      return 0;
    }
    GD.resume();

    wp = 0;
    while (wp < (mf_size - 512)) {
      loadsector();
    }

    GD.cmd_mediafifo(mf_base, mf_size);
    GD.cmd_regwrite(REG_MEDIAFIFO_WRITE, wp);
    GD.finish();

    return 1;
  }
  int service() {
    if (r.eof()) {
      return 0;
    } else {
      uint32_t fullness = (wp - GD.rd32(REG_MEDIAFIFO_READ)) & (mf_size - 1);
      while (fullness < (mf_size - 512)) {
        loadsector();
        fullness += 512;
        GD.wr32(REG_MEDIAFIFO_WRITE, wp);
      }
      return 1;
    }
  }
  void play() {
    GD.cmd_playvideo(OPT_MEDIAFIFO | OPT_FULLSCREEN);
    GD.flush();
    while (service())
      ;
    GD.cmd_memcpy(0, 0, 4);
    GD.finish();
  }
};

class Dirsearch {
  struct dirent de;
  int index;

public:
  char name[13];
  void begin() {
    index = 0;
  }
  int get(const char *ext) {
    byte i;

    GD.__end();
    char e3[3];

    do {
      GD.SD.rdn((byte*)&de, GD.SD.o_root + index++ * 32, sizeof(de));
      for (i = 0; i < 3; i++)
        e3[i] = tolower(de.ext[i]);
    } while (de.name[0] &&
             ((de.name[0] & 0x80) || (memcmp(ext, e3, 3) != 0)));

    GD.resume();

    char *pc = name;
    for (i = 0; i < 8 && de.name[i] != ' '; i++)
      *pc++ = tolower(de.name[i]);
    *pc++ = '.';
    for (i = 0; i < 3 && de.ext[i] != ' '; i++)
      *pc++ = tolower(de.ext[i]);
    *pc++ = 0;

    return de.name[0];
  }
};

/*
 * PROGMEM declarations are currently not supported by the ESP8266
 * compiler. So redefine PROGMEM to nothing.
 */

#if defined(ESP8266) || defined(ARDUINO_ARCH_STM32L4) || defined(TEENSYDUINO)
#undef PROGMEM
#define PROGMEM
#endif

#if defined(TEENSYDUINO)
static void __attribute__((__unused__)) teensy_sync()
{
  while (1) {
    delay(1000);
    Serial.write("\xa4");
    for (int i = 0; i < 1000; i++) {
      if (Serial.available()) {
        if (Serial.read() == '!') {
          return;
        }
        delay(1);
      }
    }
  }
}
#endif

#endif
