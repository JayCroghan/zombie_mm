/* ======== sample_mm ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): David "BAILOPAN" Anderson
* ============================
*/

#ifndef _INCLUDE_ZUTIL_H
#define _INCLUDE_ZUTIL_H

#include "ZombiePlugin.h"

void UTIL_ScreenFadeBuild( ScreenFade_t &fade, const color32 &color, float fadeTime, float fadeHold, int flags );

void UTIL_ScreenFadeWrite( const ScreenFade_t &fade, CBaseEntity *pEntity );

void UTIL_ScreenFade( CBaseEntity *pEntity, const color32 &color, float fadeTime, float fadeHold, int flags );

//CVGuiScreen *UTIL_CreateVGuiScreen( const char *pScreenClassname, const char *pScreenType, CBaseEntity *pAttachedTo, CBaseEntity *pOwner, int nAttachmentIndex );
//extern UTIL_ScreenFadeBuild;
//extern UTIL_ScreenFadeWrite;
//extern UTIL_ScreenFade;

#endif //_INCLUDE_ZUTIL_H
