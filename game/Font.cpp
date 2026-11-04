#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Font.h"
#include "FileBuffer.h"
#include "Game.h"
#include "Graphics.h"
#include "InstallInfo.h"
#include "Sprite.h"
#include "port/outline_font.h"

FontSlot *g_fonts;
unsigned int g_fontCount;
FontSlot *g_pCurrentFont;
int g_fontColour[2];
int g_fontBlendMode;

// Destination rectangle of the glyph being drawn (x, y, w, h)
// GLOBAL: CMR2 0x00532130
short g_fontCharRect[4];

// GLOBAL: CMR2 0x005168dc
char g_fontTgaFormat[12] = "%s\\%s.tga";
// GLOBAL: CMR2 0x005168d4
char g_fontPcfExtension[8] = ".pcf";
// GLOBAL: CMR2 0x005168e8
char g_fontPcfFormat[12] = "%s\\%s.pcf";

// FUNCTION: CMR2 0x0040b220
int Font_InitTable(unsigned int count)
{
    g_fontBlendMode = 1;
    g_fonts = (FontSlot *)CFileBuffer::AllocateLockedBuffer((count & 0xff) * sizeof(FontSlot));
    if (g_fonts != NULL) {
        memset(g_fonts, 0, (unsigned char)count * sizeof(FontSlot));
        g_fontCount = count & 0xff;
        return CGame::RegisterCallback(Font_ReleaseAll, NULL);
    }
    return -1;
}

// Exit callback registered by Font_InitTable.
// FUNCTION: CMR2 0x0040b290
BYTE Font_ReleaseAll(void)
{
    BYTE i;

    for (i = 0; i < g_fontCount; i++)
        Font_Release(i);
    CFileBuffer::FreeGenericFileBuffer(g_fonts);
    g_fonts = NULL;
    g_fontCount = 0;
    return 1;
}

// FUNCTION: CMR2 0x0040b2f0
void Font_Release(BYTE index)
{
    // PORT: the glyph table is native storage, see Font_Setup.
    if (g_fonts[index & 0xff].pGlyphs != NULL)
        CFileBuffer::FreeGenericFileBuffer(g_fonts[index & 0xff].pGlyphs);
    if (g_fonts[index & 0xff].ownsData != 0) {
        CFileBuffer::FreeGenericFileBuffer(g_fonts[index & 0xff].pData);
        g_fonts[index & 0xff].pData = NULL;
        g_fonts[index & 0xff].pGlyphs = NULL;
        return;
    }
    g_fonts[index & 0xff].pGlyphs = NULL;
}

// Binds a loaded .pcf buffer and its texture to a font slot and relocates
// the kerning pointers of every glyph.
// FUNCTION: CMR2 0x0040b350
void Font_Setup(void *pData, Texture *pTexture, unsigned int index, char bInArchive)
{
    unsigned int i;
    char *p;

    if (index < g_fontCount && pData != NULL && pTexture != NULL) {
        g_fonts[index].pData = pData;
        g_fonts[index].pTexture = pTexture;
        g_fonts[index].pHeader = (FontHeader *)g_fonts[index].pData;
        g_fonts[index].ownsData = bInArchive == 0;
        if (g_fonts[index].pHeader->glyphCount >= 1) {
            p = (char *)g_fonts[index].pData + sizeof(FontHeader);
            g_fonts[index].pCharMap = (short *)p;
            p += 256 * sizeof(short);
            // PORT: the original relocates the file's glyph table in place
            // (pKern += pData). FontGlyph holds a pointer, so its table is
            // built in native storage from the file's records instead, on
            // every target; the file buffer stays as loaded.
            {
                FontGlyphFile *pFile = (FontGlyphFile *)p;
                unsigned int count = g_fonts[index].pHeader->glyphCount;
                FontGlyph *pGlyphs = (FontGlyph *)CFileBuffer::AllocateLockedBuffer(count * sizeof(FontGlyph));
                if (pGlyphs == NULL)
                    return;
                for (i = 0; i < count; i++) {
                    memset(&pGlyphs[i], 0, sizeof(FontGlyph));
                    memcpy(&pGlyphs[i], pFile[i].header, sizeof(pFile[i].header));
                    pGlyphs[i].pKern = (FontKernPair *)((char *)g_fonts[index].pData + pFile[i].kernOffset);
                }
                g_fonts[index].pGlyphs = pGlyphs;
            }
            g_fonts[index].loaded = 1;
            // PORT: drawn from a distance field, sharp at any scale.
            {
                OutlineFontGlyph cells[256];
                int count = 0;
                for (i = 0; i < 256; i++) {
                    short glyph = g_fonts[index].pCharMap[i];
                    if (glyph < 0 || (unsigned int)glyph >= g_fonts[index].pHeader->glyphCount)
                        continue;
                    FontGlyph *pGlyph = &g_fonts[index].pGlyphs[glyph];
                    cells[count].code = (BYTE)i;
                    cells[count].x = ((short *)pGlyph->field_0x0)[0];
                    cells[count].y = ((short *)pGlyph->field_0x0)[1];
                    cells[count].width = pGlyph->width;
                    cells[count].height = pGlyph->height;
                    count++;
                }
                OutlineFont_Replace(pTexture->name, pTexture->pSurface, cells, count);
            }
        }
    }
}

// FUNCTION: CMR2 0x0040b430
void Font_Load(char *name, GenericFile *pFile, unsigned int index)
{
    Texture *pTexture;
    void *pData;
    BYTE bInArchive;
    char path[260];

    if (index < g_fontCount) {
        sprintf(path, g_fontTgaFormat, CInstallInfo::GetFontsDir(), name);
        pTexture = CTexture::FindLoadTexture(pFile, path, NULL, NULL, 0, 0);
        if (pTexture != NULL) {
            sprintf(path + strlen(path) - 4, "%s", g_fontPcfExtension);
            pData = CGenericFileLoader::FindFile(pFile, path, &bInArchive, NULL, 0);
            if (pData != NULL)
                Font_Setup(pData, pTexture, index, bInArchive);
        }
    }
}

// Reloads only the .pcf data of a font, keeping its texture.
// FUNCTION: CMR2 0x0040b4f0
void Font_Reload(char *name, GenericFile *pFile, unsigned int index)
{
    Texture *pTexture;
    void *pData;
    BYTE bInArchive;
    char path[260];

    if (index < g_fontCount) {
        pTexture = g_fonts[index].pTexture;
        Font_Release(index);
        sprintf(path, g_fontPcfFormat, CInstallInfo::GetFontsDir(), name);
        pData = CGenericFileLoader::FindFile(pFile, path, &bInArchive, NULL, 0);
        if (pData != NULL)
            Font_Setup(pData, pTexture, index, bInArchive);
    }
}

// FUNCTION: CMR2 0x0040b580
void Font_Select(BYTE index, int *pColour)
{
    g_pCurrentFont = &g_fonts[index & 0xff];
    g_fontColour[0] = *pColour;
}

// Width in pixels up to the first line separator ('\n' or '^').
// match 56%: the original keeps the character in a register and homes
// `advance` in the frame; register allocation differs.
// FUNCTION: CMR2 0x0040b5b0
int Font_GetTextWidth(BYTE index, BYTE *text)
{
    FontSlot *pFont;
    short *pCharMap;
    FontGlyph *pGlyphs;
    FontGlyph *pGlyph;
    FontKernPair *pKern;
    short spaceWidth;
    int charSpacing;
    int advance;
    short width;
    short maxWidth;
    short glyph;
    int i;
    int k;
    int kernCount;
    BYTE ch;
    BYTE next;
    BYTE *p;

    maxWidth = 0;
    pFont = &g_fonts[index & 0xff];
    pCharMap = pFont->pCharMap;
    width = 0;
    spaceWidth = (short)pFont->pHeader->spaceWidth;
    charSpacing = pFont->pHeader->charSpacing;
    pGlyphs = pFont->pGlyphs;
    if (pFont->loaded != 0) {
        ch = *text;
        i = 0;
        if (ch != 0) {
            p = text;
            do {
                if (ch == '\n' || ch == '^')
                    break;
                if (ch == ' ') {
                    width += spaceWidth;
                } else {
                    glyph = pCharMap[ch];
                    if (glyph != -1) {
                        advance = charSpacing;
                        pGlyph = &pGlyphs[glyph];
                        if (text[i + 1] != 0 && text[i + 1] != '\n' && text[i + 1] != '^' && pCharMap[text[i + 1]] != -1) {
                            pKern = pGlyph->pKern;
                            kernCount = pGlyph->kernCount;
                            k = 0;
                            if (kernCount > 0) {
                                do {
                                    if (pKern->ch >= p[1]) {
                                        if (pKern->ch == text[i + 1])
                                            advance = pGlyph->pKern[k].offset + charSpacing;
                                        break;
                                    }
                                    k++;
                                    pKern++;
                                } while (k < kernCount);
                            }
                        }
                        width += pGlyph->width + advance;
                    }
                }
                next = text[i + 1];
                if ((next == 0 || next == '\n' || next == '^') && width > maxWidth)
                    maxWidth = width;
                i++;
                p = text + i;
                ch = *p;
            } while (ch != 0);
        }
        return maxWidth;
    }
    return 0;
}

// Counts line separators and includes the font's final line gap.
// FUNCTION: CMR2 0x0040b730
int Font_GetTextHeight(BYTE index, char *text)
{
    int height = 0;
    int lines = 1;
    char *pText = text;
    char ch;
    FontSlot *pFont;
    BYTE loaded = g_fonts[index & 0xff].loaded;
    pFont = &g_fonts[index & 0xff];
    if (loaded) {
        for (ch = *pText; ch != 0; ch = *++pText) {
            if (ch == '\n' || ch == '^') lines++;
        }
        FontHeader *pHeader = pFont->pHeader;
        height = (pHeader->lineHeight + 1) * lines - 1 + pHeader->lineGap;
    }
    return height;
}

// FUNCTION: CMR2 0x0040b790
int Font_GetLineHeight(BYTE index)
{
    FontSlot *pFont;

    pFont = &g_fonts[index & 0xff];
    if (pFont->loaded != 0)
        return pFont->pHeader->lineHeight;
    return 0;
}

// FUNCTION: CMR2 0x0040b7c0
void Font_DrawChar(BYTE ch, short x, short y)
{
    FontSlot *pFont;
    FontGlyph *pGlyph;
    Texture *pTexture;
    int centre[3];

    pFont = g_pCurrentFont;
    pGlyph = &pFont->pGlyphs[pFont->pCharMap[ch & 0xff]];
    pTexture = pFont->pTexture;
    centre[0] = 0;
    centre[1] = 0;
    centre[2] = 0;
    g_fontCharRect[0] = x;
    g_fontCharRect[1] = y;
    g_fontCharRect[2] = pGlyph->width;
    g_fontCharRect[3] = pGlyph->height;
    centre[0] = g_fontCharRect[2] / 2 + x;
    centre[1] = g_fontCharRect[3] / 2 + y;
    Sprite_Queue((SpriteRect *)pGlyph, (SpriteRect *)g_fontCharRect, pTexture, g_fontBlendMode, 0, centre, NULL, (BYTE *)g_fontColour, 8);
}

// Draws text at (x, y). flags: 2 = centre, 4 = right-align (both per line),
// 0x10 = y is the vertical middle of the first line, 0x20 = y is its bottom.
// match 26%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0040b880
void Font_DrawText(BYTE index, char *text, short x, short y, int *pColour, unsigned int flags)
{
    FontSlot *pFont;
    int length;
    int i;
    int k;
    short penX;
    int width;
    int advance;
    short glyph;
    FontGlyph *pGlyph;

    pFont = &g_fonts[index & 0xff];
    rand();
    rand();
    rand();
    if (pFont->loaded != 0 && (index & 0xff) <= g_fontCount) {
        length = strlen(text);
        penX = x;
        if (flags & (FONT_ALIGN_CENTRE | FONT_ALIGN_RIGHT)) {
            width = Font_GetTextWidth(index, (BYTE *)text);
            if (flags & FONT_ALIGN_CENTRE)
                width /= 2;
            penX = x - width;
        }
        if (flags & FONT_ORIGIN_MIDDLE)
            y -= pFont->pGlyphs->field_0xa[0];
        if (flags & FONT_ORIGIN_BOTTOM)
            y = (unsigned short)((short)y - (short)pFont->pHeader->lineHeight);
        Font_Select(index, pColour);
        for (i = 0; i < length; i++) {
            if (text[i] == ' ') {
                penX += (short)pFont->pHeader->spaceWidth;
            } else if (text[i] == '\n' || text[i] == '^') {
                penX = x;
                if (flags & (FONT_ALIGN_CENTRE | FONT_ALIGN_RIGHT)) {
                    width = Font_GetTextWidth(index, (BYTE *)text + i + 1);
                    if (flags & FONT_ALIGN_CENTRE)
                        width /= 2;
                    penX = x - width;
                }
                y += (short)(pFont->pHeader->lineGap + pFont->pHeader->lineHeight);
            } else {
                glyph = pFont->pCharMap[(BYTE)text[i]];
                if (glyph != -1) {
                    advance = pFont->pHeader->charSpacing;
                    pGlyph = &pFont->pGlyphs[glyph];
                    if (i < length - 1 && pFont->pCharMap[(BYTE)text[i + 1]] != -1) {
                        for (k = 0; k < pGlyph->kernCount; k++) {
                            if (pGlyph->pKern[k].ch >= (BYTE)text[i + 1]) {
                                if (pGlyph->pKern[k].ch == (BYTE)text[i + 1])
                                    advance += pGlyph->pKern[k].offset;
                                break;
                            }
                        }
                    }
                    Font_DrawChar(text[i], (short)penX, (short)y);
                    penX += (short)(pGlyph->width + advance);
                }
            }
        }
    }
}

// FUNCTION: CMR2 0x0040bab0
void Font_SetBlendMode(int mode)
{
    g_fontBlendMode = mode;
}

// FUNCTION: CMR2 0x0040bac0
int Font_Unused(char *unused1, int unused2)
{
    return 0;
}

void Font_DrawTextScaled(BYTE index, const char *text, int x, int y, int pixelHeight,
                         int maxWidth, const BYTE *colour, int layer)
{
    if (!g_fonts || index >= g_fontCount || !g_fonts[index].loaded || !text ||
        pixelHeight <= 0 || maxWidth <= 0) return;
    FontSlot &font = g_fonts[index];
    int sourceHeight = font.pHeader->lineHeight;
    for (unsigned i = 0; i < font.pHeader->glyphCount; ++i)
        if (font.pGlyphs[i].height > sourceHeight) sourceHeight = font.pGlyphs[i].height;
    if (sourceHeight <= 0) return;
    double scale = (double)pixelHeight / sourceHeight, pen = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (*p == '\n' || *p == '^') break;
        if (*p == ' ') { pen += font.pHeader->spaceWidth * scale; continue; }
        // Uppercase cells are absent from the reported native diagnostic frame.
        // Their exact atlas failure is not established. Use lowercase for
        // labels/status words, independently of the normal game font path.
        unsigned char ch=(*p>='A' && *p<='Z')?*p-'A'+'a':*p;
        int glyph = font.pCharMap[ch];
        if (glyph < 0 || (unsigned)glyph >= font.pHeader->glyphCount) {
            pen += font.pHeader->spaceWidth * scale; continue;
        }
        FontGlyph &g = font.pGlyphs[glyph];
        int left = (int)(pen + 0.5), width = (int)(g.width * scale + 0.5);
        int height = (int)(g.height * scale + 0.5);
        if (left + width > maxWidth) break;
        if (width > 0 && height > 0) {
            SpriteRect dst = {(short)(x+left), (short)y, (short)width, (short)height};
            int centre[3] = {dst.x+width/2, y+height/2, 0};
            Sprite_Queue((SpriteRect *)&g, &dst, font.pTexture, layer, 0, centre, NULL, (BYTE *)colour, 8);
        }
        int advance = font.pHeader->charSpacing;
        for (int k=0; k<g.kernCount; ++k)
            if (g.pKern[k].ch == p[1]) { advance += g.pKern[k].offset; break; }
        pen += (g.width + advance) * scale;
    }
}
