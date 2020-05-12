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

class GDTransport {
private:
  byte cs;
  byte model;
public:
  void begin0(int _cs = CS) {
#if (BOARD == BOARD_SUNFLOWER)
    pinMode(5, OUTPUT);
    digitalWrite(5, LOW);
    delay(1);
    digitalWrite(5, HIGH);
    delay(1);
#endif

    cs = _cs;

    pinMode(9, OUTPUT);
    digitalWrite(9, HIGH);

    pinMode(cs, OUTPUT);
    digitalWrite(cs, HIGH);

    SPI.begin();
#if defined(TEENSYDUINO) || defined(ARDUINO_ARCH_STM32L4) || defined(ARDUINO_ARCH_STM32)
    SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
#else
#if !defined(__DUE__) && !defined(ESP8266) && !defined(ESP32) && !defined(ARDUINO_ARCH_STM32)
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    SPSR = (1 << SPI2X);
#endif
#endif

    hostcmd(0x42);    // SLEEP
//  hostcmd(0x61);    // CLKSEL default
    hostcmd(0x00);    // ACTIVE
#if (BOARD != BOARD_GAMEDUINO23)
    hostcmd(0x44);    // CLKEXT
#else
    hostcmd(0x48);    // CLKINT
#endif
//  hostcmd(0x49);    // PD_ROMS all up
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
      digitalWrite(cs, LOW);
      SPI.transfer(0x55);
      digitalWrite(cs, HIGH);
    }

#if 0
    // Test point: attempt to wake up FT8xx every 2 seconds
    while (1) {
      hostcmd(0x00);
      delay(120);
      hostcmd(0x68);
      delay(120);
      digitalWrite(cs, LOW);
      Serial.println(SPI.transfer(0x10), HEX);
      Serial.println(SPI.transfer(0x24), HEX);
      Serial.println(SPI.transfer(0x00), HEX);
      Serial.println(SPI.transfer(0xff), HEX);
      Serial.println(SPI.transfer(0x00), HEX);
      Serial.println(SPI.transfer(0x00), HEX);
      Serial.println();

      digitalWrite(cs, HIGH);
      delay(2000);
    }
#endif

    // So that FT800,801      FT810-3   FT815,6
    // model       0            1         2
    switch (__rd16(0x0c0000UL) >> 8) {
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13: ft8xx_model = 1; break;
    case 0x15:
    case 0x16: ft8xx_model = 2; break;
    default:   ft8xx_model = 0; break;
    }

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
    // SPI.writeBytes((uint8_t*)&x, 4);
    SPI.write32(x, 0);
#elif defined(ESP32)
    // SPI.write32(x) has the wrong byte order.
    SPI.writeBytes((uint8_t*)&x, 4);
#else
    union {
      uint32_t c;
      uint8_t b[4];
    };
    c = x;
    SPI.transfer(b[0]);
    SPI.transfer(b[1]);
    SPI.transfer(b[2]);
    SPI.transfer(b[3]);
#endif
  }
  void cmdbyte(byte x) {
    if (freespace == 0) {
      getfree(1);
    }
    wp++;
    freespace--;
    SPI.transfer(x);
  }
  void cmd_n(byte *s, uint16_t n) {
    if (freespace < n) {
      getfree(n);
    }
    wp += n;
    freespace -= n;
#if defined(ARDUINO_ARCH_STM32)
    SPI.write(s, n);
#else
    while (n > 8) {
      n -= 8;
      SPI.transfer(*s++);
      SPI.transfer(*s++);
      SPI.transfer(*s++);
      SPI.transfer(*s++);
      SPI.transfer(*s++);
      SPI.transfer(*s++);
      SPI.transfer(*s++);
      SPI.transfer(*s++);
    }
    while (n--)
      SPI.transfer(*s++);
#endif
  }

  void flush() {
    YIELD();
    getfree(0);
  }
  void coprocsssor_recovery(void) {
    __end();
    if (ft8xx_model >= 2)
      for (byte i = 0; i < 128; i += 2)
        __wr16(i, __rd16(0x309800UL + i));

    __wr16(REG_CPURESET, 1);
    __wr16(REG_CMD_WRITE, 0);
    __wr16(REG_CMD_READ, 0);
    wp = 0;
    __wr16(REG_CPURESET, 0);
    stream();
  }
  uint16_t rp() {
    uint16_t r = __rd16(REG_CMD_READ);
    if (r == 0xfff)
      GD.alert();
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
    SPI.transfer(0);  // dummy
    byte r = SPI.transfer(0);
    stream();
    return r;
  }

  void wr(uint32_t addr, byte v)
  {
    __end(); // stop streaming
    __wstart(addr);
    SPI.transfer(v);
    stream();
  }

  uint16_t rd16(uint32_t addr)
  {
    uint16_t r = 0;
    __end(); // stop streaming
    __start(addr);
    SPI.transfer(0);
    r = SPI.transfer(0);
    r |= (SPI.transfer(0) << 8);
    stream();
    return r;
  }

  void wr16(uint32_t addr, uint32_t v)
  {
    __end(); // stop streaming
    __wstart(addr);
    SPI.transfer(v);
    SPI.transfer(v >> 8);
    stream();
  }

  uint32_t rd32(uint32_t addr)
  {
    __end(); // stop streaming
    __start(addr);
    SPI.transfer(0);
    union {
      uint32_t c;
      uint8_t b[4];
    };
    b[0] = SPI.transfer(0);
    b[1] = SPI.transfer(0);
    b[2] = SPI.transfer(0);
    b[3] = SPI.transfer(0);
    stream();
    return c;
  }
  void rd_n(byte *dst, uint32_t addr, uint16_t n)
  {
    __end(); // stop streaming
    __start(addr);
    SPI.transfer(0);
    while (n--)
      *dst++ = SPI.transfer(0);
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
    SPI.writeBytes(src, n);
#elif defined(ARDUINO_ARCH_STM32)
    SPI.write(src, n);
#else
    while (n--)
      SPI.transfer(*src++);
#endif
    stream();
  }
#endif

  void wr32(uint32_t addr, unsigned long v)
  {
    __end(); // stop streaming
    __wstart(addr);
    SPI.transfer(v);
    SPI.transfer(v >> 8);
    SPI.transfer(v >> 16);
    SPI.transfer(v >> 24);
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

  void __start(uint32_t addr) // start an SPI transaction to addr
  {
    digitalWrite(cs, LOW);
    SPI.transfer(addr >> 16);
    SPI.transfer(highByte(addr));
    SPI.transfer(lowByte(addr));
  }

  void __wstart(uint32_t addr) // start an SPI write transaction to addr
  {
    digitalWrite(cs, LOW);
    SPI.transfer(0x80 | (addr >> 16));
    SPI.transfer(highByte(addr));
    SPI.transfer(lowByte(addr));
  }

  void __end() // end the SPI transaction
  {
    digitalWrite(cs, HIGH);
  }

  void stop() // end the SPI transaction
  {
    wp &= 0xffc;
    __end();
    __wr16(REG_CMD_WRITE, wp);
    // while (__rd16(REG_CMD_READ) != wp) ;
  }

  void capture_error_message(void) {
    __end();
    if (ft8xx_model >= 2)
      for (byte i = 0; i < 128; i += 2)
        __wr16(i, __rd16(0x309800UL + i));
  }

  void stream(void) {
    __end();
    __wstart(RAM_CMD + (wp & 0xfff));
  }

  unsigned int __rd16(uint32_t addr)
  {
    unsigned int r;

    __start(addr);
    SPI.transfer(0);  // dummy
    r = SPI.transfer(0);
    r |= (SPI.transfer(0) << 8);
    __end();
    return r;
  }

  void __wr16(uint32_t addr, unsigned int v)
  {
    __wstart(addr);
    SPI.transfer(lowByte(v));
    SPI.transfer(highByte(v));
    __end();
  }

  void hostcmd(byte a)
  {
    digitalWrite(cs, LOW);
    SPI.transfer(a);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    digitalWrite(cs, HIGH);
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
