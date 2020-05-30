//========= Copyright � 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef _IBASEPANEL_H__
#define _IBASEPANEL_H__

#include "vgui_controls/Panel.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/PHandle.h"
#include "vgui_controls/MenuItem.h"
#include "vgui_controls/MessageDialog.h"
#include "tier1/utllinkedlist.h"
#include "OptionsDialog.h"
#include "OptionsSubKeyboard.h"
#include "OptionsSubMouse.h"
#include "optionsmousedialog.h"
#include "CreateMultiplayerGameDialog.h"
#include "PlayerListDialog.h"

#include <GameUI/gameui_shared.h>

class COptionsDialog;
class CCreateMultiplayerGameDialog;
class COptionsMouseDialog;
class IMaterial;
class CMatchmakingBasePanel;
class CBackgroundMenuButton;
class CGameMenu;

//=============================================================================
//
//=============================================================================
class IBasePanel
{
public:
	// virtual CBaseModPanel() = 0;
	virtual ~IBasePanel() {}

	virtual vgui::Panel& GetVguiPanel() = 0;

public:
	// notifications
	virtual void OnLevelLoadingStarted( char const *levelName, bool bShowProgressDialog ) = 0;
	virtual void OnLevelLoadingFinished( KeyValues *kvEvent ) = 0;
	virtual bool UpdateProgressBar(float progress, const char *statusText) = 0;

	// update the taskbar a frame
	virtual void RunFrame() = 0;

	// fades to black then runs an engine command (usually to start a level)
	virtual void FadeToBlackAndRunEngineCommand(const char *engineCommand) = 0;

	// sets the blinking state of a menu item
	virtual void SetMenuItemBlinkingState(const char *itemName, bool state) = 0;

	// handles gameUI being shown
	virtual void OnGameUIActivated() = 0;

	// game dialogs
	virtual void OnOpenNewGameDialog(const char *chapter = NULL) = 0;
	virtual void OnOpenBonusMapsDialog() = 0;
	virtual void OnOpenLoadGameDialog() = 0;
	virtual void OnOpenSaveGameDialog() = 0;
	virtual void OnOpenServerBrowser() = 0;
	virtual void OnOpenFriendsDialog() = 0;
	virtual void OnOpenDemoDialog() = 0;
	virtual void OnOpenCreateMultiplayerGameDialog() = 0;
	virtual void OnOpenQuitConfirmationDialog() = 0;
	virtual void OnOpenDisconnectConfirmationDialog() = 0;
	virtual void OnOpenChangeGameDialog() = 0;
	virtual void OnOpenPlayerListDialog() = 0;
	virtual void OnOpenBenchmarkDialog() = 0;
	virtual void OnOpenOptionsDialog() = 0;
	virtual void OnOpenLoadCommentaryDialog() = 0;
	virtual void OpenLoadSingleplayerCommentaryDialog() = 0;
	virtual void OnOpenAchievementsDialog() = 0;

	//=============================================================================
	// HPE_BEGIN:
	// [dwenger] Specific code for CS Achievements Display
	//=============================================================================

	// $TODO(HPE): Move this to a game-specific location
	virtual void OnOpenCSAchievementsDialog() = 0;

	//=============================================================================
	// HPE_END
	//=============================================================================

	virtual void OnOpenControllerDialog() = 0;

	virtual void SessionNotification(const int notification, const int param = 0) = 0;
	virtual void SystemNotification(const int notification) = 0;
	virtual void ShowMessageDialog(const uint nType, vgui::Panel *pParent = NULL) = 0;
	virtual void CloseMessageDialog(const uint nType) = 0;
	virtual void UpdatePlayerInfo(uint64 nPlayerId, const char *pName, int nTeam, byte cVoiceState, int nPlayersNeeded, bool bHost) = 0;
	virtual void SessionSearchResult(int searchIdx, void *pHostData, XSESSION_SEARCHRESULT *pResult, int ping) = 0;
	virtual void OnChangeStorageDevice() = 0;
	virtual bool ValidateStorageDevice() = 0;
	virtual bool ValidateStorageDevice(int *pStorageDeviceValidated) = 0;
	virtual void OnCreditsFinished() = 0;

	virtual KeyValues *GetConsoleControlSettings(void) = 0;

	// forces any changed options dialog settings to be applied immediately, if it's open
	virtual void ApplyOptionsDialogSettings() = 0;

	// virtual vgui::AnimationController *GetAnimationController(void) { return m_pConsoleAnimationController; }
	virtual void RunCloseAnimation(const char *animName) = 0;
	virtual void RunAnimationWithCallback(vgui::Panel *parent, const char *animName, KeyValues *msgFunc) = 0;
	virtual void PositionDialog(vgui::PHandle dlg) = 0;

	virtual void ArmFirstMenuItem(void) = 0;

	virtual void OnGameUIHidden() = 0;

	virtual void CloseBaseDialogs(void) = 0;
	// virtual bool IsWaitingForConsoleUI(void) { return m_bWaitingForStorageDeviceHandle || m_bWaitingForUserSignIn || m_bXUIVisible; }

	virtual int  GetMenuAlpha(void) = 0;

	virtual void SetMainMenuOverride(vgui::VPANEL panel) = 0;
	
	// FIXME: This should probably become a friend relationship between the classes
	virtual bool HandleSignInRequest(const char *command) = 0;
	virtual bool HandleStorageDeviceRequest(const char *command) = 0;
	virtual void ClearPostPromptCommand(const char *pCompletedCommand) = 0;
};

#endif
