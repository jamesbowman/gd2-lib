from __future__ import print_function
import array
import zlib
import textwrap
import gameduino2.base
import gameduino2.gd3registers as gd3

class Fragment(gameduino2.base.GD2):
    def __init__(self):
        self.commands = b''

        # Blue screen
        self.cmd_dlstart()
        self.ClearColorRGB(0, 0, 96)
        self.Clear()
        # And a message
        self.run()

        print("static const PROGMEM uint8_t __%s[%d] = {" % (self.name, len(self.commands)))
        print(textwrap.fill(", ".join(["%d" % c for c in self.commands])))
        print("};")

    def c(self, s):
        self.commands += s

class Bsod(Fragment):
    name = "bsod"
    def run(self):
        self.cmd_text(240, 90, 31, gd3.OPT_CENTER, "Coprocessor exception")

class Bsod815(Fragment):
    name = "bsod_815"
    def run(self):
        self.cmd_text(240, 136, 28, gd3.OPT_CENTER | gd3.OPT_FORMAT, "Coprocessor exception:\n\n%s", 0)

class IoMessage(Fragment):
    name = "bsod_badfile"
    def run(self):
        self.cmd_text(240, 90, 29, gd3.OPT_CENTER, "Cannot open file:")

if __name__ == '__main__':
    Bsod()
    Bsod815()
    IoMessage()
