#include "ZombiePlugin.h"
#include "ZM_Util.h"
#include "VFunc.h"
#include "mathlib.h"

static unsigned short FixedUnsigned16( float value, float scale )
{
	int output;

	output = value * scale;
	if ( output < 0 )
		output = 0;
	if ( output > 0xFFFF )
		output = 0xFFFF;

	return (unsigned short)output;
}

void UTIL_ScreenFadeBuild( ScreenFade_t &fade, const color32 &color, float fadeTime, float fadeHold, int flags )
{
	fade.duration = FixedUnsigned16( fadeTime, 1<<SCREENFADE_FRACBITS );		// 7.9 fixed
	fade.holdTime = FixedUnsigned16( fadeHold, 1<<SCREENFADE_FRACBITS );		// 7.9 fixed
	fade.r = color.r;
	fade.g = color.g;
	fade.b = color.b;
	fade.a = color.a;
	fade.fadeFlags = flags;
}


void UTIL_ScreenFadeWrite( const ScreenFade_t &fade, CBaseEntity *pEntity )
{
	int						iPlayer;
	bf_write				*netmsg;
	if ( !IsValidPlayer( pEntity, &iPlayer ) )
	{
		return;
	}

	MRecipientFilter user;

	user.AddPlayer( iPlayer );

	static int iFade = -1;
	if ( iFade == -1 ) 
	{
		iFade = g_ZombiePlugin.UserMessageIndex("Fade");
	}
	if ( iFade == -1 )
	{
		return;
	}
	netmsg = m_Engine->UserMessageBegin ( &user, iFade );
		netmsg->WriteShort	( fade.duration );
		netmsg->WriteShort	( fade.holdTime );
		netmsg->WriteShort	( fade.fadeFlags );
		netmsg->WriteByte	( fade.r );
		netmsg->WriteByte	( fade.g );
		netmsg->WriteByte	( fade.b );
		netmsg->WriteByte	( fade.a );
    m_Engine->MessageEnd ();

	//UserMessageBegin( user, "Fade" );		// use the magic #1 for "one client"
	//	WRITE_SHORT( fade.duration );		// fade lasts this long
	//	WRITE_SHORT( fade.holdTime );		// fade lasts this long
	//	WRITE_SHORT( fade.fadeFlags );		// fade type (in / out)
	//	WRITE_BYTE( fade.r );				// fade red
	//	WRITE_BYTE( fade.g );				// fade green
	//	WRITE_BYTE( fade.b );				// fade blue
	//	WRITE_BYTE( fade.a );				// fade blue
	//MessageEnd();
}

void UTIL_ScreenFade( CBaseEntity *pEntity, const color32 &color, float fadeTime, float fadeHold, int flags )
{
	ScreenFade_t	fade;

	UTIL_ScreenFadeBuild( fade, color, fadeTime, fadeHold, flags );
	UTIL_ScreenFadeWrite( fade, pEntity );
}

//vgui_screen
//pPanelName = NULL;
//pScreenClassname = "vgui_screen"
/*
CVGuiScreen *UTIL_CreateVGuiScreen( const char *pScreenClassname, const char *pScreenType, CBaseEntity *pAttachedTo, CBaseEntity *pOwner, int nAttachmentIndex )
{
	
	//Assert( pAttachedTo );

	edict_t *cEdict;

	CVGuiScreen *pScreen = ( CVGuiScreen * )g_CreateEntityByName( pScreenClassname, -1 );

	int m_nPanelName = UTIL_FindOffsetMap( pScreen, "CVGuiScreen", "m_nPanelName" );
	int m_nAttachmentIndex = UTIL_FindOffsetMap( pScreen, "CVGuiScreen", "m_nAttachmentIndex" );
	int m_nOverlayMaterial = UTIL_FindOffsetMap( pScreen, "CVGuiScreen", "m_nOverlayMaterial" );
	int m_fScreenFlags = UTIL_FindOffsetMap( pScreen, "CVGuiScreen", "m_fScreenFlags" );

	cEdict = m_GameEnts->BaseEntityToEdict( pScreen );

	edict_t *cAttach = m_GameEnts->BaseEntityToEdict( pAttachedTo );

	Vector vOrigin = vec3_origin;
	QAngle vAngle = vec3_angle;
	CBaseEntity_Teleport( pScreen, &vOrigin, &vAngle, &vOrigin );

	//CVGuiScreen *pScreen = (CVGuiScreen *)CBaseEntity::Create( pScreenClassname, vec3_origin, vec3_angle, pAttachedTo );

	//pScreen->SetPanelName( pScreenType );
	//pScreen->m_nPanelName = g_ZombiePlugin.SetStringTable( "VguiScreen", pScreenType );
	UTIL_SetProperty( m_nPanelName, cEdict, g_ZombiePlugin.SetStringTable( "VguiScreen", pScreenType ) );
	
	//pScreen->FollowEntity( pAttachedTo );
	CBaseEntity_SetParent( pScreen, pAttachedTo );

	//pScreen->SetOwnerEntity( pOwner );
	CBaseEntity_SetOwnerEntity( pScreen, pOwner );

	//pScreen->SetAttachmentIndex( nAttachmentIndex );
	//pScreen->m_nAttachmentIndex = nAttachmentIndex;
	UTIL_SetProperty( m_nAttachmentIndex, cEdict, nAttachmentIndex );

	//int ZombiePlugin::GetStringTable( const char *sTable, const char *sMaterial )
	g_ZombiePlugin.SetStringTable( "Materials", "vgui/hud/zombiemod/hvision" );

	int iMaterial = g_ZombiePlugin.GetStringTable( "Materials", "vgui/hud/zombiemod/hvision" );

	if ( iMaterial == 0 )
	{
		//m_nOverlayMaterial = OVERLAY_MATERIAL_INVALID_STRING;
		META_CONPRINTF( "Not Found!" );
	}
	else
	{
		META_CONPRINTF( "Found!" );
		//m_nOverlayMaterial = iMaterial;
		UTIL_SetProperty( m_nOverlayMaterial, cEdict, iMaterial );
		//m_nOverlayMaterial = iMaterial;
	}

	CCSPlayer_ChangeTeam( (CCSPlayer*)pScreen, g_ZombiePlugin.GetTeam( cAttach ) );

	int iFlags;
	if ( UTIL_GetProperty( m_fScreenFlags, cEdict, &iFlags ) )
	{
		iFlags &= ~VGUI_SCREEN_ACTIVE;
		iFlags &= ~VGUI_SCREEN_VISIBLE_TO_TEAMMATES;
		iFlags &= ~VGUI_SCREEN_ATTACHED_TO_VIEWMODEL;
		UTIL_SetProperty( m_fScreenFlags, cEdict, iFlags );
	}

	//cEdict->StateChanged();
	return pScreen;
}


void DestroyVGuiScreen( CVGuiScreen *pVGuiScreen )
{
	if (pVGuiScreen)
	{
		g_UtilRemoveFunc( pVGuiScreen );
	}
}
*/

//void CBaseEntity::FollowEntity( CBaseEntity *pBaseEntity, bool bBoneMerge )
//{
//	if (pBaseEntity)
//	{
//		SetParent( pBaseEntity );
//		SetMoveType( MOVETYPE_NONE );
//		
//		if ( bBoneMerge )
//			AddEffects( EF_BONEMERGE );
//
//		AddSolidFlags( FSOLID_NOT_SOLID );
//		SetLocalOrigin( vec3_origin );
//		SetLocalAngles( vec3_angle );
//	}
//	else
//	{
//		StopFollowingEntity();
//	}
//}

