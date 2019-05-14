#ifndef CS
#if defined(ESP8266)
#define CS D8
#elif  defined(ESP32)
#define CS 12
#elif defined(ARDUINO_ARCH_STM32)
#define CS PB0
#elif (BOARD == BOARD_SUNFLOWER)
#define CS 6
#else
#define CS 8
#endif
#endif

#if defined(ESP8266) || defined(ESP32)
#define YIELD() yield()
#else
#define YIELD()
#endif

SPIClass *spi = NULL;

class GDTransport {
private:
  byte model;
public:
  void ios() {
#if (BOARD == BOARD_SUNFLOWER)
    pinMode(5, OUTPUT);
    digitalWrite(5, LOW);
    delay(1);
    digitalWrite(5, HIGH);
    delay(1);
#endif

    pinMode(CS, OUTPUT);
    digitalWrite(CS, HIGH);
    pinMode(SD_PIN, OUTPUT);
    digitalWrite(SD_PIN, HIGH);
  }
  void begin0() {
    ios();

    // With custom SPI pins
    //spi = new SPIClass(HSPI);
    //spi->begin(14, 12, 13, 2);
    
    // With default SPI pins
    spi = new SPIClass();
    spi->begin();
    
#if defined(TEENSYDUINO) || defined(ARDUINO_ARCH_STM32L4) || defined(ARDUINO_ARCH_STM32)
    spi->beginTransaction(SPISettings(3000000, MSBFIRST, SPI_MODE0));
#else
#if !defined(__DUE__) && !defined(ESP8266) && !defined(ESP32) && !defined(ARDUINO_ARCH_STM32)
    spi->setClockDivider(SPI_CLOCK_DIV2);
    SPSR = (1 << SPI2X);
#endif
#endif

    hostcmd(0x42);    // SLEEP
    hostcmd(0x61);    // CLKSEL default
    hostcmd(0x00);    // ACTIVE
#if (BOARD != BOARD_GAMEDUINO23)
    hostcmd(0x44);    // CLKEXT
#else
    hostcmd(0x48);    // CLKINT
#endif
    hostcmd(0x49);    // PD_ROMS all up
    hostcmd(0x68);    // RST_PULSE
  }
  void begin1() {
#if 0
    delay(120);
#else
    while ((__rd16(0xc0000UL) & 0xff) != 0x08)
      ;
#endif

    // Test point: saturate SPI
    while (0) {
      digitalWrite(CS, LOW);
      spi->transfer(0x55);
      digitalWrite(CS, HIGH);
    }

#if 0
    // Test point: attempt to wake up FT8xx every 2 seconds
    while (1) {
      hostcmd(0x00);
      delay(120);
      hostcmd(0x68);
      delay(120);
      digitalWrite(CS, LOW);
      Serial.println(spi->transfer(0x10), HEX);
      Serial.println(spi->transfer(0x24), HEX);
      Serial.println(spi->transfer(0x00), HEX);
      Serial.println(spi->transfer(0xff), HEX);
      Serial.println(spi->transfer(0x00), HEX);
      Serial.println(spi->transfer(0x00), HEX);
      Serial.println();

      digitalWrite(CS, HIGH);
      delay(2000);
    }
#endif

    // So that FT800,801      FT81x
    // model       0            1
    ft8xx_model = __rd16(0x0c0000) >> 12;

    wp = 0;
    freespace = 4096 - 4;

    stream();
  }

  void external_crystal() {
    __end();
    hostcmd(0x44);
  }

  void cmd32(uint32_t x) {
    if (freespace < 4) {
      getfree(4);
    }
    wp += 4;
    freespace -= 4;
#if defined(ESP8266)
    // spi->writeBytes((uint8_t*)&x, 4);
    spi->write32(x, 0);
#elif defined(ESP32)
    // spi->write32(x) has the wrong byte order.
    spi->writeBytes((uint8_t*)&x, 4);
#else
    union {
      uint32_t c;
      uint8_t b[4];
    };
    c = x;
    spi->transfer(b[0]);
    spi->transfer(b[1]);
    spi->transfer(b[2]);
    spi->transfer(b[3]);
#endif
  }
  void cmdbyte(byte x) {
    if (freespace == 0) {
      getfree(1);
    }
    wp++;
    freespace--;
    spi->transfer(x);
  }
  void cmd_n(byte *s, uint16_t n) {
    if (freespace < n) {
      getfree(n);
    }
    wp += n;
    freespace -= n;
#if defined(ARDUINO_ARCH_STM32)
    spi->write(s, n);
#else
    while (n > 8) {
      n -= 8;
      spi->transfer(*s++);
      spi->transfer(*s++);
      spi->transfer(*s++);
      spi->transfer(*s++);
      spi->transfer(*s++);
      spi->transfer(*s++);
      spi->transfer(*s++);
      spi->transfer(*s++);
    }
    while (n--)
      spi->transfer(*s++);
#endif
  }

  void flush() {
    YIELD();
    getfree(0);
  }
  uint16_t rp() {
    uint16_t r = __rd16(REG_CMD_READ);
    if (r == 0xfff) {
      GD.alert("COPROCESSOR EXCEPTION");
    }
    return r;
  }
  void finish() {
    wp &= 0xffc;
    __end();
    __wr16(REG_CMD_WRITE, wp);
    while (rp() != wp)
      YIELD();
    stream();
  }

  byte rd(uint32_t addr)
  {
    __end(); // stop streaming
    __start(addr);
    spi->transfer(0);  // dummy
    byte r = spi->transfer(0);
    stream();
    return r;
  }

  void wr(uint32_t addr, byte v)
  {
    __end(); // stop streaming
    __wstart(addr);
    spi->transfer(v);
    stream();
  }

  uint16_t rd16(uint32_t addr)
  {
    uint16_t r = 0;
    __end(); // stop streaming
    __start(addr);
    spi->transfer(0);
    r = spi->transfer(0);
    r |= (spi->transfer(0) << 8);
    stream();
    return r;
  }

  void wr16(uint32_t addr, uint32_t v)
  {
    __end(); // stop streaming
    __wstart(addr);
    spi->transfer(v);
    spi->transfer(v >> 8);
    stream();
  }

  uint32_t rd32(uint32_t addr)
  {
    __end(); // stop streaming
    __start(addr);
    spi->transfer(0);
    union {
      uint32_t c;
      uint8_t b[4];
    };
    b[0] = spi->transfer(0);
    b[1] = spi->transfer(0);
    b[2] = spi->transfer(0);
    b[3] = spi->transfer(0);
    stream();
    return c;
  }
  void rd_n(byte *dst, uint32_t addr, uint16_t n)
  {
    __end(); // stop streaming
    __start(addr);
    spi->transfer(0);
    while (n--)
      *dst++ = spi->transfer(0);
    stream();
  }
#if defined(ARDUINO) && !defined(__DUE__) && !defined(ESP8266) && !defined(ESP32) && !defined(ARDUINO_ARCH_STM32L4) && !defined(ARDUINO_ARCH_STM32)
  void wr_n(uint32_t addr, byte *src, uint16_t n)
  {
    __end(); // stop streaming
    __wstart(addr);
    while (n--) {
      SPDR = *src++;
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
    }
    while (!(SPSR & _BV(SPIF))) ;
    stream();
  }
#else
  void wr_n(uint32_t addr, byte *src, uint16_t n)
  {
    __end(); // stop streaming
    __wstart(addr);
#if defined(ESP8266) || defined(ESP32)
    spi->writeBytes(src, n);
#elif defined(ARDUINO_ARCH_STM32)
    spi->write(src, n);
#else
    while (n--)
      spi->transfer(*src++);
#endif
    stream();
  }
#endif

  void wr32(uint32_t addr, unsigned long v)
  {
    __end(); // stop streaming
    __wstart(addr);
    spi->transfer(v);
    spi->transfer(v >> 8);
    spi->transfer(v >> 16);
    spi->transfer(v >> 24);
    stream();
  }

  uint32_t getwp(void) {
    return RAM_CMD + (wp & 0xffc);
  }

  void bulk(uint32_t addr) {
    __end(); // stop streaming
    __start(addr);
  }
  void resume(void) {
    stream();
  }

  static void __start(uint32_t addr) // start an SPI transaction to addr
  {
    digitalWrite(CS, LOW);
    spi->transfer(addr >> 16);
    spi->transfer(highByte(addr));
    spi->transfer(lowByte(addr));
  }

  static void __wstart(uint32_t addr) // start an SPI write transaction to addr
  {
    digitalWrite(CS, LOW);
    spi->transfer(0x80 | (addr >> 16));
    spi->transfer(highByte(addr));
    spi->transfer(lowByte(addr));
  }

  static void __end() // end the SPI transaction
  {
    digitalWrite(CS, HIGH);
  }

  void stop() // end the SPI transaction
  {
    wp &= 0xffc;
    __end();
    __wr16(REG_CMD_WRITE, wp);
    // while (__rd16(REG_CMD_READ) != wp) ;
  }

  void stream(void) {
    __end();
    __wstart(RAM_CMD + (wp & 0xfff));
  }

  static unsigned int __rd16(uint32_t addr)
  {
    unsigned int r;

    __start(addr);
    spi->transfer(0);  // dummy
    r = spi->transfer(0);
    r |= (spi->transfer(0) << 8);
    __end();
    return r;
  }

  static void __wr16(uint32_t addr, unsigned int v)
  {
    __wstart(addr);
    spi->transfer(lowByte(v));
    spi->transfer(highByte(v));
    __end();
  }

  static void hostcmd(byte a)
  {
    digitalWrite(CS, LOW);
    spi->transfer(a);
    spi->transfer(0x00);
    spi->transfer(0x00);
    digitalWrite(CS, HIGH);
  }

  void getfree(uint16_t n)
  {
    wp &= 0xfff;
    __end();
    __wr16(REG_CMD_WRITE, wp & 0xffc);
    do {
      uint16_t fullness = (wp - rp()) & 4095;
      freespace = (4096 - 4) - fullness;
    } while (freespace < n);
    stream();
  }

  byte streaming;
  uint16_t wp;
  uint16_t freespace;
};
