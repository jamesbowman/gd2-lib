#!/usr/bin/python
#
# This program assembles the distribution file Gameduino2.zip
# from the source .ino files, and the asset files in
# converted-assets.
#

version = open("version").read().strip()

properties = """\
name=Gameduino2
version=%s
author=James Bowman <jamesb@excamera.com>
maintainer=James Bowman <jamesb@excamera.com>
sentence=Gameduino 2 and 3 driver
paragraph=for the popular Gameduino series of graphics, audio video shields.
category=Display
url=http://gameduino.com
architectures=*
""" % version

inventory = {
    '1.Basics'      : "helloworld fizz blobs simon jpeg",
    '2.Graphics'    : "logo walk tiled mono slotgag reflection",
    '3.Peripherals' : "sketch tilt noisy song",
    '4.Utilities'   : "viewer radarchart selftest",
    '5.Demos'       : "cobra jnr kenney sprites widgets",
    '6.Games'       : "nightstrike chess frogger",
    '7.GD3'         : "video1 video2 cube cube2",
}

import zipfile

def clean(src, is_due = False):
    vis = 1
    dst = []
    for l in src:
        assert not chr(9) in l, "Tab found in source"
        if is_due and ('EEPROM' in l):
            continue
        if "//'" in l:
            l = l[:l.index("//'")]
        if vis and not "JCB" in l:
            # assert not "dumpscreen" in l
            dst.append(l.rstrip() + "\n")
        else:
            if "JCB{" in l:
                vis = 0
            if "}JCB" in l:
                vis = 1
    return "".join(dst)

for (is_due, suffix) in [(False, ""), (True, "_Due")]:
    z = zipfile.ZipFile("Gameduino2%s.zip" % suffix, "w", zipfile.ZIP_DEFLATED)

    for f in "GD2.cpp GD2.h transports/wiring.h".split():
        c = open(f).read().replace('%VERSION', version)
        z.writestr("Gameduino2/%s" % f, c)
    z.writestr("Gameduino2/library.properties", properties)

    for d,projs in inventory.items():
        dir = "Gameduino2" + "/" + d
        for p in projs.split():
            pd = dir + "/" + p
            z.writestr("%s/%s.ino" % (pd, p), clean(open("examples/%s.ino" % p), is_due))
            for l in open("examples/%s.ino" % p):
                if '#include "' in l:
                    hdr = l[10:l.rindex('"')]
                    z.write("converted-assets/%s" % hdr, "%s/%s" % (pd, hdr))

    z.close()

# print "\n".join(["./mkino %s" % s for s in " ".join(inventory.values()).split()])
