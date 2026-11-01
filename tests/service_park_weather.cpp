// Exercise the forecast builder and the actual sprite queue without game assets.
#include "port/types.h"
#include "Font.h"
#include "GameInfo.h"
#include "Graphics.h"
#include "Sprite.h"

#include <cstdio>
#include <cstdlib>
#include <initializer_list>

extern void OptionPreview_FillTextureSetIndices();
extern void RallyData_DrawCarSplitTimesPanel(int);
extern int *RallyData_GetDriverSettingPair(int);
extern int *RallyData_GetDriverPrimaryPairRecord(int);
extern int *RallyData_GetDriverSecondaryPairRecord(int);
extern BYTE g_optionSlotTextureSets[20];
extern BYTE g_unk0x0082ca18;
extern Texture *g_weatherSymbolTextures[9];
extern void *g_symbolArrowTexture;
extern short g_optionPreviewExpandedRect[4];
extern short g_unk0x0082c9fe, g_unk0x0082ca02;

#define REQUIRE(x) do { if (!(x)) { fprintf(stderr, "service park weather: line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

int main()
{
    Graphics graphics{};
    g_pGraphics = &graphics;
    CGraphics::m_65536 = 65536.0;
    CGraphics::m_d3dDeviceDesc7.minTextureWidth = CGraphics::m_d3dDeviceDesc7.minTextureHeight = 1;
    CGraphics::m_d3dDeviceDesc7.maxTextureWidth = CGraphics::m_d3dDeviceDesc7.maxTextureHeight = 4096;
    // Text is irrelevant to the forecast's sprite count; leave its fonts unloaded.
    FontSlot fonts[2]{};
    g_fonts = fonts;
    Texture symbols[9]{}, arrow{}, map{};
    for (int i = 0; i < 9; ++i) {
        symbols[i].width = symbols[i].height = 16;
        g_weatherSymbolTextures[i] = &symbols[i];
    }
    arrow.width = arrow.height = 8;
    g_symbolArrowTexture = &arrow;
    map.width = map.height = 256;
    g_optionPreviewExpandedRect[0] = 28;
    g_optionPreviewExpandedRect[1] = 114;
    g_optionPreviewExpandedRect[2] = 256;
    g_optionPreviewExpandedRect[3] = 246;
    g_unk0x0082c9fe = 124;
    g_unk0x0082ca02 = 57;
    for (int stage = 0; stage < 11; ++stage) {
        *RallyData_GetDriverSettingPair(stage) = (stage + 1) % 9;
        *RallyData_GetDriverPrimaryPairRecord(stage) = (stage + 2) % 9;
        *RallyData_GetDriverSecondaryPairRecord(stage) = 0;
    }

    const int resolutions[][2] = {{640, 480}, {1024, 768}, {1920, 1080}, {3440, 1440}};
    // Start with issue #30: Australia, day 2, stages 7 and 8. Then cover all
    // service stops, including the final day with two or three stages.
    const int firstStages[] = {6, 0, 2, 4, 8, 10};
    for (const auto &resolution : resolutions) {
        graphics.resX = resolution[0];
        graphics.resY = resolution[1];
        CGameInfo::SetScreenWidth(resolution[0]);
        CGameInfo::SetScreenHeight(resolution[1]);
        for (Texture &symbol : symbols)
            symbol.width = symbol.height = resolution[0] >= 1024 ? 32 : 16;
        arrow.width = arrow.height = resolution[0] >= 1024 ? 16 : 8;
        for (bool scaled : {false, true}) {
            if (scaled) MenuScreen_Begin();
            for (int first : firstStages) {
                for (int stages : {1, 2}) {
                    if (first + stages > 11) continue;
                    g_optionPreviewPanelCount = stages;
                    for (int i = 0; i < stages; ++i) {
                        FixInterp &panel = g_optionPreviewPanels[i];
                        panel = {};
                        panel.count = first + i;
                        panel.texture = &map;
                        panel.current = panel.phase1 = 0x10000;
                        panel.dstX1 = 28; panel.dstY1 = 114;
                        panel.dstX2 = 256; panel.dstY2 = 246;
                    }
                    OptionPreview_FillTextureSetIndices();
                    REQUIRE(g_unk0x0082ca18 == stages + 1);
                    REQUIRE(g_optionSlotTextureSets[0] == (first + 1) % 9);
                    for (int i = 0; i < stages; ++i)
                        REQUIRE(g_optionSlotTextureSets[i + 1] == (first + i + 2) % 9);
                    for (int i = 0; i < stages; ++i) {
                        g_spriteCount1 = g_spriteCount3 = 0;
                        g_tri2DCount1 = 0;
                        RallyData_DrawCarSplitTimesPanel(i);
                        REQUIRE(g_spriteCount1 == unsigned(2 * (stages + 1) - 1));
                        for (unsigned n = 0; n < g_spriteCount1; ++n) {
                            const Sprite &sprite = g_spriteLayer1[n];
                            REQUIRE(sprite.pTexture == (n % 2 ? &arrow : &symbols[g_optionSlotTextureSets[n / 2]]));
                            // The forecast stays on the map, left of the weather legend.
                            REQUIRE(sprite.dst.x >= 28 * graphics.resX / 640);
                            REQUIRE(sprite.dst.x + sprite.dst.w <= 284 * graphics.resX / 640);
                        }
                    }
                }
            }
            if (scaled) MenuScreen_End();
        }
    }
    g_fonts = nullptr;
    g_pGraphics = nullptr;
    puts("service park weather: forecast values, counts and layout passed at 4:3, 16:9 and ultrawide");
}
