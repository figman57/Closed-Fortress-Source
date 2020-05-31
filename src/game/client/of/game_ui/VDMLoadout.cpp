//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "VDMLoadout.h"
#include "game_ui\VFooterPanel.h"
#include "game_ui\VDropDownMenu.h"
#include "game_ui\VSliderControl.h"
#include "game_ui\vhybridbutton.h"
#include "game_ui\gameui_util.h"
// #include "EngineInterface.h"
#include "vgui/ISurface.h"
#include "game_ui\VGenericConfirmation.h"
#include "materialsystem/materialsystem_config.h"

#ifdef _X360
#include "xbox/xbox_launch.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

//=============================================================================
DMLoadout::DMLoadout(Panel *parent, const char *panelName): BaseClass(parent, panelName)
{
	// SetDeleteSelfOnClose(true);

	// engine->ExecuteClientCmd("gameui_preventescape");

	SetProportional( true );

	SetUpperGarnishEnabled(true);
	SetLowerGarnishEnabled(true);
}

//=============================================================================
DMLoadout::~DMLoadout()
{
	// engine->ExecuteClientCmd("gameui_allowescape");
}


//=============================================================================
void DMLoadout::OnCommand(const char *command)
{
	if( Q_stricmp( "Back", command ) == 0 )
	{
		// OnApplyChanges();
		OnKeyCodePressed( KEY_XBUTTON_B );
	}
	else if (Q_stricmp("Cancel", command) == 0)
	{
		OnKeyCodePressed(KEY_XBUTTON_B);
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void DMLoadout::PaintBackground()
{
	// BaseClass::PaintBackground();
	BaseClass::DrawDialogBackground( "#L4D360UI_DMLoadout", NULL, "#L4D360UI_Controller_Desc", NULL, NULL, true );
}

void DMLoadout::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// required for new style
	SetPaintBackgroundEnabled( true );
	// SetupAsDialogStyle();
}
