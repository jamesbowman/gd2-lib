// This file was generated with the command-line:
//    /usr/local/bin/gd3asset -f xkcd.gd2 -o xkcd_assets.h /home/james/Downloads/xkcd-script.ttf,size=26

#define XKCD_SCRIPT_HANDLE 0
#define XKCD_SCRIPT_WIDTH 18
#define XKCD_SCRIPT_HEIGHT 37
#define XKCD_SCRIPT_CELLS 96
#define ASSETS_END 32116UL
#define LOAD_ASSETS()  (GD.safeload("xkcd.gd2"), GD.loadptr = ASSETS_END)

static const shape_t XKCD_SCRIPT_SHAPE = {0, 18, 37, 0};
struct {
  Bitmap xkcd_script;
} bitmaps = {
 /*      xkcd_script */  {{ 18,  37}, {  9,  18},      0x0UL,  2,  0}
};
