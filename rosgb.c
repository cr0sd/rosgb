#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<inttypes.h>
#include<assert.h>
#include<stdbool.h>
#include<time.h>

#define VIDEO
// #define PRINTING

// #ifndef PRINTING
// #define printf(x)
// #endif

#define F_Z	0x80//zero flag
#define F_N	0x40//subtract
#define F_H	0x20//half-carry
#define F_C	0x10//carry
uint8_t st[13];
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

uint8_t paused=false;
typedef struct St //cpu state
{
	uint8_t a,f,b,c,d,e,h,l; //regs signed, flags unsigned
	uint16_t sp,pc;
	uint8_t ime;
}St;
extern void write_ram(uint16_t addr,uint8_t val,St *st,uint8_t *ram);
int main(int c,char **v)
{
	St *stp=st;
	uint8_t op;
	uint8_t *ram;
	FILE *f=fopen(v[1],"rb");
	uint8_t *rom;
	uint32_t romsize;
	st[8]=0xfe;//sp
	st[9]=0xff;

	uint32_t nclocks=clock();

	ram=malloc(0x10000);
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

	ram[0xff4f]=0;//VRAM bank
	st[12]=1;//IME enable interrupts

	// for(int i=0;i<70&&((st[10]|st[11]<<8)<romsize);i++)


	//-----CPU CYCLE LOOP-----
	for(;;)
	{
		#ifdef VIDEO
		if(updatewindow(ram,stp))break;
		#endif
		if(paused)continue;

		//Control Registers--
		ram[0xff44]+=1;//CURLINE (VBLANK INT AT:144-153)
		//ram[0xff05]+=1;//TIMA (TIMER INT AT OVERFLOW)
		// if(!ram[0xff05])ram[0xff05]=ram[0xff06];
		ram[0xff41]=(ram[0xff41]+1)%4;//V-BLANK


		//Interrupts--
		//if(ram[0xff44]==144 && ram[0xffff] && st[12])//V-BLANK INT
		//{
		//	stp->sp-=2;//push pc, jump to 0040h interrupt
		//	write_ram(stp->sp,stp->pc&0x00ff,stp,ram);
		//	write_ram(stp->sp+1,stp->pc>>8,stp,ram);
		//	stp->pc=0x0040;
		//}

		//if(stp->pc==0xff82)MessageBoxA(NULL,"FF82","-",MB_OK);

		printf("\n[%#10.8x]%#5.2x: ",st[10]|st[11]<<8,rom[st[10]|st[11]<<8]);
		fetch8(st,rom,&op,ram);
		decexec(st,rom,&op,ram);
	}

	printf("\n----------\nCPU State:");
	for(int i=0;i<8;i++)printf("\n%c=%12u (%#4.2x)","afbcdehl"[i],st[i],st[i]);
	printf("\nsp: %.4x\npc: %.4x\n",st[8]|st[9]<<8,st[10]|st[11]<<8);
	printf("IME: %.2x\n",st[12]);
	printf("f: (%x) Z:%x N:%x H:%x C:%x\n",st[1]>>4,(st[1]&F_Z)>>4,(st[1]&F_N)>>4,(st[1]&F_H)>>4,(st[1]&F_C)>>4);
	printf("INT: (%.2x) V-B:%.2x LCD:%.2x TIM:%.2x SER:%.2x JOYP:%.2x\n",ram[0xffff],ram[0xffff]&0x1,ram[0xffff]>>1&0x1,ram[0xffff]>>2&0x1,ram[0xffff]>>3&0x1,ram[0xffff]>>4&0x1);
	printf("STACK: ");for(int i=0xfffe;i>0xffee;i--)printf("%.2x  ",ram[i]);
	printf("\nTILE 0(RAM): ");for(int i=0;i<8;i++)printf("%.2x ",ram[0x8000+i]);
	// {
		// printf("\nTilemap:%.2x,%.2x\n",(uint8_t)ram[0x9800],(uint8_t)ram[0x9801]);
		// FILE *fp=fopen("tilemap.txt","w");
		// for(int i=0;i<0x800;i++)//lines
		// {
			// fprintf(fp,"%.2x,",ram[0x9800+i]);
			// if(i%32==0 && i!=0)fputc('\n',fp);
		// }
		// // fwrite(ram+0x9800,1,0x800,fp);
		// fclose(fp);
	// }
	printf("\nOAM Sprite 0  Tile: %.2x X: %.2x Y: %.2x",ram[0xfe02],ram[0xfe00],ram[0xfe01]);
	printf("\nOAM Sprite 1  Tile: %.2x X: %.2x Y: %.2x",ram[0xfe06],ram[0xfe04],ram[0xfe05]);
	printf("\nOAM Sprite 16 Tile: %.2x X: %.2x Y: %.2x",ram[0xfe12],ram[0xfe10],ram[0xfe11]);
	free(ram);
	free(rom);
	#ifdef VIDEO
	closewindow();
	#endif
}
