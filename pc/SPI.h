extern "C" {
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "stdint.h"
#include <unistd.h>
#include "math.h"
#include "ctype.h"
#include <assert.h>
}

static void delay(unsigned int n)
{
  usleep(1000 * n);
}
static void delayMicroseconds(unsigned int n)
{
  usleep(n);
}
static unsigned long micros()
{
  return 0;
}
static unsigned long millis()
{
  return 0;
}

#define DEC 0
#define HEX 1

class _Serial {
  public:
    void begin(int speed) {
    }
    void write(int x) {
      printf("%c", x);
    }

    void print(const char c) {
      printf("%c", c);
    }
    void print(const char s[]) {
      printf("%s", s);
    }
    void print(int x, int base) {
      printf((base == DEC) ? "%d" : "%X", x);
    }

    void println(void) {
      printf("\n");
    }
    void println(const char c) {
      printf("%c\n", c);
    }
    void println(const char s[]) {
      printf("%s\n", s);
    }
    void println(int x, int base) {
      printf((base == DEC) ? "%d\n" : "%X\n", x);
    }
};

static _Serial Serial;
#if 1
extern "C" uint8_t spix(uint8_t);
class _SPI {
  public:
    void begin() {
    }
    uint8_t transfer(uint8_t c) {
      assert(0);
      // uint8_t r = spix(c);
      return 0;
    }
    uint8_t transfer(uint8_t *m, size_t c) {
      assert(0);
    }
};
static _SPI SPI;
#endif

#define A0  0xa0
#define A1  0xa1
#define A2  0xa2

#define OUTPUT 1
#define INPUT  0

#define HIGH   1
#define LOW    0

#define min(a, b)   (((a) < (b)) ? (a) : (b))
#define max(a, b)   (((a) > (b)) ? (a) : (b))
#define abs(x)      (((x) < 0) ? (-(x)) : (x))

static void pinMode(int, int)
{
}

static int analogRead(int x)
{
  return 0;
}

static int digitalRead(int x)
{
  return 0;
}

static void digitalWrite(int pin, int state)
{
}

static uint32_t seed = 7;

static void randomSeed(unsigned int n)
{
  seed = n | (n == 0);
}

static unsigned int xrandom()
{
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return seed;
}
static unsigned int random(unsigned int n)
{
  return xrandom() % n;
}
static unsigned int random(unsigned int a, unsigned int b)
{
  return a + random(b - a);
}

typedef unsigned char byte;
typedef const unsigned char prog_uchar;
typedef const signed char prog_char;
typedef const unsigned short prog_uint16_t;
typedef const signed short prog_int16_t;
typedef const unsigned long prog_uint32_t;

#define pgm_read_byte(x) (*(prog_uchar*)(x))
#define pgm_read_byte_near(x) pgm_read_byte(x)
#define pgm_read_word(x) (*(prog_uint16_t*)(x))
#define pgm_read_word_near(x) pgm_read_word(x)
#define pgm_read_dword(x) (*(prog_uint32_t*)(x))
#define pgm_read_dword_near(x) pgm_read_dword(x)
#define PROGMEM 
#define memcpy_P(a,b,c) memcpy((a), (b), (c))
#define strcpy_P(a,b) strcpy((a), (b))
#define strlen_P(a) strlen((const char *)(a))

#define highByte(x) ((x) >> 8)
#define lowByte(x) ((x) & 255)
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

long map(long x, long in_min, long in_max, long out_min, long out_max);
