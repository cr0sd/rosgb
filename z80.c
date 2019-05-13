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

extern void updatejoypad(uint8_t*ram);
extern void decexecCB(St*,uint8_t*,uint8_t*,uint8_t*);

void fetch8(St *st,uint8_t *rom,uint8_t *u8,uint8_t *ram)
{
	*u8=rom[st->pc++];
}
void fetch16(St *st,uint8_t *rom,uint16_t *u16,uint8_t *ram)
{
	*u16=rom[st->pc]|rom[st->pc+1]<<8;
	st->pc+=2;
}

// _ram Allow access to read/write to ram
// through a filter for interrupts
void write_ram(uint16_t addr,uint8_t val,uint8_t *ram)
{
	ram[addr]=val;
	if(addr=0xff00)updatejoypad(ram);
	return ram+*(uint8_t*)&addr;
}

void romhexdump(uint8_t *rom)
{
	for(uint16_t i=0;i<0xffff;i++)printf("%.2x ",rom[i]);
}
void decexec(St *st,uint8_t *rom,uint8_t *op,uint8_t *ram)
{
	uint16_t tmp;
	uint8_t tmp8;
	switch(*op)
	{
	case 0x01://ld bc,m16
		fetch16(st,rom,&tmp,ram);
		st->b=tmp>>8;
		st->c=tmp&0x00ff;
		printf("ld bc,%.4xh",st->b<<8|st->c);
		break;
	case 0x02://ld (bc),a
		// *_ram(st->b<<8|st->c,ram)=st->a;
		// ram[st->b<<8|st->c]=st->a;
		write_ram(st->b<<8|st->c,st->a,ram);
		printf("ld (bc),a ;(%.4xh)=%.2xh",st->b<<8|st->c,st->a);
		break;
	case 0x03://inc bc
		tmp=((uint16_t)st->b<<8|st->c)+1; //lsh lower pri than add
		st->b=tmp>>8;
		st->c=tmp&0x00ff;
		printf("inc bc ;%.4xh",st->b<<8|st->c);
		break;
	case 0x04://inc b
		//flags:
		st->f&=~F_N;//N 0
		if(st->b&0xf==0xf)st->f|=F_H;//H
		else st->f&=~F_H;
		st->b++;
		if(st->b==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		printf("inc b");
		break;
	case 0x05://dec b
		if(st->b&0x10&&!(st->b&0x08))st->f|=F_H;//H
		else st->f&=~F_H;
		st->b--;
		if(st->b==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("dec b");
		break;
	case 0x06://ld b,m8
		fetch8(st,rom,&st->b,ram);
		printf("ld b,%.2xh",st->b);
		break;
	case 0x07://rlca
		if(st->a&0x80)st->f|=F_C;else st->f&=~F_C;//C
		st->f&=F_C;//N 0,Z 0,H 0
		tmp8=st->a;
		if(st->f&F_C)_asm stc
		else _asm clc
		_asm rcl tmp8,1
		st->a=tmp8;
		printf("rlca ;%.2x",tmp8);
		break;
	case 0x08://ld (m16),sp
		fetch16(st,rom,&tmp,ram);
		// ram[tmp]=st->sp;
		write_ram(tmp,st->sp,ram);
		printf("ld (%.4xh),sp",tmp);
		break;
	case 0x09://add hl,bc
		tmp=st->h<<8|st->l;
		tmp+=st->b<<8|st->c;
		st->b=tmp>>8;
		st->c=(uint8_t)tmp;//trunc
		printf("add hl,bc");
		if(tmp<(st->b<<8|st->c))st->f|=F_C;//C
		else st->f&=~F_C;
		st->f&=~F_N;//N
		//H
		// if(st->f==0)st->f|=F_Z;else st->f&=~F_Z;//Z -
		break;
	case 0x0a: //ld a,(bc)
		st->a=ram[st->b<<8|st->c];
		printf("ld a,(bc)");
		break;
	case 0x0b: //dec bc
		tmp=((uint16_t)st->b<<8|st->c)-1;
		st->b=tmp>>8;
		st->c=(uint8_t)tmp;
		printf("dec bc");
		break;
	case 0x0c: //inc c
		st->c++;
		if(st->c==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		if(st->c&0xf==0xf)st->f|=F_H;//H
		else st->f&=~F_H;
		printf("inc c");
		break;
	case 0x0d: //dec c
		if(st->c&0x10&&!(st->c&0x08))st->f|=F_H;//H
		else st->f&=~F_H;
		st->c--;
		if(st->c==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("dec c ;%.2xh",st->c);
		break;
	case 0x0e: //ld c,m8
		fetch8(st,rom,&st->c,ram);
		printf("ld c,%.2xh",st->c);
		break;
	case 0x0f: //rrca
		if(st->a&0x01)st->f|=F_C;else st->f&=~F_C;//C
		st->f&=F_C;//N 0,Z 0,H 0
		tmp8=st->a;
		_asm ror tmp8,1
		st->a=tmp8;
		printf("rrca ;%.2xh",tmp8);
		break;
	case 0x10://stop 0x00
		fetch8(st,rom,&tmp,ram);//second byte of opcode
		printf("stop %.2xh ;10h 00h",tmp);
		break;
	case 0x11://ld de,m16
		fetch16(st,rom,&tmp,ram);
		st->d=tmp>>8;
		st->e=tmp&0x00ff;
		printf("ld de,%.4xh",tmp);
		break;
	case 0x12://ld (de),a
		// ram[st->d<<8|st->e]=st->a;
		write_ram(st->d<<8|st->e,st->a,ram);
		printf("ld (de),a");
		break;
	case 0x13://inc de
		tmp=((uint16_t)st->d<<8|st->e)+1;
		st->d=tmp>>8;
		st->e=tmp&0x00ff;
		printf("inc de");
		break;
	case 0x14://inc d
		st->d++;
		if(st->d==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		if(st->d&0xf==0xf)st->f|=F_H;//H
		else st->f&=~F_H;
		printf("inc d");
		break;
	case 0x15://dec d
		if(st->d&0x10&&!(st->d&0x08))st->f|=F_H;//H
		else st->f&=~F_H;
		st->d--;
		if(st->d==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("dec d");
		break;
	case 0x16://ld d,m8
		fetch8(st,rom,&st->d,ram);
		printf("ld d,%.2xh",st->d);
		break;
	case 0x17://rla
		if(st->a&0x80)st->f|=F_C;else st->f&=~F_C;//C
		st->f&=F_C;//N 0,Z 0,H 0
		tmp8=st->a;
		_asm rol tmp8,1
		st->a=tmp8;
		printf("rla");
		break;
	case 0x18://jr m8 (signed)
		fetch8(st,rom,&tmp,ram);
		st->pc+=(int8_t)tmp;//cast to signed int8, C will sign extend
		printf("jr %.2xh ;%.2xh",(uint8_t)tmp,st->pc);
		break;
	case 0x19://add hl,de
		tmp=st->h<<8|st->l;
		tmp+=st->d<<8|st->e;
		st->h=tmp>>8;
		st->l=(uint8_t)tmp;//trunc
		printf("add hl,de");
		if(tmp<(st->d<<8|st->e))st->f|=F_C;else st->f&=~F_C;//C
		st->f&=~F_N;//N
		//H
		// if(st->f==0)st->f|=F_Z;else st->f&=~F_Z;//Z -
		break;
	case 0x1a: //ld a,(de)
		st->a=ram[st->d<<8|st->e];
		printf("ld a,(de)");
		break;
	case 0x1b: //dec de
		tmp=((uint16_t)st->d<<8|st->e)-1;
		st->d=tmp>>8;
		st->e=(uint8_t)tmp;
		printf("dec de");
		break;
	case 0x1c: //inc e
		st->e++;
		if(st->e==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		if(st->e&0xf==0xf)st->f|=F_H;//H
		else st->f&=~F_H;
		printf("inc e");
		break;
	case 0x1d: //dec e
		if(st->e&0x10&&!(st->e&0x08))st->f|=F_H;
		else st->f&=~F_H;//borrow H
		st->e--;
		if(st->e==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("dec c");
		break;
	case 0x1e: //ld e,m8
		fetch8(st,rom,&st->e,ram);
		printf("ld e,%.2xh",st->e);
		break;
	case 0x1f: //rra //TODO: fix behavior of rotate
		{
			//create new call stack
			uint8_t cf=st->f&F_C;
			if(st->a&0x01)st->f|=F_C;else st->f&=~F_C;//C
			st->f&=F_C;//Z 0,H 0,N 0
			tmp=st->a;
			tmp8=st->f&F_C;
			
			if(cf)_asm stc //set CF
			else _asm clc
			_asm rcr byte ptr [tmp],1
			
			
			st->a=(uint8_t)tmp;
			printf("rra ; %.2xh",(uint8_t)tmp);
			break;
		}
	case 0x20://jr nz m8 (signed)
		fetch8(st,rom,&tmp,ram);
		if(!(st->f&F_Z))
			st->pc+=(int8_t)tmp;//m8 is signed!
		printf("jr nz %.2xh%s",(uint8_t)tmp,(!(st->f&F_Z))?(""):(" ;skipped"));
		break;
	case 0x21://ld hl,m16
		fetch16(st,rom,&tmp,ram);
		st->h=tmp>>8;
		st->l=tmp&0x00ff;
		printf("ld hl,%.4xh",tmp);
		break;
	case 0x22://ld (hli),a
		tmp=(uint16_t)st->h<<8|st->l;
		// ram[tmp]=st->a;
		write_ram(tmp,st->a,ram);
		tmp++;
		st->h=tmp>>8;
		st->l=tmp&0x00ff;
		printf("ld (hli),a ;%.4x",tmp);
		break;
	case 0x23://inc hl
		tmp=((uint16_t)st->h<<8|st->l)+1;
		st->h=tmp>>8;
		st->l=(uint8_t)tmp;//trunc
		printf("inc hl");
		break;
	case 0x24: //inc h
		st->h++;
		printf("inc h");
		if(st->h==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		if(st->h&0xf==0xf)st->f|=F_H;//H
		else st->f&=~F_H;
		break;
	case 0x25: //dec h
		if(st->h&0x10&&!(st->h&0x08))st->f|=F_H;
		else st->f&=~F_H;//borrow H
		st->h--;
		if(st->h==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("dec h");
		break;
	case 0x26://ld h,m8
		fetch8(st,rom,&st->h,ram);
		printf("ld h,%.2xh",st->h);
		break;
	case 0x27://daa ;decimal adjust a (BCD)
		printf("daa");
		if(st->a==0)st->f|=F_Z;//Z
		st->f&=~F_H;//H
		//N -
		//C
		break;
	case 0x28://jr z m8
		fetch8(st,rom,&tmp,ram);
		if(st->f&F_Z)st->pc+=(int8_t)tmp;
		printf("jr z %.2xh ;%.2xh%s",(uint8_t)tmp,st->pc,(st->f&F_Z)?(""):(" ;skipped"));
		break;
	case 0x29://add hl,hl
		tmp=st->h<<8|st->l;
		tmp+=tmp;
		st->h=tmp>>8;
		st->l=(uint8_t)tmp;
		printf("add hl,hl ;%.4xh",tmp);
		st->f&=~F_N;//N
		//C
		//H
		break;
	case 0x2a: //ld a,(hli) ld a,(hl) ; inc hl
		tmp=(uint16_t)st->h<<8|st->l;
		st->a=ram[tmp];
		tmp++;
		st->h=tmp>>8;
		st->l=tmp&0x00ff;
		printf("ld a,(hli) ;(%.4xh)",tmp);
		break;
	case 0x2b: //dec hl
		tmp=((uint16_t)st->h<<8|st->l)-1;
		st->h=tmp>>8;
		st->l=(uint8_t)tmp;
		printf("dec hl");
		break;
	case 0x2c: //inc l
		st->l++;
		if(st->l==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		printf("inc l");
		break;
	case 0x2d: //dec l
		if(st->l&0x10&&!(st->l&0x08))st->f|=F_H;
		else st->f&=~F_H;//H
		st->l--;
		if(st->l==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("dec l");
		break;
	case 0x2e: //ld l,m8
		fetch8(st,rom,&st->l,ram);
		printf("ld l,%.2xh",st->l);
		break;
	case 0x2f: //cpl ;cpl a
		st->a=~st->a;
		st->f|=F_H;
		st->f|=F_N;
		printf("cpl ;cpl a");
		break;
	case 0x30://jr nc m8 (signed)
		fetch8(st,rom,&tmp,ram);
		if(!(st->f&F_C))
			st->pc+=(int8_t)tmp;//m8 is signed!
		printf("jr nc %.2xh%s",(uint8_t)tmp,(!(st->f&F_C))?(""):(" ;skipped"));
		break;
	case 0x31://ld sp,m16
		fetch16(st,rom,&st->sp,ram);
		printf("ld sp,%.4xh",st->sp);
		break;
	case 0x32://ld (hld),a ld (hl),a ; dec hl
		tmp=(uint16_t)st->h<<8|st->l;
		// ram[tmp]=st->a;
		write_ram(tmp,st->a,ram);
		tmp--;
		st->h=tmp>>8;
		st->l=tmp&0x00ff;
		printf("ld (hld),a ;(%.4xh)",tmp);
		break;
	case 0x33://inc sp
		st->sp++;
		printf("inc sp");
		break;
	case 0x34://inc (hl)
		tmp=st->h|st->l<<8;
		// ram[tmp]++;
		write_ram(tmp,ram[tmp]+1,ram);
		printf("inc (hl) ;%.4xh",ram[tmp]);
		break;
	case 0x35://dec (hl)
		tmp=st->h|st->l<<8;
		// ram[tmp]--;
		write_ram(tmp,ram[tmp]-1,ram);
		printf("dec (hl) ;%.4xh",ram[tmp]);
		break;
	case 0x36://ld (hl),m8
		fetch8(st,rom,&tmp,ram);
		// ram[st->h<<8|st->l]=(uint8_t)tmp;
		write_ram(st->h<<8|st->l,(uint8_t)tmp,ram);
		printf("ld (hl),%.2xh",(uint8_t)tmp);
		break;
	case 0x37://scf
		st->f&=F_C|F_Z;//reset F_H,F_N
		st->f|=F_C;//set F_C
		printf("scf ;%.2xh",st->f);
		break;
	case 0x38://jr c,m8
		fetch8(st,rom,&tmp,ram);
		if(st->f&F_C)
			st->pc+=(int8_t)tmp;//m8 is signed!
		printf("jr c %.2xh%s",(uint8_t)tmp,(!(st->f&F_C))?(""):(" ;skipped"));
		break;
	case 0x39://add hl,sp
		tmp=st->h<<8|st->l;
		tmp+=st->sp;
		st->h=tmp>>8;
		st->l=(uint8_t)tmp;//trunc
		printf("add hl,sp");
		break;
	case 0x3a: //ld a,(hld) ld a,(hl) ; dec hl
		tmp=(uint16_t)st->h<<8|st->l;
		st->a=ram[tmp];
		tmp--;
		st->h=tmp>>8;
		st->l=tmp|0x00ff;
		printf("ld a,(hld) ;(%.4xh)",tmp);
		break;
	case 0x3b: //dec sp
		st->sp--;
		printf("dec sp");
		break;
	case 0x3c: //inc a
		st->a++;
		if(st->a==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		printf("inc a");
		break;
	case 0x3d: //dec a
		st->a--;
		if(!st->a)st->f|=F_Z;
		else st->f&=~F_Z;
		st->f|=F_N;
		//F_H
		printf("dec a");
		break;
	case 0x3e: //ld a,m8
		fetch8(st,rom,&st->a,ram);
		printf("ld a,%.2xh",st->a);
		break;
	case 0x3f: //ccf ;complement F_C flag
		//flags
		st->f=(st->f&F_C)?st->f&~F_C:st->f|F_C;//cpl C
		//Z -
		st->f&=~(F_N|F_H);//N 0,H 0
		printf("ccf ;%.2x",st->f&F_C);
		break;
	case 0x40://ld b,b
		printf("ld b,b");
		break;
	case 0x41://ld b,c
		st->b=st->c;
		printf("ld b,c");
		break;
	case 0x42://ld b,d
		st->b=st->d;
		printf("ld b,d");
		break;
	case 0x43://ld b,e
		st->b=st->e;
		printf("ld b,e");
		break;
	case 0x44://ld b,h
		st->b=st->h;
		printf("ld b,h");
		break;
	case 0x45://ld b,l
		st->b=st->l;
		printf("ld b,l");
		break;
	case 0x46://ld b,(hl)
		tmp=st->h<<8|st->l;
		st->b=ram[tmp];
		printf("ld b,(hl)");
		break;
	case 0x47://ld b,a
		st->b=st->a;
		printf("ld b,a");
		break;
	case 0x48://ld c,b
		st->c=st->b;
		printf("ld c,b");
		break;
	case 0x49://ld c,c
		printf("ld c,c");
		break;
	case 0x4a: //ld c,d
		st->c=st->d;
		printf("ld c,d");
		break;
	case 0x4b: //ld c,e
		st->c=st->e;
		printf("ld c,e");
		break;
	case 0x4c: //ld c,h
		st->c=st->h;
		printf("ld c,h");
		break;
	case 0x4d: //ld c,l
		st->c=st->l;
		printf("ld c,l");
		break;
	case 0x4e: //ld c,(hl)
		tmp=st->h<<8|st->l;
		st->c=ram[tmp];
		printf("ld c,(hl)");
		break;
	case 0x4f: //ld c,a
		st->c=st->a;
		printf("ld c,a");
		break;
	case 0x50: //ld d,b
		st->d=st->b;
		printf("ld d,b");
		break;
	case 0x51: //ld d,c
		st->d=st->c;
		printf("ld d,c");
		break;
	case 0x52: //ld d,d
		printf("ld d,d");
		break;
	case 0x53: //ld d,e
		st->d=st->e;
		printf("ld d,e");
		break;
	case 0x54: //ld d,h
		st->d=st->h;
		printf("ld d,h");
		break;
	case 0x55: //ld d,l
		st->d=st->l;
		printf("ld d,l");
		break;
	case 0x56: //ld d,(hl)
		tmp=st->h<<8|st->l;
		st->d=ram[tmp];
		printf("ld d,(hl)");
		break;
	case 0x57: //ld d,a
		st->d=st->a;
		printf("ld d,a");
		break;
	case 0x58: //ld e,b
		st->e=st->b;
		printf("ld e,b");
		break;
	case 0x59: //ld e,c
		st->e=st->c;
		printf("ld e,c");
		break;
	case 0x5a: //ld e,d
		st->e=st->d;
		printf("ld e,d");
		break;
	case 0x5b: //ld e,e
		printf("ld e,e");
		break;
	case 0x5c: //ld e,h
		st->e=st->h;
		printf("ld e,h");
		break;
	case 0x5d: //ld e,l
		st->e=st->l;
		printf("ld e,l");
		break;
	case 0x5e: //ld e,(hl)
		tmp=st->h<<8|st->l;
		st->e=ram[tmp];
		printf("ld e,(hl)");
		break;
	case 0x5f: //ld e,a
		st->e=st->a;
		printf("ld e,a");
		break;
	case 0x60: //ld h,b
		st->h=st->b;
		printf("ld h,b");
		break;
	case 0x61: //ld h,c
		st->h=st->c;
		printf("ld h,c");
		break;
	case 0x62: //ld h,d
		st->h=st->d;
		printf("ld h,d");
		break;
	case 0x63: //ld h,e
		st->h=st->e;
		printf("ld h,e");
		break;
	case 0x64: //ld h,h
		printf("ld h,h");
		break;
	case 0x65: //ld h,l
		st->h=st->l;
		printf("ld h,l");
		break;
	case 0x66: //ld h,(hl)
		tmp=st->h<<8|st->l;
		st->h=ram[tmp];
		printf("ld h,(hl)");
		break;
	case 0x67: //ld h,a
		st->h=st->a;
		printf("ld h,a");
		break;
	case 0x68: //ld l,b
		st->l=st->b;
		printf("ld l,b");
		break;
	case 0x69: //ld l,c
		st->l=st->c;
		printf("ld l,c");
		break;
	case 0x6a: //ld l,d
		st->l=st->d;
		printf("ld l,d");
		break;
	case 0x6b: //ld l,e
		st->l=st->e;
		printf("ld l,e");
		break;
	case 0x6c: //ld l,h
		st->l=st->h;
		printf("ld l,h");
		break;
	case 0x6d: //ld l,l
		printf("ld l,l");
		break;
	case 0x6e: //ld l,(hl)
		tmp=st->h<<8|st->l;
		st->l=ram[tmp];
		printf("ld l,(hl)");
		break;
	case 0x6f: //ld l,a
		st->l=st->a;
		printf("ld l,a");
		break;
	case 0x70: //ld (hl),b
		tmp=st->h<<8|st->l;
		// ram[tmp]=st->b;
		write_ram(tmp,st->b,ram);
		printf("ld (hl),b ;ld (%.4xh),%.2xh",tmp,st->b);
		break;
	case 0x71: //ld (hl),c
		tmp=st->h<<8|st->l;
		// ram[tmp]=st->c;
		write_ram(tmp,st->c,ram);
		printf("ld (hl),c");
		break;
	case 0x72: //ld (hl),d
		tmp=st->h<<8|st->l;
		// ram[tmp]=st->d;
		write_ram(tmp,st->d,ram);
		printf("ld (hl),d");
		break;
	case 0x73: //ld (hl),e
		tmp=st->h<<8|st->l;
		// ram[tmp]=st->e;
		write_ram(tmp,st->e,ram);
		printf("ld (hl),e");
		break;
	case 0x74: //ld (hl),h
		tmp=st->h<<8|st->l;
		// ram[tmp]=st->h;
		write_ram(tmp,st->h,ram);
		printf("ld (hl),h");
		break;
	case 0x75: //ld (hl),l
		tmp=st->h<<8|st->l;
		// ram[tmp]=st->l;
		write_ram(tmp,st->l,ram);
		printf("ld (hl),l");
		break;
	case 0x76: //halt
		st->pc--;//stay put
		printf("halt");
		break;
	case 0x77: //ld (hl),a
		tmp=st->h<<8|st->l;
		// ram[tmp]=st->a;
		write_ram(tmp,st->a,ram);
		printf("ld (hl),a ;ld (%.4xh),%.2xh",tmp,st->a);
		break;
	case 0x78: //ld a,b
		st->a=st->b;
		printf("ld a,b");
		break;
	case 0x79: //ld a,c
		st->a=st->c;
		printf("ld a,c");
		break;
	case 0x7a: //ld a,d
		st->a=st->d;
		printf("ld a,d");
		break;
	case 0x7b: //ld a,e
		st->a=st->e;
		printf("ld a,e");
		break;
	case 0x7c: //ld a,h
		st->a=st->h;
		printf("ld a,h");
		break;
	case 0x7d: //ld a,l
		st->a=st->l;
		printf("ld a,l");
		break;
	case 0x7e: //ld a,(hl)
		tmp=st->h<<8|st->l;
		st->a=ram[tmp];
		printf("ld a,(hl)");
		break;
	case 0x7f: //ld a,a
		printf("ld a,a");
		break;
	case 0x80://add b
		st->a+=st->b;
		st->f&=~F_N;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<st->b)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("add b");
		break;
	case 0x81://add c
		st->a+=st->c;
		st->f&=~F_N;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<st->c)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("add c");
		break;
	case 0x82://add d
		st->a+=st->d;
		st->f&=~F_N;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<st->d)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("add d");
		break;
	case 0x83://add e
		st->a+=st->e;
		st->f&=~F_N;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<st->e)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("add e");
		break;
	case 0x84://add h
		st->a+=st->h;
		st->f&=~F_N;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<st->h)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("add h");
		break;
	case 0x85://add l
		st->a+=st->l;
		st->f&=~F_N;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<st->l)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("add l ;%x",st->f&F_C);
		break;
	case 0x86://add (hl)
		tmp=(uint16_t)st->h<<8|st->l;
		st->a+=ram[tmp];
		st->f&=~F_N;//N
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<st->h)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("add (hl) ;%.4xh",tmp);
		break;
	case 0x87://add a
		tmp=st->a;
		st->a+=st->a;
		st->f&=~F_N;//N
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(tmp<st->a)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("add a");
		break;
	case 0x88://adc b
		st->a+=st->b+(st->f&F_C)!=0;
		st->f&=~F_N;//N
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<st->b)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("adc b");
		break;
	case 0x89://adc c
		st->a+=st->c+(st->f&F_C)!=0;
		st->f&=~F_N;//N
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<st->c)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("adc c");
		break;
	case 0x8a: //adc d
		st->a+=st->d+(st->f&F_C)!=0;
		st->f&=~F_N;//N
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<st->d)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("adc d");
		break;
	case 0x8b: //adc e
		st->a+=st->e+(st->f&F_C)!=0;
		st->f&=~F_N;//N
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<st->e)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("adc e");
		break;
	case 0x8c: //adc h
		st->a+=st->h+(st->f&F_C)!=0;
		st->f&=~F_N;//N
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<st->h)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("adc h ;%x",st->f&F_C);
		break;
	case 0x8d: //adc a,l
		st->a+=st->l+!!(st->f&F_C);
		st->f&=~F_N;
		if(!st->a)st->f|=F_Z;
		else st->f&=~F_Z;
		//F_H
		//F_C
		printf("adc a,l");
		break;
	case 0x8e: //adc a,(hl)
		tmp=st->h<<8|st->l;
		st->a+=ram[tmp]+!!(st->f&F_C);
		if(st->a==0)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		//F_H
		//F_C
		printf("adc a,(hl) ;%.4xh",tmp);
		break;
	case 0x8f: //adc a,a
		st->a+=st->a+!!(st->f&F_C);
		if(st->a==0)st->f|=F_Z;
		st->f&=~F_N;
		if(!st->a)st->f|=F_Z;
		//F_H
		//F_C
		printf("adc a,a");
		break;
	case 0x90://sub b
		if(st->a<st->b)st->f&=~F_C;//C
		else st->f|=F_C;
		st->a-=st->b;
		if(!(st->a))st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("sub b");
		break;
	case 0x91://sub c
		if(st->a<st->c)st->f&=~F_C;//C
		else st->f|=F_C;
		st->a-=st->c;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("sub c");
		break;
	case 0x92://sub d
		if(st->a<st->d)st->f&=~F_C;//C //SUB CARRY IS REVERSED
		else st->f|=F_C;
		st->a-=st->d;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("sub d");
		break;
	case 0x93://sub e
		if(st->a<st->e)st->f&=~F_C;//C //SUB CARRY IS REVERSED
		else st->f|=F_C;
		st->a-=st->e;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("sub e");
		break;
	case 0x94://sub h
		if(st->a<st->h)st->f&=~F_C;//C
		else st->f|=F_C;
		st->a-=st->h;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("sub h");
		break;
	case 0x95://sub l
		if(st->a<st->l)st->f&=~F_C;//C //SUB CARRY IS REVERSED
		else st->f|=F_C;
		st->a-=st->l;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("sub l ;C:%x",(st->f&F_C));
		break;
	case 0x96://sub (hl)
		tmp=(uint16_t)st->h<<8|st->l;
		if(st->a<tmp)st->f&=~F_C;//C
		else st->f|=F_C;
		st->a-=ram[tmp];
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		printf("sub (hl)");
		break;
	case 0x97://sub a
		st->a-=st->a;
		st->f&=~F_C;//C
		st->f|=F_Z;//Z
		st->f|=F_N;//N
		printf("sub a");
		break;
	case 0x98://sbc b
		tmp=st->a;
		st->a-=st->b+!(st->f&F_C);
		if(tmp<st->b)st->f&=~F_C;//C
		else st->f|=F_C;
		st->f|=F_Z;//zero flag
		st->f|=F_N;//N
		printf("sbc b");
		break;
	case 0x99://sbc c
		tmp=st->a;
		st->a-=st->c+!(st->f&F_C);
		if(tmp<st->c)st->f&=~F_C;//C
		else st->f|=F_C;
		st->f|=F_Z;//zero flag
		st->f|=F_N;
		printf("sbc c");
		break;
	case 0x9a: //sbc d
		tmp=st->a;
		st->a-=st->d+!(st->f&F_C);
		if(tmp<st->d)st->f&=~F_C;//C
		else st->f|=F_C;
		st->f|=F_Z;//zero flag
		st->f|=F_N;
		printf("sbc d");
		break;
	case 0x9b: //sbc e
		tmp=st->a;
		st->a-=st->e+!(st->f&F_C);
		if(tmp<st->e)st->f&=~F_C;//C
		else st->f|=F_C;
		st->f|=F_Z;//zero flag
		st->f|=F_N;
		printf("sbc e");
		break;
	case 0x9c: //sbc h
		tmp=st->a;
		st->a-=st->h+!(st->f&F_C);
		if(tmp<st->h)st->f&=~F_C;//C
		else st->f|=F_C;
		st->f|=F_Z;//zero flag
		st->f|=F_N;
		printf("sbc h ;C:%x",(st->f&F_C));
		break;
	case 0x9d: //sbc l
		tmp=st->a;
		st->a-=st->l+!(st->f&F_C);
		if(tmp<st->l)st->f&=~F_C;//C
		else st->f|=F_C;
		st->f|=F_Z;//zero flag
		st->f|=F_N;
		printf("sbc l ;C:%u",!!st->f&F_C);
		break;
	case 0x9e: //sbc (hl)
		tmp=(uint16_t)st->h<<8|st->l;
		tmp8=st->a;
		st->a-=ram[tmp]+!(st->f&F_C);
		if(tmp8<ram[tmp])st->f&=~F_C;//C
		else st->f|=F_C;
		st->f|=F_Z;//zero flag
		st->f|=F_N;
		printf("sbc (hl)");
		break;
	case 0x9f: //sbc a
		st->a-=st->a+!(st->f&F_C);
		st->f|=F_C;//C
		st->f|=F_Z;//Z
		st->f|=F_N;//N
		printf("sbc (hl)");
		break;
	case 0xa0://and b
		st->a&=st->b;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f|=F_H;//H
		st->f&=~F_C;//C
		printf("and b");
		break;
	case 0xa1://and c
		st->a&=st->c;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f|=F_H;//H
		st->f&=~F_C;//C
		printf("and c");
		break;
	case 0xa2://and d
		st->a&=st->d;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f|=F_H;//H
		st->f&=~F_C;//C
		printf("and d");
		break;
	case 0xa3://and e
		st->a&=st->e;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f|=F_H;//H
		st->f&=~F_C;//C
		printf("and e");
		break;
	case 0xa4://and h
		st->a&=st->h;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f|=F_H;//H
		st->f&=~F_C;//C
		printf("and h");
		break;
	case 0xa5://and l
		st->a&=st->l;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f|=F_H;//H
		st->f&=~F_C;//C
		printf("and l");
		break;
	case 0xa6://and (hl)
		tmp=st->h<<8|st->l;
		st->a&=ram[tmp];
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f|=F_H;//H
		st->f&=~F_C;//C
		printf("and (hl)");
		break;
	case 0xa7://and a
		// st->a&=st->a;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f|=F_H;//H
		st->f&=~F_C;//C
		printf("and a");
		break;
	case 0xa8://xor b
		st->a^=st->b;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("xor b");
		break;
	case 0xa9://xor c
		st->a^=st->c;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("xor c");
		break;
	case 0xaa: //xor d
		st->a^=st->d;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("xor d");
		break;
	case 0xab: //xor e
		st->a^=st->e;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("xor e");
		break;
	case 0xac: //xor h
		st->a^=st->h;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("xor h");
		break;
	case 0xad: //xor l
		st->a^=st->l;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("xor l");
		break;
	case 0xae: //xor (hl)
		tmp=st->h<<8|st->l;
		st->a^=ram[tmp];
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("xor (hl)");
		break;
	case 0xaf: //xor a
		st->a=0;//xor X,X is always zero
		st->f|=F_Z;//Z
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("xor a");
		break;
	case 0xb0://or b
		st->a|=st->b;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("or b");
		break;
	case 0xb1://or c
		st->a|=st->c;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("or c");
		break;
	case 0xb2://or d
		st->a|=st->d;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("or d");
		break;
	case 0xb3://or e
		st->a|=st->e;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("or e");
		break;
	case 0xb4://or h
		st->a|=st->h;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("or h");
		break;
	case 0xb5://or l
		st->a|=st->l;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("or l");
		break;
	case 0xb6://or (hl)
		tmp=st->h<<8|st->l;
		st->a|=ram[tmp];
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("or (hl)");
		break;
	case 0xb7://or a
		// st->a|=st->a;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("or a");
		break;
	case 0xb8://cp b
		tmp=st->a-st->b;
		if(!tmp)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		//H
		if(st->a<st->b)st->f|=F_C;//C
		else st->f&=~F_C;//C
		printf("cp b");
		break;
	case 0xb9://cp c
		tmp=st->a-st->c;
		if(!tmp)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		//H
		if(st->a<st->c)st->f|=F_C;//C
		else st->f&=~F_C;//C
		printf("cp c");
		break;
	case 0xba: //cp d
		tmp=st->a-st->d;
		if(!tmp)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		//H
		if(st->a<st->d)st->f|=F_C;//C
		else st->f&=~F_C;//C
		printf("cp d");
		break;
	case 0xbb: //cp e
		tmp=st->a-st->e;
		if(!tmp)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		//H
		if(st->a<st->e)st->f|=F_C;//C
		else st->f&=~F_C;//C
		printf("cp e");
		break;
	case 0xbc: //cp h
		tmp=st->a-st->h;
		if(!tmp)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		//H
		if(st->a<st->h)st->f|=F_C;//C
		else st->f&=~F_C;//C
		printf("cp h");
		break;
	case 0xbd: //cp l
		tmp=st->a-st->l;
		if(!tmp)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		//H
		if(st->a<st->l)st->f|=F_C;//C
		else st->f&=~F_C;//C
		printf("cp l");
		break;
	case 0xbe: //cp (hl)
		tmp=ram[st->h<<8|st->l];
		tmp8=st->a-tmp;
		if(!tmp)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_N;//N
		//H
		if(st->a<tmp)st->f|=F_C;//C
		else st->f&=~F_C;//C
		printf("cp (hl)");
		break;
	case 0xbf: //cp a
		st->f|=F_Z;//Z
		st->f|=F_N;//N
		//H
		st->f&=~F_C;//C
		printf("cp a");
		break;
	case 0xc0://ret nz
		//pop ret val from stack!
		if(!(st->f&F_Z))
		{
			st->pc=ram[st->sp]|ram[st->sp+1]<<8;
			st->sp+=2;
		}
		printf("ret nz%s",st->f&F_Z?" ;skipped":"");
		break;
	case 0xc1://pop bc
		st->c=ram[st->sp];//little-endian
		st->b=ram[st->sp+1];
		st->sp+=2;
		printf("pop bc ;%.4xh",st->b<<8|st->c);
		break;
	case 0xc2://jp nz,m16
		fetch16(st,rom,&tmp,ram);
		if(!(st->f&F_Z))st->pc=tmp;
		printf("jp nz,%.4xh%s",tmp,!st->a?" ;skipped":"");
		break;
	case 0xc3://jp m16
		fetch16(st,rom,&st->pc,ram);
		st->pc-=2;//fetch incr's pc
		printf("jp %.4xh",st->pc);
		break;
	case 0xc4://call nz,m16
		fetch16(st,rom,&tmp,ram);
		if(!(st->f&F_Z))
		{
			st->sp-=2;
			// ram[st->sp]  =(uint8_t)st->pc;//push pc
			// ram[st->sp+1]=st->pc>>8;
			write_ram(st->sp,(uint8_t)st->pc,ram);
			write_ram(st->sp+1,st->pc>>8,ram);
			st->pc=tmp;
		}
		printf("call nz,%.4xh%s",tmp,!st->a?" ;skipped":"");
		break;
	case 0xc5://push bc
		st->sp-=2;
		// ram[st->sp]=st->c;//little-endian
		// ram[st->sp+1]=st->b;
		write_ram(st->sp,st->c,ram);
		write_ram(st->sp+1,st->b,ram);
		printf("push bc ;%.4xh",st->b<<8|st->c);
		break;
	case 0xc6://add a,m8
		fetch8(st,rom,&tmp,ram);
		if((st->a&0x0f)+(tmp&0x0f)&0x10)st->f|=F_H;//F_H
		else st->f&=~F_H;
		st->a+=tmp;
		if(st->a==0)st->f|=F_Z;//Z
		st->f&=~F_N;//N
		if(st->a<tmp)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("add a,%.2xh",tmp);
		break;
	case 0xc7: //rst 00h (call 0000h + 00h)
		// fetch16(st,rom,&tmp,ram);
		st->sp-=2;//alloc
		// ram[st->sp]=st->pc&0x00ff;
		// ram[st->sp+1]=st->pc>>8;
		write_ram(st->sp,st->pc&0x00ff,ram);
		write_ram(st->sp+1,st->pc>>8,ram);
		st->pc=0x0000;
		printf("rst 00h ;ret to %.4xh",ram[st->sp]|ram[st->sp+1]<<8);
		break;
	case 0xc8: //ret z //ret addr stays on stack if skipped!
		if(st->f&F_Z)
		{
			st->pc=ram[st->sp]|ram[st->sp+1]<<8;
			st->sp+=2;
		}
		printf("ret z%s",st->f&F_Z?"":" ;skipped");
		break;
	case 0xc9://ret
		//pop ret val from stack!
		st->pc=ram[st->sp]|ram[st->sp+1]<<8;
		st->sp+=2;
		printf("ret");
		break;
	case 0xca: //jp z,m16
		fetch16(st,rom,&tmp,ram);
		if(st->f&F_Z)st->pc=tmp;
		printf("jp z%s",st->f&F_Z?"":" ;skipped");
		break;
	case 0xcb: //PREFIX CB
		//IX CBPREF IX CBPREF IX CBPR EF IXCBPR EFIX CBPR
		fetch8(st,rom,&tmp8,ram);
		printf("0x%.2x: ",tmp8);
		decexecCB(st,rom,&tmp8,ram);
		break;
	case 0xcc: //call z,m16
		fetch16(st,rom,&tmp,ram);
		if(st->f&F_Z)
		{
			st->sp-=2;
			// ram[st->sp]  =(uint8_t)st->pc;//push pc
			// ram[st->sp+1]=st->pc>>8;
			write_ram(st->sp,(uint8_t)st->pc,ram);
			write_ram(st->sp+1,st->pc>>8,ram);
			st->pc=tmp;
		}
		printf("call z,%.4xh%s",tmp,!st->a?"":" ;skipped");
		break;
	case 0xcd: //call m16
		//push ret addr to stack!
		fetch16(st,rom,&tmp,ram);
		st->sp-=2;//alloc
		// ram[st->sp]=(uint16_t)st->pc&0x00ff;
		// ram[st->sp+1]=(uint16_t)st->pc>>8;
		write_ram(st->sp,(uint16_t)st->pc&0x00ff,ram);
		write_ram(st->sp+1,(uint16_t)st->pc>>8,ram);
		st->pc=tmp;
		printf("call %.4xh ;ret to %.4xh",st->pc,ram[st->sp]|ram[st->sp+1]<<8);
		break;
	case 0xce: //adc a,m8
		fetch8(st,rom,&tmp8,ram);
		
		st->a+=tmp8+!!(st->f&F_C);
		st->f&=~F_N;//N
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		//F_H
		if(st->a<tmp8)st->f|=F_C;//F_C
		else st->f&=~F_C;
		printf("adc a,%.2x",tmp8);
		break;
	case 0xcf: //rst 08h (call 0000h + 00h)
		// fetch16(st,rom,&tmp,ram);
		st->sp-=2;//alloc
		// ram[st->sp]=st->pc&0x00ff;
		// ram[st->sp+1]=st->pc>>8;
		write_ram(st->sp,st->pc&0x00ff,ram);
		write_ram(st->sp+1,st->pc>>8,ram);
		st->pc=0x0008;
		printf("rst 08h ;ret to %.4xh",ram[st->sp]|ram[st->sp+1]<<8);
		break;
	case 0xd0: //ret nc
		if(!(st->f&F_C))
		{
			st->pc=ram[st->sp]|ram[st->sp+1]<<8;
			st->sp+=2;
		}
		printf("ret nc%s",st->f&F_C?" ;skipped":"");
		break;
	case 0xd1: //pop de
		st->e=ram[st->sp];//little-endian
		st->d=ram[st->sp+1];
		st->sp+=2;
		printf("pop de ;%.4xh",st->d<<8|st->e);
		break;
	case 0xd2: //jp nc,m16
		fetch16(st,rom,&tmp,ram);
		if(!(st->f&F_C))st->pc=tmp;
		printf("jp nc,%.4xh%s",tmp,!st->a?" ;skipped":"");
		break;
	case 0xd4://call nc,m16
		fetch16(st,rom,&tmp,ram);
		if(!(st->f&F_C))
		{
			st->sp-=2;
			// ram[st->sp]  =(uint8_t)st->pc;//push pc
			// ram[st->sp+1]=st->pc>>8;
			write_ram(st->sp,(uint8_t)st->pc,ram);
			write_ram(st->sp+1,st->pc>>8,ram);
			st->pc=tmp;
		}
		printf("call nc,%.4xh%s",tmp,st->f&F_C?" ;skipped":"");
		break;
	case 0xd5://push de
		st->sp-=2;
		// ram[st->sp]=st->e;//little-endian
		// ram[st->sp+1]=st->d;
		write_ram(st->sp,st->e,ram);
		write_ram(st->sp+1,st->d,ram);
		printf("push de ;%.4xh",st->d<<8|st->e);
		break;
	case 0xd6://sub m8
		fetch8(st,rom,&tmp,ram);
		if((st->a&0x0f+(uint8_t)tmp&0x0f)&0x10)st->f|=F_H;//H
		else st->f&=~F_H;
		st->a-=(uint8_t)tmp;//2's complement add/sub aren't signed
		if(!(st->a))st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		if(st->a<(uint8_t)tmp)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("sub %.2xh",(uint8_t)tmp);
		break;
	case 0xd7://rst 10h (call 0000h + 10h)
		// fetch16(st,rom,&tmp,ram);
		st->sp-=2;//alloc
		// ram[st->sp]=st->pc&0x00ff;
		// ram[st->sp+1]=st->pc>>8;
		write_ram(st->sp,st->pc&0x00ff,ram);
		write_ram(st->sp+1,st->pc>>8,ram);
		st->pc=0x0010;
		printf("rst 10h ;ret to %.4xh",ram[st->sp]|ram[st->sp+1]<<8);
		break;
	case 0xd8: //ret c
		if(st->f&F_C)
		{
			st->pc=ram[st->sp]|ram[st->sp+1]<<8;
			st->sp+=2;
		}
		printf("ret c%s",st->f&F_C?"":" ;skipped");
		break;
	case 0xd9: //reti
		//pop ret val from stack!
		st->pc=ram[st->sp]|ram[st->sp+1]<<8;
		st->sp+=2;
		// ram[0xffff]=0x1f;//enable all interrupts
		write_ram(0xffff,0x1f,ram);
		printf("reti");
		break;
	case 0xda: //jp c,m16
		fetch16(st,rom,&tmp,ram);
		if(st->f&F_C)st->pc=tmp;
		printf("jp c%s",st->f&F_C?"":" ;skipped");
		break;
	case 0xdc: //call c,m16
		fetch16(st,rom,&tmp,ram);
		if(st->f&F_C)
		{
			st->sp-=2;
			// ram[st->sp]  =(uint8_t)st->pc;//push pc
			// ram[st->sp+1]=st->pc>>8;
			write_ram(st->sp,(uint8_t)st->pc,ram);
			write_ram(st->sp+1,st->pc>>8,ram);
			st->pc=tmp;
		}
		printf("call c,%.4xh%s",tmp,!st->a?"":" ;skipped");
		break;
	case 0xde: //sbc a,m8
		fetch8(st,rom,&tmp8,ram);
		tmp=st->a;
		st->a-=tmp8+!(st->f&F_C);
		if(tmp<tmp8)st->f&=~F_C;//C
		else st->f|=F_C;
		st->f|=F_Z;//Z
		st->f|=F_N;//N
		printf("sbc %.2x ;C:%x",tmp8,(st->f&F_C));
		break;
	case 0xdf: //rst 18h (call 0000h + 00h)
		// fetch16(st,rom,&tmp,ram);
		st->sp-=2;//alloc
		// ram[st->sp]=st->pc&0x00ff;
		// ram[st->sp+1]=st->pc>>8;
		write_ram(st->sp,st->pc&0x00ff,ram);
		write_ram(st->sp+1,st->pc>>8,ram);
		st->pc=0x0018;
		printf("rst 18h ;ret to %.4xh",ram[st->sp]|ram[st->sp+1]<<8);
		break;
	case 0xe0://ldh (m8),a ---> ld (ff00 + m8),a
		fetch8(st,rom,&tmp,ram);
		// ram[0xff00|tmp]=st->a;
		write_ram(0xff00|tmp,st->a,ram);
		printf("ldh (%.2xh),a ;ld (ff00h+%.2xh),a",tmp,tmp);
		break;
	case 0xe1://pop hl
		st->l=ram[st->sp];
		st->h=ram[st->sp+1];
		st->sp+=2;
		printf("pop hl ;%.4xh",st->h|st->l<<8);
		break;
	case 0xe2://ld (c),a
		// fetch8(st,rom,&tmp8,ram);
		// ram[0xff00|(uint16_t)st->c]=st->a;
		write_ram(0xff00|(uint16_t)st->c,st->a,ram);
		printf("ld (c),a");
		break;
	case 0xe5://push hl
		st->sp-=2;
		// ram[st->sp]=st->h;
		// ram[st->sp+1]=st->l;
		write_ram(st->sp,st->h,ram);
		write_ram(st->sp+1,st->l,ram);
		printf("push hl ;%.4xh",st->h|st->l<<8);
		break;
	case 0xe6://and c ;and a,c
		st->a&=st->c;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f|=F_H;//H
		st->f&=~F_C;//C
		st->f&=~F_N;//N
		printf("and c");
		break;
	case 0xe7://rst 20h (call 0000h + 20h)
		st->sp-=2;//alloc
		// ram[st->sp]=st->pc&0x00ff;
		// ram[st->sp+1]=st->pc>>8;
		write_ram(st->sp,st->pc&0x00ff,ram);
		write_ram(st->sp+1,st->pc>>8,ram);
		st->pc=0x0020;
		printf("rst 20h ;ret to %.4xh",ram[st->sp]|ram[st->sp+1]<<8);
		break;
	case 0xe8: //add sp,m8
		fetch8(st,rom,&tmp8,ram);
		// tmp=st->sp;
		st->sp+=tmp8;
		st->f&=~F_Z;//Z
		st->f&=~F_N;//N
		if(st->sp<tmp8)st->f|=F_C;//C
		else st->f&=~F_C;
		if( ((st->sp&0x0f00)>>8) + (tmp8&0x0f) & 0x10 )st->f|=F_H;
		else st->f&=~F_H;
		printf("add sp,%.2xh",tmp8);
		break;
	case 0xe9: //jp (hl)
		st->pc=st->h<<8|st->l;
		printf("jp (hl) ;%.4x",st->h<<8|st->l);
		break;
	case 0xea: //ld (m16),a
		fetch16(st,rom,&tmp,ram);
		// ram[tmp]=st->a;
		write_ram(tmp,st->a,ram);
		printf("ld (%.4xh),a",tmp);
		break;
	case 0xee: //xor m8
		fetch8(st,rom,&tmp8,ram);
		st->a^=tmp8;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("xor %.2xh",tmp8);
		break;
	case 0xef: //rst 28h (call 0000h + 00h)
		st->sp-=2;//alloc
		// ram[st->sp]=st->pc&0x00ff;
		// ram[st->sp+1]=st->pc>>8;
		write_ram(st->sp,st->pc&0x00ff,ram);
		write_ram(st->sp+1,st->pc>>8,ram);
		st->pc=0x0028;
		printf("rst 28h ;ret to %.4xh",ram[st->sp]|ram[st->sp+1]<<8);
		break;
	case 0xf0://ldh a,(m8) ---> ld a,(ff00 + m8)
		fetch8(st,rom,&tmp,ram);
		st->a=ram[0xff00|tmp];
		printf("ldh a,(%.2xh) ;ld a,(ff00h+%.2xh)",tmp,tmp);
		break;
	case 0xf1://pop af
		st->f=ram[st->sp];//little-endian
		st->a=ram[st->sp+1];
		st->sp+=2;
		printf("pop bc ;%.4xh",st->a<<8|st->f);
		break;
	case 0xf2://ld a,(c)
		st->a=ram[0xff00|(uint16_t)st->c];
		printf("ld a,(c)");
		break;
	case 0xf3://di
		// ram[0xffff]=0x00;//disable all int's
		write_ram(0xffff,0x00,ram);
		printf("di");
		break;
	case 0xf5://push af
		st->sp-=2;
		// ram[st->sp]=st->a;
		// ram[st->sp+1]=st->f;
		write_ram(st->sp,st->a,ram);
		write_ram(st->sp+1,st->f,ram);
		printf("push af ;%.4xh",st->a|st->f<<8);
		break;
	case 0xf6://or m8
		fetch8(st,rom,&tmp8,ram);
		st->a|=tmp8;
		if(!st->a)st->f|=F_Z;//Z
		else st->f&=~F_Z;
		st->f&=~F_N;//N
		st->f&=~F_H;//H
		st->f&=~F_C;//C
		printf("or %.2xh",tmp8);
		break;
	case 0xf7://rst 30h (call 0000h + 30h)
		st->sp-=2;//alloc
		// ram[st->sp]=st->pc&0x00ff;
		// ram[st->sp+1]=st->pc>>8;
		write_ram(st->sp,st->pc&0x00ff,ram);
		write_ram(st->sp+1,st->pc>>8,ram);
		st->pc=0x0030;
		printf("rst 30h ;ret to %.4xh",ram[st->sp]|ram[st->sp+1]<<8);
		break;
	case 0xf8://ld hl,sp+m8
		fetch8(st,rom,&tmp8,ram);
		tmp=st->sp+(int8_t)tmp8;//tmp8 needs to be sign-extended
		st->h=tmp>>8;
		st->l=(uint8_t)tmp;//trunc
		st->f&=~F_Z;//Z
		st->f&=~F_N;//N
		if(tmp<tmp8)st->f|=F_C;//C
		else st->f&=~F_C;
		if((st->sp&0xff)+(tmp8&0xff))st->f|=F_H;//H
		else st->f&=~F_H;
		printf("ld hl,sp%+ih",(int8_t)tmp8);
		break;
	case 0xf9: //ld sp,hl
		tmp=st->h<<8|st->l;
		st->sp=tmp;
		printf("ld sp,hl ;%.4xh",tmp);
		break;
	case 0xfa: //ld a,(m16)
		fetch16(st,rom,&tmp,ram);
		st->a=ram[tmp];
		printf("ld a,(%.2xh)",tmp);
		break;
	case 0xfb: //ei
		// ram[0xffff]=0x1f;//enable all int's
		write_ram(0xffff,0x1f,ram);
		printf("ei");
		break;
	case 0xfe: //cp m8
		fetch8(st,rom,&tmp,ram);
		//flags:
		if(st->a-tmp==0)st->f|=F_Z;else st->f&=~F_Z;//Z
		st->f|=F_N;//N
		if((st->a&0x0f)-(tmp&0xf))st->f|=F_H;//H
		else st->f&=~F_H;
		if(st->a<tmp)st->f|=F_C;//C
		else st->f&=~F_C;
		printf("cp %.2xh",tmp);
		break;
	case 0xff: //rst 38h (call 0000h + 00h)
		st->sp-=2;//alloc
		// ram[st->sp]=st->pc&0x00ff;
		// ram[st->sp+1]=st->pc>>8;
		write_ram(st->sp,st->pc&0x00ff,ram);
		write_ram(st->sp+1,st->pc>>8,ram);
		st->pc=0x0038;
		printf("rst 38h ;ret to %.4xh",ram[st->sp]|ram[st->sp+1]<<8);
		break;
	case 0x00: //nop
	case 0xd3: //nop
	case 0xdb: //nop
	case 0xdd: //nop
	case 0xe3: //nop
	case 0xe4: //nop
	case 0xeb: //nop
	case 0xec: //nop
	case 0xed: //nop
	case 0xf4: //nop
	case 0xfc: //nop
	case 0xfd: //nop
	default:
		printf("nop",*op);
		break;
	}
}