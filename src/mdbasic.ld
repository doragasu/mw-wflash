/* Linker script for the wflash bootloader.
 *
 * The bootloader has a 512 byte header on top of the cartridge (vectors +
 * cartridge header) and a boot sector at the bottom 8 KiB of the 32 Mbit
 * ROM. All the space inbetween is unused.
 *
 * The code and initialized data at the 8 KiB boot sector are copied to the
 * RAM by the startup code, for the bootloader to run from RAM. This is
 * necessary because Flash cannot be accessed during erase/program
 * operations.
 */

OUTPUT_ARCH(m68k)
SEARCH_DIR(.)
__DYNAMIC  =  0;

MEMORY
{
	rom : ORIGIN = 0x00000000, LENGTH = 0x00400000
	ram : ORIGIN = 0x00FF0000, LENGTH = 0x00010000
}

/*
 * allocate the stack at the top of memory, since the stack
 * grows down
 */

PROVIDE (__stack = 0x01000000);


SECTIONS
{
  .text.boot 0x00000000 :
  {
    KEEP(*(.text.boot)) *(.text.boot)
  }
  _sboot = SIZEOF(.text.boot);

  .text 0xFF0000 :
  AT (ADDR(.text.boot) + SIZEOF(.text.boot))
  {
	*(.text.*) *(.text)
	. = ALIGN(4);
    *(.rodata .rodata.*)

    . = ALIGN(0x4);
    __INIT_SECTION__ = . ;
    *(.init)
    SHORT (0x4E75)	/* rts */

    __FINI_SECTION__ = . ;
    *(.fini)
    SHORT (0x4E75)	/* rts */

    _etext = .;
  } > ram

  .data 0xFF0000 + SIZEOF(.text) :
  AT ( ADDR (.text) + SIZEOF (.text))
  {
    *(.data .data.*)
    _edata = .;
  } > ram
  _sdata = SIZEOF(.text) + SIZEOF(.data);

  .bss _edata :
  {
    _start = . ;
    *(.bss .bss.*)
    *(COMMON)
    _bend = . ;
  } > ram

  .stab 0 (NOLOAD) :
  {
    *(.stab)
  }

  .stabstr 0 (NOLOAD) :
  {
    *(.stabstr)
  }

  .eh_frame 0 (NOLOAD) :
  {
    *(.eh_frame)
  }
}
