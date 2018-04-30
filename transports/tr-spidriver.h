#include "spidriver.h"

class GDTransport {
  SPIDriver sd;
  uint16_t space, wp;

  uint8_t buf[64];
  size_t nbuf;

public:
  void begin0(void) {
    spi_connect(&sd, "/dev/ttyUSB0");

    hostcmd(0x42);    // SLEEP
    hostcmd(0x61);    // CLKSEL default
    hostcmd(0x00);    // ACTIVE
    hostcmd(0x48);    // CLKINT
    hostcmd(0x49);    // PD_ROMS all up
    hostcmd(0x68);    // RST_PULSE
    ft8xx_model = 1;
  }
  void begin1(void) {
    while ((__rd16(0xc0000UL) & 0xff) != 0x08)
      ;
    wp = 0;
    stream();
  }
  void ios(void) {}
  void external_crystal() {
    __end();
    hostcmd(0x44);
  }
  void cmdbyte(char x) {
    if (nbuf == 64)
      flush();
    buf[nbuf++] = x;
  }
  void cmd32(uint32_t x) {
    if (nbuf == 64)
      flush();
    *(uint32_t*)&buf[nbuf] = x;
    nbuf += 4;
  }
  void cmd_n(byte *s, size_t n) {
    flush();
    getspace(n);
    spi_write(&sd, n, (char*)s);
    wp += n;
  }
  void hostcmd(uint8_t a)
  {
    char buf[3] = {a, 0, 0};
    spi_sel(&sd);
    spi_write(&sd, 3, buf);
    spi_unsel(&sd);
  }

  uint16_t rd16(uint32_t a) {
    __end();
    uint16_t r = __rd16(a);
    stream();
    return r;
  }
  void wr16(uint32_t a, uint16_t v) {
    __end();
    __wr16(a, v);
    stream();
  }
  uint8_t rd(uint32_t a) {
    return rd16(a);
  }
  void wr(uint32_t a, byte v)
  {
    __end();
    char buf[4] = {(a >> 16) | 0x80, a >> 8, a, v};
    spi_sel(&sd);
    spi_write(&sd, sizeof(buf), buf);
    spi_unsel(&sd);
    stream();
  }

  void __end() {
    spi_unsel(&sd);
  }
  void stream(void) {
    space = __rd16(REG_CMDB_SPACE);
    assert((space & 3) == 0);
    __wstart(REG_CMDB_WRITE);
  }
  void resume(void) {
    stream();
  }
  void getspace(uint16_t n) {
    while (space < n) {
      __end();
      stream();
    }
    space -= n;
  }
  uint32_t getwp(void) {
    flush();
    return RAM_CMD + (wp & 0xffc);
  }
  uint32_t rd32(uint32_t a) {
    uint32_t r;
    rd_n((byte*)&r, a, 4);
    return r;
  }
  void rd_n(byte *dst, uint32_t a, uint16_t n) {
    __end();
    char buf[4] = {a >> 16, a >> 8, a, 0xff};
    spi_sel(&sd);
    spi_write(&sd, sizeof(buf), buf);
    spi_read(&sd, n, (char*)dst);
    spi_unsel(&sd);
    stream();
  }
  void wr32(uint32_t a, uint32_t v) {
  }
  void flush() {
    if (nbuf) {
      getspace(nbuf);
      spi_write(&sd, nbuf, (char*)buf);
      wp += nbuf;
      nbuf = 0;
    }
  }
  void finish() {
    flush();
    while (space < 4092) {
      __end();
      stream();
    }
  }
  void bulk(uint32_t addr) {}

  unsigned int __wstart(uint32_t a) {
    char buf[3] = {0x80 | (a >> 16), a >> 8, a};
    spi_sel(&sd);
    spi_write(&sd, sizeof(buf), buf);
  }

  unsigned int __rd16(uint32_t a) {
    char buf[6] = {a >> 16, a >> 8, a, 0xff, 0xff, 0xff};
    spi_sel(&sd);
    spi_writeread(&sd, sizeof(buf), buf);
    spi_unsel(&sd);
    return *(uint16_t*)&buf[4];
  }

  void __wr16(uint32_t a, unsigned int v) {
    char buf[5] = {(a >> 16) | 0x80, a >> 8, a, v, v >> 8};
    spi_sel(&sd);
    spi_write(&sd, sizeof(buf), buf);
    spi_unsel(&sd);
  }
  void stop() {} // end the SPI transaction
  void wr_n(uint32_t addr, byte *src, uint16_t n) {
    assert(0);
  }
};
