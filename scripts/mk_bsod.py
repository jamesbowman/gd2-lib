import array
import zlib
import textwrap
import gameduino2 as gd2

class Fragment(gd2.base.GD2):
    def __init__(self):
        self.commands = ""
        self.run()
        print "static const PROGMEM uint8_t __%s[%d] = {" % (self.name, len(self.commands))
        print textwrap.fill(", ".join(["%d" % ord(c) for c in self.commands]))
        print "};"

    def c(self, s):
        self.commands += s

class Bsod(Fragment):
    name = "bsod"
    def run(self):
        self.cmd_dlstart()
        if 1:
            self.ClearColorRGB(0, 0, 96)
            self.Clear()
            self.cmd_text(240, 90, 31, gd2.OPT_CENTER, "ERROR")
        else:
            self.BitmapLayout(gd2.TEXTVGA, 2 * 60, 17)
            self.BitmapSize(gd2.NEAREST, gd2.BORDER, gd2.BORDER, 480, 272)
            self.BlendFunc(gd2.ONE, gd2.ZERO)
            self.Begin(gd2.BITMAPS)
            self.Vertex2ii(0,0,0,0)
            full = 60 * "!"
            edge = "!                                                          !"
            text = full + 15 * edge + full
            screen = "".join(c + chr(0x1f) for c in text)
            cscreen = zlib.compress(screen)
            self.cmd_inflate(0)
            self.c(cscreen)

class IoMessage(Fragment):
    name = "bsod_badfile"
    def run(self):
        self.cmd_text(240, 148, 29, gd2.OPT_CENTER, "Cannot open file:")

if __name__ == '__main__':
    Bsod()
    IoMessage()
