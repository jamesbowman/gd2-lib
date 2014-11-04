
































.build/pro328/firmware.elf : 
	@echo [92mLinking firmware.elf[0m
	@/usr/bin/avr-gcc -mmcu=atmega328p -Wl,-Os -Wl,--gc-sections -o $@ $^ -lm


.build/pro328/firmware.hex : .build/pro328/firmware.elf
	@echo [92mConverting to firmware.hex[0m
	@/usr/bin/avr-objcopy -O ihex -R .eeprom $^ $@

include .build/pro328/src/dependencies.d

all : .build/pro328/firmware.hex
	@true

