//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef __VTestPanel1_H__
#define __VTestPanel1_H__


#include "basemodui.h"
#include "VFlyoutMenu.h"


namespace BaseModUI {

class DropDownMenu;
class SliderControl;
class BaseModHybridButton;

class TestPanel1 : public CBaseModFrame
{
	DECLARE_CLASS_SIMPLE( TestPanel1, CBaseModFrame );

public:
	TestPanel1(vgui::Panel *parent, const char *panelName);
	~TestPanel1();

protected:
	virtual void PaintBackground();
	virtual void ApplySchemeSettings( vgui::IScheme* pScheme );
	virtual void OnCommand( const char *command );
};

};

#endif // __VTestPanel1_H__