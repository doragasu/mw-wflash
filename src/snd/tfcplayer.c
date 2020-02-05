/* TFM Music Maker compiled data player */
/* C version for SMD                    */
/* by Alone Coder and Shiru, 20.10.07   */

/* gcc version 12.12.07, fix 01.03.10 */

// Trimmed by doragasu

#include "sound.h"
#include "../util.h"

typedef int8_t   i8;
typedef uint8_t  u8;
typedef int16_t  i16;
typedef uint16_t u16;

#define YM2612_A0   	0xa04000
#define YM2612_D0   	0xa04001
#define YM2612_A1   	0xa04002
#define YM2612_D1   	0xa04003

#define Z80_HALT  	0xa11100
#define Z80_RESET 	0xa11200

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

struct {
	const u8 *data;
	u16 play;
	struct {
		u16 ptr;
		u16 wait;
		u16 freq;
		u16 retblk;
		u16 loopadr;
		u16 rep;
	} chn[6];
} TFCP;

ROM_TEXT(ym2612wr)
static void ym2612wr(u8 reg,u8 val,u8 bank)
{
	volatile u8 *pw,*pa,*pd;

	pw=(u8*)YM2612_A0;

	if(!bank) {
		pa=(u8*)YM2612_A0;
		pd=(u8*)YM2612_D0;
	} else {
		pa=(u8*)YM2612_A1;
		pd=(u8*)YM2612_D1;
	}

	while(*pw&0x80);
	*pa=reg;
	while(*pw&0x80);
	*pd=val;
}

ROM_TEXT(tfc_init)
void tfc_init(const u8 *data)
{
	volatile u16 *pz;

	// Put Z80 out of reset and halted, so we can access the bus.  As Z80
	// is not used by the loader, we do not need to mess with it anymore.
	pz=(u16*)Z80_RESET;
	*pz=0x100;
	pz=(u16*)Z80_HALT;
	*pz=0x100;
	while(*pz&0x100);

	TFCP.data=data;
	tfc_play(FALSE);
}

ROM_TEXT(tfc_frame)
void tfc_frame(void)
{
	u16 aa,chn,rchn,bank,key,tag,off,freq,frameptr,getptr;

	if(!TFCP.play||!TFCP.data) return;

	frameptr=0;
	getptr=FALSE;

	for(chn=0;chn<6;chn++) {
		if(TFCP.chn[chn].wait<0xff) {
			TFCP.chn[chn].wait++;
			continue;
		}

		if(chn<3) {
			bank=0;
			key=chn;
			rchn=chn;
		} else {
			bank=1;
			key=chn+1;
			rchn=chn-3;
		}

		if(TFCP.chn[chn].rep>0) {
			TFCP.chn[chn].rep--;
			if(!TFCP.chn[chn].rep) TFCP.chn[chn].ptr=TFCP.chn[chn].retblk;
		}

		while(1) {
			tag=TFCP.data[TFCP.chn[chn].ptr++];
			switch(tag) {
				case 0x7e:/*01111110 begin*/
					TFCP.chn[chn].loopadr=TFCP.chn[chn].ptr;
					continue;
				case 0x7f:/*01111111 end*/
					TFCP.chn[chn].ptr=TFCP.chn[chn].loopadr;
					continue;
				case 0xd0:/*11010000 repeat block*/
					TFCP.chn[chn].rep=TFCP.data[TFCP.chn[chn].ptr++];
					off=TFCP.data[TFCP.chn[chn].ptr++]<<8;
					off+=TFCP.data[TFCP.chn[chn].ptr++];
					TFCP.chn[chn].retblk=TFCP.chn[chn].ptr;
					TFCP.chn[chn].ptr+=(i16)off;
					continue;
				case 0xbf:/*10111111 use old frame data disp16*/
					off=TFCP.data[TFCP.chn[chn].ptr++]<<8;
					off+=TFCP.data[TFCP.chn[chn].ptr++];
					frameptr=TFCP.chn[chn].ptr+(i16)off;
					tag=TFCP.data[frameptr++];
					break;
				case 0xff:/*11111111 use old frame data disp8*/
					off=TFCP.data[TFCP.chn[chn].ptr++]-256;
					frameptr=TFCP.chn[chn].ptr+(i16)off;
					tag=TFCP.data[frameptr++];
					break;
				default:
					if(tag>=0xe0)/*skip 32..2 frames*/ {
						TFCP.chn[chn].wait=tag;
						break;
					}
					if(tag>=0xc0)/*slide d+16*/ {
						freq=TFCP.chn[chn].freq;
						freq=(freq&0xff00)+((freq+tag+0x30)&0xff);
						TFCP.chn[chn].freq=freq;
						ym2612wr(0xa4+rchn,freq>>8,bank);
						ym2612wr(0xa0+rchn,freq&0xff,bank);
						break;
					}
					frameptr=TFCP.chn[chn].ptr;
					getptr=TRUE;
			}

			if(frameptr) {
				if(tag&0xc0) ym2612wr(0x28,key,0);/*keyoff*/
				if(tag&0x01)/*freq*/ {
					freq=TFCP.data[frameptr++]<<8;
					freq+=TFCP.data[frameptr++];
					TFCP.chn[chn].freq=freq;
					ym2612wr(0xa4+rchn,freq>>8,bank);
					ym2612wr(0xa0+rchn,freq&0xff,bank);
				}
				for(aa=0;aa<((tag>>1)&0x1f);aa++)/*0..30 regs*/ {
					ym2612wr(TFCP.data[frameptr],TFCP.data[frameptr+1],bank);
					frameptr+=2;
				}
				if(tag&0x80) ym2612wr(0x28,0xf0|key,0);/*keyon*/ 
				if(getptr) {
					TFCP.chn[chn].ptr=frameptr;
					getptr=FALSE;
				}
				frameptr=0;
			}

			break;
		}
	}
}

ROM_TEXT(tfc_play)
void tfc_play(u16 play)
{
	i16 aa,bb,cc,pp;

	TFCP.play=play;

	if(!TFCP.play) /* mute ym2612 */ {
		ym2612wr(0x2f,0,0); /* freq.scaler */
		ym2612wr(0x2d,0,0);
		ym2612wr(0x22,0,0); /* LFO off */
		ym2612wr(0x2b,0,0); /* DAC off */
		ym2612wr(0x27,0,0); /* CH3 normal mode */
		ym2612wr(0x27,0,1); /* CH3 normal mode */
		for(cc=0;cc<2;cc++) {
			for(aa=0;aa<3;aa++) {
				for(bb=0x30;bb<0x40;bb+=4) ym2612wr(aa+bb,0x00,cc); /* dt1/mul */
				for(bb=0x40;bb<0x50;bb+=4) ym2612wr(aa+bb,0x7f,cc); /* tl */
				for(bb=0x50;bb<0x60;bb+=4) ym2612wr(aa+bb,0x00,cc); /* rs/ar */
				for(bb=0x60;bb<0x70;bb+=4) ym2612wr(aa+bb,0x00,cc); /* am/d1r */
				for(bb=0x70;bb<0x80;bb+=4) ym2612wr(aa+bb,0x00,cc); /* d2r */
				for(bb=0x80;bb<0x90;bb+=4) ym2612wr(aa+bb,0x0f,cc); /* d1l/rr */
				for(bb=0x90;bb<0xa0;bb+=4) ym2612wr(aa+bb,0x00,cc); /* ssg-eg */
				ym2612wr(0xb0+aa,0x00,cc); /* fb/algo */
				ym2612wr(0xb4+aa,0xc0,cc); /* ams/fms */
				ym2612wr(0x28,aa+(cc<<2),0); /* keyoff */
			}
		}
	} else {
		pp=10;
		for(aa=0;aa<6;aa++) {
			ym2612wr(0xb4+(aa%3),0xc0,aa/3); /* stereo reset */
			TFCP.chn[aa].ptr=TFCP.data[pp]+(TFCP.data[pp+1]<<8);
			TFCP.chn[aa].loopadr=TFCP.chn[aa].ptr;
			TFCP.chn[aa].wait=0xff;
			TFCP.chn[aa].rep=0;
			pp+=2;
		}
	}
}

