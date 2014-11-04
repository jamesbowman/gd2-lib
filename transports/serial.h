class GDTransport {
public:
  void begin(void) {
    Serial.begin(115200);
    // Serial.println("----- START -----");
  }
  void cmdbyte(byte x) {
    Serial.println(x, HEX);
  }
  void cmd32(uint32_t x) {
    Serial.println(x & 0xff, HEX);
    Serial.println((x >> 8) & 0xff, HEX);
    Serial.println((x >> 16) & 0xff, HEX);
    Serial.println((x >> 24) & 0xff, HEX);
  }
  void cmd_n(byte *s, size_t n) {
    while (n--)
      Serial.println(*s++, HEX);
  }
  uint8_t rd(uint32_t a) { return 0xff; }
  void wr(uint32_t a, uint8_t v) { }
  uint16_t rd16(uint32_t a) { return 0xff; }
  void wr16(uint32_t a, uint16_t v) { }
  uint32_t rd32(uint32_t a) { return 0xff; }
  void rd_n(byte *dst, uint32_t addr, uint16_t n) { }
  void wr32(uint32_t a, uint32_t v) { }
  void flush() { }
  void finish() { }
  void __end() { }
  uint32_t getwp(void) { return 0; }
  void bulk(uint32_t addr) {}
  void resume(void) {}
};

