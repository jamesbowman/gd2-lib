gd2-lib
=======

Gameduino 2 library sources. To build the Arduino release ``Gameduino2.zip`` run:

    python publish.py

FAQ
---

**How do I use this with an 800x480 display?**

After calling ``GD.begin()`` you can set the scanout registers for 800x480 like this:

    GD.wr16(vc.REG_HCYCLE, 928);
    GD.wr16(vc.REG_HOFFSET, 88);
    GD.wr16(vc.REG_HSIZE, 800);
    GD.wr16(vc.REG_HSYNC0, 0);
    GD.wr16(vc.REG_HSYNC1, 48);
    GD.wr16(vc.REG_VCYCLE, 525);
    GD.wr16(vc.REG_VOFFSET, 32);
    GD.wr16(vc.REG_VSIZE, 480);
    GD.wr16(vc.REG_VSYNC0, 0);
    GD.wr16(vc.REG_VSYNC1, 3);
    GD.wr16(vc.REG_CSPREAD, 1);
    GD.wr16(vc.REG_DITHER, 1);
    GD.wr16(vc.REG_PCLK_POL, 0);
    GD.wr16(vc.REG_PCLK, 4);
