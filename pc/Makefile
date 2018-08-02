D=$(HOME)/Arduino/libraries/Gameduino2

all: $(foreach s, \
  helloworld fizz blobs simon jpeg \
  logo walk tiled mono slotgag reflection \
  sketch tilt noisy song \
  viewer radarchart \
  cobra jnr kenney sprites widgets \
  nightstrike chess frogger \
  video1 video2 cube cube2 \
  life \
  , build/$(s))

clean:
	rm -rf build/*

OPTS=
# OPTS=-g -Wall -Wno-unused-function

build/%: examples/%.ino GD2.cpp GD2.h transports/* pc/*
	g++ $(OPTS) -o $@ -g  -D TEENSYDUINO=1 -DARDUINO_ARCH_STM32L4=1 -DSPIDRIVER=1 \
          -I pc/ -I converted-assets/ -I $D -x c++ $< pc/main.cpp pc/spidriver.cpp GD2.cpp

build/%: contrib/%.ino GD2.cpp GD2.h transports/* pc/*
	g++ $(OPTS) -o $@ -g  -D TEENSYDUINO=1 -DARDUINO_ARCH_STM32L4=1 -DSPIDRIVER=1 \
          -I pc/ -I converted-assets/ -I $D -x c++ $< pc/main.cpp pc/spidriver.cpp GD2.cpp

