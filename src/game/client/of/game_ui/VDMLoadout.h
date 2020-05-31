//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef __VTestPanel2_H__
#define __VTestPanel2_H__

#include "game_ui\basemodui.h"
#include "game_ui\VFlyoutMenu.h"

namespace BaseModUI {

class DropDownMenu;
class SliderControl;
class BaseModHybridButton;

class DMLoadout : public CBaseModFrame
{
	DECLARE_CLASS_SIMPLE( DMLoadout, CBaseModFrame );

public:
	DMLoadout(vgui::Panel *parent, const char *panelName);
	~DMLoadout();

protected:
	virtual void PaintBackground();
	virtual void ApplySchemeSettings( vgui::IScheme* pScheme );
	virtual void OnCommand( const char *command );
};

};

#endif // __VDMLoadout_H__