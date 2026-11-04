#include "port/sys.h"
#include <string.h>
#include "FrontendDraw.h"
#include "Frontend.h"
#include "Graphics.h"
#include "Font.h"
#include "Sprite.h"
#include "RallyData.h"
#include "GenericFileLoader.h"
#include "GameInfo.h"
#include "FrontendMenus.h"
#include <stdio.h>

// GLOBAL: CMR2 0x00524968
BYTE g_colourWhite0x00524968[4] = { 0xff, 0xff, 0xff, 0xff };
// GLOBAL: CMR2 0x0052496c
BYTE g_colourText0x0052496c[4] = { 0xd7, 0xeb, 0xda, 0xff };

// GLOBAL: CMR2 0x00524970
BYTE g_colourDim0x00524970[4] = { 0x48, 0x78, 0x74, 0xff };
// GLOBAL: CMR2 0x00524974
BYTE g_colourShadowWhite0x00524974[4] = { 0xff, 0xff, 0xff, 0x80 };
// GLOBAL: CMR2 0x00524978
BYTE g_colourShadowText0x00524978[4] = { 0xd2, 0xca, 0xd2, 0x80 };
// GLOBAL: CMR2 0x0052497c
BYTE g_colourShadowDim0x0052497c[4] = { 0x74, 0x83, 0x8d, 0x80 };

// GLOBAL: CMR2 0x00524980
BYTE g_colourBlack0x00524980[4] = { 0x00, 0x00, 0x00, 0xff };
// GLOBAL: CMR2 0x00524984
BYTE g_colourTitle0x00524984[4] = { 0xff, 0xff, 0xff, 0xff };

// GLOBAL: CMR2 0x00524b88
BYTE g_helpColour0x00524b88[4] = { 0xff, 0xff, 0xff, 0xff };
// GLOBAL: CMR2 0x00524b8c
char g_helpPulseUp0x00524b8c = 1;
// GLOBAL: CMR2 0x00818270
char g_helpPulse0x00818270;

// GLOBAL: CMR2 0x00818430
char g_unk0x00818430[256];
// GLOBAL: CMR2 0x008189a8
short g_frontendDrawRect[4];

// One element of the "A | B | C" path shown at the top of the menus: the
// text and, unless it is the last one, a one pixel separator after it.
// Returns the x where the next element starts.
// FUNCTION: CMR2 0x004d3e80
int FrontendDraw_BreadcrumbItem(int x, int y, BYTE *pColour, char last, char *text)
{
    int next;

    next = x;
    Font_DrawText(1, text, next, y, (int *)pColour, 0x11);
    if (last == 0) {
        next += Font_GetTextWidth(1, (BYTE *)text) + (int)(g_pGraphics->resX * 5) / 640;
        g_frontendDrawRect[0] = next;
        g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 38) / 480 - (int)(g_pGraphics->resY * 15) / 480;
        g_frontendDrawRect[2] = 1;
        g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 23) / 480;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourText0x0052496c, 1);
        next += (int)(g_pGraphics->resX * 5) / 640;
    }
    return next;
}

// FUNCTION: CMR2 0x004d3fa0
void FrontendDraw_Breadcrumb(int x, int y, char **ppText, int count)
{
    char last;
    BYTE *pColour;
    int i;
    int cur;

    cur = x;
    for (i = 0; i < count; i++) {
        if (ppText[i] != NULL) {
            if (i == count - 1) {
                last = 1;
                pColour = g_colourWhite0x00524968;
            } else {
                last = 0;
                pColour = g_colourText0x0052496c;
            }
            strcpy(g_unk0x00818430, ppText[i]);
            cur = FrontendDraw_BreadcrumbItem(cur, y, pColour, last, g_unk0x00818430);
        }
    }
}

// FUNCTION: CMR2 0x004d4040
void FrontendDraw_MenuTitle(Menu *pMenu)
{
    char *text[3];

    text[0] = CFrontend::GetTextString(0x17c);
    text[1] = (char *)RallyData_GetRecord(0);
    text[2] = CFrontend::GetTextString(pMenu->field_0x4);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, text, 3);
}

// Draws the path of parent menus up to pMenu, most distant first. depth
// limits how many parents are shown (-1 = all); ppNames can override the
// names of the last nameCount parents.
// match 51%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004d40d0
int FrontendDraw_MenuPath(Menu *pMenu, int x, int y, char last, int depth, char **ppNames, int nameCount)
{
    BYTE *pColour;
    int pos = x;
    int n = nameCount;

    if (depth > 0 || depth == -1) {
        if (pMenu->pParent != NULL && pMenu != FrontendMenu_GetMain()) {
            if (last == 0)
                n = nameCount - 1;
            if (depth == -1)
                pos = FrontendDraw_MenuPath(pMenu->pParent, pos, y, 0, -1, ppNames, n);
            else
                pos = FrontendDraw_MenuPath(pMenu->pParent, pos, y, 0, depth - 1, ppNames, n);
        }
        if (last != 0)
            pColour = g_colourWhite0x00524968;
        else
            pColour = g_colourText0x0052496c;
        if (ppNames != NULL && nameCount > 0 && last == 0)
            strcpy(CFrontend::m_stringDest, ppNames[nameCount - 1]);
        else if (pMenu->text != 0)
            strcpy(CFrontend::m_stringDest, pMenu->text);
        else
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->field_0x4));
        pos = FrontendDraw_BreadcrumbItem(pos, y, pColour, last, CFrontend::m_stringDest);
    }
    return pos;
}

extern char g_loadRecordTimeFormat[];
// GLOBAL: CMR2 0x00524c6c
char g_strAutomode[] = "Automode";

// Total play time and the title of the current section, top right.
// FUNCTION: CMR2 0x004d41e0
void FrontendDraw_PlayTime(void)
{
    unsigned int ms;
    unsigned int minutes;
    char text[52];

    ms = CFrontend::GetFrontendElapsedMilliseconds();
    minutes = ms / 1000 / 60;
    sprintf(text, g_loadRecordTimeFormat, minutes / 60, minutes % 60, ms / 1000 % 60);
    Font_DrawText(3, text, (int)(g_pGraphics->resX * 539) / 640, (int)(g_pGraphics->resY * 38) / 480,
                  (int *)g_colourText0x0052496c, 0x11);
    Font_DrawText(1, CFrontend::GetTextString(0x4e), (int)(g_pGraphics->resX * 535) / 640,
                  (int)(g_pGraphics->resY * 38) / 480, (int *)g_colourText0x0052496c, 0x14);
    if (CGameInfo::IsRecordFlagSet(0x11))
        Font_DrawText(1, g_strAutomode, (int)(g_pGraphics->resX * 635) / 640, (int)(g_pGraphics->resY * 5) / 480,
                      (int *)g_colourText0x0052496c, 0xc);
}

// Draws the label of item `index` of `pMenu` in white when it is the cursor,
// in the normal text colour when it is enabled and dimmed when it is not;
// hidden items draw nothing.
// FUNCTION: CMR2 0x004d4350
void FrontendDraw_ItemLabel(char *text, int x, int y, unsigned int flags, int index, Menu *pMenu)
{
    if (pMenu->items[index].visible) {
        if (pMenu->cursor == index) {
            Font_DrawText(1, text, x, y, (int *)g_colourWhite0x00524968, flags);
            return;
        }
        if (pMenu->items[index].enabled) {
            Font_DrawText(1, text, x, y, (int *)g_colourText0x0052496c, flags);
            return;
        }
        Font_DrawText(1, text, x, y, (int *)g_colourDim0x00524970, flags);
    }
}

// Draw callback (menu callback slot 2, one argument) of the player-profile
// menu and its siblings: the play time, the menu title and one centred label
// per item.
// FUNCTION: CMR2 0x004d43e0
void FrontendDraw_DrawProfileMenu(Menu *pMenu)
{
    BYTE i;
    int x;

    x = (int)g_pGraphics->resX / 2;
    FrontendDraw_PlayTime();
    Font_DrawText(1, CFrontend::GetTextString(pMenu->field_0x4), x, 10,
                  (int *)g_colourWhite0x00524968, 10);
    for (i = 0; i < pMenu->itemCount; i++) {
        FrontendDraw_ItemLabel(CFrontend::GetTextString(pMenu->items[i].id), x, (i * 5 + 0x19) * 4, 10,
                               i, pMenu);
    }
}

// Help line at the bottom of the screen, its brightness pulsing up and down.
// match 85%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004d4460
void FrontendDraw_HelpText(char *text, int reset)
{
    BYTE *pText = g_colourText0x0052496c;

    if (text != NULL) {
        if (reset == 0) {
            if (g_helpPulseUp0x00524b8c != 0) {
                g_helpPulse0x00818270++;
                if (g_helpPulse0x00818270 > 32) {
                    g_helpPulse0x00818270--;
                    g_helpPulseUp0x00524b8c = 0;
                }
            } else {
                g_helpPulse0x00818270--;
                if (g_helpPulse0x00818270 < -32) {
                    g_helpPulse0x00818270++;
                    g_helpPulseUp0x00524b8c = 1;
                }
            }
            g_helpColour0x00524b88[0] = pText[0] + g_helpPulse0x00818270;
            g_helpColour0x00524b88[1] = pText[1] + g_helpPulse0x00818270;
            g_helpColour0x00524b88[2] = pText[2] + g_helpPulse0x00818270;
            g_helpColour0x00524b88[3] = pText[3];
        } else {
            g_helpPulse0x00818270 = 0;
            g_helpColour0x00524b88[0] = pText[0];
            g_helpColour0x00524b88[1] = pText[1];
            g_helpPulseUp0x00524b8c = 1;
            g_helpColour0x00524b88[2] = pText[2];
            g_helpColour0x00524b88[3] = pText[3];
        }
        sprintf(CFrontend::m_stringDest, "%s", text);
        Font_Unused(CFrontend::m_stringDest, Frontend_GetOverlayMode());
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 102) / 640,
                      (int)(g_pGraphics->resY * 64) / 480 + (int)(g_pGraphics->resY * 384) / 480,
                      (int *)g_helpColour0x00524b88, 0x11);
    }
}

#define CAROUSEL_Y() ((int)(g_pGraphics->resY * 32) / 480 + (int)(g_pGraphics->resY * 384) / 480)

// Horizontal menu at the bottom of the screen: the selected item at a fixed
// place (plus the slide offset of its MenuScroller), the following items to
// its right and the previous ones to its left, each with a separator.
// match 89%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004d45b0
void FrontendDraw_Carousel(Menu *pMenu, char active, char *help)
{
    MenuScroller *p;
    DWORD now;
    int state[2];
    int x0;
    int x;
    int i;
    int w;
    BYTE *pColour;
    BYTE *pShadow;
    BYTE *pSep;
    BYTE *pSepShadow;

    p = FrontendScroller_GetMainScroller();
    g_frontendDrawRect[2] = 1;
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 384) / 480;
    g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 45) / 480;
    now = Sys_GetTicks();
    FrontendScroller_GetSelectionAndTimestamp(&state[1], &state[0]);
    now -= state[0];
    if (help == NULL)
        help = CFrontend::GetTextString(0x57);
    FrontendDraw_HelpText(help, 1);
    if (state[1] != -1 && now <= 250) {
        CFrontend::GetTextString(pMenu->items[pMenu->cursor].id);
        x0 = (int)(g_pGraphics->resX * 102) / 640 + p->offset;
    } else {
        CFrontend::GetTextString(pMenu->items[pMenu->cursor].id);
        x0 = (int)(g_pGraphics->resX * 102) / 640 + p->offset;
    }
    if (p->offset == 0 && active != 0)
        Font_DrawText(2, CFrontend::GetTextString(pMenu->items[pMenu->cursor].id), x0, CAROUSEL_Y(),
                      (int *)g_colourWhite0x00524968, 0x11);
    else
        Font_DrawText(2, CFrontend::GetTextString(pMenu->items[pMenu->cursor].id), x0, CAROUSEL_Y(),
                      (int *)g_colourText0x0052496c, 0x11);
    if (active != 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = p->widths[pMenu->cursor] + p->spacing / 2 + x0;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    g_frontendDrawRect[0]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);

    i = pMenu->cursor;
    x = p->widths[i] + x0 + p->spacing;
    if (active != 0) {
        pSep = g_colourText0x0052496c;
        pSepShadow = g_colourShadowText0x00524978;
    } else {
        pSep = g_colourDim0x00524970;
        pSepShadow = g_colourShadowDim0x0052497c;
    }
    while (x < (int)g_pGraphics->resX) {
        i++;
        if (i >= pMenu->itemCount)
            i = 0;
        if (pMenu->items[i].enabled)
            Font_DrawText(2, CFrontend::GetTextString(pMenu->items[i].id), x, CAROUSEL_Y(), (int *)pSep, 0x11);
        else
            Font_DrawText(2, CFrontend::GetTextString(pMenu->items[i].id), x, CAROUSEL_Y(), (int *)pSepShadow, 0x11);
        g_frontendDrawRect[0] = p->spacing / 2 + p->widths[i] + x;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pSep, 1);
        g_frontendDrawRect[0]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pSepShadow, 1);
        x += p->widths[i] + p->spacing;
    }

    i = pMenu->cursor - 1;
    if (i < 0)
        i = pMenu->itemCount - 1;
    x = x0 - p->spacing - Font_GetTextWidth(2, (BYTE *)CFrontend::GetTextString(pMenu->items[i].id));
    if (active != 0) {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
        pSep = g_colourWhite0x00524968;
        pSepShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourDim0x00524970;
        pShadow = g_colourShadowDim0x0052497c;
        pSep = g_colourText0x0052496c;
        pSepShadow = g_colourShadowText0x00524978;
    }
    for (w = Font_GetTextWidth(2, (BYTE *)CFrontend::GetTextString(pMenu->items[i].id)) + x; w > 0;
         w = Font_GetTextWidth(2, (BYTE *)CFrontend::GetTextString(pMenu->items[i].id)) + x) {
        if (pMenu->items[i].enabled)
            Font_DrawText(2, CFrontend::GetTextString(pMenu->items[i].id), x, CAROUSEL_Y(), (int *)pColour, 0x11);
        else
            Font_DrawText(2, CFrontend::GetTextString(pMenu->items[i].id), x, CAROUSEL_Y(), (int *)pShadow, 0x11);
        g_frontendDrawRect[0] = p->widths[i] + p->spacing / 2 + x;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pSep, 1);
        g_frontendDrawRect[0]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pSepShadow, 1);
        i--;
        if (i < 0)
            i = pMenu->itemCount - 1;
        x -= p->widths[i] + p->spacing;
        if (active != 0) {
            pColour = g_colourText0x0052496c;
            pShadow = g_colourShadowText0x00524978;
        } else {
            pColour = g_colourDim0x00524970;
            pShadow = g_colourShadowDim0x0052497c;
        }
        pSepShadow = pShadow;
        pSep = pColour;
    }
}

#define ROW_H() ((int)(g_pGraphics->resY * 36) / 480)

// Vertical menu: an optional title row and the visible items from "first"
// on, each with its background sprite and a separator line below.
// match 71%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004d3360
void FrontendDraw_MenuList(Menu *pMenu, char *title, int y, int xOffset, int first, int active)
{
    int count;
    int i;
    int x;
    int hasTitle;
    short top;
    int row;
    SpriteRect dst;
    BYTE *pColour;
    BYTE *pShadow;
    int curFirst;

    curFirst = first;
    count = 0;
    hasTitle = 0;
    dst.x = (int)(g_pGraphics->resX * 100) / 640;
    dst.y = 0;
    dst.w = CFrontend::m_pAr640ATexture->width;
    dst.h = CFrontend::m_pAr640ATexture->height;
    for (i = 0; i < pMenu->itemCount; i++) {
        if (pMenu->items[i].visible)
            count++;
    }
    if (xOffset == -1) {
        x = (int)(g_pGraphics->resX * 122) / 640;
    } else {
        x = (int)(g_pGraphics->resX * 122) / 640 - (int)(g_pGraphics->resX * 24) / 640 + xOffset;
        dst.x = (int)(g_pGraphics->resX * 100) / 640 - (int)(g_pGraphics->resX * 24) / 640 + xOffset;
    }
    if (title != NULL)
        hasTitle = 1;
    if (y == -1)
        y = ((int)(g_pGraphics->resY * 8) / 480 + (int)(g_pGraphics->resY * 38) / 480 +
             (int)(g_pGraphics->resY * 384) / 480) / 2;
    hasTitle = count - curFirst + hasTitle;
    top = y - ROW_H() * hasTitle / 2;
    if (pMenu->cursor == curFirst) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    }
    if (xOffset == -1)
        g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    else
        g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640 - (int)(g_pGraphics->resX * 24) / 640 + xOffset;
    g_frontendDrawRect[1] = top;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 282) / 640;
    g_frontendDrawRect[3] = 1;
    if (title != NULL)
        g_frontendDrawRect[1] = ROW_H() + top;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    for (row = 0; row < hasTitle; row++) {
        while (!pMenu->items[curFirst].visible)
            curFirst++;
        dst.y = (int)(g_pGraphics->resY * 20) / 480 + top + ROW_H() * row - CFrontend::m_pAr640ATexture->height / 2;
        if (title == NULL || row != 0) {
            if (pMenu->items[curFirst].visible) {
                if (pMenu->cursor == curFirst && active != 0) {
                    pColour = g_colourWhite0x00524968;
                    Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, &dst, CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, pColour, 8);
                } else {
                    if (pMenu->items[curFirst].enabled && active != 0)
                        pColour = g_colourText0x0052496c;
                    else
                        pColour = g_colourDim0x00524970;
                    Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, &dst, CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, pColour, 8);
                }
                if (pMenu->items[curFirst].id == -1)
                    Font_DrawText(1, pMenu->items[curFirst].text, x,
                                  (short)((int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1]), (int *)pColour, 0x11);
                else
                    Font_DrawText(1, CFrontend::GetTextString(pMenu->items[curFirst].id), x,
                                  (short)((int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1]), (int *)pColour, 0x11);
                if (pMenu->cursor == curFirst || pMenu->cursor == curFirst + 1) {
                    pColour = g_colourWhite0x00524968;
                    pShadow = g_colourShadowWhite0x00524974;
                } else if (!pMenu->items[curFirst].enabled && !pMenu->items[curFirst + 1].enabled) {
                    pColour = g_colourDim0x00524970;
                    pShadow = g_colourShadowDim0x0052497c;
                } else {
                    pColour = g_colourText0x0052496c;
                    pShadow = g_colourShadowText0x00524978;
                }
                g_frontendDrawRect[1] = ROW_H() * (row + 1) + top;
                Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
                g_frontendDrawRect[1]++;
                Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
            }
            curFirst++;
        } else {
            g_frontendDrawRect[3] = ROW_H();
            g_frontendDrawRect[1] = top;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
            Font_DrawText(1, title, x, (short)((int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1]),
                          (int *)g_colourTitle0x00524984, 0x11);
            g_frontendDrawRect[3] = 1;
            g_frontendDrawRect[1] = ROW_H() + top;
        }
    }
}

// Draws the rally-info entry list: an optional title row and one row per
// entry, each with its 640-wide banner texture (bright for the selected
// entry) and a highlight line under the selected ones. index -1 picks the
// vertically centred default row.
// match 82%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// logic verified against the dump; remaining diff is the local layout of the
// per-row rect and register allocation.
// FUNCTION: CMR2 0x004d39a0
void FrontendDraw_RallyEntryList(BYTE *pList, char *pTitle, int index, char **ppStrings)
{
    short rect[4];
    int hasTitle;
    int top;
    int i;
    unsigned int u;
    Texture *pTexture;
    BYTE *pColour;
    BYTE *pShadow;
    BYTE *pLine;
    BYTE *pLineShadow;

    rect[1] = 0;
    rect[0] = (int)(g_pGraphics->resX * 0x50) / 640 - (int)(g_pGraphics->resX * 0x14) / 640;
    rect[2] = CFrontend::m_pAr640ATexture->width;
    rect[3] = CFrontend::m_pAr640ATexture->height;
    hasTitle = 0;
    if (pTitle != NULL)
        hasTitle = 1;
    if (index == -1)
        index = ((int)(g_pGraphics->resY * 8) / 480 + (int)(g_pGraphics->resY * 0x26) / 480 +
                 (int)(g_pGraphics->resY * 0x180) / 480) / 2;
    top = index - ((pList[0xa] + hasTitle) * ((int)(g_pGraphics->resY * 0x1a) / 480)) / 2;
    if (pList[0xb] == 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourText0x0052496c;
        if (!(pTitle == NULL))
            pShadow = g_colourShadowWhite0x00524974;
        else
            pShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[1] = top;
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 0x2d) / 640;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 0xe6) / 640;

    g_frontendDrawRect[3] = 1;
    if (pTitle != NULL)
        g_frontendDrawRect[1] = top + (int)(g_pGraphics->resY * 0x1a) / 480;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    for (i = 0; i < pList[0xa] + hasTitle; i++) {
        rect[1] = (short)(top + (int)(g_pGraphics->resY * 0x10) / 480 +
                          ((int)(g_pGraphics->resY * 0x1a) / 480) * i -
                          CFrontend::m_pAr640ATexture->height / 2);
        if (pTitle != NULL && i == 0) {
            g_frontendDrawRect[1] = top;
            g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 0x1a) / 480;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
            Font_DrawText(1, pTitle, (int)(g_pGraphics->resX * 0x50) / 640,
                          g_frontendDrawRect[1] + (int)(g_pGraphics->resY * 0x14) / 480,
                          (int *)g_colourTitle0x00524984, 0x11);
            g_frontendDrawRect[3] = 1;
            g_frontendDrawRect[1] = top + (int)(g_pGraphics->resY * 0x1a) / 480;
            continue;
        }
        u = i - hasTitle;
        if (pList[0xb] == u) {
            pTexture = CFrontend::m_pAr640ATexture;
            pColour = g_colourWhite0x00524968;
            Sprite_Queue((SpriteRect *)&pTexture->field_0x11c, (SpriteRect *)rect, pTexture, 1, 0, NULL, NULL, pColour, 8);
        } else {
            pTexture = CFrontend::m_pAr640DTexture;
            pColour = g_colourText0x0052496c;
            Sprite_Queue((SpriteRect *)&pTexture->field_0x11c, (SpriteRect *)rect, pTexture, 1, 0, NULL, NULL, pColour, 8);
        }
        Font_DrawText(1, ppStrings[u], (int)(g_pGraphics->resX * 0x50) / 640,
                      g_frontendDrawRect[1] + (int)(g_pGraphics->resY * 0x14) / 480, (int *)pColour, 0x11);
        if (pList[0xb] == u || pList[0xb] == u + 1) {
            pLine = g_colourWhite0x00524968;
            pLineShadow = g_colourShadowWhite0x00524974;
        } else {
            pLine = g_colourText0x0052496c;
            pLineShadow = g_colourShadowText0x00524978;
        }
        g_frontendDrawRect[1] = top + ((int)(g_pGraphics->resY * 0x1a) / 480) * (i + 1);
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
        g_frontendDrawRect[1]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLine, 1);
    }
}

#define SCROLLER_TEXT(i) \
    strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[i].id)); \
    CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest)

// Same as FrontendDraw_Carousel for a menu driven by a MenuScroller, with
// the item names in lower case.
// match 66%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004d2cd0
void FrontendDraw_ScrollerRow(MenuScroller *p, char active)
{
    Menu *pMenu;
    DWORD now;
    int x0;
    int x;
    int i;
    int next;
    BYTE *pColour;
    BYTE *pSep;
    BYTE *pShadow;

    pMenu = p->pMenu;
    g_frontendDrawRect[2] = 1;
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 384) / 480;
    g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 45) / 480;
    now = Sys_GetTicks();
    if (p->previous != -1 && now - p->startTime <= 250) {
        CFrontend::GetTextString(pMenu->items[pMenu->cursor].id);
        x0 = (int)(g_pGraphics->resX * 102) / 640 + p->offset;
    } else {
        CFrontend::GetTextString(pMenu->items[pMenu->cursor].id);
        x0 = (int)(g_pGraphics->resX * 102) / 640 + p->offset;
    }
    SCROLLER_TEXT(pMenu->cursor);
    if (p->offset == 0 && active != 0)
        Font_DrawText(2, CFrontend::m_stringDest, x0, CAROUSEL_Y(), (int *)g_colourWhite0x00524968, 0x11);
    else
        Font_DrawText(2, CFrontend::m_stringDest, x0, CAROUSEL_Y(), (int *)g_colourText0x0052496c, 0x11);
    if (active != 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = p->widths[pMenu->cursor] + p->spacing / 2 + x0;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    g_frontendDrawRect[0]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);

    x = p->widths[pMenu->cursor] + x0 + p->spacing;
    if (active != 0) {
        pSep = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    } else {
        pSep = g_colourDim0x00524970;
        pShadow = g_colourShadowDim0x0052497c;
    }
    i = pMenu->cursor;
    while (x < (int)g_pGraphics->resX) {
        i++;
        if (i >= pMenu->itemCount)
            i = 0;
        SCROLLER_TEXT(i);
        if (!pMenu->items[i].enabled) {
            Font_DrawText(2, CFrontend::m_stringDest, x, CAROUSEL_Y(), (int *)g_colourDim0x00524970, 0x11);
        } else {
            Font_DrawText(2, CFrontend::m_stringDest, x, CAROUSEL_Y(), (int *)pSep, 0x11);
        }
        if (pMenu->items[i].enabled) {
            g_frontendDrawRect[0] = p->spacing / 2 + p->widths[i] + x;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pSep, 1);
            g_frontendDrawRect[0]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
        } else {
            g_frontendDrawRect[0] = p->spacing / 2 + p->widths[i] + x;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourDim0x00524970, 1);
            g_frontendDrawRect[0]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowDim0x0052497c, 1);
        }
        x += p->widths[i] + p->spacing;
    }

    i = pMenu->cursor - 1;
    if (i < 0)
        i = pMenu->itemCount - 1;
    x = x0 - p->widths[i] - p->spacing;
    if (active != 0) {
        pColour = g_colourText0x0052496c;
        pSep = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourDim0x00524970;
        pSep = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    }
    while (p->widths[i] + p->spacing + x > 0) {
        SCROLLER_TEXT(i);
        if (!pMenu->items[i].enabled) {
            Font_DrawText(2, CFrontend::m_stringDest, x, CAROUSEL_Y(), (int *)g_colourDim0x00524970, 0x11);
        } else {
            Font_DrawText(2, CFrontend::m_stringDest, x, CAROUSEL_Y(), (int *)pColour, 0x11);
        }
        next = i + 1;
        if (next >= pMenu->itemCount)
            next = 0;
        if (pMenu->items[next].enabled) {
            g_frontendDrawRect[0] = p->spacing / 2 + p->widths[i] + x;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pSep, 1);
            g_frontendDrawRect[0]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
        } else {
            g_frontendDrawRect[0] = p->spacing / 2 + p->widths[i] + x;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourDim0x00524970, 1);
            g_frontendDrawRect[0]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowDim0x0052497c, 1);
        }
        i--;
        if (i < 0)
            i = pMenu->itemCount - 1;
        x -= p->widths[i] + p->spacing;
        if (active != 0) {
            pShadow = g_colourShadowText0x00524978;
            pSep = g_colourText0x0052496c;
            pColour = g_colourText0x0052496c;
        } else {
            pShadow = g_colourShadowDim0x0052497c;
            pColour = g_colourDim0x00524970;
            pSep = g_colourDim0x00524970;
        }
    }
}
