ZOS_PATH ?= ../Zeal-8-bit-OS
ZVB_SDK_PATH ?= ../Zeal-VideoBoard-SDK
BIN=bin/pillbox.bin
OBJ=obj/main.rel obj/game.rel obj/menu.rel obj/sound.rel obj/img.rel
IMG=img/pillboxtiles.zts
CC=sdcc
CFLAGS=-mz80 --std-c2x -c -I $(ZOS_PATH)/kernel_headers/sdcc/include/ -I $(ZVB_SDK_PATH)/include --codeseg TEXT --debug
AS=sdasz80 -o -l -s
OBJCOPY=objcopy
LD=sdldz80
LDFLAGS=-n -y -mjwx -i -b _HEADER=0x4000 -k $(ZOS_PATH)/kernel_headers/sdcc/lib -l z80 $(ZOS_LDFLAGS)
ZOS_LIBS=-k $(ZVB_SDK_PATH)/lib -l zvb_sound -l zvb_gfx
all: init $(BIN)

PHONY: init clean reallyclean

init:
	@mkdir -p obj
	@mkdir -p bin

$(BIN): $(OBJ)
	$(LD) $(LDFLAGS) -o $(BIN:.bin=.ihx) \
	../Zeal-8-bit-OS/kernel_headers/sdcc/bin/zos_crt0.rel $(OBJ) $(ZOS_LIBS)
	$(OBJCOPY) --input-target=ihex --output-target binary $(BIN:.bin=.ihx) $(BIN)
	cp $(BIN) s

obj/%.rel: src/%.c
	$(CC) $(CFLAGS) -o $@ $<

obj/%.rel: src/%.asm
	$(AS) $@ $<

img/%.zts: img/%.gif
	$(ZVB_SDK_PATH)/tools/zeal2gif/gif2zeal.py -z lz -i $<

img/title.zts: img/title.gif
	$(ZVB_SDK_PATH)/tools/zeal2gif/gif2zeal.py -b 1 -i $<

obj/game.rel: src/game.c src/game.h src/main.h
obj/menu.rel: src/menu.c src/menu.h src/game.h src/sound.h
obj/main.rel: src/main.c src/game.h src/menu.h src/img.h img/title.h src/sound.h
obj/sound.rel: src/sound.c src/sound.h
obj/img.rel: src/img.asm $(IMG)

clean:
	-rm $(OBJ)
	-rm $(BIN)
	-rm obj/*.lst obj/*.sym obj/*.adb obj/*.asm
	-rm bin/*.ihx bin/*.noi bin/*.map bin/*.cdb

reallyclean: clean
	-rm img/*.zts img/*.ztp
