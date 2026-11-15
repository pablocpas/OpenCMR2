#ifndef _GAME_H
#define _GAME_H

#include "port/net.h"


#include "CallbackState.h"

struct SessionPlayerRecord {
    char shortName[100];              // 0x00
    char longName[100];               // 0x64
    unsigned int playerId;            // 0xc8  DirectPlay player id
    unsigned int active;              // 0xcc  slot is occupied
};

// GLOBAL: CMR2 0x00511a28
// CLSID_DirectPlay

// GLOBAL: CMR2 0x00511a18
// IID_IDirectPlay4A

// GLOBAL: CMR2 0x00511ae8
// CLSID_DirectPlayLobby

// GLOBAL: CMR2 0x00511ad8
// IID_IDirectPlayLobby3A

// PORT: a network service provider (pConnection holds its index).
struct DPlayConnection {
    char name[256];
    void *pConnection;
    NetGuid guidSP;
};

extern NetSessionDesc g_networkSession;

// GLOBAL: CMR2 0x00511a38
// DPSPGUID_IPX

class CGame
{
public:
    static BOOL IsActive(void);
    static int GetCallbackCount(void);
    static void UnwindCallbacks(int count);
    static void SkipNextCallbackRenderPass(void);
    static int GetDrawnMeshTriangleCount(void);
    static int GetDrawnOverlayTriangleCount(void);
    static void SetObjectRenderMode(int param1);
    static int GetObjectRenderMode(void);
    static void QueuePrimaryDrawObject(void *param1);
    static void QueueSecondaryDrawObject(void *param1);
    static void SetSectorDrawState(int param1);
    static int GetSectorDrawState(void);

    // GLOBAL: CMR2 0x0059ce14
    static int m_unk0x0059ce14;
    // GLOBAL: CMR2 0x0059ce18
    static int m_unk0x0059ce18;
    static int m_unk0x0059ce1c;
    // GLOBAL: CMR2 0x0059ce20
    static int m_unk0x0059ce20;
    // GLOBAL: CMR2 0x0059ce28
    static int m_deferredDrawCount;
    // GLOBAL: CMR2 0x0059ce2c
    static int m_transparentDrawCount;
    // The list ends at 0x593cb0 + 4096 * 4: the original's frame stamps
    // (0x597cb0/0x597cb4) are compiler statics, so they cannot be list slots.
    // GLOBAL: CMR2 0x00593cb0
    static void *m_deferredDrawObjects[4096];
    // GLOBAL: CMR2 0x00597d04
    static void *m_transparentDrawNodes[4096];
    // GLOBAL: CMR2 0x005207f8
    static int m_unk0x005207f8;
    static void SetGameInputFocusState(int param1);
    static int GetGameInputFocusState(void);
    static bool CreateDirectPlay(void);
    static bool CreateDirectPlayLobby(void);
    static void ClearConnections(void);
    static void AddConnection(char *name, void *pConnection, unsigned int size, NetGuid *pGuidSP);
    static int __cdecl CompareConnections(const void *a, const void *b);
    static unsigned int GetConnectionCount(void);
    static DPlayConnection *GetConnection(BYTE index);
    static bool RejectUnsupportedNetworkOperation(BYTE param1, int param2, int param3);

    // GLOBAL: CMR2 0x00663dc4
    static int m_unk0x00663dc4;
    // GLOBAL: CMR2 0x00664650
    static DPlayConnection m_connections[10];
    // GLOBAL: CMR2 0x00665118
    static BYTE m_maxConnections;
    // GLOBAL: CMR2 0x00665119
    static BYTE m_connectionCount;
    static void SetShouldExit(void);
    static BOOL DispatchFrontendResourceState(void);
    static int GetFrontendResourceMode(void);
    static void RunStateUpdateCallbacks(CallbackStateMachine *param1);
    static void RunStateRenderCallbacks(CallbackStateMachine *param1);
    static void AdvanceCallbackStateTimers(CallbackStateMachine *param1);
    static void InitializeCallbackStateRecord(CallbackStateRecord *param1, int param2, int param3);
    static void InitializeCallbackStateMachine(CallbackStateMachine *p1, BYTE count, CallbackStateRecord *records, StateCallbackPair *callbacks, unsigned int *rules);
    static void InitializeGame(CallbackStateMachine *p1, BYTE p2);
    static BOOL UpdateSecondaryCallbackMachine();
    static BOOL UpdateInRaceCallbackMachine();
    static BOOL UpdateFrontendCallbackMachine(void);
    static void NoOpSecondaryStateCallback(struct CallbackStateMachine *, BYTE);
    static int PromoteCallbackEntryByRule(CallbackStateMachine *p, BYTE index, BYTE value, int level);
    static BYTE GetConfigurationStateByte(void);
    static void SetProfileSelectionState(BYTE param1);
    static void SetSecondaryOptionStateByte(BYTE param1);
    static bool ConsumeOptionRefreshRequest(void);
    static int RegisterCallback(void *param1, void *param2);
    // PORT: MSVC converted function pointers to void * implicitly; this
    // overload does it for any release callback (they return BYTE, BOOL,
    // bool or int).
    template <typename R>
    static int RegisterCallback(R (*param1)(void), void *param2)
    {
        return RegisterCallback((void *)param1, param2);
    }
    static void UpdateActiveSoundSlots(void);
    static void SaveRaceCallbackDepth(void);
    static void RegisterNetworkResourceRelease(void);
    static void FreeNetworkReceiveBuffer(void);
    static void ReleaseNetworkReceiveBuffer(void);
    static void ResetSessionPlayerTable(bool param1);
    static BOOL DestroyLocalNetworkPlayer(void);
    static void FreeServiceProviderConnections(void);
    static bool Cleanup(void);
    static void DestroyDirectPlay(void);
    static void DestroyDirectPlayLobby(void);
    static void *GetDirectPlay(void);
    static bool LoadAndInitializeSplashScreens(bool param1);
    static bool InitializeNetworkSubsystem(void);
    static void LoadFrontendCommonAndCountryTextures(void);
    static void SetStartupFlag(void);
    static void SetFrontendResourceMode(int param1);
    
    // GLOBAL: CMR2 0x00663db8
    static BOOL m_shouldExit;
    // GLOBAL: CMR2 0x00663db4
    static BOOL m_isActive;
    // GLOBAL: CMR2 0x00523c58
    static int m_unk0x00523c58;
    // GLOBAL: CMR2 0x00523c5c
    static int m_unk0x00523c5c;

// GLOBAL: CMR2 0x0052ea4c
    static int m_unk0x0052ea4c;
    // GLOBAL: CMR2 0x0052ea51
    static BYTE m_unk0x0052ea51;
    // GLOBAL: CMR2 0x00817eb0
    static bool m_frontendCallbackInitialized;
    // GLOBAL: CMR2 0x00817da0
    static CallbackStateMachine m_frontendCallbackMachine;
    // GLOBAL: CMR2 0x00817d98
    static CallbackStateRecord m_frontendCallbackRecord;
    // State transition rules of the grouped callback machine (PromoteCallbackEntryByRule):
    // byte 0 = current state, byte 1 = match (0xff = any), byte 2 = next
    // state, byte 3 = level. Terminated by 0xffffffff.
    // GLOBAL: CMR2 0x00523c18
    static unsigned int m_frontendStateRules[16];
    // GLOBAL: CMR2 0x00593cac
    static BYTE m_skipRenderCallbacks;

    // GLOBAL: CMR2 0x00593ba4
    static CallbackStateRecord *m_currentCallbackRecord;
    // GLOBAL: CMR2 0x00593ba8
    static CallbackIndex m_callbackIndex;

    // GLOBAL: CMR2 0x00523bc8
    static StateCallbackPair m_initializeGameGroupedFuncTable[10];

    // GLOBAL: CMR2 0x00523d68
    static BYTE m_unk0x00523d68;
    // GLOBAL: CMR2 0x008180f9
    static BYTE m_unk0x008180f9;
    // GLOBAL: CMR2 0x008180fc
    static BYTE m_unk0x008180fc;
    // GLOBAL: CMR2 0x00516120
    static BYTE m_unk0x00516120;
    // GLOBAL: CMR2 0x00531768
    static BYTE m_unk0x00531768;
    // GLOBAL: CMR2 0x0052ea58
    static BYTE m_unk0x0052ea58;
    // GLOBAL: CMR2 0x0052ea59
    static BYTE m_unk0x0052ea59;

    static void *m_callbacks[64];
    static int m_unk0x00593ba0;

    // GLOBAL: CMR2 0x005a1818
    static BYTE m_sessionPlayerCount;

    // GLOBAL: CMR2 0x005a1819
    static BYTE m_unk0x005a1819;

    // GLOBAL: CMR2 0x005a1820
    static SessionPlayerRecord m_sessionPlayers[7];
    
    // GLOBAL: CMR2 0x005a1e34
    static int m_unk0x005a1e34;

    // GLOBAL: CMR2 0x005a1fc0
    static bool m_unk0x005a1fc0;

    // GLOBAL: CMR2 0x0066521c
    static void *m_pDirectPlay4A;        // PORT: non-NULL once the network layer is in use

    // GLOBAL: CMR2 0x005a1ea0
    static NetPlayerID m_localPlayerId;

    // GLOBAL: CMR2 0x00665220
    static void *m_pDirectPlayLobby3A;   // PORT: unused (DirectPlay lobby)

    // GLOBAL: CMR2 0x005a1fb8 
    static void *m_unk0x005a1fb8;    

    // GLOBAL: CMR2 0x005a1fbc
    static BOOL m_unk0x005a1fbc;

    
    
};

// PORT: saving the configuration outside the original's race transitions.
// The menus call Game_SaveOnMenuChange every frame; when another screen is
// shown it saves the configuration and the edited profiles that changed.
void Game_SaveOnMenuChange(void);
// Records the configuration just saved or loaded as the saved one.
void Game_NoteSavedConfiguration(void);
// The menus are left for a race, which changes the configuration for itself.
void Game_LeaveMenus(void);
// Saves the changes of the menus right away (the window lost the focus).
void Game_SaveMenuChanges(void);
// The window is closed: saves what Quit in the main menu saves.
void Game_SaveBeforeQuit(void);

#endif
