#include "effects_ui.h"
#include "board.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *const effect_names[EFFECT_COUNT]={"MIRROR","POP ART","KALEIDO","PIXEL"};
const uint8_t effect_colors[EFFECT_COUNT][3]={{48,190,255},{255,83,176},{255,190,65},{112,239,173}};
static const uint8_t glyphs[36][7]={
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
    {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14},
};
static uint16_t rgb(int r,int g,int b) {return ((r>>3)<<11)|((g>>2)<<5)|(b>>3);}
static void rect(uint16_t *s,int x,int y,int w,int h,uint16_t color)
{
    for(int j=y;j<y+h;++j)for(int i=x;i<x+w;++i)
        if(i>=0&&i<SCREEN_W&&j>=0&&j<SCREEN_H)s[j*SCREEN_W+i]=color;
}
static void round_rect(uint16_t *s,int x,int y,int w,int h,uint16_t color)
{
    for(int j=0;j<h;++j)for(int i=0;i<w;++i) {
        int dx=i<6?6-i:i>=w-6?i-(w-7):0;
        int dy=j<6?6-j:j>=h-6?j-(h-7):0;
        if(dx*dx+dy*dy<=36)rect(s,x+i,y+j,1,1,color);
    }
}
static void text(uint16_t *s,int x,int y,const char *str,int scale,uint16_t color)
{
    for(;*str;++str,x+=6*scale) {
        int index=*str>='A'&&*str<='Z'?*str-'A':*str>='0'&&*str<='9'?26+*str-'0':-1;
        if(index>=0)for(int row=0;row<7;++row)for(int col=0;col<5;++col)
            if(glyphs[index][row]&(1<<(4-col)))rect(s,x+col*scale,y+row*scale,scale,scale,color);
        if(*str=='>')for(int row=0;row<7;++row)rect(s,x+(3-abs(3-row))*scale,y+row*scale,scale,scale,color);
        if(*str=='-')rect(s,x,y+3*scale,5*scale,scale,color);
    }
}
static int luma(uint16_t p) {return (((p>>11)&31)*77+((p>>5)&63)*75+(p&31)*29)>>5;}

void effects_render(uint16_t *s,const uint16_t *camera,int effect,bool camera_ok,
                  bool frozen,bool muted,int level,int fps,bool flash)
{
    static const uint16_t retro[]={0x0864,0x2290,0x5db8,0xdff5};
    for(int y=0;y<240;++y)for(int x=0;x<240;++x) {
        int sx=x,sy=y;
        if(effect==2) {sx=(x<120?119-x:x-120)*2;sy=(y<120?119-y:y-120)*2;}
        if(effect==3) {sx=(x/8)*8+4;sy=(y/8)*8+4;}
        uint16_t pixel=camera_ok?camera[sy*240+sx]:rgb(12+x/6,14+y/5,45+(x+y)/5);
        if(effect==1&&camera_ok) {
            int r=(pixel>>11)&31,g=(pixel>>5)&63,b=pixel&31;
            r=(r/8)*255/3;g=(g/16)*255/3;b=(b/8)*255/3;
            int edge=x<239?abs(luma(pixel)-luma(camera[y*240+x+1])):0;
            pixel=edge>35?0x0842:rgb(r,g,b);
        }
        if(effect==3&&camera_ok) {int value=luma(pixel);int index=value/64;if(index>3)index=3;pixel=retro[index];}
        s[y*240+x]=pixel;
    }
    uint16_t accent=rgb(effect_colors[effect][0],effect_colors[effect][1],effect_colors[effect][2]);
    round_rect(s,8,8,224,25,0x0842);
    text(s,16,14,effect_names[effect],2,accent);
    unsigned display_fps=(unsigned)(fps<0?0:fps>99?99:fps);
    char status[12];snprintf(status,sizeof(status),"%u FPS",display_fps);
    text(s,188,14,status,1,0xffff);
    text(s,188,24,muted?"MUTE":"SOUND",1,muted?0x8410:accent);
    for(int bar=0;bar<10;++bar)round_rect(s,219,231-bar*7,8,4,level>bar*10?accent:0x18c3);
    if(!camera_ok) {
        round_rect(s,24,96,192,48,0x0842);
        text(s,42,112,"CAMERA OFFLINE",2,0xffff);
    }
    if(frozen) {
        round_rect(s,76,208,88,22,0x0842);
        text(s,84,213,"FROZEN",2,accent);
    }
    rect(s,0,240,240,40,0x0842);
    round_rect(s,8,245,108,29,0x2127);
    round_rect(s,124,245,108,29,accent);
    text(s,18,253,"EFFECT",2,0xffff);text(s,99,253,">",2,accent);
    text(s,142,253,frozen?"RESUME":"FREEZE",2,0x0842);
    if(flash) {
        rect(s,0,0,240,4,0xffff);rect(s,0,236,240,4,0xffff);
        rect(s,0,0,4,240,0xffff);rect(s,236,0,4,240,0xffff);
    }
}
