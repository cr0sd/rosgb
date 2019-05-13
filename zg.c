#include"tigr.h"
#include<stdio.h>
#include<inttypes.h>
#include<stdbool.h>
extern uint8_t bittest(uint8_t b1,uint8_t b2,uint8_t bit);//bt.asm

uint32_t palette[4]={0x00ffffff,0x00389048,0x00284028,0x000000};

union _i{TPixel *t;uint32_t *i;};
union _i im;
Tigr *tp;
uint32_t openwindow(uint8_t*str)
{
	tp=tigrWindow(160,144,str,0);
	if(tp)im.t=tp->pix;
	return tp==NULL;
}
void updatejoypad(uint8_t*ram)
{
	static uint8_t jp=0;
	//R L U D
	jp|=(tigrKeyHeld(tp,TK_RIGHT)!=0);
	jp|=(tigrKeyHeld(tp,TK_LEFT )!=0)<<1;
	jp|=(tigrKeyHeld(tp,TK_UP   )!=0)<<2;
	jp|=(tigrKeyHeld(tp,TK_DOWN )!=0)<<3;
	if(ram[0xff00]==0x20)ram[0xff00]=jp;
	
	//A B SEL START
	jp|=(tigrKeyHeld(tp,'Z'      )!=0);
	jp|=(tigrKeyHeld(tp,'X'      )!=0)<<1;
	jp|=(tigrKeyHeld(tp,TK_SPACE )!=0)<<2;
	jp|=(tigrKeyHeld(tp,TK_RETURN)!=0)<<3;
	if(ram[0xff00]==0x10)ram[0xff00]=jp;
}

void DrawTile(uint8_t x,uint8_t y,uint16_t o,uint16_t t,uint8_t *ram)
{
	for(int j=0;j<16;j+=2)//bitplane
		for(int i=0;i<8;i++)//bit
			im.i[(j/2+y)*160+(i+x)]=palette[bittest(ram[o+j+t*16],ram[o+1+j+t*16],7-i)];
}

uint8_t tilemap=false;
uint32_t updatewindow(uint8_t*ram)
{
	static uint32_t delay=0;
	if(!tp)return 1;
	delay++;
	if(delay%400!=0)return 0;
	if(!tigrClosed(tp)&&!tigrKeyHeld(tp,TK_ESCAPE))
	{
		tigrClear(tp,tigrRGB(255,255,255));
		
		if(tigrKeyDown(tp,TK_SPACE))tilemap=!tilemap;
		
		
		//Draw all tiles
		if(tilemap)
		{
			for(int j=0;j<18;j++)//tile map veritcal
				for(int i=0;i<20;i++)//tile map horizontal
					DrawTile(i*8,j*8,0x8000,ram[0x9800+(j*0x20+i)],ram);
			for(int i=0;i<40;i+=4)//OAM sprites
				if(ram[0xfe00+i*4]>=8&&ram[0xfe00+i*4+1]>=16)
					DrawTile(ram[0xfe00+i*4],ram[0xfe00+i*4+1],0x8000,ram[0xfe00+i*4+2],ram);
		}
		else
			for(int i=0;i<256;i++)
				DrawTile(i%16*8,(i/16)*8,0x8000,i,ram);
		
		
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
	return 0;
}