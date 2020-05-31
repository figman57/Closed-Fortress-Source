//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_LOADOUT2_H
#define OF_LOADOUT2_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_controls.h"
#include "basemodel_panel.h"
#include <vgui_controls/Slider.h>

class CStudioHdr;
class CCvarToggleCheckButton;
class CModelPanel;

struct t_CosmeticID
{
	int iID;
};

enum
{
	BORDER_IDLE = 0,
	BORDER_HOVEROVER,
	BORDER_PRESSED,
	BORDER_SELECTED,
};

class CTFEditableButtonFunc : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFEditableButtonFunc, vgui::EditablePanel );

public:
	CTFEditableButtonFunc(vgui::Panel *parent, const char *panelName);	 

	virtual void OnCursorExited();
	virtual void OnCursorEntered();
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);
};

class CTFEditableButton : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFEditableButton, vgui::EditablePanel );

public:
	CTFEditableButton(vgui::Panel *parent, const char *panelName);	 

	virtual void ApplySettings(KeyValues *inResourceData);

	virtual void OnCursorExited();
	virtual void OnCursorEntered();
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void OnReleasedSelected();
	virtual void OnReleasedUnselected();
	void AddOnPressSound( char *szPressedSound );
	
	void	SetBorderType( int iBorder );
	virtual void SetSelected( bool bSelected );
public:
	char	szBorderIdle[128];
	char	szBorderHover[128];
	char	szBorderPressed[128];
	char	szBorderSelected[128];
	
	int		iCurrentBorder;
	int		m_iSoundChance;
	
	bool	m_bSelected;
	
	CUtlVector<char*> m_hPressedSounds;
	
	CTFEditableButtonFunc	*pButton;
};

class CTFCommandButton : public CTFEditableButton
{
private:
	DECLARE_CLASS_SIMPLE( CTFCommandButton, CTFEditableButton );

public:
	CTFCommandButton(vgui::Panel *parent, const char *panelName);	 

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void OnReleasedSelected();
	virtual void OnReleasedUnselected();
	virtual void PaintBackground();
public:
	char	szCommand[128];
	char	szUnselectCommand[128];
	char	szConvref[128];
	char	szTargetVal[128];
};

struct ItemTemplate_t
{
	char wide[8];
	char tall[8];
	char border_idle[64];
	char border_hover[64];
	char border_pressed[64];
	char border_selected[64];
	
	char button_wide[8];
	char button_tall[8];
	char button_xpos[8];
	char button_ypos[8];
	char button_zpos[8];
	
	char extra_wide[8];
	char extra_tall[8];
	char extra_xpos[8];
	char extra_ypos[8];
	char extra_zpos[8];
};

class CTFItemSelection : public CTFEditableButton
{
private:
	DECLARE_CLASS_SIMPLE( CTFItemSelection, CTFEditableButton );

public:
	CTFItemSelection(vgui::Panel *parent, const char *panelName, const int iID);	 

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void OnReleasedSelected();
	virtual void OnReleasedUnselected();	
	virtual void Paint();
	void	SetItemID( int iID );
	virtual void SetSelected( bool bSelected );

public:
	int		iItemID;
	char	szCommand[64];
	bool	bParsedBPImage;
	
	CTFImagePanel	*pItemImage;
};

struct PanelListItem_t
{
	vgui::EditablePanel *pPanel;
	int	def_xpos;
	int def_ypos;
};

class CTFScrollablePanelList : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFScrollablePanelList, vgui::EditablePanel );
public:
	CTFScrollablePanelList( vgui::Panel *parent, const char *panelName );
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void PerformLayout();
	
	void AddItem( CTFEditableButton *pPanel );
	void ClearItemList( void );
	
	MESSAGE_FUNC( OnScrollBarSliderMoved, "ScrollBarSliderMoved" );
	MESSAGE_FUNC_PTR( OnSelectionChanged, "SetSelected", panel );
	virtual void OnMouseWheeled(int delta);	// respond to mouse wheel events	

public:
	CUtlVector<PanelListItem_t> m_hItems;
	vgui::ScrollBar *pScrollBar;
	
	CTFEditableButton *pSelectedItem;
	
	char szCategoryName[32];
	
	int iCollumnSpacing;
	int iRowSpacing;
	
	int iElementWidth;
	int iElementHeight;
	
	int iElementsPerRow;
	int iElementsPerScroll;
	
	int	iLastX;
	int	iLastY;
	
	PanelListItem_t t_PanelTemplate;
};

struct ItemListItem_t
{
	CTFItemSelection *pItemPanel;
	int	def_xpos;
	int def_ypos;
};

class CTFScrollableItemList : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFScrollableItemList, vgui::EditablePanel );
public:
	CTFScrollableItemList( vgui::Panel *parent, const char *panelName );
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void PerformLayout();
	
	void AddItem( int iID );
	void ClearItemList( void );
	
	MESSAGE_FUNC( OnScrollBarSliderMoved, "ScrollBarSliderMoved" );
	virtual void OnMouseWheeled(int delta);	// respond to mouse wheel events	

public:
	CUtlVector<ItemListItem_t> m_hItems;
	vgui::ScrollBar *pScrollBar;
	
	CTFItemSelection *pSelectedItem;
	
	char szCategoryName[32];
	
	int iCollumnSpacing;
	int iRowSpacing;
	
	int iElementWidth;
	int iElementHeight;
	
	int iElementsPerRow;
	int iElementsPerScroll;
	
	int iLastX;
	int iLastY;
	
	ItemTemplate_t t_ItemTemplate;
};

class CTFSelectionPanel;

class CTFSelectionManager : public vgui::Panel
{
private:
	DECLARE_CLASS_SIMPLE( CTFSelectionManager, vgui::Panel );

public:
	CTFSelectionManager(vgui::Panel *parent, const char *panelName);	 

	virtual void ApplySettings(KeyValues *inResourceData);
	MESSAGE_FUNC_PTR( OnPanelSelected, "OnPanelSelected", panel );
public:
	vgui::Panel *pSelectedItem;
	bool bHasSelectedItem;
	
	CUtlVector<CTFSelectionPanel*> m_hPanels;
};

class CTFSelectionPanel : public CTFEditableButton
{
private:
	DECLARE_CLASS_SIMPLE( CTFSelectionPanel, CTFEditableButton );

public:
	CTFSelectionPanel(vgui::Panel *parent, const char *panelName);	 

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void OnReleasedSelected();
	virtual void OnReleasedUnselected();
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void SetSelected( bool bSelected );
	void	SetConnectedPanel( const char *szConnectedPanel );
public:
	vgui::Panel		*pConnectedPanel;
	CTFSelectionManager *pManger;

	int iBaseX;
	int iBaseY;
	
	int iXAdj;
	int iYAdj;
	vgui::HFont 	m_hTextFont;
	CTFImagePanel	*pImage;
};

class CTFHeaderItem : public CTFEditableButton
{
private:
	DECLARE_CLASS_SIMPLE( CTFHeaderItem, CTFEditableButton );

public:
	CTFHeaderItem(vgui::Panel *parent, const char *panelName);	 

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void OnReleasedSelected();
	virtual void OnReleasedUnselected();
	virtual void PerformLayout();
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void SetSelected( bool bSelected );
	void	SetConnectedPanel( const char *szConnectedPanel );
	void CalculateBoxSize();
public:
	vgui::Panel		*pConnectedPanel;

	int iBaseHeight;
	vgui::HFont 	m_hTextFont;
	CExLabel		*pLabel;
};

struct HeaderListItem_t
{
	HeaderListItem_t()
	{
		pHeaderItem = NULL;
	}
	CTFHeaderItem *pHeaderItem;
	int	def_xpos;
	int def_ypos;
};

class CTFLoadoutHeader : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFLoadoutHeader, vgui::EditablePanel );
public:
	CTFLoadoutHeader( vgui::Panel *parent, const char *panelName );
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void PerformLayout();
	
	void AddCategory( const char *szCategory );
	void ClearCategoryList( void );
	
	MESSAGE_FUNC( OnScrollBarSliderMoved, "ScrollBarSliderMoved" );
	virtual void OnMouseWheeled(int delta);	// respond to mouse wheel events	

public:
	CUtlVector<HeaderListItem_t> m_hCategories;
	vgui::ScrollBar *pScrollBar;
	
	CTFHeaderItem *pSelectedHeader;

	int iLastXPos;
	
	ItemTemplate_t t_ItemTemplate;
};

class CTFColorPanel;
namespace vgui
{
	class CTFModelPanel : public CBaseModelPanel
	{
		DECLARE_CLASS_SIMPLE( CTFModelPanel, CBaseModelPanel );

	public:
		CTFModelPanel( vgui::Panel *pParent, const char *pszName );

		virtual void	ApplySettings( KeyValues *inResourceData );
		virtual void	OnThink();
		virtual void	Update();
		virtual void	Paint();

		virtual void	SetModelName( const char* pszModelName, int nSkin = 0 );
		virtual void	SetAnimationIndex( int index ) { m_iAnimationIndex = index; };
		void SetParticleName(const char* name);
		void SetBodygroup( int iGroup, int iValue );
		int GetBodygroup( int iGroup );

		const char *GetBodygroupName( int iGroup );
		int FindBodygroupByName( const char *name );
		int GetBodygroupCount( int iGroup );
		int GetNumBodyGroups( void );

		Vector			m_vecDefPosition;
		QAngle			m_vecDefAngles;

		void			RefreshModel();
		CStudioHdr		*GetModelPtr();
		int				m_iAnimationIndex;
		float			m_flParticleZOffset;
		bool			m_bLoopParticle;
		float			m_flLoopTime;
		float			m_flLoopParticleAfter;
		char			szLoopingParticle[128];
	private:
		CStudioHdr		m_StudioHdr;
		particle_data_t *m_pData;
	};
	
	class CTFColorSlider : public Slider
	{
		DECLARE_CLASS_SIMPLE( CTFColorSlider, Slider );
		public:
			CTFColorSlider(Panel *parent, const char *panelName);
			virtual void SetValue(int value, bool bTriggerChangeMessage = true); 
			void SetValueRaw(int value, bool bTriggerChangeMessage = true); 
			virtual void DrawNob();
			virtual void PaintBackground();
			virtual void ApplySettings( KeyValues *inResourceData );
		private:
			CTFColorPanel *pParent;
			int iSliderTextureID;
			int iSliderWidth;
	};
}

class CTFColorPanel : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFColorPanel, vgui::EditablePanel );
public:
	CTFColorPanel(Panel *parent, const char *panelName);
	void OnColorChanged( bool bTriggerChangeMessage = true, bool bUpdateHexValue = true );
	virtual void PaintBackground();
	virtual void OnThink();
	virtual void ApplySettings( KeyValues *inResourceData );
	bool ShouldUpdateHex(void){ return bUpdateHexValue; };
	void RecalculateColorValues();
private:
	MESSAGE_FUNC_PTR( OnControlModified, "ControlModified", panel );
    MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel )
	{
		OnControlModified( panel );
	}

	vgui::CTFColorSlider *pHue;
	vgui::CTFColorSlider *pSaturation;
	vgui::CTFColorSlider *pBrightness;
	
	vgui::TextEntry *pHueEntry;
	vgui::TextEntry *pSaturationEntry;
	vgui::TextEntry *pBrightnessEntry;

	vgui::CTFColorSlider *pRed;
	vgui::CTFColorSlider *pGreen;
	vgui::CTFColorSlider *pBlue;	
	
	vgui::TextEntry *pRedEntry;
	vgui::TextEntry *pGreenEntry;
	vgui::TextEntry *pBlueEntry;
	
	vgui::TextEntry *pHexEntry;
	
	Color cHueS;
	Color cHueB;
	Color cHueBnoS;

	int iCurrRed;
	int iCurrGreen;
	int iCurrBlue;
	
	CCvarToggleCheckButton *pRGBToggle;
	
	bool bUpdateHexValue;
	
	bool bRGBOn;
	
	bool bReset;	
};


#endif // OF_LOADOUT_H
