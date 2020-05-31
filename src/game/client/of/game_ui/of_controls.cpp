//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/QueryBox.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include <game/client/iviewport.h>
#include "tf_tips.h"
#include "renderparm.h"
#include "animation.h"
#include "tf_controls.h"
#include "cvartogglecheckbutton.h"
#include "datacache/imdlcache.h"

#include "of_controls.h"

#include "engine/IEngineSound.h"
#include "basemodelpanel.h"
#include "tf_gamerules.h"
#include "of_shared_schemas.h"
#include <convar.h>
#include <vgui_controls/ScrollBarSlider.h>
#include <vgui_controls/Slider.h>
#include "fmtstr.h"

#include "tier0/dbg.h"

using namespace vgui;

// "Simple" function to convert Hue Saturation and Brightness to RGB
// Hue goes from 0 to 360
// Saturation and brightness goes from 0 to 1 ( float value which represents 0% to 100% )
// Maybe we should try to do this with inline assembly code later?
// Definetly beneficial for such a common function

typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

static rgb   hsv2rgb(hsv in);
static hsv   rgb2hsv(rgb in);

hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}

rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;     
}

Color HSBtoRGB( float iH, float iS, float iB )
{
	hsv in;
	in.h = iH;
	in.s = iS;
	in.v = iB;
	rgb out = hsv2rgb( in );
	
	Color vecCol;
	vecCol.SetColor((int)(out.r * 255.0f),(int)(out.g * 255.0f),(int)(out.b * 255.0f), 255);
	return vecCol;
}

extern ConVar of_color_r;
extern ConVar of_color_g;
extern ConVar of_color_b;



DECLARE_BUILD_FACTORY( CTFModelPanel );

CTFModelPanel::CTFModelPanel( Panel *pParent, const char *pszName )
	: CBaseModelPanel( pParent, pszName )
{
	SetParent( pParent );
	SetScheme( "ClientScheme" );
	SetProportional( true );
	SetVisible( true );
	SetThumbnailSafeZone( false );

	m_flParticleZOffset = 0.0f;
	m_flLoopParticleAfter = 0.0f;
	m_iAnimationIndex = 0;
}

void CTFModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
	
	m_bLoopParticle = inResourceData->GetBool( "particle_loop" );
	m_flLoopTime = inResourceData->GetFloat( "particle_loop_time" );
	m_flParticleZOffset = inResourceData->GetFloat( "particle_z_offset" );
		
	Color bgColor = inResourceData->GetColor( "bgcolor_override" );
	SetBackgroundColor( bgColor );
	

	KeyValues *pModelData = inResourceData->FindKey( "model" );
	if( pModelData )
	{
		m_vecDefPosition = Vector( pModelData->GetFloat( "origin_x", 0 ), pModelData->GetFloat( "origin_y", 0 ), pModelData->GetFloat( "origin_z", 0 ) );
		m_vecDefAngles = QAngle( pModelData->GetFloat( "angles_x", 0 ), pModelData->GetFloat( "angles_y", 0 ), pModelData->GetFloat( "angles_z", 0 ) );
		SetModelName( ReadAndAllocStringValue ( pModelData, "modelname" ), pModelData->GetInt("skin", 0) );
	}
}

void CTFModelPanel::OnThink()
{
	BaseClass::OnThink();

	// TODO: autorotation?
}

void CTFModelPanel::Update()
{
	MDLHandle_t hSelectedMDL = g_pMDLCache->FindMDL( m_BMPResData.m_pszModelName );
	g_pMDLCache->PreloadModel( hSelectedMDL );
	SetMDL( hSelectedMDL );


	if ( m_iAnimationIndex < m_BMPResData.m_aAnimations.Size() )
	{
		SetModelAnim( m_iAnimationIndex );
	}

	SetSkin( m_BMPResData.m_nSkin );
}

void CTFModelPanel::Paint()
{
	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA, false );

	pRenderContext->SetFlashlightMode(false);
	
	if( m_bLoopParticle )
	{
		if( m_flLoopParticleAfter < gpGlobals->curtime )
		{
			SetParticleName(szLoopingParticle);
		}
	}
	BaseClass::Paint();
}

// Update our Studio Hdr to our current model
void CTFModelPanel::RefreshModel()
{
	studiohdr_t *pStudioHdr = m_RootMDL.m_MDL.GetStudioHdr();
	if ( !pStudioHdr )
		return;

	CStudioHdr StudioHdr( pStudioHdr, g_pMDLCache );
	m_StudioHdr = StudioHdr;
}

CStudioHdr *CTFModelPanel::GetModelPtr()
{
	RefreshModel();
	return &m_StudioHdr;
}

// Most of the bodygroup stuff was copied over from c_BaseAnimating
// The only stuff removed is Dynamic loading tests
// The following comment is from that

// SetBodygroup is not supported on pending dynamic models. Wait for it to load!
// XXX TODO we could buffer up the group and value if we really needed to. -henryg

void CTFModelPanel::SetBodygroup( int iGroup, int iValue )
{

	Assert( GetModelPtr() );
	::SetBodygroup( GetModelPtr( ), m_RootMDL.m_MDL.m_nBody, iGroup, iValue );
}

int CTFModelPanel::GetBodygroup( int iGroup )
{
	Assert( GetModelPtr() );
	return ::GetBodygroup( GetModelPtr( ), m_RootMDL.m_MDL.m_nBody, iGroup );
}

const char *CTFModelPanel::GetBodygroupName( int iGroup )
{
	Assert( GetModelPtr() );
	return ::GetBodygroupName( GetModelPtr( ), iGroup );
}

int CTFModelPanel::FindBodygroupByName( const char *name )
{
	Assert( GetModelPtr() );
	return ::FindBodygroupByName( GetModelPtr( ), name );
}

int CTFModelPanel::GetBodygroupCount( int iGroup )
{
	Assert( GetModelPtr() );
	return ::GetBodygroupCount( GetModelPtr( ), iGroup );
}

int CTFModelPanel::GetNumBodyGroups( void )
{
	Assert( GetModelPtr() );
	return ::GetNumBodyGroups( GetModelPtr( ) );
}

void CTFModelPanel::SetModelName( const char* pszModelName, int nSkin )
{
	m_BMPResData.m_pszModelName = pszModelName;
	m_BMPResData.m_nSkin = nSkin;
}

extern ConVar of_tennisball;

void CTFModelPanel::SetParticleName(const char* name)
{
	m_bUseParticle = true;

	if (m_pData)
	{
		SafeDeleteParticleData(&m_pData);
	}

	m_pData = CreateParticleData(name);
	if( m_bLoopParticle )
	{
		Q_strncpy( szLoopingParticle, name ,sizeof(szLoopingParticle) );
		m_flLoopParticleAfter = gpGlobals->curtime + m_flLoopTime; 
	}
	// We failed at creating that particle for whatever reason, bail (!)
	if (!m_pData) return;

	CUtlVector<int> vecAttachments;

	Vector vecParticleOffset( 0, 0, m_flParticleZOffset );
	
	m_pData->UpdateControlPoints( GetModelPtr(), &m_RootMDL.m_MDLToWorld, vecAttachments, 0, vecParticleOffset);
	int iRed = of_tennisball.GetBool() ? 0 : of_color_r.GetInt();
	int iGreen = of_tennisball.GetBool() ? 255 : of_color_g.GetInt();
	int iBlue = of_tennisball.GetBool() ? 0 : of_color_b.GetInt();
	
	m_pData->SetParticleColor( GetModelPtr(), &m_RootMDL.m_MDLToWorld, iRed, iGreen, iBlue );

	m_pData->m_bIsUpdateToDate = true;
}

Panel *CTFItemSelection_Factory()
{
	return new CTFItemSelection(NULL, NULL, 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFEditableButton::CTFEditableButton(Panel *parent, const char *panelName) : EditablePanel(parent, panelName)
{
	pButton = new CTFEditableButtonFunc(this, "Button");
	
	m_bSelected = false;
	ivgui()->AddTickSignal(GetVPanel());
	if( parent )
		AddActionSignalTarget( parent );
}

void CTFEditableButton::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );

	Q_strncpy(szBorderIdle, inResourceData->GetString("border_idle"), sizeof(szBorderIdle));
	Q_strncpy(szBorderHover, inResourceData->GetString("border_hover"), sizeof(szBorderHover));
	Q_strncpy(szBorderPressed, inResourceData->GetString("border_pressed"), sizeof(szBorderPressed));
	Q_strncpy(szBorderSelected, inResourceData->GetString("border_selected"), sizeof(szBorderSelected));
	
	char szPressedSound[64];
	szPressedSound[0] = '\0';
	Q_strncpy(szPressedSound, inResourceData->GetString("pressed_sound"), sizeof(szPressedSound));

	if( szPressedSound[0] != '\0' )
		AddOnPressSound( szPressedSound );
	
	m_iSoundChance = inResourceData->GetInt("sound_chances", 1);

	SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderIdle));
	
	KeyValues *inItemButton = inResourceData->FindKey("Button");
	if( inItemButton )
	{
		pButton->ApplySettings(inItemButton);
	}
}

void CTFEditableButton::AddOnPressSound( char *szPressedSound )
{
	int len = 128;

	char *pName = new char[ len ];
	V_strncpy( pName, szPressedSound, len );
	m_hPressedSounds.AddToTail( pName );
	
	PrecacheUISoundScript( szPressedSound );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButton::OnCursorEntered()
{
	BaseClass::OnCursorEntered();
	if( !m_bSelected )
		SetBorderType(BORDER_HOVEROVER);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButton::OnCursorExited()
{
	BaseClass::OnCursorExited();
	if( !m_bSelected && iCurrentBorder != BORDER_SELECTED )
		SetBorderType(BORDER_IDLE);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButton::OnMousePressed(MouseCode code)
{
	BaseClass::OnMousePressed(code);
	if ( code == MOUSE_LEFT )
	{
		SetBorderType(BORDER_PRESSED);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButton::OnMouseReleased(MouseCode code)
{
	BaseClass::OnMouseReleased(code);
	if ( code == MOUSE_LEFT )
	{
		if( m_bSelected )
			OnReleasedSelected();
		else
			OnReleasedUnselected();

		SetSelected( !m_bSelected );
	}
}

void CTFEditableButton::SetSelected( bool bSelected )
{
	m_bSelected = bSelected;
	
	if( !m_bSelected )
		SetBorderType(BORDER_IDLE);
	else
		SetBorderType(BORDER_SELECTED);

	// send a changed message
	PostActionSignal(new KeyValues("SetSelected", "selected", bSelected));
}

void CTFEditableButton::OnReleasedSelected()
{

}

void CTFEditableButton::OnReleasedUnselected()
{
	if( !m_hPressedSounds.Count() )
		return;

	if( random->RandomInt( 1, m_iSoundChance ) == 1 )
	{
		if( GetSoundScriptWave( m_hPressedSounds[ random->RandomInt( 0, m_hPressedSounds.Count() - 1 ) ] ) != NULL )
		{
			vgui::surface()->PlaySound
			( 
				GetSoundScriptWave
				(
					m_hPressedSounds[ random->RandomInt( 0, m_hPressedSounds.Count() - 1 ) ]
				)
			);
		}
	}
}

void CTFEditableButton::SetBorderType( int iBorder )
{
	if( iCurrentBorder == iBorder )
		return;

	switch( iBorder )
	{
		case BORDER_IDLE:
			if( szBorderIdle[0] == '\0' )
				return;
			SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderIdle));
			iCurrentBorder = BORDER_IDLE;
			break;
		case BORDER_HOVEROVER:
			if( szBorderHover[0] == '\0' )
				return;
			SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderHover));
			iCurrentBorder = BORDER_HOVEROVER;
			break;		
		case BORDER_PRESSED:
			if( szBorderPressed[0] == '\0' )
				return;
			SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderPressed));
			iCurrentBorder = BORDER_PRESSED;
			break;
		case BORDER_SELECTED:
			if( szBorderSelected[0] == '\0' )
				return;
			SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderSelected));
			iCurrentBorder = BORDER_SELECTED;
			break;
	}
}

CTFEditableButtonFunc::CTFEditableButtonFunc(Panel *parent, const char *panelName) : EditablePanel(parent, panelName)
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButtonFunc::OnCursorEntered()
{
	if( GetParent() )
		GetParent()->OnCursorEntered();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButtonFunc::OnCursorExited()
{
	if( GetParent() )
		GetParent()->OnCursorExited();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButtonFunc::OnMousePressed(MouseCode code)
{
	if( GetParent() )
		GetParent()->OnMousePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButtonFunc::OnMouseReleased(MouseCode code)
{
	if( GetParent() )
		GetParent()->OnMouseReleased( code );
}

DECLARE_BUILD_FACTORY( CTFCommandButton );
DECLARE_BUILD_FACTORY( CTFHeaderItem );
DECLARE_BUILD_FACTORY( CTFSelectionPanel );
DECLARE_BUILD_FACTORY( CTFSelectionManager );
DECLARE_BUILD_FACTORY_CUSTOM( CTFItemSelection, CTFItemSelection_Factory );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFCommandButton::CTFCommandButton(Panel *parent, const char *panelName) : CTFEditableButton(parent, panelName)
{
	szConvref[0] = '\0';
	szTargetVal[0] = '\0';
}

void CTFCommandButton::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );

	Q_strncpy(szCommand, inResourceData->GetString("command"), sizeof(szCommand));
	Q_strncpy(szUnselectCommand, inResourceData->GetString("unselect_command"), sizeof(szUnselectCommand));
	Q_strncpy(szConvref, inResourceData->GetString("convref"), sizeof(szConvref));
	Q_strncpy(szTargetVal, inResourceData->GetString("targetval"), sizeof(szTargetVal));
}

void CTFCommandButton::PaintBackground()
{
	BaseClass::PaintBackground();

	if( szConvref[0] == '\0' )
	{
		return;
	}

	ConVarRef var( szConvref );
	if ( !var.IsValid() )
		return;
	
	if( szTargetVal[0] == '\0' )
	{
		bool value = var.GetBool();
		if( m_bSelected != value )
			SetSelected( value );
	}
	else
	{
		bool value = !strcmp( szTargetVal, var.GetString() );
		if( m_bSelected != value )
			SetSelected( value );
	}
}

void CTFCommandButton::OnReleasedSelected()
{
	BaseClass::OnReleasedSelected();
	
	PostActionSignal(new KeyValues("Command", "command", szUnselectCommand));
	engine->ExecuteClientCmd( szUnselectCommand );
}

void CTFCommandButton::OnReleasedUnselected()
{
	BaseClass::OnReleasedUnselected();

	PostActionSignal(new KeyValues("Command", "command", szCommand));
	engine->ExecuteClientCmd( szCommand );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFItemSelection::CTFItemSelection(Panel *parent, const char *panelName, const int iID) : CTFEditableButton(parent, panelName)
{
	pItemImage = new CTFImagePanel(this, "ItemImage");
	
	SetItemID( iID );
	
	for( int i = 1; i <= 5; i++ )
		AddOnPressSound( VarArgs( "Mercenary.PositiveVocalization0%d", i ) );
}

void CTFItemSelection::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );

	Q_strncpy(szCommand, inResourceData->GetString("command"), sizeof(szCommand));
	
	KeyValues *inItemImage = inResourceData->FindKey("ItemImage");
	if( inItemImage )
		pItemImage->ApplySettings(inItemImage);

	if( inResourceData->GetInt("ItemID") )
		SetItemID( inResourceData->GetInt("ItemID") );
	
	m_iSoundChance = 4;
}

void CTFItemSelection::SetItemID( int iID )
{
	iItemID = iID;
	Q_strncpy(szCommand, VarArgs("loadout_equip cosmetics mercenary %d", iItemID), sizeof(szCommand));
	
	KeyValues *pCosmetic = GetCosmetic(iItemID);
	if( !pCosmetic )
		return;

	pItemImage->SetImage( pCosmetic->GetString("backpack_icon", "../backpack/blocked") );
	pItemImage->InvalidateLayout( false, true );
	bParsedBPImage = true;
}

void CTFItemSelection::Paint()
{
	BaseClass::Paint();
	if( iItemID && !bParsedBPImage )
	{
		SetItemID( iItemID );
	}
}

void CTFItemSelection::OnReleasedSelected()
{
	BaseClass::OnReleasedSelected();

	PostActionSignal(new KeyValues("Command", "command", szCommand));
	engine->ExecuteClientCmd( szCommand );
}

void CTFItemSelection::OnReleasedUnselected()
{
	BaseClass::OnReleasedUnselected();

	PostActionSignal(new KeyValues("Command", "command", szCommand));
	engine->ExecuteClientCmd( szCommand );
}

void CTFItemSelection::SetSelected( bool bSelected )
{
	BaseClass::SetSelected( bSelected );

	if( !bSelected )
		Q_strncpy(szCommand, VarArgs("loadout_equip cosmetics mercenary %d", iItemID), sizeof(szCommand));
	else
		Q_strncpy(szCommand, VarArgs("loadout_unequip cosmetics mercenary %d", iItemID), sizeof(szCommand));

#if 0
	if( GLoadoutPanel() )
	{
		int iTemp;
		iTemp = iItemID;
		if( bSelected )
		{
			GLoadoutPanel()->m_iCosmetics.AddToTail( iTemp );
		}
		else
		{
			for( int i = 0; i < GLoadoutPanel()->m_iCosmetics.Count(); i++ )
			{
				if( iTemp == GLoadoutPanel()->m_iCosmetics[i] )
				{
					GLoadoutPanel()->m_iCosmetics.Remove( i );
				}
			}
		}
		GLoadoutPanel()->m_bUpdateCosmetics = true;
	}
#endif

	CTFScrollableItemList *pItemList = dynamic_cast<CTFScrollableItemList*>( GetParent() );

	if( pItemList )
	{
		if( bSelected )
		{
			if( pItemList->pSelectedItem )
			{
				pItemList->pSelectedItem->SetSelected( false );
			}
			pItemList->pSelectedItem = this;
		}
		else if( pItemList->pSelectedItem == this )
		{
			pItemList->pSelectedItem = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFScrollableItemList::CTFScrollableItemList( Panel *parent, const char *panelName ) : EditablePanel( parent, panelName )
{
	pScrollBar = new ScrollBar( this, "ScrollBar", true );
	pScrollBar->AddActionSignalTarget( this );
	
	pSelectedItem = NULL;
	iLastX = 0;
	iLastY = 0;
}

void CTFScrollableItemList::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	iCollumnSpacing = inResourceData->GetInt( "CollumnSpacing" );
	iRowSpacing = inResourceData->GetInt( "RowSpacing" );

	KeyValues *kvItemNew = inResourceData->FindKey("ItemTemplate");

	if( kvItemNew )
	{
		Q_strncpy(t_ItemTemplate.wide, kvItemNew->GetString("wide"), sizeof(t_ItemTemplate.wide));
		Q_strncpy(t_ItemTemplate.tall, kvItemNew->GetString("tall"), sizeof(t_ItemTemplate.tall));
		Q_strncpy(t_ItemTemplate.border_idle, kvItemNew->GetString("border_idle"), sizeof(t_ItemTemplate.border_idle));
		Q_strncpy(t_ItemTemplate.border_hover, kvItemNew->GetString("border_hover"), sizeof(t_ItemTemplate.border_hover));		
		Q_strncpy(t_ItemTemplate.border_pressed, kvItemNew->GetString("border_pressed"), sizeof(t_ItemTemplate.border_pressed));
		Q_strncpy(t_ItemTemplate.border_selected, kvItemNew->GetString("border_selected"), sizeof(t_ItemTemplate.border_selected));			

		KeyValues *kvItemButton = kvItemNew->FindKey( "Button" );
		if( kvItemButton )
		{
			Q_strncpy(t_ItemTemplate.button_wide, kvItemButton->GetString("wide"), sizeof(t_ItemTemplate.button_wide));
			Q_strncpy(t_ItemTemplate.button_tall, kvItemButton->GetString("tall"), sizeof(t_ItemTemplate.button_tall));
			Q_strncpy(t_ItemTemplate.button_xpos, kvItemButton->GetString("xpos"), sizeof(t_ItemTemplate.button_xpos));		
			Q_strncpy(t_ItemTemplate.button_ypos, kvItemButton->GetString("ypos"), sizeof(t_ItemTemplate.button_ypos));
			Q_strncpy(t_ItemTemplate.button_zpos, kvItemButton->GetString("zpos"), sizeof(t_ItemTemplate.button_zpos));			
		}

		KeyValues *kvItemImage = kvItemNew->FindKey( "ItemImage" );
		if( kvItemImage )
		{
			Q_strncpy(t_ItemTemplate.extra_wide, kvItemImage->GetString("wide"), sizeof(t_ItemTemplate.extra_wide));
			Q_strncpy(t_ItemTemplate.extra_tall, kvItemImage->GetString("tall"), sizeof(t_ItemTemplate.extra_tall));
			Q_strncpy(t_ItemTemplate.extra_xpos, kvItemImage->GetString("xpos"), sizeof(t_ItemTemplate.extra_xpos));		
			Q_strncpy(t_ItemTemplate.extra_ypos, kvItemImage->GetString("ypos"), sizeof(t_ItemTemplate.extra_ypos));
			Q_strncpy(t_ItemTemplate.extra_zpos, kvItemImage->GetString("zpos"), sizeof(t_ItemTemplate.extra_zpos));	
		}		

		iElementWidth = kvItemNew->GetInt( "wide" );
		iElementHeight = kvItemNew->GetInt( "tall" );
		iElementsPerRow = ( inResourceData->GetInt("wide") - 30 ) / ( iElementWidth + iRowSpacing );
		iElementsPerScroll = iElementsPerRow * (( inResourceData->GetInt("tall") ) / ( iElementHeight  + iCollumnSpacing ));
	}
}

void CTFScrollableItemList::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( pScrollBar->GetSlider() )
	{
		pScrollBar->GetSlider()->SetFgColor( pScrollBar->GetFgColor() );
	}
	if ( pScrollBar->GetButton(0) )
	{
		pScrollBar->GetButton(0)->SetFgColor( pScrollBar->GetFgColor() );
	}
	if ( pScrollBar->GetButton(1) )
	{
		pScrollBar->GetButton(1)->SetFgColor( pScrollBar->GetFgColor() );
	}
}

//-----------------------------------------------------------------------------
// Called when the scroll bar moves
//-----------------------------------------------------------------------------
void CTFScrollableItemList::OnScrollBarSliderMoved()
{
	int nScrollAmount = pScrollBar->GetValue();
	
	for( int i = 0; i < m_hItems.Count(); i++ )
	{
		m_hItems[i].pItemPanel->SetPos( m_hItems[i].def_xpos, m_hItems[i].def_ypos - nScrollAmount );
	}
}

//-----------------------------------------------------------------------------
// respond to mouse wheel events
//-----------------------------------------------------------------------------
void CTFScrollableItemList::OnMouseWheeled(int delta)
{
	int val = pScrollBar->GetValue();
	val -= (delta * 50);
	pScrollBar->SetValue( val );
}

void CTFScrollableItemList::AddItem( int iID )
{
	ItemListItem_t pNewItem;;

	pNewItem.pItemPanel = new CTFItemSelection(this, VarArgs("%d", iID), iID);
	
	KeyValues *kvItemTemplate = new KeyValues("Template");
	
	kvItemTemplate->SetString("wide", t_ItemTemplate.wide);
	kvItemTemplate->SetString("tall", t_ItemTemplate.tall);
	kvItemTemplate->SetString("border_idle", t_ItemTemplate.border_idle);
	kvItemTemplate->SetString("border_hover", t_ItemTemplate.border_hover);
	kvItemTemplate->SetString("border_pressed", t_ItemTemplate.border_pressed);
	kvItemTemplate->SetString("border_selected", t_ItemTemplate.border_selected);
	
	KeyValues *kvButton = new KeyValues("Button");
	
	kvButton->SetString("wide", t_ItemTemplate.button_wide);
	kvButton->SetString("tall", t_ItemTemplate.button_tall);
	kvButton->SetString("xpos", t_ItemTemplate.button_xpos);
	kvButton->SetString("ypos", t_ItemTemplate.button_ypos);
	kvButton->SetString("zpos", t_ItemTemplate.button_zpos);
	kvButton->SetString( "proportionalToParent", "1" );
	
	KeyValues *kvItemImage = new KeyValues("ItemImage");
	
	kvItemImage->SetString("wide", t_ItemTemplate.extra_wide);
	kvItemImage->SetString("tall", t_ItemTemplate.extra_tall);
	kvItemImage->SetString("xpos", t_ItemTemplate.extra_xpos);
	kvItemImage->SetString("ypos", t_ItemTemplate.extra_ypos);
	kvItemImage->SetString("zpos", t_ItemTemplate.extra_zpos);
	
	kvItemImage->SetString("visible", "1" );
	kvItemImage->SetString("scaleImage", "1" );
	kvItemImage->SetString("proportionalToParent", "1" );
	
	kvItemTemplate->AddSubKey(kvButton);
	kvItemTemplate->AddSubKey(kvItemImage);
	
	pNewItem.pItemPanel->ApplySettings(kvItemTemplate);

	if( !iLastX )
	{
		iLastX = iCollumnSpacing;
	}
	if( !iLastY )
	{
		iLastY = iRowSpacing;
	}
	
	int x = iLastX;
	int y = iLastY;

	pNewItem.pItemPanel->SetPos(x,y);
	pNewItem.pItemPanel->SetItemID(iID);

	pNewItem.def_xpos = x;
	pNewItem.def_ypos = y;
	
	m_hItems.AddToTail(pNewItem);

	int w, h;
	int gw, gh;
	pNewItem.pItemPanel->GetSize( w, h );
	GetSize( gw, gh );
	
	iLastX = iLastX + w + iCollumnSpacing;
	
	if( iLastX + w + iCollumnSpacing > gw )
	{
		iLastX = 0;
		iLastY = iLastY + h + iRowSpacing;
	}

	int iWide, iTall;
	pNewItem.pItemPanel->GetSize( iWide, iTall );
	
	pScrollBar->SetRange( 0, y + iTall + iCollumnSpacing );
	pScrollBar->SetRangeWindow( gh );
	
	if( GetLoadout() )
	{
		KeyValues *kvCosmetics = GetLoadout()->FindKey( "Cosmetics" );
		if( kvCosmetics )
		{
			KeyValues *kvMerc = kvCosmetics->FindKey( "mercenary" );
			if( kvMerc )
			{
				if ( iID == kvMerc->GetInt(szCategoryName) && pNewItem.pItemPanel )
					pNewItem.pItemPanel->SetSelected( true );
			}
		}
	}
}

void CTFScrollableItemList::ClearItemList()
{
	for( int i = 0; i < m_hItems.Count(); i++ )
	{
		m_hItems[i].pItemPanel->DeletePanel();
	}
	m_hItems.Purge();

	iLastX = 0;
	iLastY = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFScrollablePanelList::CTFScrollablePanelList( Panel *parent, const char *panelName ) : EditablePanel( parent, panelName )
{
	pScrollBar = new ScrollBar( this, "ScrollBar", true );
	pScrollBar->AddActionSignalTarget( this );
	
	pSelectedItem = NULL;
	
	iElementsPerRow = 1;
	iElementsPerScroll = 1;
	
	iLastX = 0;
	iLastY = 0;
}

void CTFScrollablePanelList::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	iCollumnSpacing = inResourceData->GetInt( "CollumnSpacing" );
	iRowSpacing = inResourceData->GetInt( "RowSpacing" );

	iElementWidth = inResourceData->GetInt( "element_width" );
	iElementHeight = inResourceData->GetInt( "element_height" );
	iElementsPerRow = ( inResourceData->GetInt("wide") - 30 ) / ( iElementWidth + iRowSpacing );
	iElementsPerScroll = iElementsPerRow * (( inResourceData->GetInt("tall") ) / ( iElementHeight  + iCollumnSpacing ));
}

void CTFScrollablePanelList::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( pScrollBar->GetSlider() )
	{
		pScrollBar->GetSlider()->SetFgColor( pScrollBar->GetFgColor() );
	}
	if ( pScrollBar->GetButton(0) )
	{
		pScrollBar->GetButton(0)->SetFgColor( pScrollBar->GetFgColor() );
	}
	if ( pScrollBar->GetButton(1) )
	{
		pScrollBar->GetButton(1)->SetFgColor( pScrollBar->GetFgColor() );
	}
}

//-----------------------------------------------------------------------------
// Called when the scroll bar moves
//-----------------------------------------------------------------------------
void CTFScrollablePanelList::OnScrollBarSliderMoved()
{
	int nScrollAmount = pScrollBar->GetValue();
	
	for( int i = 0; i < m_hItems.Count(); i++ )
	{
		m_hItems[i].pPanel->SetPos( m_hItems[i].def_xpos, m_hItems[i].def_ypos - nScrollAmount );
	}
}

//-----------------------------------------------------------------------------
// respond to mouse wheel events
//-----------------------------------------------------------------------------
void CTFScrollablePanelList::OnMouseWheeled(int delta)
{
	int val = pScrollBar->GetValue();
	val -= (delta * 50);
	pScrollBar->SetValue( val );
}

void CTFScrollablePanelList::OnSelectionChanged( Panel *panel )
{
	CTFEditableButton *pEditableButton = dynamic_cast<CTFEditableButton*>( panel );
	
	if( pEditableButton )
	{
		if( pEditableButton->m_bSelected )
		{
			if( pSelectedItem )
			{
				pSelectedItem->OnReleasedSelected();
				pSelectedItem->SetSelected( false );
			}
			pSelectedItem = pEditableButton;
		}
		else if( pSelectedItem == pEditableButton )
		{
				pSelectedItem = NULL;
		}
	}
}

void CTFScrollablePanelList::AddItem( CTFEditableButton *pPanel )
{
	PanelListItem_t pNewItem;

	pNewItem.pPanel = pPanel;
	
	int iWide, iTall;
	pPanel->GetSize( iWide, iTall );
	
	if( !iLastX )
	{
		iLastX = iCollumnSpacing;
	}
	if( !iLastY )
	{
		iLastY = iRowSpacing;
	}

	int x = iLastX;
	int y = iLastY;

	pNewItem.pPanel->SetPos(x,y);

	pNewItem.def_xpos = x;
	pNewItem.def_ypos = y;
	
	m_hItems.AddToTail(pNewItem);

	int w, h;
	int gw, gh;
	pNewItem.pPanel->GetSize( w, h );
	GetSize( gw, gh );
	
	iLastX = iLastX + w + iCollumnSpacing;
	
	if( iLastX + w + iCollumnSpacing > gw )
	{
		iLastX = 0;
		iLastY = iLastY + h + iRowSpacing;
	}

	pScrollBar->SetRange( 0, y + iTall + iCollumnSpacing );
	pScrollBar->SetRangeWindow( gh );
}

void CTFScrollablePanelList::ClearItemList()
{
	for( int i = 0; i < m_hItems.Count(); i++ )
	{
		m_hItems[i].pPanel->DeletePanel();
	}
	m_hItems.Purge();
	
	iLastX = 0;
	iLastY = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFHeaderItem::CTFHeaderItem(Panel *parent, const char *panelName) : CTFEditableButton(parent, panelName)
{
	pLabel = new CExLabel(this, "Label", "");
	iBaseHeight = -999;
}

void CTFHeaderItem::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *inLabel = inResourceData->FindKey("Label");
	if( inLabel )
	{
		pLabel->ApplySettings(inLabel);
		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		m_hTextFont = pScheme->GetFont( inLabel->GetString("font"), true );
	}
	SetConnectedPanel( inResourceData->GetString("panel") );
	
	CalculateBoxSize();
}

void CTFHeaderItem::CalculateBoxSize()
{
	int textLen = 0;

	wchar_t szUnicode[512];
	pLabel->GetText( szUnicode, sizeof( szUnicode ) );
	
	int len = wcslen( szUnicode );
	for ( int i=0;i<len;i++ )
	{
		textLen += surface()->GetCharacterWidth( m_hTextFont, szUnicode[i] );
	}
	
	textLen += 20; // Padding
	
	int w,h;
	GetSize( w, h );
	SetSize( textLen, h );
	
	pLabel->GetSize( w, h );
	pLabel->SetSize( textLen, h );
	
	pButton->GetSize( w, h );
	pButton->SetSize( textLen, h );
	
	pLabel->GetPos( w, h );
	pLabel->SetPos( w + 10, h );
}

void CTFHeaderItem::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CTFHeaderItem::SetConnectedPanel( const char *szConnectedPanel )
{
	Panel *pContainer = GetParent();
	if( !pContainer )
		return;
	
	Panel *pParent = pContainer->GetParent();
	if( !pParent )
	{
		pParent = GetParent();
	}
	
	if( !pParent )
		return;

	pConnectedPanel = pParent->FindChildByName( szConnectedPanel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHeaderItem::OnMouseReleased(MouseCode code)
{
	if( m_bSelected )
		return;

	BaseClass::OnMouseReleased(code);
}

void CTFHeaderItem::OnReleasedSelected()
{
	BaseClass::OnReleasedSelected();

	if( pConnectedPanel )
		pConnectedPanel->SetVisible( false );
	
}

void CTFHeaderItem::OnReleasedUnselected()
{
	BaseClass::OnReleasedUnselected();

	if( pConnectedPanel )
		pConnectedPanel->SetVisible( true );
	
}

void CTFHeaderItem::SetSelected( bool bSelected )
{
	BaseClass::SetSelected( bSelected );
	
	int x, y;
	GetPos( x, y );	
	
	if( !bSelected )
	{
		if( iBaseHeight == -999 )
			iBaseHeight = y;
		SetPos( x , iBaseHeight );
	}
	else
	{
		if( iBaseHeight == -999 )
			iBaseHeight = y;			
		SetPos( x , iBaseHeight - 25 );
	}

	CTFLoadoutHeader *pHeaderList = dynamic_cast<CTFLoadoutHeader*>( GetParent() );
	
	if( pHeaderList )
	{
		if( bSelected )
		{
			if( pHeaderList->pSelectedHeader )
			{
				pHeaderList->pSelectedHeader->OnReleasedSelected();
				pHeaderList->pSelectedHeader->SetSelected( false );
			}
			pHeaderList->pSelectedHeader = this;
		}
		else if( pHeaderList->pSelectedHeader == this )
		{
				pHeaderList->pSelectedHeader = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFLoadoutHeader::CTFLoadoutHeader( Panel *parent, const char *panelName ) : EditablePanel( parent, panelName )
{
	pScrollBar = new ScrollBar( this, "ScrollBar", false );
	pScrollBar->AddActionSignalTarget( this );
	
	iLastXPos = 0;
}

void CTFLoadoutHeader::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);
	
	iLastXPos = 0;
}

void CTFLoadoutHeader::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( pScrollBar->GetSlider() )
	{
		pScrollBar->GetSlider()->SetFgColor( pScrollBar->GetFgColor() );
	}
	if ( pScrollBar->GetButton(0) )
	{
		pScrollBar->GetButton(0)->SetFgColor( pScrollBar->GetFgColor() );
	}
	if ( pScrollBar->GetButton(1) )
	{
		pScrollBar->GetButton(1)->SetFgColor( pScrollBar->GetFgColor() );
	}
}

//-----------------------------------------------------------------------------
// Called when the scroll bar moves
//-----------------------------------------------------------------------------
void CTFLoadoutHeader::OnScrollBarSliderMoved()
{
	int nScrollAmount = pScrollBar->GetValue();
	
	for( int i = 0; i < m_hCategories.Count(); i++ )
	{
		int x, y;
		m_hCategories[i].pHeaderItem->GetPos( x, y );
		m_hCategories[i].pHeaderItem->SetPos( m_hCategories[i].def_xpos - nScrollAmount, y );
	}
}

//-----------------------------------------------------------------------------
// respond to mouse wheel events
//-----------------------------------------------------------------------------
void CTFLoadoutHeader::OnMouseWheeled(int delta)
{
	int val = pScrollBar->GetValue();
	val -= (delta * 50);
	pScrollBar->SetValue( val );
}

void CTFLoadoutHeader::AddCategory( const char *szCategory )
{
	HeaderListItem_t pNewItem;

	pNewItem.pHeaderItem = new CTFHeaderItem(this, szCategory);
	
	KeyValues *kvHeaderTemplate = new KeyValues("Template");
	
	kvHeaderTemplate->SetString("panel", VarArgs("%sList",szCategory));
	kvHeaderTemplate->SetString("wide", "10");
	kvHeaderTemplate->SetString("tall", "57");
	kvHeaderTemplate->SetString("ypos", "20");
	kvHeaderTemplate->SetString("border_idle", "LoadoutThickBorder");
	kvHeaderTemplate->SetString("border_hover", "LoadoutThickBorder");
	kvHeaderTemplate->SetString("border_pressed", "LoadoutThickBorder");
	kvHeaderTemplate->SetString("border_selected", "LoadoutThickBorder");
	kvHeaderTemplate->SetString("proportionalToParent", "1" );
	
	KeyValues *kvButton = new KeyValues("Button");
	
	kvButton->SetString("wide", "10");
	kvButton->SetString("tall", "100");
	kvButton->SetString("zpos", "10");
	kvButton->SetString("ypos", "c-50");
	kvButton->SetString( "proportionalToParent", "1" );
	
	KeyValues *kvLabel = new KeyValues("Label");
	
	kvLabel->SetString("wide", "10");
	kvLabel->SetString("tall", "50");
	kvLabel->SetString("ypos", "c-32");
	kvLabel->SetString("zpos", "1");
	
	kvLabel->SetString("font", "HudFontMediumBold" );
	kvLabel->SetString("fgcolor", "TanLight" );
	kvLabel->SetString("textAlignment", "right" );
	kvLabel->SetString("proportionalToParent", "1" );
	
	wchar_t wszLocalized[32];
	g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( VarArgs("#OF_Loadout_Region_%s", szCategory) ), 0 );
	
	kvLabel->SetWString("labelText", wszLocalized );
	
	kvHeaderTemplate->AddSubKey(kvButton);
	kvHeaderTemplate->AddSubKey(kvLabel);
	
	pNewItem.pHeaderItem->ApplySettings(kvHeaderTemplate);

	int x,y;
	pNewItem.pHeaderItem->GetPos( x, y );
	pNewItem.pHeaderItem->SetPos(iLastXPos , y);

	pNewItem.def_xpos = iLastXPos;
	pNewItem.def_ypos = y;
	
	m_hCategories.AddToTail(pNewItem);
	
	int w, h;
	pNewItem.pHeaderItem->GetSize( w, h );
	iLastXPos += w;
	
	GetSize( w, h );
	pScrollBar->SetRange( 0, iLastXPos );
	pScrollBar->SetRangeWindow( w );
}

void CTFLoadoutHeader::ClearCategoryList()
{
	for( int i = 0; i < m_hCategories.Count(); i++ )
	{
		m_hCategories[i].pHeaderItem->DeletePanel();
	}
	m_hCategories.Purge();
}

CTFSelectionManager::CTFSelectionManager(Panel *parent, const char *panelName) : Panel(parent, panelName)
{
	pSelectedItem = NULL;
	bHasSelectedItem = false;
}

void CTFSelectionManager::OnPanelSelected( Panel *panel )
{
	if( bHasSelectedItem )
	{
		CTFSelectionPanel *pTarget = (CTFSelectionPanel*)pSelectedItem;
		if(pTarget)
		{
			pTarget->SetSelected(false);
			pTarget->OnReleasedSelected();
		}
	}
	
	pSelectedItem = panel;
	bHasSelectedItem = true;
}

void CTFSelectionManager::ApplySettings(KeyValues *inResourceData)
{
	KeyValues *pPanels = inResourceData->FindKey("Panels");
	
	bool bFirst = true;
	
	if( pPanels )
	{
		FOR_EACH_VALUE(pPanels, kvValue)
		{
			CTFSelectionPanel *pPanel = (CTFSelectionPanel*) GetParent()->FindChildByName( kvValue->GetName() );
			if( pPanel )
			{
				m_hPanels.AddToTail(pPanel);
				pPanel->AddActionSignalTarget(this);
				if( bFirst )
				{
					pPanel->SetSelected(true);
					pPanel->OnReleasedUnselected();
					bFirst = false;
				}
			}
		}
	}
	
	BaseClass::ApplySettings(inResourceData);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFSelectionPanel::CTFSelectionPanel(Panel *parent, const char *panelName) : CTFEditableButton(parent, panelName)
{
	pImage = new CTFImagePanel(this, "Image");
	iBaseX = -999;
	iBaseY = -999;
	
	pManger = NULL;
}

void CTFSelectionPanel::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );
	
	iXAdj = inResourceData->GetInt( "XAdjustment", 0 );
	iYAdj = inResourceData->GetInt( "YAdjustment", 0 );

	KeyValues *inImage = inResourceData->FindKey("Image");
	if( inImage )
	{
		pImage->ApplySettings(inImage);
	}

	iBaseX = -999;
    iBaseY = -999;

	SetConnectedPanel( inResourceData->GetString("panel") );
}

void CTFSelectionPanel::SetConnectedPanel( const char *szConnectedPanel )
{
	Panel *pParent = GetParent();
	if( !pParent )
		return;

	pConnectedPanel = pParent->FindChildByName( szConnectedPanel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSelectionPanel::OnMouseReleased(MouseCode code)
{
	if( m_bSelected )
		return;

	BaseClass::OnMouseReleased(code);
}

void CTFSelectionPanel::OnReleasedSelected()
{
	BaseClass::OnReleasedSelected();

	if( pConnectedPanel )
		pConnectedPanel->SetVisible( false );
}

void CTFSelectionPanel::OnReleasedUnselected()
{
	BaseClass::OnReleasedUnselected();

	if( pConnectedPanel )
		pConnectedPanel->SetVisible( true );
}

void CTFSelectionPanel::SetSelected( bool bSelected )
{
	BaseClass::SetSelected( bSelected );
	
	int x, y;
	GetPos( x, y );	
	
	if( !bSelected )
	{
		if( iBaseX == -999 )
			iBaseX = x;
		
		if( iBaseY == -999 )
			iBaseY = y;
		SetPos( iBaseX , iBaseY );
	}
	else
	{
		if( iBaseX == -999 )
			iBaseX = x;
		
		if( iBaseY == -999 )
			iBaseY = y;	

		SetPos( iBaseX - iXAdj , iBaseY - iYAdj );
		
		PostActionSignal(new KeyValues("OnPanelSelected"));
	}
}
DECLARE_BUILD_FACTORY( CTFColorPanel );
DECLARE_BUILD_FACTORY( CTFColorSlider );

ConVar of_use_rgb("of_use_rgb", "0", FCVAR_CLIENTDLL | FCVAR_USERINFO | FCVAR_ARCHIVE );

CTFColorPanel::CTFColorPanel(Panel *parent, const char *panelName) : EditablePanel(parent, panelName)
{
	pHue = new CTFColorSlider(this ,"Hue");
	pSaturation = new CTFColorSlider(this ,"Saturation");
	pBrightness = new CTFColorSlider(this ,"Brightness");

	pHueEntry = new TextEntry(this, "HueEntry");
	pHueEntry->AddActionSignalTarget(this);	
	
	pSaturationEntry = new TextEntry(this, "SaturationEntry");
	pSaturationEntry->AddActionSignalTarget(this);	
	
	pBrightnessEntry = new TextEntry(this, "BrightnessEntry");
	pBrightnessEntry->AddActionSignalTarget(this);

	pRed = new CTFColorSlider(this ,"Red");
	pGreen = new CTFColorSlider(this ,"Green");
	pBlue = new CTFColorSlider(this ,"Blue");
	
	pRedEntry = new TextEntry(this, "RedEntry");
	pRedEntry->AddActionSignalTarget(this);
	
	pGreenEntry = new TextEntry(this, "GreenEntry");
	pGreenEntry->AddActionSignalTarget(this);
	
	pBlueEntry = new TextEntry(this, "BlueEntry");
	pBlueEntry->AddActionSignalTarget(this);
	
	pHexEntry = new TextEntry(this, "HexEntry");
	pHexEntry->AddActionSignalTarget(this);
	
	cHueS = Color( 255, 255, 255, 255 );
	cHueB = Color( 255, 255, 255, 255 );
	cHueBnoS = Color( 255, 255, 255, 255 );
	
	iCurrRed = 0;
	iCurrGreen = 0;	
	iCurrBlue = 0;	

    pRGBToggle = new CCvarToggleCheckButton(
        this,
        "RGBToggle",
        "Use RGB",
        "of_use_rgb");
	
	bReset = false;
	bRGBOn = false;
	bUpdateHexValue = true;
	
	SetScheme( "ClientScheme" );
}

void CTFColorPanel::OnColorChanged( bool bTriggerChangeMessage, bool bUpdateHexValue )
{
	if( !bReset )
		return;
	
	if( !of_use_rgb.GetBool() )
	{
		if( !pHue || !pSaturation || !pBrightness )
			return;

		float iH = pHue->GetValue();
		float iS = pSaturation->GetValue() / 100.0f;
		float iB = pBrightness->GetValue() / 100.0f;
		
		Color RGB = HSBtoRGB( iH, iS, iB );
		cHueB = HSBtoRGB( iH, 1, iB );
		cHueBnoS = HSBtoRGB( iH, 0, iB );
		cHueS = HSBtoRGB( iH, iS, 1 );

		int iRed, iGreen, iBlue, iAlpha;
		RGB.GetColor(iRed, iGreen, iBlue, iAlpha);
		
		engine->ExecuteClientCmd( VarArgs("of_color_r %d", iRed) );
		engine->ExecuteClientCmd( VarArgs("of_color_g %d", iGreen) );
		engine->ExecuteClientCmd( VarArgs("of_color_b %d", iBlue) );
		
		iCurrRed = iRed;
		iCurrGreen = iGreen;
		iCurrBlue = iBlue;
	}
	else
	{
		if( !pRed || !pGreen || !pBlue )
			return;

		engine->ExecuteClientCmd( VarArgs("of_color_r %d", pRed->GetValue()) );
		engine->ExecuteClientCmd( VarArgs("of_color_g %d", pGreen->GetValue()) );
		engine->ExecuteClientCmd( VarArgs("of_color_b %d", pBlue->GetValue()) );

		iCurrRed = pRed->GetValue();
		iCurrGreen = pGreen->GetValue();
		iCurrBlue = pBlue->GetValue();
	}
	
	if( bTriggerChangeMessage )
	{
		OnControlModified( pHue );
		OnControlModified( pSaturation );
		OnControlModified( pBrightness );

		OnControlModified( pRed );
		OnControlModified( pGreen );
		OnControlModified( pBlue );			
	}
	
	if( bUpdateHexValue )
	{
		char szRedHex[3];
		Q_snprintf( szRedHex, sizeof(szRedHex), of_color_r.GetInt() > 15 ? "%X" : "0%X", of_color_r.GetInt() );
		
		char szGreenHex[3];
		Q_snprintf( szGreenHex, sizeof(szGreenHex), of_color_g.GetInt() > 15 ? "%X" : "0%X", of_color_g.GetInt() );
		
		char szBlueHex[3];
		Q_snprintf( szBlueHex, sizeof(szBlueHex), of_color_b.GetInt() > 15 ? "%X" : "0%X", of_color_b.GetInt() );	
		
		pHexEntry->SetText(VarArgs("#%s%s%s", szRedHex, szGreenHex, szBlueHex ));		
	}
}

static Color iFade[] = 
{
	Color( 255, 0, 0, 255 ),
	Color( 255, 255, 0, 255 ),
	Color( 0, 255, 0, 255 ),
	Color( 0, 255, 255, 255 ),
	Color( 0, 0, 255, 255 ),
	Color( 255, 0, 255, 255 ),
	Color( 255, 0, 0, 255 ),
};

void CTFColorPanel::RecalculateColorValues()
{
	if( pRed && pGreen && pBlue && pRedEntry && pGreenEntry && pBlueEntry )
	{
		pRed->SetVisible( of_use_rgb.GetBool() );
		pGreen->SetVisible( of_use_rgb.GetBool() );
		pBlue->SetVisible( of_use_rgb.GetBool() );
		
		pRedEntry->SetVisible( of_use_rgb.GetBool() );
		pGreenEntry->SetVisible( of_use_rgb.GetBool() );
		pBlueEntry->SetVisible( of_use_rgb.GetBool() );
	}
	
	if( pHue && pSaturation && pBrightness && pHueEntry && pSaturationEntry && pBrightnessEntry )
	{
		pHue->SetVisible( !of_use_rgb.GetBool() );
		pSaturation->SetVisible( !of_use_rgb.GetBool() );
		pBrightness->SetVisible( !of_use_rgb.GetBool() );
		
		pHueEntry->SetVisible( !of_use_rgb.GetBool() );
		pSaturationEntry->SetVisible( !of_use_rgb.GetBool() );
		pBrightnessEntry->SetVisible( !of_use_rgb.GetBool() );
	}
	
	Panel *pHueLabel = FindChildByName( "HueLabel" );
	Panel *pSaturationLabel = FindChildByName( "SaturationLabel" );
	Panel *pBrightnessLabel = FindChildByName( "BrightnessLabel" );

	if( pHueLabel && pSaturationLabel && pBrightnessLabel )
	{
		pHueLabel->SetVisible( !of_use_rgb.GetBool() );
		pSaturationLabel->SetVisible( !of_use_rgb.GetBool() );
		pBrightnessLabel->SetVisible( !of_use_rgb.GetBool() );
	}
	
	Panel *pRedLabel = FindChildByName( "RedLabel" );
	Panel *pGreenLabel = FindChildByName( "GreenLabel" );
	Panel *pBlueLabel = FindChildByName( "BlueLabel" );

	if( pRedLabel && pGreenLabel && pBlueLabel )
	{
		pRedLabel->SetVisible( of_use_rgb.GetBool() );
		pGreenLabel->SetVisible( of_use_rgb.GetBool() );
		pBlueLabel->SetVisible( of_use_rgb.GetBool() );
	}
	
	bRGBOn = of_use_rgb.GetBool();

	if( bRGBOn )
	{
		bReset = false;

		pRed->SetValue(of_color_r.GetInt(), false);
		pGreen->SetValue(of_color_g.GetInt(), false);
		pBlue->SetValue(of_color_b.GetInt(), false);

		char buf[64];
		Q_snprintf(buf, sizeof(buf), "%d", (int)of_color_r.GetInt() );
		pRedEntry->SetText(buf);                               
											   
		Q_snprintf(buf, sizeof(buf), "%d", (int)of_color_g.GetInt() );
		pGreenEntry->SetText(buf);                        
											   
		Q_snprintf(buf, sizeof(buf), "%d", (int)of_color_b.GetInt() );
		pBlueEntry->SetText(buf);

		bReset = true;
	}
	else
	{
		bReset = false;
		rgb in;
		
		in.r = of_color_r.GetFloat() / 255;
		in.g = of_color_g.GetFloat() / 255;
		in.b = of_color_b.GetFloat() / 255;

		hsv out = rgb2hsv( in );
		
		float iH = out.h;
		float iS = out.s;
		float iB = out.v;
		
		cHueB = HSBtoRGB( iH, 1, iB );
		cHueBnoS = HSBtoRGB( iH, 0, iB );
		cHueS = HSBtoRGB( iH, iS, 1 );
		
		char buf[64];
		Q_snprintf(buf, sizeof(buf), "%d", (int)out.h);
		pHueEntry->SetText(buf);
		
		Q_snprintf(buf, sizeof(buf), "%d",(int)(out.s * 100));
		pSaturationEntry->SetText(buf);
		
		Q_snprintf(buf, sizeof(buf), "%d",(int)(out.v * 100));
		pBrightnessEntry->SetText(buf);
		
		pHue->SetValue(out.h, false);
		pSaturation->SetValue(out.s * 100, false);
		pBrightness->SetValue(out.v * 100, false);
		
		bReset = true;
	}
	
	char szRedHex[3];
	Q_snprintf( szRedHex, sizeof(szRedHex), of_color_r.GetInt() > 15 ? "%X" : "0%X", of_color_r.GetInt() );
	
	char szGreenHex[3];
	Q_snprintf( szGreenHex, sizeof(szGreenHex), of_color_g.GetInt() > 15 ? "%X" : "0%X", of_color_g.GetInt() );

	char szBlueHex[3];
	Q_snprintf( szBlueHex, sizeof(szBlueHex), of_color_b.GetInt() > 15 ? "%X" : "0%X", of_color_b.GetInt() );	

	pHexEntry->SetText(VarArgs("#%s%s%s", szRedHex, szGreenHex, szBlueHex ));
	
	iCurrRed = of_color_r.GetInt();
	iCurrGreen = of_color_g.GetInt();
	iCurrBlue = of_color_b.GetInt();
}

void CTFColorPanel::PaintBackground()
{
	if( iCurrRed != of_color_r.GetInt() ||
	iCurrGreen != of_color_g.GetInt() ||
	iCurrBlue != of_color_b.GetInt() )
		bReset = false;
	
	if( !bReset )
		bRGBOn = !of_use_rgb.GetBool();

	if( bRGBOn != of_use_rgb.GetBool() )
	{	
		RecalculateColorValues();
	}

	if( !of_use_rgb.GetBool() )
	{
		if( !pHue )
			return;

		int wide, tall;
		pHue->GetSize( wide, tall );
		int x,y;
		pHue->GetPos( x, y );

		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;
		
		for( int i = 0; i < 6; i++ )
		{
			int lastwide = x + ( wide * ( ( 1.0f / 6.0f ) * (i) ) );
			int curwide = x + ( wide * ( ( 1.0f / 6.0f ) * (1 + i) ) );

			vgui::surface()->DrawSetColor( iFade[i] );
			vgui::surface()->DrawFilledRectFade( lastwide, y, curwide , y + tall, 255, 0, true );
			
			vgui::surface()->DrawSetColor( iFade[i+1] );
			vgui::surface()->DrawFilledRectFade( lastwide, y, curwide , y + tall, 0, 255, true );
		}

		if( !pSaturation )
			return;

		pSaturation->GetSize( wide, tall );
		pSaturation->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;

		vgui::surface()->DrawSetColor( cHueBnoS );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );
		
		vgui::surface()->DrawSetColor( cHueB );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 55, 255, true );
		
		if( !pBrightness )
			return;

		pBrightness->GetSize( wide, tall );
		pBrightness->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;

		vgui::surface()->DrawSetColor( Color( 0, 0, 0, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );

		vgui::surface()->DrawSetColor( cHueS );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 0, 255, true );
	}
	else
	{
		int wide, tall;
		int x,y;
		
		int iRed = of_color_r.GetInt();
		int iGreen = of_color_g.GetInt();
		int iBlue = of_color_b.GetInt();

		if( !pRed )
			return;

		pRed->GetSize( wide, tall );
		pRed->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;

		vgui::surface()->DrawSetColor( Color( 0, iGreen, iBlue, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );

		vgui::surface()->DrawSetColor( Color( 255, iGreen, iBlue, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 0, 255, true );
		
		if( !pGreen )
			return;

		pGreen->GetSize( wide, tall );
		pGreen->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;
		
		vgui::surface()->DrawSetColor( Color( iRed, 0, iBlue, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );

		vgui::surface()->DrawSetColor( Color( iRed, 255, iBlue, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 0, 255, true );
		
		if( !pBlue )
			return;

		pBlue->GetSize( wide, tall );
		pBlue->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;

		vgui::surface()->DrawSetColor( Color( iRed, iGreen, 0, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );

		vgui::surface()->DrawSetColor( Color( iRed, iGreen, 255, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 0, 255, true );
	}
}

void CTFColorPanel::OnThink()
{
	BaseClass::OnThink();
}

void CTFColorPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFColorPanel::OnTextChanged(Panel *panel)
{
	if( !bReset )
		return;

    if( panel == pHueEntry )
    {
        char buf[64];
        pHueEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0.0 && iValue <= 360)
        {
            pHue->SetValue(iValue, false);
        }
    }
    else if( panel == pSaturationEntry )
    {
        char buf[64];
        pSaturationEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0.0  && iValue <= 100)
        {
            pSaturation->SetValue(iValue, false);
        }
    }
    else if( panel == pBrightnessEntry )
    {
        char buf[64];
        pBrightnessEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0.0  && iValue <= 100)
        {
            pBrightness->SetValue(iValue, false);
        }
    }
	else if( panel == pRedEntry )
    {
        char buf[64];
        pRedEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0.0  && iValue <= 255)
        {
            pRed->SetValue(iValue, false);
        }
    }
    else if( panel == pGreenEntry )
    {
        char buf[64];
        pGreenEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0 && iValue <= 255)
        {
            pGreen->SetValue(iValue, false);
        }
    }
    else if( panel == pBlueEntry )
    {
        char buf[64];
        pBlueEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0 && iValue <= 255)
        {
            pBlue->SetValue(iValue, false);
        }
    }
	else if( panel == pHexEntry )
	{
		bUpdateHexValue = false;

		int i = 0;
        char buf[64];
        pHexEntry->GetText(buf, 64);
		strupr(buf);
		if( buf[0] == '#' )
			i++;
		
		char szRedVal[2];
		szRedVal[0] = buf[i];
		szRedVal[1] = buf[i+1];
		
		char szGreenVal[2];
		szGreenVal[0] = buf[i+2];
		szGreenVal[1] = buf[i+3];	
		
		char szBlueVal[2];
		szBlueVal[0] = buf[i+4];
		szBlueVal[1] = buf[i+5];	
		
		int iRed = -1;
		sscanf(szRedVal, "%X", &iRed);

		int iGreen = -1;
		sscanf(szGreenVal, "%X", &iGreen);

		int iBlue = -1;
		sscanf(szBlueVal, "%X", &iBlue);
		
		if( of_use_rgb.GetBool() )
		{
			pRed->SetValue(iRed, false);
			OnControlModified( pRed );
			
			pGreen->SetValue(iGreen, false);
			OnControlModified( pGreen );
			
			pBlue->SetValue(iBlue, false);
			OnControlModified( pBlue );	
		}
		else
		{
			rgb in;
			in.r = iRed ? (float)iRed / 255.0f : 0;
			in.g = iGreen ? (float)iGreen / 255.0f : 0;
			in.b = iBlue ? (float)iBlue / 255.0f : 0;
			hsv out = rgb2hsv( in );
			
			pHue->SetValue(out.h, false);
			OnControlModified( pHue );
			
			pSaturation->SetValue(out.s * 100, false);
			OnControlModified( pSaturation );
			
			pBrightness->SetValue(out.v * 100, false);
			OnControlModified( pBrightness );	
		}
		
		bUpdateHexValue = true;
	}
}

void CTFColorPanel::OnControlModified(Panel *panel)
{
    if( panel == pHue )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pHue->GetValue());
		pHueEntry->SetText(buf);
    }
	else if( panel == pSaturation )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pSaturation->GetValue());
		pSaturationEntry->SetText(buf);
    }
	else if( panel == pBrightness )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pBrightness->GetValue());
		pBrightnessEntry->SetText(buf);
    }
	else if( panel == pRed )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pRed->GetValue());
		pRedEntry->SetText(buf);
    }
	else if( panel == pGreen )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pGreen->GetValue());
		pGreenEntry->SetText(buf);
    }
	else if( panel == pBlue )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pBlue->GetValue());
		pBlueEntry->SetText(buf);
    }
}

CTFColorSlider::CTFColorSlider(Panel *parent, const char *panelName) : Slider(parent, panelName)
{
	pParent = dynamic_cast<CTFColorPanel*>(parent);
	AddActionSignalTarget( this );
	iSliderTextureID = -1;
}

void CTFColorSlider::SetValue(int value, bool bTriggerChangeMessage)
{
	BaseClass::SetValue(value, bTriggerChangeMessage);
	
	if( bTriggerChangeMessage )
		PostActionSignal(new KeyValues("ControlModified"));
	
	bool bUpdateHex = true;
	if( GetParent() )
	{
		CTFColorPanel *pParent = dynamic_cast<CTFColorPanel*>(GetParent());
		bUpdateHex = pParent->ShouldUpdateHex();		
	}
	
	if( pParent )
	{
		pParent->OnColorChanged(bTriggerChangeMessage, bUpdateHex);
	}
}

void CTFColorSlider::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);
	SetThumbWidth(15);
	SetDragOnRepositionNob( true );

	vgui::HScheme scheme = vgui::scheme()->GetScheme( "ClientScheme" );
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( scheme );
	SetBorder( pScheme->GetBorder( inResourceData->GetString("border") ));

	iSliderWidth = inResourceData->GetInt("thumbwidth", 0);
}

//-----------------------------------------------------------------------------
// Purpose: Draw the nob part of the slider.
//-----------------------------------------------------------------------------
void CTFColorSlider::DrawNob()
{
	// horizontal nob
	int x, y;
	int wide,tall;
	GetTrackRect( x, y, wide, tall );
	Color col = GetFgColor();
#ifdef _X360
	if(HasFocus())
	{
		col = m_DepressedBgColor;
	}
#endif
	surface()->DrawSetColor(Color( 255, 255, 255, 255 ));

	int iPanelWide, iPanelTall;
	int iPanelX, iPanelY;
	GetPos( iPanelX, iPanelY );
	GetSize( iPanelWide, iPanelTall );
//	int nobheight = iPanelTall;

	// Set up the image ID's if they've somehow gone bad.
	if( iSliderTextureID == -1 )
		iSliderTextureID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( iSliderTextureID, "vgui/loadout/color_slider", true, false );	
	
	vgui::surface()->DrawSetTexture(iSliderTextureID);
	
//	x, y, x + wide , y + tall
	
	surface()->DrawTexturedRect(
		_nobPos[0], 
		0, 
		_nobPos[0] + iSliderWidth, 
		iPanelTall);
	/* border
	if (_sliderBorder)
	{
		_sliderBorder->Paint(
			_nobPos[0], 
			y + tall / 2 - nobheight / 2, 
			_nobPos[1], 
			y + tall / 2 + nobheight / 2);
	}*/
}

void CTFColorSlider::PaintBackground()
{
	BaseClass::BaseClass::PaintBackground();
}