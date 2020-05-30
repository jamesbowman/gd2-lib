import os
import sys
import serial
import struct
import array
import time
import binascii
def crc(s):     # CRC-32 of string s
    return binascii.crc32(s) & 0xffffffff
from PIL import Image, ImageChops

def fetch8(pix, x, y, ser):
    difference = ord(ser.read(1))
    if (difference & 1) == 0:
        assert 0, "difference is %02x\n" % difference
    for i in range(8):
        if (difference >> i) & 1:
            (b, g, r) = list(ser.read(3))
        pix[x + i, y] = (r, g, b)

if __name__ == '__main__':
    import sys, getopt
    try:
        optlist, args = getopt.getopt(sys.argv[1:], "vh:s:a:")

    except getopt.GetoptError as reason:
        print(reason)
        print
        print('usage: listenscreenshot.py [options]')
        print
        print
        sys.exit(1)
    optdict = dict(optlist)

    verbose = '-v' in optdict
    assetfile = optdict.get('-a', None)
    if '-s' in optdict:
        speed = int(optdict['-s'])
    else:
        speed = 1000000

    port = optdict.get('-h', "/dev/ttyUSB0")
    if verbose:
        print('Opening %s at %d' % (port, speed))
    ser = serial.Serial(port, speed)

    ser.setDTR(0)
    time.sleep(0.01)
    ser.setDTR(1)

    l = b''
    ESCAPE = "%H%"
    log = open("log", "w")
    frame = 0

    if 0:
        grid = Image.fromstring("L", (1280, 720), 360 * (
            ((chr(255) + chr(240)) * 640) +
            ((chr(240) + chr(224)) * 640))).convert("RGB")

    try:
        if assetfile is not None:
            srcf = open(assetfile, "rb")
            totsize = os.path.getsize(assetfile)
    except IOError:
        srcf = None

    inputs = open("inputs")
    while True:
        s = ser.read(1)
        if s == b'\n':
            l = b''
        else:
            l = (l + s)[-3:]
        if l == ESCAPE:
            print
            break

        chk = s[0]
        if (chk == 0xa4):
            print("[Synced]")
            ser.write(b"!")
        elif (chk == 0xa5):
            (w,h) = struct.unpack("HH", ser.read(4))
            filename = "%06d.png" % frame
            sys.stdout.flush() 
            im = Image.new("RGBA", (w, h))
            pix = im.load()
            total = 0
            t0 = time.time()

            ser.timeout = 0.5
            for y in range(-2, h):
                sys.stdout.write("\rdumping %dx%d frame to %s [%-50s]" % (w, h, filename, ("#" * (50 * y // h ))))
                ser.write(b"!")
                (licrc, ) = struct.unpack("I", ser.read(4))
                li = ser.read(4 * w)
                if licrc != crc(li):
                    print("\nCRC mismatch line %d %08x %08x\n" % (y, licrc, crc(li)))
                strip = Image.frombytes("RGBA", (w, 1), li)
                im.paste(strip, (0, max(0, y)))
            # print('%.1f' % (time.time() - t0))
            print(' %08x' % crc(im.tobytes()))
            ser.timeout = 0

            (b,g,r,a) = im.split()
            im = Image.merge("RGB", (r, g, b))

            if 1:
                im.save(filename)
            else:
                im = im.resize((im.size[0] * 2, im.size[1] * 2))
                full = Image.new("RGB", (1280, 720))
                full.paste(im, ((1280 - im.size[0]) / 2, (680 - im.size[1]) / 2))
                ImageChops.multiply(grid, full).save(filename)
            frame += 1
        elif False and (chk == 0xa6):
            d = srcf.read(0x30)
            print("\rtransfer ", "[" + ("#" * (72 * srcf.tell() / totsize)).ljust(72) + "]",)
            while len(d) & 3:
                d += chr(0)
            ser.write(chr(len(d)) + d)
        elif (chk == 0xa7):
            for line in inputs:
                if line.startswith("INPUTS "):
                    break
            if not line.startswith("INPUTS "):
                sys.exit(0)
            for v in line.split()[1:]:
                ser.write(chr(int(v, 16)))
        else:
            # sys.stdout.write(s.decode('UTF-8'))
            print(s)
            sys.stdout.flush() 
            # log.write(s.decode('UTF-8'))
