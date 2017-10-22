# Makefile for the wflash WiFi bootloader.
# Written by Jesus Alonso (doragasu)

TARGET  = wflash
CFLAGS  = -Os -Wall -Wextra -m68000 -ffunction-sections
AFLAGS  = --register-prefix-optional -m68000
#LFLAGS  = -T $(LFILE) -nostdlib -Wl,-gc-sections
LFLAGS  = -T $(LFILE) -Wl,-gc-sections
LFILE   = mdbasic.ld
CC      = gcc
AS      = as
LD      = ld
OBJCOPY = objcopy
MDMA   ?= $(HOME)/src/github/mw-mdma-cli/mdma
PREFIX ?= /opt/toolchains/gen/m68k-elf/bin/m68k-elf-
#EMU ?= wine $(HOME)/src/gendev/gens/gens.exe
EMU ?= gens
OBJDIR  = tmp

CSRCS = $(wildcard *.c) $(wildcard mw/*.c)
COBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(CSRCS))
ASRCS = $(wildcard *.s)
AOBJECTS := $(patsubst %.s,$(OBJDIR)/%.o,$(ASRCS)) 

all: $(TARGET)

.PHONY: cart
cart: head.bin tail.bin
	@$(MDMA) -s 0
	@$(MDMA) -s 0x1FC000
	@$(MDMA) -s 0x1FD000
	@$(MDMA) -s 0x1FE000
	@$(MDMA) -s 0x1FF000
	@$(MDMA) -Vf head.bin
	@$(MDMA) -Vf tail.bin:0x1FC000

.PHONY: emu
emu: $(TARGET).bin
	@$(EMU) $< 2>/dev/null &

$(TARGET): $(TARGET).bin
	dd if=$< of=$@ bs=8k conv=sync

head.bin: $(TARGET)
	dd if=$< of=$@ bs=512 count=1

tail.bin: $(TARGET)
	dd if=$< of=$@ bs=32k skip=127 conv=sync
#	dd if=$< of=$@ bs=8k skip=510 conv=sync

$(TARGET).bin: $(TARGET).elf
	$(PREFIX)$(OBJCOPY) -O binary $< $@

$(TARGET).elf: boot/boot.o $(AOBJECTS) $(COBJECTS)
	$(PREFIX)$(CC) -o $(TARGET).elf boot/boot.o $(AOBJECTS) $(COBJECTS) $(LFLAGS) -Wl,-Map=$(OBJDIR)/$(TARGET).map -lgcc

boot/boot.o: boot/rom_head.bin boot/sega.s
	$(PREFIX)$(AS) $(AFLAGS) boot/sega.s -o boot/boot.o

boot/rom_head.bin: boot/rom_head.o
#	$(PREFIX)$(LD) $(LFLAGS) --oformat binary $< -o $@
	$(PREFIX)$(OBJCOPY) -O binary $< $@

boot/rom_head.o: boot/rom_head.c
	$(PREFIX)$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)/mw
	$(PREFIX)$(CC) -c -MMD -MP $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: %.s | $(OBJDIR)
	$(PREFIX)$(AS) $(AFLAGS) $< -o $@

$(OBJDIR)/mw:
	mkdir -p $@

$(OBJDIR):
	mkdir -p $@

.PHONY: clean
clean:
	@rm -rf $(OBJDIR) boot/rom_head.bin boot/rom_head.o boot/boot.o $(TARGET).elf $(TARGET).bin

.PHONY: mrproper
mrproper: | clean
	@rm -f $(TARGET).bin $(TARGET) head.bin tail.bin

# Include auto-generated dependencies
-include $(patsubst %.c,$(OBJDIR)/%.d,$(CSRCS))

