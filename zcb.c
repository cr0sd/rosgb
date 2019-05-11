#include<stdio.h>
#include<stdlib.h>
#include<inttypes.h>

// #define PRINTING //cl file.c /UPRINTING

// #ifndef PRINTING
// #define printf(x)
// #endif

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
	
	//08h - 0fh RRC
	if(*op>=0x08 && *op<=0x0f)
	{
		tmp8=(*op&0xf)%8;
		
		if(tmp8==6)
		{
			if(ram[stp->h<<8|stp->l]&0x1)stp->f|=F_C;//C
			else stp->f&=~F_C;
			
			tmp=ram[stp->h<<8|stp->l];
			
			if(stp->f&F_C)_asm stc//set CF
			else _asm clc
			_asm mov al,byte ptr [tmp]
			_asm rcr al,1
			_asm mov byte ptr [tmp],al
			
			ram[stp->h<<8|stp->l]=(uint8_t)tmp;
		}
		else
		{
			if(st[cbreg[tmp8]]&0x1)stp->f|=F_C;//C
			else stp->f&=~F_C;
			
			tmp=st[cbreg[tmp8]];
			
			if(stp->f&F_C)_asm stc//set CF
			else _asm clc
			_asm mov al,byte ptr [tmp]
			_asm rcr al,1
			_asm mov byte ptr [tmp],al
			
			st[cbreg[tmp8]]=(uint8_t)tmp;
		}
		
		stp->f&=~F_N;//N
		stp->f&=~F_H;//H
		if(!(uint8_t)tmp)stp->f|=F_Z;//Z
		else stp->f&=~F_Z;
		
		printf("rrc %s",cbrnm[(*op&0xf)%8]);
	}
	
	/*NOTE: For some reason, RRC/RLC doesn't use CF, but RR/RL does*/
	//00h - 07h RLC
	if(*op>=0x00 && *op<=0x07)
	{
		tmp8=(*op&0xf)%8;
		
		if(tmp8==6)
		{
			if(ram[stp->h<<8|stp->l]&0x80)stp->f|=F_C;//C
			else stp->f&=~F_C;
			
			tmp=ram[stp->h<<8|stp->l];
			
			if(stp->f&F_C)_asm stc//set CF
			else _asm clc
			_asm mov al,byte ptr [tmp]
			_asm rcl al,1
			_asm mov byte ptr [tmp],al
			
			ram[stp->h<<8|stp->l]=(uint8_t)tmp;
		}
		else
		{
			if(st[cbreg[tmp8]]&0x80)stp->f|=F_C;//C
			else stp->f&=~F_C;
			
			tmp=st[cbreg[tmp8]];
			
			if(stp->f&F_C)_asm stc//set CF
			else _asm clc
			_asm mov al,byte ptr [tmp]
			_asm rcl al,1
			_asm mov byte ptr [tmp],al
			
			st[cbreg[tmp8]]=(uint8_t)tmp;
		}
		
		stp->f&=~F_N;//N
		stp->f&=~F_H;//H
		if(!(uint8_t)tmp)stp->f|=F_Z;//Z
		else stp->f&=~F_Z;
		
		printf("rlc %s",cbrnm[(*op&0xf)%8]);
	}
	
	//18h - 1fh RR
	if(*op>=0x18 && *op<=0x1f)
	{
		tmp8=(*op&0xf)%8;
		
		if(tmp8==6)
		{
			uint8_t cf=stp->f&F_C;
			if(ram[stp->h<<8|stp->l]&0x1)stp->f|=F_C;//C
			else stp->f&=~F_C;
			
			tmp=ram[stp->h<<8|stp->l];
			
			
			if(cf)_asm stc//set CF
			else _asm clc
			_asm mov al,byte ptr [tmp]
			_asm rcr al,1
			_asm mov byte ptr [tmp],al
			
			ram[stp->h<<8|stp->l]=(uint8_t)tmp;
		}
		else
		{
			uint8_t cf=stp->f&F_C;
			if(st[cbreg[tmp8]]&0x1)stp->f|=F_C;//C
			else stp->f&=~F_C;
			
			tmp=st[cbreg[tmp8]];
			
			
			if(cf)_asm stc//set CF
			else _asm clc
			_asm mov al,byte ptr [tmp]
			_asm rcr al,1
			_asm mov byte ptr [tmp],al
			
			st[cbreg[tmp8]]=(uint8_t)tmp;
		}
		
		stp->f&=~F_N;//N
		stp->f&=~F_H;//H
		if(!(uint8_t)tmp)stp->f|=F_Z;//Z
		else stp->f&=~F_Z;
		
		printf("rr %s",cbrnm[(*op&0xf)%8]);
	}
	
	//10h - 17h RL
	if(*op>=0x10 && *op<=0x17)
	{
		tmp8=(*op&0xf)%8;
		
		if(tmp8==6)
		{
			uint8_t cf=stp->f&F_C;
			if(ram[stp->h<<8|stp->l]&0x80)stp->f|=F_C;//C
			else stp->f&=~F_C;
			
			tmp=ram[stp->h<<8|stp->l];
			
			
			if(cf)_asm stc//set CF
			else _asm clc
			_asm mov al,byte ptr [tmp]
			_asm rcl al,1
			_asm mov byte ptr [tmp],al
			
			ram[stp->h<<8|stp->l]=(uint8_t)tmp;
		}
		else
		{
			uint8_t cf=stp->f&F_C;
			if(st[cbreg[tmp8]]&0x80)stp->f|=F_C;//C
			else stp->f&=~F_C;
			
			tmp=st[cbreg[tmp8]];
			
			
			if(cf)_asm stc//set CF
			else _asm clc
			_asm mov al,byte ptr [tmp]
			_asm rcl al,1
			_asm mov byte ptr [tmp],al
			
			st[cbreg[tmp8]]=(uint8_t)tmp;
		}
		
		stp->f&=~F_N;//N
		stp->f&=~F_H;//H
		if(!(uint8_t)tmp)stp->f|=F_Z;//Z
		else stp->f&=~F_Z;
		
		printf("rl %s",cbrnm[(*op&0xf)%8]);
	}
	
	//28h - 2fh SRA
	if(*op>=0x28 && *op<=0x2f)
	{
		tmp8=(*op&0xf)%8;
		if(tmp8==6)
		{
			//NOTE: We are depending on arithmetic shift here,
			//		sra is a 'signed' operation
			tmp=ram[stp->h<<8|stp->l]&0x1;
			tmp8=(int8_t)ram[stp->h<<8|stp->l]>>=1;
		}
		else
		{
			tmp=st[cbreg[tmp8]]&0x1;
			tmp8=(int8_t)st[cbreg[tmp8]]>>=1;
		}
		
		if(tmp)stp->f|=F_C;//C
		else stp->f&=~F_C;
		stp->f&=~F_N;//N
		stp->f&=~F_H;//H
		if(!tmp8)stp->f|=F_Z;//Z
		else stp->f&=~F_Z;
		
		printf("sra %s",cbrnm[(*op&0xf)%8]);
	}
	
	//20h - 27h SLA
	if(*op>=0x20 && *op<=0x27)
	{
		tmp8=(*op&0xf)%8;
		if(tmp8==6)
		{
			tmp=ram[stp->h<<8|stp->l]&0x80;
			tmp8=ram[stp->h<<8|stp->l]<<=1;
		}
		else
		{
			tmp=st[cbreg[tmp8]]&0x80;
			tmp8=st[cbreg[tmp8]]<<=1;
			
		}
		
		if(tmp)stp->f|=F_C;//C
		else stp->f&=~F_C;
		stp->f&=~F_N;//N
		stp->f&=~F_H;//H
		if(!tmp8)stp->f|=F_Z;//Z
		else stp->f&=~F_Z;
		
		printf("sla %s",cbrnm[(*op&0xf)%8]);
	}
	
	//30h - 3fh SRL
	if(*op>=0x38 && *op<=0x3f)
	{
		tmp8=(*op&0xf)%8;
		if(tmp8==6)
		{
			//NOTE: We are depending on logical shift here,
			//		srl is an 'unsigned' operation
			tmp=ram[stp->h<<8|stp->l]&0x1;
			tmp8=ram[stp->h<<8|stp->l]>>=1;
		}
		else
		{
			tmp=st[cbreg[tmp8]]&0x1;
			tmp8=st[cbreg[tmp8]]>>=1;
			
		}
		
		if(tmp)stp->f|=F_C;//C
		else stp->f&=~F_C;
		stp->f&=~F_N;//N
		stp->f&=~F_H;//H
		if(!tmp8)stp->f|=F_Z;//Z
		else stp->f&=~F_Z;
		
		printf("srl %s",cbrnm[(*op&0xf)%8]);
	}
	
	//30h - 37h SWAP
	if(*op>=0x30 && *op<=0x37)
	{
		tmp8=(*op&0xf)%8;
		if(tmp8==6)tmp=ram[stp->h<<8|stp->l];
		else tmp=st[cbreg[tmp8]];//reg
		
		_asm mov al,byte ptr [tmp]//x86 allows this in a few lines
		_asm ror al,4
		_asm mov byte ptr [tmp],al//why doesn't C support this?
		
		if(tmp8==6)ram[stp->h<<8|stp->l]=(uint8_t)tmp;
		else st[cbreg[tmp8]]=(uint8_t)tmp;//reg
		
		stp->f&=~F_C;//C
		stp->f&=~F_N;//N
		stp->f&=~F_H;//H
		if(!(uint8_t)tmp)stp->f|=F_Z;//Z
		else stp->f&=~F_Z;
		
		printf("swap %s ; %.2xh",cbrnm[(*op&0xf)%8],(uint8_t)tmp);
	}
	
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