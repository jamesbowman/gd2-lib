gd2-lib
=======

[![Build Status](https://travis-ci.org/jamesbowman/gd2-lib.svg?branch=master)](https://travis-ci.org/jamesbowman/gd2-lib)

This is the source repository of the GD library - the software side of the Gameduino project. The installation library and instructions are at:

http://gameduino.com/code

To build the release library ``Gameduino2.zip`` run:

    python publish.py

FAQ
---

**How do I use GD with a 800x480 display?**

If you're using the Gameduino 3 7" model, this is done for you by the Gameduino 3's configuration flash.
If you're using another screen the setup procedure must be done explicitly.
After calling ``GD.begin()`` you can set the scanout registers for 800x480 like this:

    GD.wr16(REG_HCYCLE, 928);
    GD.wr16(REG_HOFFSET, 88);
    GD.wr16(REG_HSIZE, 800);
    GD.wr16(REG_HSYNC0, 0);
    GD.wr16(REG_HSYNC1, 48);
    GD.wr16(REG_VCYCLE, 525);
    GD.wr16(REG_VOFFSET, 32);
    GD.wr16(REG_VSIZE, 480);
    GD.wr16(REG_VSYNC0, 0);
    GD.wr16(REG_VSYNC1, 3);
    GD.wr16(REG_CSPREAD, 1);
    GD.wr16(REG_DITHER, 1);
    GD.wr16(REG_PCLK_POL, 0);
    GD.wr16(REG_PCLK, 4);

**How do I use GD with a 320x480 display?**

There is a writeup here: http://excamera.com/sphinx/article-ili9488.html

**How do I use GD with the [Sunflower Shield](https://www.kickstarter.com/projects/cowfishstudios/sunflower-shield-35-hmi-display-w-cap-touch-for-ar#)?**

At the start of GD2.cpp change ``BOARD`` and ``CALIBRATION``:

    #define BOARD         BOARD_SUNFLOWER   // board, from above
    #define STORAGE       1                 // Want SD storage?
    #define CALIBRATION   0                 // Want touchscreen calibration?

**How do I change the select pin assignments?**

To change the GPU select from pin 8, modify ``#define CS`` at the start of ``transports/wiring.h``

To change microSD select from pin 9, modify ``#define SD_PIN`` at the start of ``GD2.cpp``

**How do you run in portrait mode?**

After calling ``GD.begin()`` set the orientation like this::

    GD.cmd_setrotate(2);

to enter portrait mode. The argument controls orientation, 0 and 1 are landscape. 2 and 3 are portrait.

**I can only get coordinates in the range 0-511! What about bigger screens?**

The short-form drawing opcode ``Vertex2ii()`` has a limited coordinate range.
But the opcode ``Vertex2f()`` has huge range. It works in subpixel coordinates, with programmable precision. At the start of drawing do:

    GD.VertexFormat(3); // means points are scaled by 8 from here on

then instead of this:

    GD.Vertex2ii(x, y, handle, cell);

do:

    GD.BitmapHandle(handle);
    GD.Cell(cell);
    GD.Vertex2f(8 * x, 8 * y);

This has enough range for displays up to 2048x2048.

If you don't need subpixel precision, then you can use a precision of zero:

    GD.VertexFormat(0); // integer coordinates
    GD.Vertex2f(x, y);

**My JPEG does not load!**

First, make sure you call ``GD.Begin()`` without arguments. This initializes the microSD code.

Second, filenames on the microSD must not be too long. They have to
be in 8.3 form so that they can be recognized by the quite limited
Arduino-based loader.

Second, make sure the JPEG is not progressive; the hardware supports baseline jpegs only.

If there's doubt, post your JPEG to the
[Gameduino Forum](https://gameduino2.proboards.com/),
and I can take a look at it.
