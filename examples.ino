#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include "examples_assets.h"
#include "examples2_assets.h"

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin();
}

void f1()
{
}

// Apply a rotation around pixel (64, 64) //' rota{
static void rotate_64_64(uint16_t a)
{
    GD.cmd_translate(F16(64),F16(64));
    GD.cmd_rotate(a);
    GD.cmd_translate(F16(-64), F16(-64));
} //' }rota

// Apply a scale (s, t) around pixel (64, 64) //' scalea{
static void scale_64_64(uint32_t s, uint32_t t)
{
    GD.cmd_translate(F16(64),F16(64));
    GD.cmd_scale(s, t);
    GD.cmd_translate(F16(-64), F16(-64));
} //' }scalea

static void zigzag(const char *title, int x)
{
  for (int i = 0; i < 3; i++) {
    GD.Vertex2ii(x - 14,   25 + i * 90, SPRITES_HANDLE, 2);
    GD.Vertex2ii(x + 14,   25 + 45 + i * 90, SPRITES_HANDLE, 2);
  }
  GD.cmd_text(x, 0, 27, OPT_CENTERX, title);
}

static void button(int x, int y, byte label);

void loop()
{
  GD.ClearColorRGB(0x103000); //' a{
  GD.Clear();
  GD.cmd_text(240, 136, 31, OPT_CENTER, "Hello world");
  GD.Begin(POINTS);
  GD.Vertex2ii(220, 100);
  GD.Vertex2ii(260, 170);
  GD.swap(); //' }a

  GD.ClearColorRGB(0x103000); //' b{
  GD.Clear();
  GD.cmd_text(240, 136, 31, OPT_CENTER, "Hello world");
  GD.PointSize(16 * 30);  // means 30 pixels
  GD.Begin(POINTS);
  GD.Vertex2ii(220, 100);
  GD.Vertex2ii(260, 170);
  GD.swap(); //' }b

  GD.ClearColorRGB(0x103000); //' c{
  GD.Clear();
  GD.cmd_text(240, 136, 31, OPT_CENTER, "Hello world");
  GD.PointSize(16 * 30);
  GD.Begin(POINTS);
  GD.ColorRGB(0xff8000);  // orange
  GD.Vertex2ii(220, 100);
  GD.ColorRGB(0x0080ff);  // teal
  GD.Vertex2ii(260, 170);
  GD.swap(); //' }c

  GD.ClearColorRGB(0x103000); //' d{
  GD.Clear();
  GD.cmd_text(240, 136, 31, OPT_CENTER, "Hello world");
  GD.PointSize(16 * 30);
  GD.Begin(POINTS);
  GD.ColorA(128);         // 50% transparent
  GD.ColorRGB(0xff8000);  // orange
  GD.Vertex2ii(220, 100);
  GD.ColorRGB(0x0080ff);  // teal
  GD.Vertex2ii(260, 170); 
  GD.swap(); //' }d

  GD.ColorRGB(0, 128, 255); // teal //' triplet{
  GD.Vertex2ii(260, 170);   //' }triplet

  GD.play(PIANO, 60);   //' e{
  delay(1000);
  GD.play(ORGAN, 64);   //' }e

  GD.ClearColorRGB(0x103000); //' f{
  GD.Clear();
  GD.cmd_text(240, 136, 31, OPT_CENTER, "Hello world");
  GD.PointSize(16 * 30);
  GD.Begin(POINTS);
  GD.ColorRGB(0xff8000);
  GD.Tag(100);
  GD.Vertex2ii(220, 100);
  GD.ColorRGB(0x0080ff);
  GD.Tag(101);
  GD.Vertex2ii(260, 170);
  GD.swap();  //' }f
  
#ifndef DUMPDEV    // JCB
  Serial.begin(9600);
  for (;;) {      //' g{
    GD.get_inputs();
    Serial.println(GD.inputs.tag);
  } //' }g
#endif          // JCB

  GD.Clear(); //' h{
  for (int i = 0; i <= 254; i++) {
    GD.Tag(i);
    GD.cmd_number((i % 16) * 30, (i / 16) * 17, 26, 0, i);
  }
  GD.swap();
#ifndef DUMPDEV    // JCB
  GD.get_inputs(); //' }h
  Serial.println(GD.inputs.tag);
#endif          // JCB

  GD.cmd_loadimage(0, 0);
  GD.load("healsky3.jpg");

  GD.Clear(); //' i{
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0);
  GD.Vertex2ii(100, 100);
  GD.Vertex2ii(200, 0);
  GD.Vertex2ii(300, 100);
  GD.swap(); //' }i

  GD.Clear(); //' j{
  GD.Begin(BITMAPS);
  GD.BitmapSize(NEAREST, BORDER, BORDER, 64, 64);
  GD.Vertex2ii(0, 0);
  GD.swap(); //' }j

  GD.Clear(); //' k{
  GD.Begin(BITMAPS);
  GD.BitmapSize(NEAREST, BORDER, BORDER, 480, 272);
  GD.Vertex2ii(0, 0);
  GD.swap(); //' }k

  GD.Clear(); //' l{
  GD.Begin(BITMAPS);
  GD.BitmapSize(NEAREST, REPEAT, REPEAT, 480, 272);
  GD.Vertex2ii(0, 0);
  GD.swap(); //' }l

  GD.BitmapHandle(0);
  GD.cmd_loadimage(0, 0);
  GD.load("sunrise.jpg");
  GD.BitmapHandle(1);
  GD.cmd_loadimage(-1, 0);
  GD.load("healsky3.jpg");

  GD.Clear(); //' m{
  GD.Begin(BITMAPS);
  for (int i = 0; i < 100; i++)
    GD.Vertex2ii(GD.random(480), GD.random(272), GD.random(2));
  GD.swap(); //' }m

  GD.Clear(); //' n{
  GD.Begin(BITMAPS);

  GD.ColorRGB(0x00ff00);                  // pure green
  GD.Vertex2ii(240 - 130, 136 - 130, 1);

  GD.ColorRGB(0xff8080);                  // pinkish
  GD.Vertex2ii(240      , 136 - 130, 1);

  GD.ColorRGB(0xffff80);                  // yellowish
  GD.Vertex2ii(240 - 130, 136      , 1);

  GD.ColorRGB(0xffffff);                  // white
  GD.Vertex2ii(240      , 136      , 1);
  GD.swap(); //' }n

  GD.cmd_gradient(0, 0,   0x0060c0, //' grad{
                  0, 271, 0xc06000);
  GD.cmd_text(240, 136, 31, OPT_CENTER, "READY PLAYER ONE");
  GD.swap(); //' }grad

#define WIDGET(c) do { \
  GD.ClearColorRGB(0x404040); \
  GD.Clear(); \
  (c); \
  GD.swap(); \
  } while (0)

  WIDGET(GD.cmd_clock(240, 136, 120, 0, 8, 27, 13, 0));
  WIDGET(GD.cmd_dial(240, 136, 120, 0, 5333));
  WIDGET(GD.cmd_gauge(240, 136, 120, 0, 4, 2, 5333, 65535));
  WIDGET(GD.cmd_keys(0, 136 - 40, 480, 80, 31, 'u', "qwertyuiop"));
  WIDGET((
    GD.cmd_number(240, 45, 31, OPT_CENTER, 42),
    GD.cmd_number(240, 136, 31, OPT_CENTER | 4, 42),
    GD.cmd_number(240, 226, 31, OPT_CENTER | 8, 42)
    ));
  WIDGET(GD.cmd_progress(10, 136, 460, 10, 0, 25333, 65535));
  WIDGET(GD.cmd_scrollbar(30, 136, 420, 30, 0, 25333, 10000, 65535));
  WIDGET(GD.cmd_slider(30, 136, 420, 30, 0, 25333, 65535));
  WIDGET(GD.cmd_spinner(240, 136, 0, 1));
  WIDGET((
    GD.cmd_toggle(180,  20, 120, 31, 0, 0,     "yes\xffno"),
    GD.cmd_toggle(180, 120, 120, 31, 0, 32768, "yes\xffno"),
    GD.cmd_toggle(180, 220, 120, 31, 0, 65535, "yes\xffno")
  ));
  WIDGET(GD.cmd_button(240 - 100, 136 - 40, 200, 80, 31, 0, "1 UP"));


  GD.cmd_gradient(0, 0,   0x0060c0, //' dropshadow{
                  0, 271, 0xc06000);
  GD.ColorRGB(0x000000);
  GD.cmd_text(237, 139, 31, OPT_CENTER, "READY PLAYER ONE");
  GD.ColorRGB(0xffffff);
  GD.cmd_text(240, 136, 31, OPT_CENTER, "READY PLAYER ONE");
  GD.swap(); //' }dropshadow

  int x, y;
  GD.Clear();
  GD.Begin(LINES); //' linewidth{
  for (int i = 0; i < 136; i++) {
    GD.ColorRGB(GD.random(255), GD.random(255), GD.random(255));
    GD.LineWidth(i);
    GD.polar(x, y, i, i * 2500);
    GD.Vertex2ii(240 + x, 136 + y);
  } //' }linewidth
  GD.swap();

  GD.Clear();
  GD.Begin(POINTS); //' pointsize{
  for (int i = 0; i < 136; i++) {
    GD.ColorRGB(GD.random(255), GD.random(255), GD.random(255));
    GD.PointSize(i);
    GD.polar(x, y, i, i * 2500);
    GD.Vertex2ii(240 + x, 136 + y);
  } //' }pointsize
  GD.swap();

  GD.Clear();
  GD.Begin(RECTS); //' colorrgb{
  GD.ColorRGB(255, 128, 30);    // orange
  GD.Vertex2ii(10, 10); GD.Vertex2ii(470, 130); 
  GD.ColorRGB(0x4cc417);        // apple green
  GD.Vertex2ii(10, 140); GD.Vertex2ii(470, 260); //' }colorrgb
  GD.swap();

  GD.Clear();
  GD.Begin(POINTS); //' colora{
  GD.PointSize(12 * 16);
  for (int i = 0; i < 255; i += 5) {
    GD.ColorA(i);
    GD.Vertex2ii(2 * i, 136 + GD.rsin(120, i << 8));
  } //' }colora
  GD.swap();

  GD.ClearColorRGB(0x008080);         // teal //' clearcolorrgb{
  GD.Clear();
  GD.ScissorSize(100, 200);
  GD.ScissorXY(10, 20);
  GD.ClearColorRGB(0xf8, 0x80, 0x17); // orange
  GD.Clear(); //' }clearcolorrgb
  GD.swap();

  GD.Clear();
  GD.Begin(POINTS); //' blendfunc{
  GD.ColorRGB(0xf88017);
  GD.PointSize(80 * 16);
  GD.BlendFunc(SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
  GD.Vertex2ii(150, 76); GD.Vertex2ii(150, 196); 
  GD.BlendFunc(SRC_ALPHA, ONE);
  GD.Vertex2ii(330, 76); GD.Vertex2ii(330, 196); 
  //' }blendfunc
  GD.swap();

  LOAD_ASSETS();

  GD.Clear();
  GD.Begin(BITMAPS); //' alphafunc{
  GD.Vertex2ii(110, 6);             // Top left: ALWAYS
  GD.AlphaFunc(EQUAL, 255);         // Top right: (A == 255)
  GD.Vertex2ii(240, 6);
  GD.AlphaFunc(LESS, 160);          // Bottom left: (A < 160)
  GD.Vertex2ii(110, 136);
  GD.AlphaFunc(GEQUAL, 160);        // Bottom right: (A >= 160)
  GD.Vertex2ii(240, 136); //' }alphafunc
  GD.swap();

  GD.Clear();
  GD.PointSize(135 * 16); //' colormask{
  GD.Begin(POINTS); //' colormask{
  GD.ColorMask(1, 0, 0, 0); // red only
  GD.Vertex2ii(240 - 100, 136);
  GD.ColorMask(0, 1, 0, 0); // green only
  GD.Vertex2ii(240, 136);
  GD.ColorMask(0, 0, 1, 0); // blue only
  GD.Vertex2ii(240 + 100, 136); //' }colormask
  GD.swap();

  GD.ClearColorRGB(0x003000);
  GD.Clear();
  GD.BitmapHandle(CARDS_HANDLE);  // CARDS has 53 cells, 0-52 //' cell{
  GD.Begin(BITMAPS);
  for (int i = 1; i <= 52; i++) {
    GD.Cell(i);
    GD.Vertex2f(GD.random(16 * 480), GD.random(16 * 272));
  } //' }cell
  GD.swap();

  GD.ClearColorRGB(0x0000ff); // Clear color to blue //' clear{
  GD.ClearStencil(0x80);      // Clear stencil to 0x80
  GD.ClearTag(100);           // Clear tag to 100
  GD.Clear(1, 1, 1);          // Go! //' }clear
  GD.swap();

  GD.Clear();
  GD.cmd_text(240,  64, 31, OPT_CENTER, "WHITE"); //' context{
  GD.SaveContext();
  GD.ColorRGB(0xff0000);
  GD.cmd_text(240, 128, 31, OPT_CENTER, "RED");
  GD.RestoreContext();
  GD.cmd_text(240, 196, 31, OPT_CENTER, "WHITE AGAIN"); //' }context
  GD.swap();

  GD.Clear();
  GD.ScissorSize(400, 100); //' scissor{
  GD.ScissorXY(35, 36);
  GD.ClearColorRGB(0x008080); GD.Clear();
  GD.cmd_text(240, 136, 31, OPT_CENTER, "Scissor Example");
  GD.ScissorXY(45, 140);
  GD.ClearColorRGB(0xf88017); GD.Clear();
  GD.cmd_text(240, 136, 31, OPT_CENTER, "Scissor Example"); //' }scissor
  GD.swap();

  GD.Clear();
  GD.StencilOp(INCR, INCR); // incrementing stencil //' stencil{
  GD.PointSize(135 * 16);
  GD.Begin(POINTS);         // Draw three white circles
  GD.Vertex2ii(240 - 100, 136);
  GD.Vertex2ii(240, 136);
  GD.Vertex2ii(240 + 100, 136);
  GD.ColorRGB(0xff0000);    // Draw pixels with stencil==2 red
  GD.StencilFunc(EQUAL, 2, 255);
  GD.Begin(RECTS);          // Visit every pixel on the screen
  GD.Vertex2ii(0,0); GD.Vertex2ii(480,272); //' }stencil
  GD.swap();

  GD.Clear();   //' rects{
  GD.Begin(RECTS);

  GD.Vertex2ii(30, 30);
  GD.Vertex2ii(450, 50);

  GD.LineWidth(10 * 16);    // corner radius 10.0 pixels
  GD.Vertex2ii(30, 120);
  GD.Vertex2ii(450, 140);

  GD.LineWidth(20 * 16);    // corner radius 20.0 pixels
  GD.Vertex2ii(30, 220);
  GD.Vertex2ii(450, 230);

  GD.swap();    //' }rects

  GD.Clear();   
  GD.Begin(POINTS);  // draw 50-pixel wide green circles //' blend1{
  GD.ColorRGB(20, 91, 71);
  GD.PointSize(50 * 16);

  for (int x = 100; x <= 380; x += 40)
    GD.Vertex2ii(x, 72);

  GD.BlendFunc(SRC_ALPHA, ONE);   // additive blending
  for (int x = 100; x <= 380; x += 40)
    GD.Vertex2ii(x, 200); //' }blend1

  GD.swap();    

  GD.cmd_loadimage(0, 0);
  GD.load("tree.jpg");

  GD.Begin(BITMAPS);
  GD.ColorRGB(0xc0c0c0);
  GD.Vertex2ii(0, 0);
  GD.ColorRGB(0xffffff);

  GD.Begin(POINTS);     //' comp1{
  GD.PointSize(16 * 120); // White outer circle
  GD.Vertex2ii(136, 136);
  GD.ColorRGB(0x000000);
  GD.PointSize(16 * 110); // Black inner circle
  GD.Vertex2ii(136, 136);

  GD.ColorRGB(0xffffff);
  GD.cmd_clock(136, 136, 130,
               OPT_NOTICKS | OPT_NOBACK, 8, 41, 39, 0); //' }comp1
  GD.swap();

  GD.ColorRGB(0xc0c0c0);
  GD.Clear();             // now alpha is all zeroes //' comp2{ 
  GD.ColorMask(1,1,1,0);  // draw tree, but leave alpha zero
  GD.Begin(BITMAPS);          
  GD.Vertex2ii(0, 0);

  GD.ColorMask(0,0,0,1);      //' comp2a{
  GD.BlendFunc(ONE, ONE_MINUS_SRC_ALPHA); //' }comp2a
  GD.Begin(POINTS); //' comp2b{
  GD.PointSize(16 * 120); // outer circle
  GD.Vertex2ii(136, 136); //' }comp2b
  GD.BlendFunc(ZERO, ONE_MINUS_SRC_ALPHA); //' comp2c{
  GD.PointSize(16 * 110); // inner circle
  GD.Vertex2ii(136, 136); //' }comp2c
  GD.BlendFunc(ONE, ONE_MINUS_SRC_ALPHA); //' comp2d{
  GD.cmd_clock(136, 136, 130,
               OPT_NOTICKS | OPT_NOBACK, 8, 41, 39, 0); //' }comp2d

  GD.ColorMask(1,1,1,0); //' comp2e{
  GD.BlendFunc(DST_ALPHA, ONE);
  GD.ColorRGB(0x808080);
  GD.Begin(RECTS);        // Visit every pixel on the screen
  GD.Vertex2ii(0,0);
  GD.Vertex2ii(480,272); //' }comp2 }comp2e
  GD.swap();

  GD.cmd_loadimage(0, 0);
  GD.load("healsky3.jpg");
  GD.ClearColorRGB(0x602010); //' rot1{
  GD.Clear();             
  GD.Begin(BITMAPS);

  GD.Vertex2ii(10, 72);

  GD.cmd_rotate(DEGREES(22.5));
  GD.cmd_setmatrix();
  GD.Vertex2ii(176, 72);

  GD.cmd_rotate(DEGREES(22.5));
  GD.cmd_setmatrix();
  GD.Vertex2ii(342, 72);

  GD.swap();  //' }rot1

  GD.ClearColorRGB(0x602010);
  GD.Clear();             //' rot2{
  GD.BlendFunc(SRC_ALPHA, ZERO);
  GD.Begin(BITMAPS);

  GD.Vertex2ii(10, 72);

  GD.cmd_rotate(DEGREES(22.5));
  GD.cmd_setmatrix();
  GD.Vertex2ii(176, 72);

  GD.cmd_rotate(DEGREES(22.5));
  GD.cmd_setmatrix();
  GD.Vertex2ii(342, 72);

  GD.swap();  //' }rot2

  GD.ClearColorRGB(0x602010); //' rot3{
  GD.Clear();             
  GD.BlendFunc(SRC_ALPHA, ZERO);
  GD.Begin(BITMAPS);

  GD.Vertex2ii(10, 72);

  rotate_64_64(DEGREES(22.5));
  GD.cmd_setmatrix();
  GD.Vertex2ii(176, 72);

  rotate_64_64(DEGREES(22.5));
  GD.cmd_setmatrix();
  GD.Vertex2ii(342, 72);

  GD.swap();   //' }rot3

  GD.BitmapSize(NEAREST, BORDER, BORDER, 128, 128);
  GD.ClearColorRGB(0x602010); //' zoom{
  GD.Clear();             
  GD.Begin(BITMAPS);

  GD.cmd_scale(F16(2), F16(2));
  GD.cmd_setmatrix();

  GD.Vertex2ii(10, 8);
  GD.BitmapSize(NEAREST, BORDER, BORDER, 256, 256);
  GD.Vertex2ii(200, 8);

  GD.swap();   //' }zoom

  LOAD_ASSETS();

  GD.Clear();

  GD.LineWidth(20);
  GD.PointSize(20);

  GD.Begin(BITMAPS);
  zigzag("BITMAPS", 48);

  GD.Begin(POINTS);
  zigzag("POINTS",  48 + 1 * 96);

  GD.Begin(LINES);
  zigzag("LINES",  48 + 2 * 96);

  GD.Begin(LINE_STRIP);
  zigzag("LINE_STRIP",  48 + 3 * 96);

  GD.Begin(RECTS);
  zigzag("RECTS",  48 + 4 * 96);

  GD.swap();
  
  for (int j = 0; j < 2; j++) {
    uint32_t rgb[2] = {0x00ff00, 0x60c030};
    uint32_t color = rgb[j];
    GD.ColorRGB(color);
    GD.seed(0x947a);  

    GD.Clear(); //' additive{
    GD.BlendFunc(SRC_ALPHA, ONE);
    GD.PointSize(8 * 16);
    GD.Begin(POINTS);
    for (int i = 0; i < 1000; i++)
      GD.Vertex2ii(20 + GD.random(440), 20 + GD.random(232));
    GD.swap(); //' }additive
  }

  GD.Clear();   //' polygon{
  GD.ColorRGB(0xf3d417);
  Poly po;
  po.begin();
  po.v(16 * 154, 16 * 262);
  po.v(16 * 256, 16 * 182);
  po.v(16 * 312, 16 * 262);
  po.v(16 * 240, 16 *  10);
  po.draw();
  GD.swap();  //' }polygon

  GD.cmd_gradient(0, 0, 0x82670e, 272, 272, 0xa78512);
  GD.SaveContext();
  GD.ColorRGB(0xf3d417);
  po.begin();
  po.v(16 * 154, 16 * 262);
  po.v(16 * 256, 16 * 182);
  po.v(16 * 312, 16 * 262);
  po.v(16 * 240, 16 *  10);
  po.draw();
  GD.RestoreContext();
  GD.ColorRGB(0x000000);  //' polygon2{
  GD.LineWidth(3 * 16);
  po.outline();    //' }polygon2
  GD.swap();

  GD.Clear();
  GD.BitmapLayout(L8, 1, 1); //' 1x1{
  GD.BlendFunc(ONE, ZERO);
  GD.BitmapSize(NEAREST, REPEAT, REPEAT, 60, 30);

  GD.Begin(BITMAPS);
  GD.Vertex2ii(10, 10); // each vertex draws a 60X30 rectangle
  GD.Vertex2ii(110, 110);
  GD.Vertex2ii(210, 210);

  GD.swap(); //' }1x1

  GD.BitmapHandle(SPECTRUM_HANDLE); //' 1d{
  GD.BitmapSize(NEAREST, REPEAT, REPEAT, 512, 512);
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0, SPECTRUM_HANDLE);  //' }1d
  GD.swap();

  GD.cmd_rotate(DEGREES(73));   //' 1d-twist{
  GD.cmd_scale(F16(0.3), F16(1));
  GD.cmd_setmatrix(); //' }1d-twist
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0, SPECTRUM_HANDLE);  
  GD.swap();

  GD.ClearColorRGB(0x00324d);   //' 2bit{
  GD.Clear();

  GD.ColorMask(0, 0, 0, 1);
  GD.Begin(BITMAPS);

  GD.BlendFunc(ONE, ONE);
  GD.ColorA(0xaa);  // draw bit 1 into A
  GD.Vertex2ii(240 - ABE_WIDTH / 2, 0, ABE_HANDLE, 1);
  GD.ColorA(0x55);  // draw bit 0 into A
  GD.Vertex2ii(240 - ABE_WIDTH / 2, 0, ABE_HANDLE, 0);

  // Now draw the same pixels, controlled by DST_ALPHA
  GD.ColorMask(1, 1, 1, 1);
  GD.ColorRGB(0xfce4a8);
  GD.BlendFunc(DST_ALPHA, ONE_MINUS_DST_ALPHA);
  GD.Vertex2ii(240 - ABE_WIDTH / 2, 0, ABE_HANDLE, 1);

  GD.swap(); //' }2bit

  EX2_LOAD_ASSETS();

  GD.cmd_gradient(0, 0,   0x8080c0, //' dropshadow2{
                  0, 271, 0xc0d080);
  GD.ColorRGB(0x000000);
  GD.ColorA(128);
  GD.Begin(BITMAPS);
  GD.Vertex2ii(240 - 12 - EX2_GAMEDUINO_WIDTH / 2,
               136 + 12 - EX2_GAMEDUINO_HEIGHT / 2,
               EX2_GAMEDUINO_HANDLE);
  GD.ColorRGB(0xffffff);
  GD.ColorA(0xff);
  GD.Vertex2ii(240 - EX2_GAMEDUINO_WIDTH / 2,
               136 - EX2_GAMEDUINO_HEIGHT / 2,
               EX2_GAMEDUINO_HANDLE);
  GD.swap(); //' }dropshadow2

  {
    int w = 65;
    GD.Begin(LINES);
    GD.LineWidth(28 * 16);
    GD.Vertex2ii(240 - w, 136, 0, 0);
    GD.Vertex2ii(240 + w, 136, 0, 0);
    GD.ColorRGB(0x000480);
    GD.LineWidth(24 * 16);
    GD.Vertex2ii(240 - w, 136, 0, 0);
    GD.Vertex2ii(240 + w, 136, 0, 0);
    GD.ColorRGB(0xffffff);
    GD.cmd_text(240, 132, 31, OPT_CENTER, "upvote");
    GD.swap();
  }

  int WALK_HANDLE = EX2_WALK_HANDLE;
  GD.ClearColorRGB(0x80c2dd);
  GD.Clear();
  GD.Begin(BITMAPS);  //' cell{
  GD.Vertex2ii(  0, 10, WALK_HANDLE, 0);
  GD.Vertex2ii( 50, 10, WALK_HANDLE, 1);
  GD.Vertex2ii(100, 10, WALK_HANDLE, 2);
  GD.Vertex2ii(150, 10, WALK_HANDLE, 3);
  GD.Vertex2ii(200, 10, WALK_HANDLE, 4);
  GD.Vertex2ii(250, 10, WALK_HANDLE, 5);
  GD.Vertex2ii(300, 10, WALK_HANDLE, 6);
  GD.Vertex2ii(350, 10, WALK_HANDLE, 7); //' }cell
  GD.swap();

  int STRIPE_HANDLE = EX2_STRIPE_HANDLE;
  GD.BitmapHandle(STRIPE_HANDLE);    //' stripe1{
  GD.BitmapSize(NEAREST, REPEAT, REPEAT, 480, 272);
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0, STRIPE_HANDLE);  //' }stripe1
  GD.swap();      

  GD.BitmapHandle(STRIPE_HANDLE);    //' stripe2{
  GD.BitmapSize(NEAREST, REPEAT, REPEAT, 480, 272);

  GD.ClearColorRGB(0x103000);
  GD.Clear();

  GD.ColorMask(0, 0, 0, 1);   // write A only
  GD.BlendFunc(ONE, ONE);
  GD.cmd_text(240, 136, 31, OPT_CENTER,
              "STRIPES ARE IN, BABY!");

  GD.ColorMask(1, 1, 1, 0);   // write R,G,B only
  GD.BlendFunc(DST_ALPHA, ONE_MINUS_DST_ALPHA);
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0, STRIPE_HANDLE);  //' }stripe2

  GD.swap();

  GD.ClearColorRGB(0x80c2dd);
  GD.Clear();
  GD.Begin(BITMAPS);  //' mirror{
  GD.Vertex2ii(  0, 10, WALK_HANDLE, 0);
  GD.cmd_translate(F16(16), F16(0));
  GD.cmd_scale(F16(-1), F16(1));
  GD.cmd_translate(F16(-16), F16(0));
  GD.cmd_setmatrix();
  GD.Vertex2ii( 30, 10, WALK_HANDLE, 0);
  GD.swap();  //' }mirror

  if (0) {
  GD.play(PIANO, 60);   //' waitnote{
  while (GD.rd(REG_PLAY) != 0)
    ; //' }waitnote
  }

  GD.cmd_loadimage(0, 0);
  GD.load("healsky3.jpg");
  GD.BlendFunc(SRC_ALPHA, ZERO);
  GD.ClearColorRGB(0x602010); //' scale1{
  GD.Clear();             
  GD.Begin(BITMAPS);

  GD.Vertex2ii(10, 72);

  scale_64_64(F16(2.0), F16(2.0));
  GD.cmd_setmatrix();
  GD.Vertex2ii(176, 72);

  GD.cmd_loadidentity();
  scale_64_64(F16(0.4), F16(0.4));
  GD.cmd_setmatrix();
  GD.Vertex2ii(342, 72);

  GD.swap();  //' }scale1

#define draw_left_circle()   (GD.Begin(POINTS), GD.PointSize(16 * 60), GD.Vertex2ii(200, 136))
#define draw_right_circle()  (GD.Begin(POINTS), GD.PointSize(16 * 70), GD.Vertex2ii(280, 136))
              
#define PAINT_ALPHA()  GD.BlendFunc(ONE, ONE_MINUS_SRC_ALPHA) //' alpha1{
#define CLEAR_ALPHA()  GD.BlendFunc(ZERO, ONE_MINUS_SRC_ALPHA)

  GD.ClearColorA(0x80);
  GD.Clear();

  PAINT_ALPHA();
  draw_left_circle();

  CLEAR_ALPHA();
  draw_right_circle();       //' }alpha1

  GD.BlendFunc(DST_ALPHA, ZERO);
  GD.Begin(RECTS);
  GD.Vertex2ii(0, 0);
  GD.Vertex2ii(480, 272);

  GD.swap();

  GD.cmd_loadimage(0, 0);
  GD.load("healsky3.jpg");
  GD.cmd_gradient(0, 0, 0x403040,
                  0, 272, 0xd8bfd8);
  GD.Begin(BITMAPS);    //' corners{

  GD.Vertex2ii( 52, 50);                    // left bitmap

  GD.ColorMask(0, 0, 0, 1);                 // only draw A
  GD.Clear();
  int r = 20;                               // corner radius
  GD.LineWidth(16 * r);
  GD.Begin(RECTS);
  GD.Vertex2ii(300 + r,       50 + r);       // top-left
  GD.Vertex2ii(300 + 127 - r, 50 + 127 - r); // bottom-right

  GD.ColorMask(1, 1, 1, 0);                 // draw bitmap
  GD.BlendFunc(DST_ALPHA, ONE_MINUS_DST_ALPHA);
  GD.Begin(BITMAPS);
  GD.Vertex2ii( 300, 50); //' }corners

  GD.swap();

  GD.Clear();
  GD.LineWidth(16 * r);
  GD.Begin(RECTS);
  GD.Vertex2ii(300 + r,       50 + r);       // top-left
  GD.Vertex2ii(300 + 127 - r, 50 + 127 - r); // bottom-right
  GD.swap();

  GD.ColorRGB(0x808080);
  GD.cmd_loadimage(0, 0);     //' keypad{
  GD.load("tree.jpg");
  GD.Clear();
  GD.ColorMask(1, 1, 1, 0);
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0);

  GD.ColorMask(0, 0, 0, 1);
  int x0 = 160, x1 = 240, x2 = 320;
  int y0 =  56, y1 = 136, y2 = 216;
  button(x0, y0, 1); button(x1, y0, 2); button(x2, y0, 3);
  button(x0, y1, 4); button(x1, y1, 5); button(x2, y1, 6);
  button(x0, y2, 7); button(x1, y2, 8); button(x2, y2, 9);

  GD.ColorMask(1, 1, 1, 1);
  GD.ColorRGB(0xffffff);
  GD.BlendFunc(DST_ALPHA, ONE_MINUS_DST_ALPHA);
  GD.Begin(RECTS);
  GD.Vertex2ii(0, 0); GD.Vertex2ii(480, 272);  //' }keypad

  GD.swap();

  GD.cmd_memwrite(0, 8);  //' handmade{
  static const PROGMEM uint8_t picture[] = {
    0b01110111,
    0b11100010,
    0b11000001,
    0b10100011,
    0b01110111,
    0b00111010,
    0b00011100,
    0b00101110,
  };
  GD.copy(picture, 8);

  GD.BitmapSource(0);
  GD.BitmapSize(NEAREST, REPEAT, REPEAT, 480, 272);
  GD.BitmapLayout(L1, 1, 8);

  GD.Clear();
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0);  //' }handmade
  GD.swap();

  GD.ColorRGB(176, 224, 230);
  GD.Clear();
  GD.PointSize(32);
  GD.Begin(POINTS);
  for (int i = 0; i < 16; i++)
    GD.Vertex2f(64 + 16 * i * 7, 64 + i);
  GD.swap();

  GD.ClearColorRGB(0x103000);   //' replace{
  GD.Clear();
  GD.BlendFunc(SRC_ALPHA, ZERO);  // replace operation
  GD.cmd_text(240, 136, 31, OPT_CENTER, "Hello world");
  GD.swap();                    //' }replace

  GD.cmd_loadimage(0, 0);
  GD.load("healsky3.jpg");
  GD.finish();
}

static void button(int x, int y, byte label)  //' button{
{
  int sz = 18;                    // button size in pixels

  GD.Tag(label);
  PAINT_ALPHA();
  GD.Begin(RECTS);
  GD.LineWidth(16 * 20);
  GD.Vertex2ii(x - sz, y - sz);
  GD.Vertex2ii(x + sz, y + sz);

  CLEAR_ALPHA();
  GD.ColorA(200);
  /* JCB
  GD.ColorA(200);
  JCB */
  GD.LineWidth(16 * 15);
  GD.Vertex2ii(x - sz, y - sz);
  GD.Vertex2ii(x + sz, y + sz);

  GD.ColorA(0xff);
  PAINT_ALPHA();
  GD.cmd_number(x, y, 31, OPT_CENTER, label);
} //' }button
