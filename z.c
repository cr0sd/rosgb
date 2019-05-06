#include<stdio.h>
#include<stdlib.h>
#include<inttypes.h>
#include<assert.h>

#define VIDEO
#define PRINTING

#ifndef PRINTING
#define printf(x)
#endif

#define F_Z	0x80//zero flag
#define F_N	0x40//subtract
#define F_H	0x20//half-carry
#define F_C	0x10//carry
uint8_t st[98];
extern void fetch8(uint8_t*,uint8_t*,uint8_t*,uint8_t*);
extern void fetch16(uint8_t*,uint8_t*,uint16_t*,uint8_t*);
extern void decexec(uint8_t*,uint8_t*,uint8_t*,uint8_t*);
extern void romhexdump(uint8_t*);
#ifdef VIDEO
extern uint32_t openwindow(uint8_t*);
extern uint32_t updatewindow(uint8_t*);
extern uint32_t closewindow();
extern uint32_t updatejoypad(uint8_t*);
#endif
int main(int c,char **v)
{
	uint8_t op;
	uint8_t *ram;
	FILE *f=fopen(v[1],"rb");
	uint8_t *rom;
	uint32_t romsize;
	st[8]=0xfe;
	st[9]=0xff;
	
	ram=malloc(0xffff); //I think this is enough
	if(!ram)return 3;
	
	if(c<2 || !f)return 1;
	// fseek(f,0x100,SEEK_SET);
	
	//copy to memory
	fseek(f,0,SEEK_END);
	romsize=ftell(f);
	if(0x8000>romsize)romsize=0x8000;
	rom=malloc(romsize);
	
	if(!rom)return 2;
	
	rewind(f);
	fread(rom,1,romsize,f);
	printf("Loaded ROM \'%s\' (%u B)\n",v[1],romsize);
	
	
	printf("ROM Size: %ukB\n",32<<rom[0x148]);
	printf("RAM Size: %ukB\n",16<<rom[0x149]*2);
	
	if(c>2 && strcmp(v[2],"-d")==0)
	{
		romhexdump(rom);
		return 0;
	}
	
	st[10]=0x00;//0x100
	st[11]=0x01;
	
	
	
	//load program into RAM
	for(int i=0;i<0x8000;i++)
	{
		ram[i]=rom[i];
		assert(ram[i]==rom[i]);
	}
	printf("Loaded 8000h (32768) bytes into RAM\n");
	
	
	#ifdef VIDEO
	openwindow(rom+0x134);
	#endif
	
	
	// for(int i=0;i<70&&((st[10]|st[11]<<8)<romsize);i++)
	for(;;)
	{
		updatejoypad(ram);
		ram[0xff44]=0x90;
		
		printf("\n[%#10.8x]%#5.2x: ",st[10]|st[11]<<8,rom[st[10]|st[11]<<8]);
		fetch8(st,rom,&op,ram);
		decexec(st,rom,&op,ram);
		
		#ifdef VIDEO
		if(updatewindow(ram))break;
		#endif
	}
	
	printf("\n----------\nCPU State:");
	for(int i=0;i<8;i++)printf("\n%c=%12u (%#4.2x)","afbcdehl"[i],st[i],st[i]);
	printf("\nsp: %.4x\npc: %.4x\n",st[8]|st[9]<<8,st[10]|st[11]<<8);
	printf("f: (%x) Z:%x N:%x H:%x C:%x\n",st[1]>>4,(st[1]&F_Z)>>4,(st[1]&F_N)>>4,(st[1]&F_H)>>4,(st[1]&F_C)>>4);
	printf("STACK: ");for(int i=0xfffe;i>0xffee;i--)printf("%.2x  ",ram[i]);
	printf("\nTILE 0(RAM): ");for(int i=0;i<8;i++)printf("%.2x ",ram[0x8000+i]);
	free(ram);
	free(rom);
	#ifdef VIDEO
	closewindow();
	#endif
}