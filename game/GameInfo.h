#ifndef _GAME_INFO_H
#define _GAME_INFO_H

#include "FixedPoint.h"
#include "port/net.h"

// Record words as the original's bitfields (it merges adjacent field stores).
struct RecordTimeBits {
	unsigned car : 6;
	unsigned automatic : 1;  // automatic gearbox ("AT" in the record lists)
	unsigned time : 16;
	unsigned rest : 9;
};

struct RecordFlagBits {
	unsigned car : 6;
	unsigned automatic : 1;  // automatic gearbox ("AT" in the record lists)
	unsigned level : 4;
	unsigned extra : 4;
	unsigned rest : 17;
};

struct GameInfo0xa4SubStruct8 {
	char ident[4];
	union {
		unsigned int value;
		RecordTimeBits bits;
	};
};

struct GameInfo0xa4SubStruct12 {
	char ident[4];
	union {
		unsigned int flags;
		RecordFlagBits bits;
	};
	unsigned int value;
};

// and this is actually meant to be 0x12c4 (4804) bytes in size.
struct GameInfo0xa4
{
	GameInfo0xa4SubStruct12 firstLoop[15];
	GameInfo0xa4SubStruct12 secondLoop[120];
	GameInfo0xa4SubStruct8 rallyStageRecordTimes[88];
	short rallyStageRecordSplits[88][10];
	GameInfo0xa4SubStruct12 thirdLoop[45];
	GameInfo0xa4SubStruct8 arcadeRecordTimes[9];
	short arcadeRecordSplits[9][6];
};

// looks like this should be 0x3958 (14680) bytes in size.
struct RallyUnlockProgress {
    unsigned otherOptions : 8;
    unsigned easyRallies : 4;
    unsigned normalRallies : 4;
    unsigned hardRallies : 4;
    unsigned reserved : 12;
};

struct GameInfo
{
    unsigned char empty[16];
    int magicNumber;          /* Always 0x11 */
    unsigned int field_0x14; /* first byte of this contains language */
    unsigned int field_0x18;
    unsigned int field_0x1c;
    unsigned int field_0x20;
    int screenWidth;
    int screenHeight;
    int screenColourDepth;
    unsigned int graphicsOptions; // first byte is fullscreen
    unsigned int field_0x34;
    char graphicsCardName[80];
    unsigned int chaseCameraHeight;   // 0x88 default ChaseCameraSettings
    unsigned int chaseCameraDistance; // 0x8c
    unsigned int chaseCameraLag;      // 0x90
    WORD chaseCameraPitch;            // 0x94
    BYTE field_0x96_padding[2];
    short field_0x98;
    short field_0x9a;
    union { unsigned int field_0x9c; RallyUnlockProgress rallyUnlockProgress; };
    unsigned int field_0xa0;
    GameInfo0xa4 field_0xa4;
    GameInfo0xa4 field_0x1368;
    GameInfo0xa4 field_0x262c;
    unsigned int field_0x38f0;
    unsigned int field_0x38f4;
    char field_0x38f8[88];
    char field_0x3950[21];
    char field_0x3965[21];
    short field_0x397a;
    int field_0x397c;
    BYTE field_0x3980;
    BYTE field_0x3981_padding[3];
    int field_0x3984;
    int field_0x3988;
    int field_0x398c;
    int field_0x3990;
    BYTE field_0x3994_padding[5]; // seems wrong that this 5 bytes, but we're a byte short without it
};

struct Unk0x0059fa20 {
    DWORD field_0x0;
    DWORD field_0x4;
    BYTE field_0x10;
};

// 0x14-byte entry of the table at 0x82b2c0.
struct Unk0x0082b2c0 {
    int field_0x0;
    int field_0x4;
    int field_0x8;
    int field_0xc;
    int field_0x10;
};

// 0x50-byte entry of the option panel table at 0x82c6c8 (g_optionPreviewPanels): a
// picture slid from its source to its destination rectangle by a 16.16 value
// animated between start and end at 50 units/s, then faded in two phases.
// The panel, split and animation code all read these entries.
struct Texture;
struct FixInterp {
    Texture *texture;       // 0x00 picture displayed by the panel
    short srcX1;            // 0x04 rectangle while the value is 0
    short srcY1;
    short srcX2;
    short srcY2;
    short dstX1;            // 0x0c rectangle when it is 1
    short dstY1;
    short dstX2;
    short dstY2;
    int start;              // 0x14
    int end;                // 0x18
    int current;            // 0x1c
    int distance;           // 0x20 |end - start| * 50
    int phase1;             // 0x24
    int phase2;             // 0x28
    int u0;                 // 0x2c texture coordinates in the map texture (16.16)
    int u1;
    int v0;
    int v1;
    unsigned int startTime; // 0x3c
    int startTime2;         // 0x40
    int startTime3;         // 0x44
    union {
        int field_0x48;     // 0x48
        BYTE count;         // 0x48 low byte, read by the next entry's code
    };
    int active;             // 0x4c
};

extern FixInterp g_optionPreviewPanels[16];
extern BYTE g_selectedOptionPreviewPanel; // 0xff means no selection
extern int g_optionPreviewPanelCount;
void OptionMenu_TransformHudPointToScreen(FixInterp *p, short *pX, short *pY);

struct Menu *OptionMenu_GetControlSetupMenu(void);
void FixInterp_StartToOne(FixInterp *p);
void FixInterp_StartToZero(FixInterp *p);

class CGameInfo
{
public:
    static unsigned char GetGameLanguage(void);
    static unsigned int GetGameRegion(void);
    static void SetGameRegion(unsigned int region);
    static unsigned char GetConfiguredGameMode(void);
    static unsigned char GetConfiguredDifficulty(void);
    static unsigned char GetConfiguredPlayerCount(void);
    static char *GetCodeEntryText(int index);
    static void SaveOptionMenuCallbackCount(void);
    static int GetActiveOptionSlot(void);
    static void StartOptionMenuTimeout(void);
    static int GetOptionMenuRemainingTime(void);
    static void StartOptionRecordTransition(int index, int param2, int param3);
    static int IsOptionMenuTimeoutPulseOn(void);
    static int GetPreviewLayoutMode(void);
    static int SelectIdleOptionPreviewEntry(BYTE param1);
    static int EnumerateNetworkSessions(void);
    static int EnumerateNetworkSessionsWithUserData(char *pPassword);
    static void SwitchOptionPreviewMode(int param1);
    static int GetNetworkStageBestTime(int index);
    static unsigned char IsConfiguredMultiplayer(void);
    static unsigned char IsSplitBarEnabled(void);
    static unsigned char GetGameModeOptionBits20To22(void);
    static unsigned char GetGameModeOptionBit19(void);
    static void SetMasterSoundVolume(unsigned int param1);
    static unsigned int GetMasterSoundVolume(void);
    static void SetEffectsSoundVolume(unsigned int param1);
    static unsigned int GetEffectsSoundVolume(void);
    static void SetCoDriverSoundVolume(unsigned int param1);
    static unsigned int GetCoDriverSoundVolume(void);
    static unsigned int IsDashOptionEnabled(void);
    static void SetDashOptionEnabled(BYTE param1);
    static unsigned int GetNetworkOptionBits1To2(void);
    static void SetSoundOptionBits24To25(unsigned int param1);
    static void SetSoundOptionBit27(BYTE param1);
    static void SetSoundOptionBit26(BYTE param1);
    static void SetSoundOptionBit28(BYTE param1);
    static void SetSoundOptionBit29(BYTE param1);
    static void SetDefaultCameraParameters(FixVector *pOffset, WORD pitch, DWORD lag);
    static void SetGameInfoField98(unsigned int param1);
    static unsigned int *GetGameInfoField9CAddress(void);
    static GameInfo0xa4 *GetGameInfoFieldA4Address(void);
    static GameInfo0xa4 *GetPlayerRecordTable(int param1);
    static void SetFullscreen(BYTE fullscreen);
    static unsigned int GetGraphicsOptionBits1To2(void);
    static void SetGraphicsOptionBits1To2(unsigned int param1);
    static void SetGraphicsOptionBit3(BYTE param1);
    static unsigned int GetGraphicsOptionBit4(void);
    static void SetGraphicsOptionBit4(BYTE param1);
    static void SetGraphicsOptionBits5To8(unsigned int param1);
    static unsigned int GetGraphicsOptionBits18To19(void);
    static void SetGraphicsOptionBits18To19(unsigned int param1);
    static void SetGraphicsOptionBits21To24(unsigned int param1);
    static unsigned int GetGraphicsOptionBits25To26(void);
    static void SetGraphicsOptionBits25To26(unsigned int param1);
    static unsigned int GetGraphicsOptionBits27To28(void);
    static void SetGraphicsOptionBits27To28(unsigned int param1);
    static unsigned int GetGraphicsOptionBit29(void);
    static unsigned int GetPreviewMode(void);
    static void SetPreviewMode(unsigned int param1);
    static unsigned int GetScreenWidth(void);
    static void SetScreenWidth(unsigned int width);
    static unsigned int GetScreenHeight(void);
    static void SetScreenHeight(unsigned int height);
    static unsigned int GetColourDepth(void);
    static void SetColourDepth(unsigned int depth);
    static char *GetGameRegionDirectory(void);
    static void InitRegionLanguageCount(void);
    static void SetGameModeOptionBit19(BYTE param1);
    static void InitDefaultGameInfo(void);
    static void ResetDefaultCameraParameters(void);
    static void InitProfileRecordDefaults(GameInfo0xa4 *param1);
    static void ResetStageOptionStates(void);
    static void ApplyStageOptionUnlockFlags(void);
    static DWORD SetupInputs(int unused);
    static bool LoadGameInfo(void);
    static unsigned int IsFullscreen(void);
    static int GetGraphicsOptionBits21To24(void);
    static bool IsRecordFlagSet(int param1);
    static bool ToggleRecordFlag(int param1);
    static void SetFrontendSessionFlag(BYTE param1);
    static void SetInputAndGamePaused(unsigned int param1);
    static void SetInputPausedFlag(unsigned int param1);
    static void InitFrontendSessionBuffer(void);
    static bool ReleaseFrontendSessionBuffer(void);
    static void ResetNetworkSessionState(void);
    static unsigned int GetGraphicsOptionBits5To8(void);
    static unsigned int GetGraphicsOptionBits9To12(void);
    static unsigned char GetSoundOptionBit30(void);
    static BYTE GetGameInfoSessionFlag(void);
    static void SetGameInfoSessionFlag(BYTE param1);
    static void SetSplitBarEnabled(BYTE param1);
    static unsigned int GetUnlockFlagMask(int param1);
    static void SetUnlockFlag(int param1, int param2);
    static int IsConfiguredCheatEnabled(int param1);
    static int IsActiveCheatEnabled(int param1);
    static unsigned int GetActiveCheatMask(void);
    static unsigned int GetNetworkOptionBit11(void);
    static unsigned int GetRecordFlagsWord(unsigned int **param1);
    static void SaveFrontendOptionSettings(void);
    static BYTE GetStageOptionState(int param1, int param2);
    static void SetStageOptionState(int param1, int param2, BYTE param3);
    static char *GetSessionName(void);
    static void SetSessionName(char *name);
    static char *GetSessionPassword(void);
    static void SetSessionPassword(char *name);
    static unsigned char IsInRaceMenuOpen(void);
    // Menu/pause input mode (set by SetInputAndGamePaused): CInput::ReadKeyboardDevice reads the arrows for the menu while it is non-zero.
    static unsigned int IsInputAndGamePaused(void);

    // GLOBAL: CMR2 0x0052afa0
    static GameInfo m_gameInfo;

    // GLOBAL: CMR2 0x0052ea52
    static BYTE m_unk0x0052ea52;

    // cached copies of bitfield values, refreshed by SaveFrontendOptionSettings
    // GLOBAL: CMR2 0x0052af40
    static unsigned char m_unk0x0052af40;   // byte flag, siblings are the cached bitfields below
    // GLOBAL: CMR2 0x0052af80
    static unsigned int m_unk0x0052af80;
    // GLOBAL: CMR2 0x0052af84
    static unsigned int m_unk0x0052af84;
    // GLOBAL: CMR2 0x0052af88
    static unsigned int m_unk0x0052af88;
    // GLOBAL: CMR2 0x0052af8c
    static unsigned int m_unk0x0052af8c;
    // GLOBAL: CMR2 0x0052af94
    static unsigned int m_unk0x0052af94;
    // GLOBAL: CMR2 0x0052af98
    static unsigned int m_unk0x0052af98;
    // GLOBAL: CMR2 0x0052af9c
    static unsigned int m_unk0x0052af9c;
    // GLOBAL: CMR2 0x0052e93c
    static unsigned int m_unk0x0052e93c;
    // GLOBAL: CMR2 0x0052e940
    static unsigned int m_unk0x0052e940;
    // GLOBAL: CMR2 0x0052ea44
    static unsigned int m_unk0x0052ea44;
    // GLOBAL: CMR2 0x0052ea48
    static unsigned int m_unk0x0052ea48;

    // GLOBAL: CMR2 0x0052ea54
    static unsigned int m_gameRegion;

    // GLOBAL: CMR2 0x00516124
    static char *m_gameRegionStrings[4];

    // GLOBAL: CMR2 0x00817574
    static BYTE m_unk0x00817574;    

    // GLOBAL: CMR2 0x0081a754
    static int m_unk0x0081a754;

    // GLOBAL: CMR2 0x00516184
    static char m_stringCMR[4];

    // GLOBAL: CMR2 0x005250dc
    static char m_stringGameInfoRCF[32];

    // GLOBAL: CMR2 0x0059f8d0
    static unsigned int m_unk0x0059f8d0;

    // GLOBAL: CMR2 0x00520870
    static unsigned int m_unk0x00520870;

    // GLOBAL: CMR2 0x0081777c
    static void * m_unk0x0081777c;
    
    // GLOBAL: CMR2 0x00817678
    static BOOL m_unk0x00817678;

    // GLOBAL: CMR2 0x005a00b8
    static char m_unk0x005a00b8[0x104];     // session name
    // GLOBAL: CMR2 0x005a02c0
    static char m_unk0x005a02c0[0x104];     // session password
    // GLOBAL: CMR2 0x005a03c4
    static char m_sessionNames[20][0x104];
    // GLOBAL: CMR2 0x005a0060
    static BOOL m_unk0x005a0060;
    // GLOBAL: CMR2 0x005a1814
    static HRESULT m_unk0x005a1814;

    // GLOBAL: CMR2 0x005a01bc
    static BYTE m_unk0x005a01bc;
    // GLOBAL: CMR2 0x0059fa20
    static NetSessionDesc m_enumeratedSessions[20];

};

// NetSessionDesc of the hosted/joined session (0x50 bytes, defined in Game.cpp);
// its lpszSessionName/lpszPassword fields are set to the name buffers below.
extern NetSessionDesc g_networkSession;

#endif
