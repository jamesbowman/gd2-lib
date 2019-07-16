#include "spidriver.h"

class GDTransport {
  SPIDriver sd;
  uint16_t space, wp;

  uint8_t buf[64];
  size_t nbuf;

public:
  void begin0(int _cs) {
    char *port = getenv("PORT");
    spi_connect(&sd, port ? port : "/dev/ttyUSB0");

//     hostcmd(0x42);    // SLEEP
//     hostcmd(0x61);    // CLKSEL default
     hostcmd(0x00);    // ACTIVE
     hostcmd(0x48);    // CLKINT
//     hostcmd(0x49);    // PD_ROMS all up
     hostcmd(0x68);    // RST_PULSE
  }
  void begin1(void) {
    uint16_t id;
    while (((id = __rd16(0xc0000UL)) & 0xff) != 0x08)
      ;
    // So that FT800,801      FT810-3   FT815,6
    // model       0            1         2
    switch (id >> 8) {
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13: ft8xx_model = 1; break;
    case 0x15:
    case 0x16: ft8xx_model = 2; break;
    default:   ft8xx_model = 0; break;
    }
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
    for (size_t i = 0; i < n; i++) {
      cmdbyte(s[i]);
    }
  }
  void hostcmd(uint8_t a)
  {
    char buf[3] = {(char)a, 0, 0};
    spi_sel(&sd);
    spi_write(&sd, buf, 3);
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
#define ADDR3(a)  (char)((a >> 16)       ), (char)(a >> 8), (char)(a)
#define WADDR3(a) (char)((a >> 16) | 0x80), (char)(a >> 8), (char)(a)

  void wr(uint32_t a, byte v)
  {
    __end();
    char buf[4] = {WADDR3(a), (char)v};
    spi_sel(&sd);
    spi_write(&sd, buf, sizeof(buf));
    spi_unsel(&sd);
    stream();
  }

  void __end() {
    spi_unsel(&sd);
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
  void stream(void) {
    space = __rd16(REG_CMDB_SPACE);
    if (space & 3) {
      GD.alert();
    }
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
    char buf[4] = {ADDR3(a)};
    spi_sel(&sd);
    spi_write(&sd, buf, sizeof(buf));
    spi_read(&sd, (char*)dst, n);
    spi_unsel(&sd);
    stream();
  }
  void wr32(uint32_t a, uint32_t v) {
  }
  void flush() {
    if (nbuf) {
      getspace(nbuf);
      if (1) {
        spi_write(&sd, (char*)buf, nbuf);
        wp += nbuf;
      } 
      if (1) {
        static FILE *log = NULL;
        if (log == NULL)
          log = fopen("log", "w");
        fwrite(buf, 1, nbuf, log);
        fflush(log);
      }
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
  void bulk(uint32_t addr) {
    __end();
    char buf[3] = {ADDR3(addr)};
    spi_sel(&sd);
    spi_write(&sd, buf, sizeof(buf));
  }

  void __wstart(uint32_t a) {
    char buf[3] = {WADDR3(a)};
    spi_sel(&sd);
    spi_write(&sd, buf, sizeof(buf));
  }

  unsigned int __rd16(uint32_t a) {
    char buf[6] = {ADDR3(a), (char)-1, (char)-1, (char)-1};
    spi_sel(&sd);
    spi_writeread(&sd, buf, sizeof(buf));
    spi_unsel(&sd);
    return *(uint16_t*)&buf[4];
  }

  void __wr16(uint32_t a, unsigned int v) {
    char buf[5] = {WADDR3(a), (char)v, (char)(v >> 8)};
    spi_sel(&sd);
    spi_write(&sd, buf, sizeof(buf));
    spi_unsel(&sd);
  }
  void stop() {} // end the SPI transaction
  void wr_n(uint32_t addr, byte *src, uint16_t n) {
    __end();
    __wstart(addr);
    spi_write(&sd, (char*)src, n);
    spi_unsel(&sd);
    stream();
  }
};
