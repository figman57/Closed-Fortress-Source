//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_VIEWMODEL_H
#define TF_VIEWMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "predictable_entity.h"
#include "utlvector.h"
#include "baseplayer_shared.h"
#include "shared_classnames.h"
#include "tf_weaponbase.h"

#if defined( CLIENT_DLL )
#define CTFViewModel C_TFViewModel
#define CTFHandModel C_TFHandModel
#endif

class CTFViewModel : public CBaseViewModel
{
	DECLARE_CLASS( CTFViewModel, CBaseViewModel );
public:

	DECLARE_NETWORKCLASS();

	CTFViewModel( void );
	virtual ~CTFViewModel( void );

	virtual void CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles );
	virtual void CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles );
	virtual void AddViewModelBob( CBasePlayer *owner, Vector& eyePosition, QAngle& eyeAngles );

#if defined( CLIENT_DLL )
	virtual bool ShouldPredict( void )
	{
		if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
			return true;

		return BaseClass::ShouldPredict();
	}

	virtual void StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
	virtual void ProcessMuzzleFlashEvent( void );

	virtual int GetSkin();
	BobState_t	&GetBobState() { return m_BobState; }

	virtual int DrawModel( int flags );
	
	virtual C_BaseEntity	*GetItemTintColorOwner( void ) { return GetOwner(); }
#endif

private:

#if defined( CLIENT_DLL )

	// This is used to lag the angles.
	CInterpolatedVar<QAngle> m_LagAnglesHistory;
	QAngle m_vLagAngles;
	BobState_t		m_BobState;		// view model head bob state

	CTFViewModel( const CTFViewModel & ); // not defined, not accessible

	QAngle m_vLoweredWeaponOffset;
public:
	CNewParticleEffect	*m_pCritEffect;
#endif
};

class CTFHandModel : public CTFViewModel
{
	DECLARE_CLASS( CTFHandModel, CTFViewModel );
public:
	DECLARE_NETWORKCLASS(); 
};

#endif // TF_VIEWMODEL_H
