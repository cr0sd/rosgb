#include"tigr.h"
#include<stdio.h>
#include<inttypes.h>

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
uint32_t updatejoypad(uint8_t*ram)
{
	static uint8_t jp=0;
	if(ram[0xff00]==0x20)//R L U D
	{
		jp|=(tigrKeyHeld(tp,TK_RIGHT)!=0);
		jp|=(tigrKeyHeld(tp,TK_LEFT )!=0)<<1;
		jp|=(tigrKeyHeld(tp,TK_UP   )!=0)<<2;
		jp|=(tigrKeyHeld(tp,TK_DOWN )!=0)<<3;
		ram[0xff00]=jp;
	}
	else if(ram[0xff00]==0x10)//A B SEL START
	{
		jp|=(tigrKeyHeld(tp,'Z'      )!=0);
		jp|=(tigrKeyHeld(tp,'X'      )!=0)<<1;
		jp|=(tigrKeyHeld(tp,TK_SPACE )!=0)<<2;
		jp|=(tigrKeyHeld(tp,TK_RETURN)!=0)<<3;
		ram[0xff00]=jp;
	}
}

void DrawTile(uint8_t x,uint8_t y,uint16_t t,uint8_t *ram)
{
	uint8_t tmp,tmp2;
	for(uint32_t j=t*16;j<t*16+16;j+=8*2)
	{
		for(uint32_t i=0;i<64;i++)//draw single tile
		{
			tmp =ram[0x8000+j+(i/8)*2];
			tmp2=ram[0x8000+j+(i/8)*2+1];
			tmp &=0x80>>(i%8);
			tmp2&=0x80>>(i%8);
			tmp >>=7-(i%8);
			tmp2>>=7-(i%8);
			tmp|=tmp2<<1;
			
			// im.i[j+i%8+(i/8)*160+0]=palette[tmp];
			im.i[(i/8+y)*160 + i%8+x]=palette[tmp];
		}
	}
}


uint32_t updatewindow(uint8_t*ram)
{
	static uint32_t delay=0;
	if(!tp)return 1;
	delay++;
	if(delay%400!=0)return 0;
	if(!tigrClosed(tp)&&!tigrKeyHeld(tp,TK_ESCAPE))
	{
		tigrClear(tp,tigrRGB(255,255,255));
		
		//Draw all tiles
		for(int i=0;i<256;i++)DrawTile(i%16*8,(i/16)*8,i,ram);
		DrawTile(50,50,23,ram);
		
		tigrUpdate(tp);
	}
	else
	{
		return -1; //0xffffffff
	}
	return 0;
}
uint32_t closewindow()
{
	if(!tp)return 1;
	tigrFree(tp);
	return 0;
}