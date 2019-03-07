# Makefile for the wflash WiFi bootloader.
# Written by Jesus Alonso (doragasu)

TARGET  = wflash
PREFIX ?= m68k-elf-
#CFLAGS  = -Os -Wall -Wextra -m68000 -fomit-frame-pointer -ffast-math -ffunction-sections -flto -ffat-lto-objects
#CFLAGS  = -Os -Wall -Wextra -m68000 -fomit-frame-pointer -ffast-math -ffunction-sections
CFLAGS  = -Og -g -Wall -Wextra -m68000 -ffast-math -ffunction-sections
AFLAGS  = --register-prefix-optional -m68000
#LFLAGS  = -T $(LFILE) -nostdlib -Wl,-gc-sections
LFLAGS  = -T $(LFILE) -Wl,-gc-sections
#LFILE   = mdbasic.ld
LFILE   = all_ram.ld
CC      = gcc
AS      = as
LD      = ld
GDB     = cgdb -d $(PREFIX)gdb --
OBJCOPY = objcopy
MDMA   ?= $(HOME)/src/github/mw-mdma-cli/mdma
#PREFIX ?= /opt/toolchains/gen/m68k-elf/bin/m68k-elf-
#EMU ?= wine $(HOME)/src/gendev/gens/gens.exe
EMU ?= gens
OBJDIR  = tmp

# List of directories with sources, excluding the boot stuff
DIRS=. mw menu_imp menu_mw
OBJDIRS = $(foreach DIR, $(DIRS), $(OBJDIR)/$(DIR))
CSRCS = $(foreach DIR, $(DIRS), $(wildcard $(DIR)/*.c))
COBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(CSRCS))
ASRCS = $(foreach DIR, $(DIRS), $(wildcard *.s))
AOBJECTS := $(patsubst %.s,$(OBJDIR)/%.o,$(ASRCS)) 

all: $(TARGET)

.PHONY: cart
cart: head.bin tail.bin
#	@$(MDMA) -s 0
#	@$(MDMA) -r 0x1FC000:0x1FFFFF
	@$(MDMA) -Vaf head.bin
#	@$(MDMA) -Vaf tail.bin:0x1F8000
	@$(MDMA) -Vaf tail.bin:0x1FC000

.PHONY: emu debug
emu: $(TARGET).bin
	@$(EMU) $< 2>/dev/null &

debug: $(TARGET).bin
	$(GDB) $(TARGET).elf -ex "target remote | blastem -D $(TARGET).bin"

$(TARGET): $(TARGET).bin
	dd if=$< of=$@ bs=8k conv=sync

head.bin: $(TARGET)
	dd if=$< of=$@ bs=512 count=1

tail.bin: $(TARGET)
	dd if=$< of=$@ bs=32k skip=127 conv=sync
#	dd if=$< of=$@ bs=64k skip=63 conv=sync
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

$(OBJDIR)/%.o: %.c | $(OBJDIRS)
	$(PREFIX)$(CC) -c -MMD -MP $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: %.s | $(OBJDIRS)
	$(PREFIX)$(AS) $(AFLAGS) $< -o $@

$(OBJDIRS):
	mkdir -p $@

.PHONY: clean
clean:
	@rm -rf $(OBJDIR) boot/rom_head.bin boot/rom_head.o boot/boot.o $(TARGET).elf $(TARGET).bin

.PHONY: mrproper
mrproper: | clean
	@rm -f $(TARGET).bin $(TARGET) head.bin tail.bin

# Include auto-generated dependencies
-include $(patsubst %.c,$(OBJDIR)/%.d,$(CSRCS))

