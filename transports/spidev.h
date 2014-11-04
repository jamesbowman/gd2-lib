class GDTransport {
  int fd;
  uint8_t writebuf[4096];
  size_t nwrite;

public:
  void begin() {
    const char *DEV = "/dev/spidev0.0";
    fd = open(DEV, O_RDWR);
    if (fd <= 0) {
      perror(DEV);
      exit(1);
    }

    uint32_t speed = 500000;
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    scu(0, 0, 0);
#if PROTO == 0
    scu(0x44, 0, 0); // from external crystal
#endif
#if PROTO == 1
    scu(0x48, 0, 0);    // from internal oscillator
#endif
    scu(0x68, 0, 0);
    delay(50);

    wp = 0;
    freespace = 4096 - 4;

    stream();
  }

  static void default_spi_ioc(struct spi_ioc_transfer *ps) {
    memset(ps, 0, sizeof(*ps));
    ps->speed_hz = 20000000;
    ps->bits_per_word = 8;
  }

  // Raw read from GD2 memory
  void __read(void *dst, uint32_t addr, uint32_t n) {
    struct spi_ioc_transfer mesg[2];
    default_spi_ioc(&mesg[0]);
    default_spi_ioc(&mesg[1]);

    uint8_t tx[] = {(addr >> 16) & 0xff, (addr >> 8) & 0xff, (addr >> 0) & 0xff, 0xff};

    mesg[0].tx_buf = (__u64)tx;
    mesg[0].len = 4;
    mesg[0].cs_change = 0;

    mesg[1].rx_buf = (__u64)dst;
    mesg[1].len = n;
    mesg[1].cs_change = 1;

    ioctl(fd, SPI_IOC_MESSAGE(2), mesg);
    // printf("__read(%x %d) %x\n", addr, n, *(byte*)dst);
  }

  // Raw write to GD2 memory
  void __write(uint32_t addr, void *src, uint32_t n) {
    // printf("__write(%x %d)\n", addr, n);
    fflush(stdout);
    struct spi_ioc_transfer mesg[2];
    default_spi_ioc(&mesg[0]);
    default_spi_ioc(&mesg[1]);

    uint8_t tx[] = {0x80 | ((addr >> 16) & 0xff), (addr >> 8) & 0xff, (addr >> 0) & 0xff};

    mesg[0].tx_buf = (__u64)tx;
    mesg[0].len = 3;
    mesg[0].cs_change = 0;

    mesg[1].tx_buf = (__u64)src;
    mesg[1].len = n;
    mesg[1].cs_change = 1;

    ioctl(fd, SPI_IOC_MESSAGE(2), mesg);
  }

  void spisend(uint8_t b) {
    writebuf[nwrite++] = b;
  }
  void cmd32(uint32_t x) {
    if (freespace < 4) {
      getfree(4);
    }
    wp += 4;
    freespace -= 4;
    union {
      uint32_t c;
      uint8_t b[4];
    };
    c = x;
    spisend(b[0]);
    spisend(b[1]);
    spisend(b[2]);
    spisend(b[3]);
  }
  void cmdbyte(byte x) {
    if (freespace == 0) {
      getfree(1);
    }
    wp++;
    freespace--;
    spisend(x);
  }
  void cmd_n(byte *s, uint16_t n) {
    if (freespace < n) {
      getfree(n);
    }
    wp += n;
    freespace -= n;
    while (n) {
      n --;
      spisend(*s++);
    }
  }

  void flush() {
    getfree(0);
  }
  uint16_t rp() {
    uint16_t r = __rd16(REG_CMD_READ);
    if (r == 0xfff) {
      REPORT(/*EXCEPTION*/r);
      for (;;) ;
    }
    return r;
  }
  void finish() {
    wp &= 0xffc;
    stopstream();
    __wr16(REG_CMD_WRITE, wp);
    while (rp() != wp)
      ;
    stream();
  }

  byte rd(uint32_t addr)
  {
    stopstream(); // stop streaming
    byte r;
    __read(&r, addr, 1);
    stream();
    return r;
  }

  void wr(uint32_t addr, byte v)
  {
    stopstream(); // stop streaming
    __write(addr, &v, 1);
    stream();
  }

  uint16_t rd16(uint32_t addr)
  {
    stopstream(); // stop streaming
    uint16_t r = 0;
    __read(&r, addr, 2);
    stream();
    return r;
  }

  void wr16(uint32_t addr, uint32_t v)
  {
    stopstream(); // stop streaming
    __write(addr, &v, 2);
    stream();
  }

  uint32_t rd32(uint32_t addr)
  {
    stopstream(); // stop streaming
    uint32_t r = 0;
    __read(&r, addr, 4);
    stream();
    return r;
  }
  void rd_n(byte *dst, uint32_t addr, uint32_t n)
  {
    stopstream(); // stop streaming
    __read(dst, addr, n);
    stream();
  }
  void wr_n(uint32_t addr, byte *src, uint32_t n)
  {
    stopstream(); // stop streaming
    __write(addr, src, n);
    stream();
  }

  void wr32(uint32_t addr, unsigned long v)
  {
    stopstream(); // stop streaming
    __write(addr, &v, 4);
    stream();
  }

  uint32_t getwp(void) {
    return RAM_CMD + (wp & 0xffc);
  }

  void bulk(uint32_t addr) {
    /*
    __end(); // stop streaming
    __start(addr);
    */
  }
  void resume(void) {
    stream();
  }

  void stopstream() // flush any streamed output
  {
    if (nwrite != 0) {
      __write(wptr0, writebuf, nwrite);
      nwrite = 0;
    }
  }

  void stop() // end the SPI transaction
  {
    wp &= 0xffc;
    stopstream();
    __wr16(REG_CMD_WRITE, wp);
    // while (__rd16(REG_CMD_READ) != wp) ;
  }
  void __end() // end the SPI transaction
  {
    stopstream();
  }

  void stream(void) {
    nwrite = 0;
    wptr0 = RAM_CMD + wp;
  }

  uint16_t __rd16(uint32_t addr)
  {
    uint16_t r;
    __read(&r, addr, sizeof(r));
    return r;
  }

  void __wr16(uint32_t addr, uint16_t v)
  {
    __write(addr, &v, sizeof(v));
  }

  void scu(byte a, byte b, byte c)
  {
    uint8_t scu0[3] = { a, 0x00, 0x00 };
    write(fd, scu0, 3);
    delay(4);
  }

  void getfree(uint16_t n)
  {
    wp &= 0xfff;
    stopstream();
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
  uint32_t wptr0;
};

