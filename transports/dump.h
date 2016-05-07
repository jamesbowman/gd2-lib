class GDTransport {
  FILE *dumpfile;
  int counter;
  void hose(void) {
    char fn[2048];
    sprintf(fn, "%04d.cmd", counter);
    dumpfile = fopen(fn, "wb");
    if (!dumpfile) {
      perror(fn);
      exit(1);
    }
  }
public:
  void begin(void) {
    counter = 0;
    ft8xx_model = 1;
    hose();
  }
  static void hostcmd(byte a) {}
  void cmdbyte(byte x) {
    putc(x, dumpfile);
  }
  void cmd32(uint32_t x) {
    putc(x & 0xff, dumpfile);
    putc((x >> 8) & 0xff, dumpfile);
    putc((x >> 16) & 0xff, dumpfile);
    putc((x >> 24) & 0xff, dumpfile);
  }
  void cmd_n(byte *s, size_t n) {
    while (n--)
      putc(*s++, dumpfile);
  }
  uint8_t rd(uint32_t a) { return 0xff; }
  void wr(uint32_t a, uint8_t v) { }
  uint16_t rd16(uint32_t a) { return 0xff; }
  void wr16(uint32_t a, uint16_t v) { }
  uint32_t rd32(uint32_t a) { return 0xff; }
  void rd_n(byte *dst, uint32_t addr, uint16_t n) { }
  void wr_n(uint32_t addr, byte *src, uint16_t n) { }
  void wr32(uint32_t a, uint32_t v) { }
  void flush() { }
  void finish() { }
  void __end() { }
  uint32_t getwp(void) { return 0; }
  void bulk(uint32_t addr) {}
  void resume(void) {}
  void swap() {
    fclose(dumpfile);
    counter++;
    hose();
  }
};

