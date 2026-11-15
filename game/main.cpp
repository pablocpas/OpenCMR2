#include "port/sys.h"
#include "port/audio.h"
#include "port/diagnostics.h"
#include "main.h"
#include "Graphics.h"
#include "Logger.h"
#include "Game.h"
#include "Input.h"
#include "Sound.h"

void Main_InitD3DX(void);
void Input_ClearKeyPressQueue(void);
int Args_Parse(char *pCommandLine);

int CMain::m_frameTime;
BOOL CMain::m_frameDeltaInitialised;
unsigned int CMain::m_frameDeltaStart;
unsigned int CMain::m_frameDeltaLast;
unsigned int CMain::m_frameDelta;
unsigned int CMain::m_frameDeltaMax;
int CMain::m_unk0x00663dbc;
int CMain::m_unk0x00663dc0;

// GLOBAL: CMR2 0x005210ac
int g_unk0x005210ac = 1;

char CMain::m_logFileLocation[14] = "c:\\error.txt";
char CMain::m_gameName[20] = "Colin McRae Rally 2";
char CMain::m_logFileHeader1[29] = "FILE_PRINT DEBUG INFORMATION";
char CMain::m_logFileAsterisks[29] = "****************************";
// Second asterisk banner the log footer prints twice: a separate copy that
// lives just before the header's one in the original .data.
// GLOBAL: CMR2 0x00520c14
char g_logFileFooterAsterisks[30] = "*****************************";
char CMain::m_logFileBlankLine[1] = "";
char CMain::m_logFileFinishedNormally[30] = "* Program finished normally *";
BOOL CMain::m_isShowingCursor = TRUE;

int CMain::m_exitCode;


// FUNCTION: CMR2 0x004a9720
// PORT: WinMain's body. The Win32 message loop is an SDL event loop: all
// pending events are handled, then a frame runs unless the game is inactive
// (CGame::m_isActive is set while the window is in the background), in which
// case it sleeps until the next event. The single-instance check is gone.
unsigned int CMain::Initialize(const char *commandLine)
{
	SysEvent event;
	char commandLineCopy[1024];

	CLogger::OpenLogFile(m_logFileLocation);
	CLogger::LogToFile(m_logFileHeader1);
	CLogger::LogToFile(m_logFileAsterisks);
	CLogger::LogToFile(m_logFileBlankLine);
	Main_InitD3DX();
	CGame::RegisterNetworkResourceRelease();
	Input_ClearKeyPressQueue();
	strncpy(commandLineCopy, commandLine, sizeof(commandLineCopy) - 1);
	commandLineCopy[sizeof(commandLineCopy) - 1] = 0;
	Args_Parse(commandLineCopy);

	CreateGameWindow(m_gameName);

	while (!CGame::m_shouldExit)
	{
		if (CGame::m_isActive)
		{
			if (Sys_WaitEvent(&event))
				HandleEvent(&event);
		}
		while (!CGame::m_shouldExit && Sys_PollEvent(&event))
			HandleEvent(&event);
		if (CGame::m_shouldExit)
			break;

		if (!CGame::m_isActive)
		{
			CGame::UpdateActiveSoundSlots();
			if (Diagnostics::observer)
				Diagnostics::observer->BeginFrame();
			CGame::DispatchFrontendResourceState();
			if (Diagnostics::observer)
				Diagnostics::observer->EndFrame();
		}

		if (g_pGraphics->isFullscreen != FALSE)
		{
			if (m_isShowingCursor != FALSE)
			{
				Sys_ShowCursor(0);
				m_isShowingCursor = FALSE;
			}
		}
		else if (m_isShowingCursor == FALSE)
		{
			Sys_ShowCursor(1);
			m_isShowingCursor = TRUE;
		}
	}

	UnwindGameCallbacks();
	CLogger::LogToFile(m_logFileBlankLine);
	CLogger::LogToFile(g_logFileFooterAsterisks);
	CLogger::LogToFile(m_logFileFinishedNormally);
	CLogger::LogToFile(g_logFileFooterAsterisks);
	CLogger::CloseLogFile();

	return m_exitCode;
}

// Destroys the current game window.
// FUNCTION: CMR2 0x004a8270
BOOL Main_DestroyGameWindow(void)
{
	// PORT: one SDL window, owned by the platform layer.
	Sys_DestroyWindow();
	return TRUE;
}

// match 65%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a8140
// PORT: the window class, icon and menu are gone; the platform layer creates
// the SDL window (shown and sized by the renderer).
BOOL CMain::CreateGameWindow(LPCSTR sWindowName)
{
	if (!Sys_CreateWindow(sWindowName, g_pGraphics->isFullscreen))
		return FALSE;
	CGame::RegisterCallback(Main_DestroyGameWindow, NULL);
	return TRUE;
}

// FUNCTION: CMR2 0x004a98b0
// PORT: the window procedure, on platform events. Focus changes stop and
// restart the music like WM_KILLFOCUS/WM_SETFOCUS; minimising or losing the
// focus deactivates the game like WM_SIZE/WM_ACTIVATEAPP (surfaces are never
// lost with the SDL renderer, so there is nothing to restore); closing the
// window quits.
void CMain::HandleEvent(const SysEvent *event)
{
	switch (event->type)
	{
	case SYS_EVENT_FOCUS_LOST:
		CSound::StopSharedMusicBuffer();
		SetGameActiveState(1);
		Game_SaveMenuChanges();   // PORT
		break;

	case SYS_EVENT_FOCUS_GAINED:
		CSound::NoOpSoundDeviceCallback();
		SetGameActiveState(0);
		break;

	case SYS_EVENT_MINIMIZED:
		SetGameActiveState(1);
		break;

	case SYS_EVENT_RESTORED:
		SetGameActiveState(0);
		break;

	// PORT: closing the window saves like Quit in the main menu; the original
	// lost the options changed since the last race.
	case SYS_EVENT_QUIT:
		Game_SaveBeforeQuit();
		m_exitCode = 0;
		CGame::m_shouldExit = 1;
		break;

	case SYS_EVENT_KEY_DOWN:
		CInput::QueueVirtualKeyPress(event->key);
		break;

	case SYS_EVENT_TEXT:
		CInput::QueueInputCharacter(event->character);
		break;

	// PORT: Alt+Enter switches between fullscreen and a window, and is
	// remembered in opencmr2.ini. The main loop shows or hides the cursor;
	// the mouse is captured only in fullscreen.
	case SYS_EVENT_TOGGLE_FULLSCREEN:
		g_pGraphics->isFullscreen = !g_pGraphics->isFullscreen;
		Gfx_SetFullscreen(g_pGraphics->isFullscreen);
		Sys_SetOption("video.fullscreen", g_pGraphics->isFullscreen);
		if (!CGame::m_isActive)
			CInput::SetMouseCoopLevel(1);
		break;
	}
}

// Switches the game between active and inactive: input/mouse cooperative
// level, the sound "hooked" flag and the menu bar, plus the inactive time.
// FUNCTION: CMR2 0x004a9a50
// PORT: the DirectDraw FlipToGDISurface and the menu bar redraw are gone.
void CMain::SetGameActiveState(int param1)
{
	int frameTime;

	if (param1 != 0) {
		CInput::SetMouseCoopLevel(0);
		CSound::RunSoundDeviceCallback();
		// PORT: DirectSound silenced the game's buffers while another
		// window had the focus; the SDL output is held instead.
		Audio_SetPaused(TRUE);
		m_unk0x00663dbc = GetFrameTime();
		CGame::m_isActive = 1;
		return;
	}

	CInput::SetMouseCoopLevel(1);
	CSound::RunSoundDeviceCallback();
	Audio_SetPaused(FALSE);
	CInput::ClearFirstJoystickControlBindings();
	ResetFpsWarmup();
	frameTime = GetFrameTime();
	CGame::m_isActive = 0;
	m_unk0x00663dc0 = frameTime - m_unk0x00663dbc;
}

// FUNCTION: CMR2 0x004b2390
void CMain::ResetFpsWarmup(void)
{
	g_unk0x005210ac = 1;
}

// FUNCTION: CMR2 0x0049c130
void CMain::UnwindGameCallbacks(void)
{
	CGame::UnwindCallbacks(0);
}

// Shuts D3DX down (registered as a callback by Main_InitD3DX).
// PORT: there is no D3DX; the callback stays so the shutdown chain keeps its
// shape.
// FUNCTION: CMR2 0x004a9b50
BYTE Main_ShutdownD3DX(void)
{
    return 1;
}

// Starts D3DX and registers its shutdown.
// FUNCTION: CMR2 0x004a9b30
void Main_InitD3DX(void)
{
    CGame::RegisterCallback(Main_ShutdownD3DX, NULL);
}

// FUNCTION: CMR2 0x004a9b60
int CMain::GetFrameTime(void)
{
    return m_frameTime;
}

// FUNCTION: CMR2 0x004a9b70
void CMain::UpdateFrameTime(void)
{
    m_frameTime = Sys_GetTicks();
}

// FUNCTION: CMR2 0x004a9c20
bool CMain::ResetFrameDelta(void)
{
    m_frameDeltaInitialised = FALSE;
    return true;
}

// match 59%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a9b80
unsigned int CMain::GetFrameDelta(void)
{
    unsigned int now;
    unsigned int step;

    now = (unsigned int)GetFrameTime() / 10;
    if (m_frameDeltaInitialised == 0) {
        m_frameDeltaStart = now;
        m_frameDeltaLast = now;
        m_frameDelta = 0;
        m_frameDeltaMax = 0;
        m_frameDeltaInitialised = TRUE;
        CGame::RegisterCallback(ResetFrameDelta, NULL);
    }
    step = now - m_frameDeltaLast;
    if (step != 0) {
        m_frameDelta += step;
        if (m_frameDelta < m_frameDeltaMax)
            m_frameDelta = m_frameDeltaMax;
        else
            m_frameDeltaMax = m_frameDelta;
    }
    m_frameDeltaLast = now;
    return m_frameDelta;
}

// The game's statically linked CRT sprintf (ours comes from the import library).
// LIBRARY: CMR2 0x00405620
// _sprintf

// Statically linked DirectX helpers called by the startup/shutdown callbacks.
// LIBRARY: CMR2 0x004c6794
// _D3DXInitialize@0
// LIBRARY: CMR2 0x004c686d
// _D3DXUninitialize@0
