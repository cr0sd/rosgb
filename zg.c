#include"tigr.h"
#include<windows.h>
#include<stdio.h>
#include<inttypes.h>
#include<stdbool.h>
#define print(x,y,str) tigrPrint(tpInfo,tfont,x,y,tigrRGB(0xdd,0x66,0x77),str);
extern uint8_t bittest(uint8_t b1,uint8_t b2,uint8_t bit);//bt.asm

uint32_t palette[4]={0x00ffffff,0x00389048,0x00284028,0x000000};
extern uint8_t paused;

union _i{TPixel *t;uint32_t *i;};
union _i im;
Tigr *tp,*tpInfo;
typedef struct St //cpu state
{
	uint8_t a,f,b,c,d,e,h,l; //regs signed, flags unsigned
	uint16_t sp,pc;
}St;
uint32_t openwindow(uint8_t*str)
{
	tpInfo=tigrWindow(160,144,"memory",0);
	tp=tigrWindow(160,144,str,0);
	if(tp)im.t=tp->pix;
	return tp==NULL||tpInfo==NULL;
}
void updatejoypad(uint8_t*ram)
{
	static uint8_t jp=0;
	//R L U D
	jp =(tigrKeyHeld(tp,TK_RIGHT)!=0);
	jp|=(tigrKeyHeld(tp,TK_LEFT )!=0)<<1;
	jp|=(tigrKeyHeld(tp,TK_UP   )!=0)<<2;
	jp|=(tigrKeyHeld(tp,TK_DOWN )!=0)<<3;
	if(ram[0xff00]==0x20)ram[0xff00]=jp;
	
	//A B SEL START
	jp =(tigrKeyHeld(tp,'Z')!=0);
	jp|=(tigrKeyHeld(tp,'X')!=0)<<1;
	jp|=(tigrKeyHeld(tp,'A')!=0)<<2;//sl
	jp|=(tigrKeyHeld(tp,'S')!=0)<<3;//st
	if(ram[0xff00]==0x10)ram[0xff00]=jp;
	// fprintf(stderr,"joyp (0xff00):%.2xh\n",jp);
}

void DrawTile(uint8_t x,uint8_t y,uint16_t o,uint16_t t,uint8_t *ram)
{
	for(int j=0;j<16;j+=2)//bitplane
		for(int i=0;i<8;i++)//bit
			im.i[(j/2+y)*160+(i+x)]=palette[bittest(ram[o+j+t*16],ram[o+1+j+t*16],7-i)];
}

uint8_t tilemap=false;
uint32_t updatewindow(uint8_t*ram,St*st)
{
	static uint32_t delay=0;
	if(!tp||!tpInfo)return 1;
	delay++;
	if(delay%400!=0)return 0;
	if(!tigrClosed(tp)&&!tigrKeyHeld(tp,TK_ESCAPE)&&!tigrClosed(tpInfo)&&!tigrKeyHeld(tpInfo,TK_ESCAPE))
	{
		tigrClear(tp,tigrRGB(255,255,255));
		tigrClear(tpInfo,tigrRGB(0x22,0x22,0x22));
		if(tigrKeyDown(tp,'P'))paused=!paused;
		if(tigrKeyDown(tp,TK_SPACE))tilemap=!tilemap;
		
		
		//Draw all tiles
		if(tilemap)//LCD DISPLAY ON
		{
			//uint16_t tilemap_select=0x9800;
			//if(ram[0xff40]&0x40)tilemap_select=0x9c00;
			for(int j=0;j<18;j++)//tile map veritcal
				for(int i=0;i<20;i++)//tile map horizontal
					DrawTile(i*8,j*8,0x8000,ram[0x9800+(j*0x20+i)],ram);
			
			uint16_t oam_offs=0xfe00;//should be 0xfe00
			for(int i=0;i<40;i+=4)//OAM sprites
				if(ram[oam_offs+i*4]>=8&&ram[oam_offs+i*4+1]>=16)
					DrawTile(ram[oam_offs+i*4]-8,ram[oam_offs+i*4+1]-16,0x8000,ram[oam_offs+i*4+2],ram);
		}
		else
			// for(int i=0;i<256+128;i++)
			for(int i=0;i<256;i++)
				DrawTile(i%16*8,(i/16)*8,0x8000,i,ram);
		
		// for(int i=0;i<0xff;i++)
		{
			static char str[128];
			auto uint16_t ad=0xff46;
			auto uint8_t i=0;
		info:
				sprintf(str,"%.4x: %.2x %.2x %.2x %.2x %.2x %.2x",ad+i*6,ram[ad+i*6],ram[ad+i*6+1],ram[ad+i*6+2],ram[ad+i*6+3],ram[ad+i*6+4],ram[ad+i*6+5]);
				print(0,i*12+1,str);
				if(i++<9)_asm jmp info
			sprintf(str,"PC: %.4x%s",st->pc,paused?" [paused]":"");
			print(0,1+i*12,str);
		}
		
		
		tigrUpdate(tpInfo);
		tigrUpdate(tp);
	}
	else
		return -1; //0xffffffff
	return 0;
}
uint32_t closewindow()
{
	if(!tp)return 1;
	tigrFree(tp);
	if(!tpInfo)return 1;
	tigrFree(tpInfo);
	return 0;
}