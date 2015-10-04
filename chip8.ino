#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include "chip8_assets.h"

#define ZOOM 5                  // Pixel zoom factor
#define SCREENLOC   0x3f000     // Address of 64x32 screen in FT800 RAM

/*
    An interpreted Chip-8 emulator.
    Copyright (C) 2015, Corey Prophitt.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
    This code was adapted from Corey Prophitt's Chip-8 emulator
    by James Bowman.
*/

struct chip8 {
    uint16_t pc;                /* program counter */
    uint16_t i;                 /* address pointer */
    uint8_t sp;                 /* stack pointer   */
    uint8_t st;                 /* sound timer     */
    uint8_t dt;                 /* delay timer     */

    uint8_t v[16];              /* registers V0-VF */

    uint8_t keys[16];

    uint16_t s[16];
};

static byte game = 0;           // Game number
static struct chip8 c8;

static void chip8_clrs()
{
  GD.cmd_memset(SCREENLOC, 0, 64 * 32);
  GD.finish();
}

#define X(x)   ((x & 0x0F00) >> 8)
#define Y(x)   ((x & 0x00F0) >> 4)
#define N(x)   (x & 0x000F)
#define NN(x)  (x & 0x00FF)
#define NNN(x) (x & 0x0FFF)

void chip8_init()
{
    /* clear memory, registers, stack, keys buffers */
    memset(c8.v, 0, sizeof c8.v);
    memset(c8.s, 0, sizeof c8.s);
    memset(c8.keys, 0, sizeof c8.keys);

    // chip8_load_font();
    chip8_clrs();

    /* initialize program counter, stack pointer and I register */
    c8.pc = 0x200;
    c8.sp = 0;
    c8.i = 0;

    /* reset the sound timer and delay timer */
    c8.st = 0;
    c8.dt = 0;
}

static uint8_t c8fetch(uint16_t addr)
{
  uint8_t v = GD.rd(((uint32_t)game << 12) + addr);
  return v;
}

static void c8store(uint16_t addr, uint8_t v)
{
  return GD.wr(((uint32_t)game << 12) + addr, v);
}

static void ic8_error(char *msg, uint16_t opcode)
{
  GD.Clear();
  GD.cmd_text(240, 136, 27, OPT_CENTER, "fail");
  GD.cmd_number(240, 200, 27, OPT_CENTER, opcode);
  GD.swap();
  for (;;)
    ;
}

static int chip8_tick()
{
    int finish = 0;
    uint16_t opcode = (c8fetch(c8.pc) << 8) | (c8fetch(c8.pc + 1));

    // Serial.print("pc="); Serial.print(c8.pc, HEX);
    // Serial.print(" opcode="); Serial.print(opcode, HEX);
    // Serial.println();

    /* convert this to a jump table? */
    switch (opcode & 0xF000) {
        /* double branch */
        case 0x0000:
            switch (opcode & 0x00FF) {
                /* 00E0: Clears the screen */
                case 0x00E0:
                    chip8_clrs();
                    c8.pc += 2;
                    break;

                /* 00EE: Returns from a subroutine */
                case 0x00EE:
                    c8.sp -= 1;
                    c8.pc = c8.s[c8.sp];
                    c8.pc += 2;
                    break;

                default:
                    ic8_error("Unsupported instruction (%X).\n", opcode);
                    c8.pc += 2;
                    break;
            }
            break;

        /* 1NNN: Jumps to address NNN */
        case 0x1000:
            c8.pc = NNN(opcode);
            break;

        /* 2NNN: Calls subroutine at NNN */
        case 0x2000:
            c8.s[c8.sp] = c8.pc;
            c8.sp += 1;
            c8.pc = NNN(opcode);
            break;

        /* 3XNN: Skips the next instruction if VX equals NN */
        case 0x3000:
            c8.pc += c8.v[X(opcode)] == NN(opcode) ? 4 : 2;
            break;

        /* 4XNN: Skips the next instruction if VX doesn't equal NN */
        case 0x4000:
            c8.pc += c8.v[X(opcode)] != NN(opcode) ? 4 : 2;
            break;

        /* 5XY0: Skips the next instruction if VX equals VY */
        case 0x5000:
            c8.pc += c8.v[X(opcode)] == c8.v[Y(opcode)] ? 4 : 2;
            break;

        /* 6XNN: Sets VX to NN */
        case 0x6000:
            c8.v[X(opcode)] = NN(opcode);
            c8.pc += 2;
            break;

        /* 7XNN: Adds NN to VX */
        case 0x7000:
            c8.v[X(opcode)] += NN(opcode);
            c8.pc += 2;
            break;

        /* double branch */
        case 0x8000:
            switch (opcode & 0x000F) {
                /* 8XY0: Sets VX to the value of VY */
                case 0x0000:
                    c8.v[X(opcode)] = c8.v[Y(opcode)];
                    c8.pc += 2;
                    break;

                /* 8XY1: Sets VX to VX or VY */
                case 0x0001:
                    c8.v[X(opcode)] |= c8.v[Y(opcode)];
                    c8.pc += 2;
                    break;

                /* 8XY2: Sets VX to VX and VY */
                case 0x0002:
                    c8.v[X(opcode)] &= c8.v[Y(opcode)];
                    c8.pc += 2;
                    break;

                /* 8XY3: Sets VX to VX xor VY */
                case 0x0003:
                    c8.v[X(opcode)] ^= c8.v[Y(opcode)];
                    c8.pc += 2;
                    break;

                /*
                 * 8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to
                 * 0 when there isn't
                 * */
                case 0x0004:
                    {
                        int vx = c8.v[X(opcode)];
                        int vy = c8.v[Y(opcode)];

                        int results = vx + vy;

                        c8.v[0xF] = results > 255 ? 1 : 0;

                        c8.v[X(opcode)] = results & 0xFF;

                        c8.pc += 2;
                    }
                    break;

                /*
                 * 8XY5: VY is subtracted from VX. VF is set to 0 when there's a
                 * borrow, and 1 when there isn't
                 * */
                case 0x0005:
                    {
                        int vx = c8.v[X(opcode)];
                        int vy = c8.v[Y(opcode)];

                        c8.v[0xF] = vy > vx ? 0 : 1;

                        c8.v[X(opcode)] = vx - vy;

                        c8.pc += 2;
                    }
                    break;

                /*
                 * 8XY6: Shifts VX right by one. VF is set to the value of the
                 * least significant bit of VX before the shift
                 * */
                case 0x0006:
                    c8.v[0xF] = c8.v[X(opcode)] & 0x01;
                    c8.v[X(opcode)] >>= 1;
                    c8.pc += 2;
                    break;

                /*
                 * 8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a
                 * borrow, and 1 when there isn't
                 * */
                case 0x0007:
                    {
                        int vx = c8.v[X(opcode)];
                        int vy = c8.v[Y(opcode)];

                        c8.v[0xF] = vx > vy ? 0 : 1;

                        c8.v[X(opcode)] = vy - vx;

                        c8.pc += 2;
                    }
                    break;

                /*
                 * 8XYE: Shifts VX left by one. VF is set to the value of the
                 * most significant bit of VX before the shift
                 * */
                case 0x000E:
                    c8.v[0xF] = (c8.v[X(opcode)] & 0x80) >> 7;
                    c8.v[X(opcode)] <<= 1;
                    c8.pc += 2;
                    break;

                default:
                    ic8_error("Unsupported instruction (%X).\n", opcode);
                    c8.pc += 2;
                    break;
            }
            break;

        /* 9XY0: Skips the next instruction if VX doesn't equal VY */
        case 0x9000:
            c8.pc += c8.v[X(opcode)] != c8.v[Y(opcode)] ? 4 : 2;
            break;

        /* ANNN: Sets I to the address NNN */
        case 0xA000:
            c8.i = NNN(opcode);
            c8.pc += 2;
            break;

        /* BNNN: Jumps to the address NNN plus V0 */
        case 0xB000:
            c8.pc = NNN(opcode) + c8.v[0x00];
            break;

        /* CXNN: Sets VX to a random number, masked by NN */
        case 0xC000:
            c8.v[X(opcode)] = GD.random(256) & NN(opcode);
            c8.pc += 2;
            break;

        /*
         * DXYN: Sprites stored in memory at location in index register (I),
         * maximum 8bits wide. Wraps around the screen. If when drawn,
         * clears a pixel, register VF is set to 1 otherwise it is zero.
         * All drawing is XOR drawing (i.e. it toggles the screen pixels)
         * */
        case 0xD000:
            {
                uint8_t vx = c8.v[X(opcode)];
                uint8_t vy = c8.v[Y(opcode)];

                c8.v[0xF] = 0;

                uint8_t pixel = 0;

                int w = 0;
                int h = 0;

                for (uint8_t y = 0; y < N(opcode); ++y) {
                    pixel = c8fetch(c8.i + y);
                    for (uint8_t x = 0; x < 8; ++x) {
                        if ((pixel & (0x80 >> x)) != 0)  {
                            uint8_t old;

                            /* clamp cooridnates, don't draw off the screen
                             *  - Needed for VERS
                             *  */
                            w = (vx + x) > 64 ? 64 : vx + x;
                            h = (vy + y) > 32 ? 32 : vy + y;

                            old = GD.rd(SCREENLOC + (w + (h * 64)));
                            if (old == 255)
                                c8.v[0xF] = 1;

                            GD.wr(SCREENLOC + w + (h * 64), old ^ 255);
                        }
                    }
                }

                c8.pc += 2;
                finish = 1;
            }
            break;

        /* double branch */
        case 0xE000:
            switch (opcode & 0x00FF) {
                /* EX9E: Skips the next instruction if the key stored in VX is pressed */
                case 0x9E:
                    c8.pc += c8.keys[c8.v[X(opcode)]] ? 4 : 2;
                    break;

                /* EXA1: Skips the next instruction if the key stored in VX isn't pressed */
                case 0xA1:
                    c8.pc += c8.keys[c8.v[X(opcode)]] ? 2 : 4;
                    break;

                default:
                    ic8_error("Unsupported instruction (%X).\n", opcode);
                    c8.pc += 2;
                    break;
            }
            break;

        /* double branch */
        case 0xF000:
            switch (opcode & 0x00FF) {
                /* FX07: Sets VX to the value of the delay timer */
                case 0x0007:
                    c8.v[X(opcode)] = c8.dt;
                    c8.pc += 2;
                    break;

                /* FX0A: A key press is awaited, and then stored in VX */
                case 0x000A:

                    GD.get_inputs();
                    {

                      uint8_t i;
                      for (i = 0; i < 16; i++) {
                        if (c8.keys[i]) {
                          c8.v[X(opcode)] = i;
                          c8.pc += 2;

                          finish = 2;

                          break;
                        }
                      }
                    }
                    break;

                /* FX15: Sets the delay timer to VX */
                case 0x0015:
                    c8.dt = c8.v[X(opcode)];
                    c8.pc += 2;
                    break;

                /* FX18: Sets the sound timer to VX */
                case 0x0018:
                    c8.st = c8.v[X(opcode)];
                    c8.pc += 2;
                    break;

                /* FX1E: Adds VX to I */
                case 0x001E:
                    /* Spaceflight 2019 feature, wrap player around world */
                    c8.v[0xF] = 0;

                    if ((c8.i + c8.v[X(opcode)]) > 4095) {
                        c8.v[0xF] = 1;
                    }

                    c8.i += c8.v[X(opcode)];
                    c8.pc += 2;
                    break;

                /*
                 * FX29: Sets I to the location of the sprite for the character
                 * in VX. Characters 0-F (in hexadecimal) are represented by a
                 * 4x5 font.
                 * */
                case 0x0029:
                    c8.i = c8.v[X(opcode)] * 5;
                    c8.pc += 2;
                    break;

                /*
                 * FX33: Stores the Binary-coded decimal representation of VX, with
                 * the most significant of three digits at the address in I,
                 * the middle digit at I plus 1, and the least significant digit
                 * at I plus 2. (In other words, take the decimal representation
                 * of VX, place the hundreds digit in memory at location in I,
                 * the tens digit at location I+1, and the ones digit at location
                 * I+2.)
                 * */
                case 0x0033:
                    c8store(c8.i, c8.v[X(opcode)] / 100);
                    c8store(c8.i + 1, (c8.v[X(opcode)] % 100) / 10);
                    c8store(c8.i + 2, c8.v[X(opcode)] % 10);

                    c8.pc += 2;
                    break;

                /* FX55: Stores V0 to VX in memory starting at address I */
                case 0x0055:
                    for (int i = 0; i <= X(opcode); i++) {
                        c8store(c8.i + i, c8.v[i]);
                    }
                    c8.pc += 2;
                    break;

                /* FX65: Fills V0 to VX with values from memory starting at address I */
                case 0x0065:
                    for (int i = 0; i <= X(opcode); i++) {
                        c8.v[i] = c8fetch(c8.i + i);
                    }
                    c8.pc += 2;
                    break;

                default:
                    ic8_error("Unsupported instruction (%X).\n", opcode);
                    c8.pc += 2;
                    break;
            }
            break;

        default:
            ic8_error("Unknown opcode 0x%X\n", opcode);
            c8.pc += 2;
    }
    return finish;
}

static void chip8_update_timers()
{
    if (c8.dt > 0) {
        c8.dt -= 1;
    }

    if (c8.st > 0) {
        GD.wr(REG_VOL_SOUND, 255);
        c8.st -= 1;
    } else {
        GD.wr(REG_VOL_SOUND, 0);
    }
}

static char title[48];
static char hint[128];

static void set_game(byte g)
{
  game = g;

  chip8_init();

  for (byte i = 0; i < 48; i++)
    title[i] = c8fetch(0xf00 + i);
  for (byte i = 0; i < 128; i++)
    hint[i] = c8fetch(0xf30 + i);
}

void setup()
{
  Serial.begin(1000000);

  GD.begin();

  LOAD_ASSETS();
  GD.finish();

  GD.play(SQUAREWAVE);    // continuous beep
  GD.wr(REG_VOL_SOUND, 0);

  GD.BitmapHandle(0);
  GD.BitmapSource(SCREENLOC);
  GD.BitmapSize(NEAREST, BORDER, BORDER, ZOOM * 64, ZOOM * 32);
  GD.BitmapLayout(L8, 64, 32);

  GD.BitmapHandle(1);
  GD.BitmapSource(0);
  GD.BitmapSize(NEAREST, BORDER, BORDER, 256, 15);
  GD.BitmapLayout(RGB332, 256, 16);

  set_game(0);
}

static uint16_t t;

void loop()
{
  GD.get_inputs();

  GD.ClearColorRGB(0x172580);
  GD.Clear();

  // Draw title and hint
  GD.PointSize(19 * 16);
  GD.Begin(POINTS);
  GD.ColorRGB(0x000000);
  GD.Vertex2ii(20, 20);
  GD.ColorRGB(0xffffff);
  GD.cmd_number(20, 20, 27, OPT_CENTER | 2, game);
  GD.cmd_text(42, 20, 27, OPT_CENTERY, title);
  GD.ColorRGB(0xd0d0d0);
  GD.cmd_text(42, 40, 26, OPT_CENTERY, hint);

  // Draw the - + buttons for game selection
  GD.ColorRGB(0x909070);
  GD.Begin(POINTS);
  GD.Vertex2ii(420, 20);
  GD.Vertex2ii(460, 20);
  GD.ColorRGB(0x000000);
  GD.Tag('-');
  GD.cmd_text(420, 20, 29, OPT_CENTER, "-");
  GD.Tag('+');
  GD.cmd_text(460, 20, 29, OPT_CENTER, "+");

  // Draw the hex keypad
  static uint8_t keys[] = {
    0x1, 0x2, 0x3, 0xc,
    0x4, 0x5, 0x6, 0xd,
    0x7, 0x8, 0x9, 0xe,
    0xa, 0x0, 0xb, 0xf,
  };
  for (byte i = 0; i < 16; i++) {
    char cc[2];
    byte h = keys[i];
    if (h < 10)
      cc[0] = '0' + h;
    else
      cc[0] = 'A' - 10 + h;
    cc[1] = 0;
    int x = 340 + 40 * (i % 4);
    int y = 80 + 40 * (i / 4);
    byte touch = GD.inputs.tag == (0x80 + h);
    if (touch)
      GD.ColorRGB(0xc04000);
    else
      GD.ColorRGB(0xc0c0c0);
    c8.keys[h] = touch;
    GD.Begin(POINTS);
    GD.Vertex2ii(x, y);
    GD.Tag(0x80 + h);
    GD.ColorRGB(0x202020);
    GD.cmd_text(x, y, 30, OPT_CENTER, cc);
  }

  GD.ColorRGB(0xffffff);
  GD.Begin(BITMAPS);
  GD.BlendFunc(SRC_ALPHA, ZERO);
  GD.Vertex2ii((480 - 256) / 2, 272 - 20, 1, game);

  GD.cmd_scale(F16(ZOOM), F16(ZOOM));
  GD.cmd_setmatrix();
  GD.Vertex2ii(0, 61, 0);
  GD.swap();
  GD.finish();

  for (byte i = (2000 / 60); i; i--) {
    int fincode = chip8_tick();
    if (fincode == 2) {
      do {
        GD.get_inputs();
      } while (GD.inputs.tag != 0);
    }
    if (fincode)
      break;
  }
  chip8_update_timers();

  static byte prev_tag;
  if (prev_tag == 0) {
    if (GD.inputs.tag == '+')
      set_game((game + 1) % NGAMES);
    if (GD.inputs.tag == '-')
      set_game((game ? game : NGAMES) - 1);
  }

  // if ((t % 240) == 239)
  //   set_game((game + 1) % NGAMES);
  // if ((t % 2) == 0)
  //   GD.dumpscreen(), delay(100);
  // t++;

  prev_tag = GD.inputs.tag;
}
