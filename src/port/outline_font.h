/*
 * Sharp text at any scale: the game's bitmap fonts drawn from distance fields
 * (src/render/outline_font.cpp).
 */
#ifndef OPENCMR2_PORT_OUTLINE_FONT_H
#define OPENCMR2_PORT_OUTLINE_FONT_H

#include "port/types.h"
#include "port/gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A glyph of a bitmap font: its character code and its cell in the font's
   texture, as high as the font's line, with the baseline at the same row in
   every cell. */
typedef struct OutlineFontGlyph {
    BYTE code;
    short x, y, width, height;
} OutlineFontGlyph;

/* Gives a font texture, named as the game names it (".\Fonts\general\
   hel_15pt.DDS"), a distance-field replacement with the same cells
   (Gfx_SetTextureReplacement): matching ASCII letters and digits in the
   Helvetica fonts (hel_*) from TeX Gyre Heros, fitted to each glyph's size,
   weight and place, the others from their bitmaps. Localised glyphs,
   including Cyrillic, keep the shapes supplied by the game's fonts.
   Text keeps the original's layout and draws with
   sharp edges however much it is scaled. [video] outline_fonts = 0 keeps the
   bitmaps. */
void OutlineFont_Replace(const char *name, GfxTexture *texture, const OutlineFontGlyph *glyphs, int count);

#ifdef __cplusplus
}
#endif

#endif
