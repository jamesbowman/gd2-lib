import os
import sys
import time
import math
import array
import Image
import math
import pickle
import zlib

import gameduino2 as gd2

packlist = {
    "Chip-8 Demos/Maze [David Winter, 199x]"                          : "Demo. Random maze made from 4x4 bitmaps",
    "Chip-8 Demos/Particle Demo [zeroZshadow, 2008]"                  : "Demo. Some particles",
    "Chip-8 Demos/Sierpinski [Sergey Naydenov, 2010]"                 : "Demo. Renders the Sierpinski gasket",
    "Chip-8 Demos/Stars [Sergey Naydenov, 2010]"                      : "Demo. Twinking stars",
    "Chip-8 Demos/Trip8 Demo (2008) [Revival Studios]"                : "Demo: intro, 3D vectorballs, and 4 randomized dot-effects",
    "Chip-8 Demos/Zero Demo [zeroZshadow, 2007]"                      : "Demo. Four bouncing sprites ",
    "Chip-8 Games/Addition Problems [Paul C. Moews]"                  : "Addition fun. Enter the three digit sum",
    "Chip-8 Games/Astro Dodge [Revival Studios, 2008]"                : "Dodge the asteroids. 5 starts. 2,4,6,8 move",
    "Chip-8 Games/Blinky [Hans Christian Egeberg, 1991]"              : "Pacman",
    "Chip-8 Games/Bowling [Gooitzen van der Wal]"                     : "Bowling. Controls are complicated",
    "Chip-8 Games/Breakout [Carmelo Cortez, 1979]"                    : "You have six walls and 20 balls. 4 and 6 move the paddle",
    "Chip-8 Games/Brix [Andreas Gustafsson, 1990]"                    : "4 and 6 move the paddle",
    "Chip-8 Games/Cave"                                               : "F starts. 2,4,6,8 guide the explorer",
    "Chip-8 Games/Coin Flipping [Carmelo Cortez, 1978]"               : "Coin flipping simulator",
    "Chip-8 Games/Connect 4 [David Winter]"                           : "Two player game. 4,6 move, 5 to drop coin",
    "Chip-8 Games/Deflection [John Fort]"                             : "it's complicated",
    "Chip-8 Games/Figures"                                            : "4,6 move the descending number",
    "Chip-8 Games/Filter"                                             : "Catch the ball. 4,6 move",
    "Chip-8 Games/Guess [David Winter]"                               : "Think of a number 1-63. Press 5 if you see it, 2 if not",
    "Chip-8 Games/Hidden [David Winter, 1996]"                        : "Find pairs. 2,4,6,8 move, 5 reveals",
    "Chip-8 Games/Hi-Lo [Jef Winsor, 1978]"                           : "10 guesses to find a random number between 00 and 99",
    "Chip-8 Games/Kaleidoscope [Joseph Weisbecker, 1978]"             : "2,4,6,8 create a pattern, 0 repeats. Try 44444442220",
    "Chip-8 Games/Landing"                                            : "Clear your landing path. 8 drops a bomb",
    "Chip-8 Games/Lunar Lander (Udo Pernisz, 1979)"                   : "Lunar landing game",
    "Chip-8 Games/Merlin [David Winter]"                              : "Memory game. Repeat the sequence using keys 4,5,7,8",
    "Chip-8 Games/Missile [David Winter]"                             : "8 to shoot",
    "Chip-8 Games/Nim [Carmelo Cortez, 1978]"                         : "F to go first, B to go second. 1,2,3 removes counters",
    "Chip-8 Games/Paddles"                                            : "4,6 control top paddle. 7,9 bottom paddle",
    "Chip-8 Games/Pong (1 player)"                                    : "1,4 move",
    "Chip-8 Games/Pong 2 (Pong hack) [David Winter, 1997]"            : "left player: 1,4  right player C,D",
    "Chip-8 Games/Pong [Paul Vervalin, 1990]"                         : "left player: 1,4  right player C,D",
    "Chip-8 Games/Puzzle"                                             : "After the shuffle, put tiles back in order with 2,4,6,8",
    "Chip-8 Games/Sequence Shoot [Joyce Weisbecker]"                  : "Memory game. Remember the blink sequence, then repeat with C,D,E,F",
    "Chip-8 Games/Shooting Stars [Philip Baltzer, 1978]"              : "Dodge stars, 2,4,6,8 move",
    "Chip-8 Games/Space Invaders [David Winter]"                      : "5 to start, 4,6 move and 5 shoots",
    "Chip-8 Games/Tank"                                               : "shoot the target 2,4,6,8 and 5 shoots",
    "Chip-8 Games/Tetris [Fran Dachille, 1991]"                       : "4 rotates, 5,6 move, 7 drops",
    "Chip-8 Games/Tic-Tac-Toe [David Winter]"                         : "Two player game. 1-9 move",
    "Chip-8 Games/Vertical Brix [Paul Robson, 1996]"                  : "7 starts, 1,4 move the paddle",
    "Chip-8 Games/Wall [David Winter]"                                : "1,4 move",
}

class Chip8(gd2.prep.AssetBin):

    header = "../converted-assets/chip8_assets.h"

    def addall(self):
        font512 = "".join([chr(c) for c in [
            0xF0, 0x90, 0x90, 0x90, 0xF0,   # 0
            0x20, 0x60, 0x20, 0x20, 0x70,   # 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0,   # 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0,   # 3
            0x90, 0x90, 0xF0, 0x10, 0x10,   # 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0,   # 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0,   # 6
            0xF0, 0x10, 0x20, 0x40, 0x40,   # 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0,   # 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0,   # 9
            0xF0, 0x90, 0xF0, 0x90, 0x90,   # A
            0xE0, 0x90, 0xE0, 0x90, 0xE0,   # B
            0xF0, 0x80, 0x80, 0x80, 0xF0,   # C
            0xE0, 0x90, 0x90, 0x90, 0xE0,   # D
            0xF0, 0x80, 0xF0, 0x80, 0xF0,   # E
            0xF0, 0x80, 0xF0, 0x80, 0x80,   # F
        ]]).ljust(512, chr(0));
        names = "15PUZZLE BLINKY BLITZ BRIX CONNECT4 GUESS HIDDEN INVADERS KALEID MAZE MERLIN MISSILE PONG PONG2 PUZZLE SYZYGY TANK TETRIS TICTAC UFO VBRIX VERS WIPEOFF"

        for p,desc in sorted(packlist.items()):
            # Each game image is:
            # 000-1ff font
            # 200-ebf game code
            # ea0-ecf game title
            # ed0-fff game abstract
            code = font512 + open(p + ".ch8").read()
            code = code.ljust(0xf00, chr(0))
            title = p.split('/')[1]
            assert len(title) < 48
            if os.access(p + ".txt", os.R_OK):
                print "--------------------"
                print title
                print open(p + ".txt").read()
            code += title.ljust(48, chr(0))
            code += desc.ljust(208, chr(0))
            self.add(None, code)
        self.define("NGAMES", len(packlist));
        
if __name__ == '__main__':
    mm = Chip8()
    mm.make()
