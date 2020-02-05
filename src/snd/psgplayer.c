/* PSG multichannel sound effects player v1.0 by Shiru, 03.11.07 */

/* gcc version 12.12.07 */

// Adapted by doragasu, 2019

#include "sound.h"
#include "../util.h"

#define PSG_VCH_MAX	4
#define PSG_DATA	0xc00011

typedef int8_t   i8;
typedef uint8_t  u8;
typedef int16_t  i16;
typedef uint16_t u16;

struct {
	const u8 *data;
	i16 total;
	struct {
		struct {
			i16 ptr;
			i16 wait;
			i16 time;
			u16 div;
			u8 vol;
		} slot[PSG_VCH_MAX];
	} chn[4];
} PSGFX;

ROM_TEXT(psgfx_init)
u16 psgfx_init(const u8 *data)
{
	volatile u8 *pb;
	u16 i,j;

	pb=(u8*)PSG_DATA;

	*pb=0x9f;
	*pb=0xbf;
	*pb=0xdf;
	*pb=0xff;

	PSGFX.data=data;
	PSGFX.total=(data[0]<<8)+data[1];

	for(i=0;i<4;i++) {
		for(j=0;j<PSG_VCH_MAX;j++) {
			PSGFX.chn[i].slot[j].ptr=-1;
			PSGFX.chn[i].slot[j].wait=0;
		}
	}

	return PSGFX.total;
}

ROM_TEXT(psgfx_deinit)
void psgfx_deinit(void)
{
	PSGFX.data = NULL;
}

ROM_TEXT(psgfx_frame)
void psgfx_frame(void)
{
	volatile u8 *pb;
	u8 mbyte;
	i16 pchn,vchn,rchn,mvol,nvol;
	u16 div;

	if(!PSGFX.data) return;

	pb=(u8*)PSG_DATA;

	for(pchn=0;pchn<4;pchn++) {
		for(vchn=0;vchn<PSG_VCH_MAX;vchn++) {
			if(PSGFX.chn[pchn].slot[vchn].ptr<0) continue;
			PSGFX.chn[pchn].slot[vchn].time++;
			if(PSGFX.chn[pchn].slot[vchn].wait) {
				PSGFX.chn[pchn].slot[vchn].wait--;
				continue;
			}

			mbyte=PSGFX.data[PSGFX.chn[pchn].slot[vchn].ptr++];
			switch(mbyte&0xc0) {
				case 0x00:/*0=eof 1..31=wait*/
					if(!mbyte) PSGFX.chn[pchn].slot[vchn].ptr=-1; else PSGFX.chn[pchn].slot[vchn].wait=mbyte-1;
					break;
				case 0x40:/*vol only*/
					PSGFX.chn[pchn].slot[vchn].vol=mbyte&0x0f;
					break;
				case 0x80:/*div only*/
					PSGFX.chn[pchn].slot[vchn].div=((u16)mbyte<<8)|PSGFX.data[PSGFX.chn[pchn].slot[vchn].ptr++];
					break;
				case 0xc0:/*vol and div*/
					PSGFX.chn[pchn].slot[vchn].vol=(mbyte>>2)&0x0f;
					PSGFX.chn[pchn].slot[vchn].div=((u16)(mbyte&0x03)<<8)|PSGFX.data[PSGFX.chn[pchn].slot[vchn].ptr++];
					break;
			}
		}

		rchn=-1;
		mvol=16;
		for(vchn=0;vchn<PSG_VCH_MAX;vchn++) {
			if(PSGFX.chn[pchn].slot[vchn].ptr<0) continue;
			nvol=PSGFX.chn[pchn].slot[vchn].vol;
			if(nvol<mvol) {
				mvol=nvol;
				rchn=vchn;
			}
		}

		if(rchn>=0) {
			vchn=rchn;
			rchn=pchn<<5;
			*pb=0x80|0x10|rchn|PSGFX.chn[pchn].slot[vchn].vol;
			div=PSGFX.chn[pchn].slot[vchn].div;
			*pb=0x80|rchn|(div&0x0f);
			*pb=div>>4;
		}
	}
}

ROM_TEXT(_psgfx_addch)
static void _psgfx_addch(u16 chn,u16 off)
{
	i16 i,j,vchn,ntime,tmax,vcnt;

	if(chn<2) {
		tmax=PSG_VCH_MAX;
		for(i=2;i>=0;i--) {
			vcnt=0;
			for(j=0;j<PSG_VCH_MAX;j++) if(PSGFX.chn[i].slot[j].ptr>=0) vcnt++;
			if(vcnt==0) {
				chn=i;
				break;
			}
			if(vcnt<tmax) {
				tmax=vcnt;
				chn=i;
			}
		}
	}

	vchn=-1;
	for(i=0;i<PSG_VCH_MAX;i++) {
		if(PSGFX.chn[chn].slot[i].ptr<0) {
			vchn=i;
			break;
		}
	}
	if(vchn<0) {
		tmax=-1;
		for(i=0;i<PSG_VCH_MAX;i++) {
			ntime=PSGFX.chn[chn].slot[i].time;
			if(ntime>tmax) {
				tmax=ntime;
				vchn=i;
			}
		}
	}

	PSGFX.chn[chn].slot[vchn].ptr=off;
	PSGFX.chn[chn].slot[vchn].wait=0;
	PSGFX.chn[chn].slot[vchn].time=0;
}

ROM_TEXT(psgfx_play)
void psgfx_play(u16 num)
{
	i16 aa,chn,eoff,doff,chcnt;

	if(num>=PSGFX.total) return;
	if(!PSGFX.data) return;

	eoff=2+(num<<1);
	doff=(PSGFX.data[eoff]<<8)+PSGFX.data[eoff+1];
	chcnt=PSGFX.data[doff++];

	for(aa=0;aa<chcnt;aa++) {
		eoff=(PSGFX.data[doff++]<<8);
		eoff+=PSGFX.data[doff++];
		chn=PSGFX.data[eoff++];
		_psgfx_addch(chn,eoff);
	}
}

