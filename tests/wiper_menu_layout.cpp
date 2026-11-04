#include "port/types.h"
#include "Menu.h"
#include "Frontend.h"
#include "Graphics.h"
#include "Font.h"
#include "Sprite.h"
#include "Input.h"
#include <cstdio>
#include <cstring>
extern void FrontendMenu_BuildDeviceBindings(),FrontendControls_DrawDeviceBindings(Menu*);
extern Menu *FrontendMenu_GetDeviceBindings();
static FILE*out;static int fail=0;
extern "C" void __wrap__Z21FrontendDraw_PlayTimev(){}
extern "C" void __wrap__Z23FrontendDraw_BreadcrumbiiPPci(int,int,char**,int){}
extern "C" void __real__Z13Font_DrawTexthPcssPij(BYTE,char*,short,short,int*,unsigned);
extern "C" void __wrap__Z13Font_DrawTexthPcssPij(BYTE slot,char*text,short x,short y,int*c,unsigned f){
 if(out)fprintf(out,"%d\t%d\t%s\n",x,y,text);
 if(y<60||y>455)fail++;
 __real__Z13Font_DrawTexthPcssPij(slot,text,x,y,c,f);
}
int main(int argc,char**argv){
 Graphics gfx={};g_pGraphics=&gfx;gfx.resX=640;gfx.resY=480;
 Texture arrow={};arrow.width=arrow.height=8;CFrontend::m_pAr640ATexture=CFrontend::m_pAr640DTexture=&arrow;
 char*texts[256];for(auto&p:texts)p=(char*)"binding";
 const char*labels[]={"left","right","accelerate","brake","handbrake","gear up","gear down","camera","look back","calibrate"};
 for(int i=0;i<10;i++)texts[0x6b+i]=(char*)labels[i];texts[100]=(char*)"player %d";
 CFrontend::m_textStrings=texts;CFrontend::m_textCount=256;CFrontend::m_textFirstId=0;
 FontHeader h={};h.glyphCount=1;h.lineHeight=16;h.spaceWidth=4;h.charSpacing=1;
 FontGlyph glyph={};glyph.width=7;glyph.height=16;glyph.field_0xa[0]=8;short map[256];for(auto&v:map)v=0;
 FontSlot fonts[2]={};for(auto&f:fonts){f.loaded=1;f.pHeader=&h;f.pGlyphs=&glyph;f.pCharMap=map;f.pTexture=&arrow;}g_fonts=fonts;g_fontCount=2;Font_SetBlendMode(1);
 FrontendMenu_BuildDeviceBindings();Menu*m=FrontendMenu_GetDeviceBindings();m->items[9].enabled=m->items[9].visible=0;m->cursor=10;
 CInput::m_availableDevices[0].type=1;
 if(argc>1)out=fopen(argv[1],"w");FrontendControls_DrawDeviceBindings(m);if(out)fclose(out);
 for(unsigned i=0;i<g_spriteCount1;i++){auto r=g_spriteLayer1[i].dst;if(r.y<55||r.y+r.h>470)fail++;}
 m->items[9].enabled=m->items[9].visible=1;FrontendControls_DrawDeviceBindings(m);
 puts("actual driving-list draw with wiper+save fits480p; synthetic glyph metrics not native atlas");return fail?1:0;
}
