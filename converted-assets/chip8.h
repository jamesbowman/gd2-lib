static const PROGMEM uint8_t __assets[50] = {
34, 255, 255, 255, 0, 0, 0, 0, 120, 156, 91, 36, 119, 136, 209, 136,
113, 145, 212, 5, 145, 2, 22, 3, 7, 33, 134, 4, 134, 66, 22, 67, 5,
33, 6, 33, 137, 6, 7, 5, 1, 5, 135, 6, 1, 0, 150, 232, 7, 143
};
#define LOAD_ASSETS()  GD.copy(__assets, sizeof(__assets))
#define MAZE 0UL
#define ASSETS_END 34UL
