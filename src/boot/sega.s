.section .text.boot

*-------------------------------------------------------
*
*       Sega startup code for the GNU Assembler
*       Translated from:
*       Sega startup code for the Sozobon C compiler
*       Written by Paul W. Lee
*       Modified by Charles Coty
*       Modified by Stephane Dallongeville
*       Massaged and trimmed by Jesus Alonso (@doragasu)
*
*-------------------------------------------------------

        .org    0x00000000

_Start_Of_Rom:
_Vecteurs_68K:
        dc.l    0x00FFFE00              /* Stack address */
        dc.l    _Entry_Point            /* Program start address */
        dc.l    _Bus_Error
        dc.l    _Address_Error
        dc.l    _Illegal_Instruction
        dc.l    _Zero_Divide
        dc.l    _Chk_Instruction
        dc.l    _Trapv_Instruction
        dc.l    _Privilege_Violation
        dc.l    _Trace
        dc.l    _Line_1010_Emulation
        dc.l    _Line_1111_Emulation
        dc.l     _Error_Exception, _Error_Exception, _Error_Exception, _Error_Exception
        dc.l     _Error_Exception, _Error_Exception, _Error_Exception, _Error_Exception
        dc.l     _Error_Exception, _Error_Exception, _Error_Exception, _Error_Exception
        dc.l    _Error_Exception, _INT, _EXTINT, _INT
        dc.l    _HINT
        dc.l    _INT
        dc.l    _VINT
        dc.l    _INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT
        dc.l    _INT,_INT,_INT,_INT,_INT,_INT,_INT,_INT

        .incbin "boot/rom_head.bin", 0, 0x100

* Locate the boot sector at the last 64 KiB sector of the Flash chip.
        .org    0x003F0000
_Entry_Point:
        move    #0x2700,%sr
        lea     Table,%a5
        movem.w (%a5)+,%d5-%d7
        movem.l (%a5)+,%a0-%a4
* Check Version Number
        move.b  -0x10ff(%a1),%d0
        andi.b  #0x0f,%d0
        beq.s   WrongVersion
* Sega Security Code (SEGA)
        move.l  #0x53454741,0x2f00(%a1)
WrongVersion:
*        move.w  (%a4),%d0
* Put Z80 and YM2612 in reset (note: might not be enough cycles)
        moveq   #0x00,%d0
		move.b	%d0,(%a2)
		jmp		Continue

Table:
        dc.w    0x8000,0x3fff,0x0100
        dc.l    0xA00000,0xA11100,0xA11200,0xC00000,0xC00004
		dc.b	0x9F,0xBF,0xDF,0xFF

Continue:
* Set stack pointer, release Z80 from reset and request bus
		move	%d0,%sp
        move.w  %d7,(%a1)
        move.w  %d7,(%a2)

* Mute PSG
		moveq	#3,%d1
PsgMute:
		move.b	(%a5)+,0x11(%a3)
		dbf		%d1, PsgMute

* clear Genesis RAM
        lea     _end_dirty,%a0
        move.w  #0x3FFE,%d1

ClearRam:
        move.l  %d0,(%a0)+
        dbra    %d1,ClearRam

* Copy initialized variables from ROM to Work RAM
        lea     _sboot,%a0
        lea     _end_dirty,%a1
        move.l  #_sdata,%d0
        lsr.l   #1,%d0
        beq     2f

        subq.w  #1,%d0
1:
        move.w  (%a0)+,(%a1)+
        dbra    %d0,1b

* Copy flash routines to RAM
2:
*	* a0 already contains the load address of the routines
**	lea		_lflash,%a0
*	lea		_oflash,%a1
*	move.l	#_sflash,%d0
*	lsr.l	#1,%d0
*	beq 	3f
*
*	subq.w	#1,%d0
*1:
*	move.w	(%a0)+,(%a1)+
*	dbra	%d0,1b
*
** Jump to initialisation process...
*3:
        move.w  #0,%a7
        jmp     _start_entry


*------------------------------------------------
*
*       interrupt functions
*
*------------------------------------------------

* Interrupts are not supported. So provide a minimal interrupt handler
* that just loops forever if the interrupt occurs.
_Bus_Error:
_Address_Error:
_Illegal_Instruction:
_Zero_Divide:
_Chk_Instruction:
_Trapv_Instruction:
_Privilege_Violation:
_Trace:
_Line_1010_Emulation:
_Line_1111_Emulation:
_Error_Exception:
_INT:
_EXTINT:
_HINT:
_VINT:
	jmp _Bus_Error


* Boot ROM at specified address
	.align 2
	.globl boot_addr
	.type	boot_addr, @function
boot_addr:
    move    #0x2700,sr              /* disable interrupts */
* Get boot address
	move.l 4(%sp),%d2

* De-initialize pads. This is necessary for some games like uwol,
* that skip initialization if they detect pads initialized.
	moveq	#0x00,%d0
	lea 	0xA10008, %a0
	move.l	%d0,(%a0)
	addq 	#4, %a0
	move.w	%d0,(%a0)

* Release Z80 from reset
	moveq #1, %d1
	lea 0xA11100, %a1
	move.b %d1, 0x100(%a1)
* Request Z80 bus
	move.b %d1, (%a1)
* Wait until bus is granted
1:
	move.b (%a1), %d1
	beq 1b

* Clear Z80 RAM
	lea 0xA00000, %a2
	move #0x1FFF, %d3
2:	
	move.b %d0, (a2)+
	dbra %d3, 2b

* Set Z80 reset
	move.b %d0, 0x100(%a1)

* Release Z80 bus
	move.b %d0, (%a1)

* Clear WRAM (skip dirty DW)
	move.w  #0x3FFE,%d1
	lea     _end_dirty,%a0
WRamClear:
	move.l  %d0,(a0)+
	dbra    %d1,WRamClear

* Set default stack pointer
	move.l	%d0,%a0
	move.l	(%a0),%sp

* Boot from entry point
	move.l %d2,%a0

	move.l	%d0,%d1
	move.l	%d0,%d2
	move.l	%d0,%d3
	move.l	%d0,%d4
	move.l	%d0,%d5
	move.l	%d0,%d6
	move.l	%d0,%d7

	move.l	%d0,%a1
	move.l	%d0,%a2
	move.l	%d0,%a3
	move.l	%d0,%a4
	move.l	%d0,%a5
	move.l	%d0,%a6

	jmp (%a0)
	.size	boot_addr, .-boot_addr
