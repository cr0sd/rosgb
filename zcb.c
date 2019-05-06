#include<stdio.h>
#include<stdlib.h>
#include<inttypes.h>

#define PRINTING //cl file.c /UPRINTING

#ifndef PRINTING
#define printf(x)
#endif

#define F_Z	0x80//zero flag
#define F_N	0x40//subtract
#define F_H	0x20//half-carry
#define F_C	0x10//carry
typedef struct St //cpu state
{
	uint8_t a,f,b,c,d,e,h,l; //regs signed, flags unsigned
	uint16_t sp,pc;
}St;
uint8_t cbreg[]={2,3,4,5,6,7,0xff,0}; //(op&0xf)%8
uint8_t *cbrnm[]={"b","c","d","e","h","l","(hl)","a"};
void decexecCB(uint8_t *st,uint8_t *rom,uint8_t *op,uint8_t *ram)
{
	uint16_t tmp;
	uint8_t tmp8;
	St *stp=st;//'union'
	
	//40h - 7fh BIT
	if(*op>=0x40 && *op<=0x80)//why are bitwise ops such low pri??
	{
		tmp=((*op&0xf0)>>4)%4*2+((*op&0x0f)>=8);//bit
		tmp8=(*op&0xf)%8;
		if(tmp8==6)tmp8=ram[stp->h<<8|stp->l];
		else tmp8=st[cbreg[tmp8]];//reg
		if(tmp8&0x1<<tmp)stp->f&=~F_Z;//Z
		else stp->f|=F_Z;
		stp->f|=F_H;//H
		stp->f&=~F_N;//N
		printf("bit %u,%s ;%x (%x)",tmp,cbrnm[(*op&0xf)%8],stp->f&F_Z,tmp8&0x1<<tmp);
		return;
	}
	
	//80h - bfh RES
	if(*op>=0x80 && *op<=0xbf)
	{
		tmp=((*op&0xf0)>>4)%4*2+((*op&0x0f)>=8);//bit
		tmp8=(*op&0xf)%8;
		
		if(tmp8==6)ram[stp->h<<8|stp->l]&=~(1<<tmp);
		else st[cbreg[tmp8]]&=~(1<<tmp);//reg
		
		printf("res %u,%s",tmp,cbrnm[(*op&0xf)%8]);
		return;
	}
	
	//c0h - ffh SET
	if(*op>=0xc0 && *op<=0xff)
	{
		tmp=((*op&0xf0)>>4)%4*2+((*op&0x0f)>=8);//bit
		tmp8=(*op&0xf)%8;
		
		if(tmp8==6)ram[stp->h<<8|stp->l]|=(1<<tmp);
		else st[cbreg[tmp8]]|=(1<<tmp);//reg
		
		printf("set %u,%s",tmp,cbrnm[(*op&0xf)%8]);
		return;
	}
}