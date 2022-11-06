//
//  Hook_Direct3D8.h
//  gpumon
//
//  Created by Aeroshogun on 4/20/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#ifndef Hook_Direct3D8_h
#define Hook_Direct3D8_h

/* 
 * Direct3D8 header 
 */

#ifdef INCLUDE_D3D8	// Avoiding collisions with D3D9, because I'm too lazy to use namespaces...
#include <d3d8.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifdef INCLUDE_D3D8	// Avoiding collisions with D3D9, because I'm too lazy to use namespaces...
/*
 * Typedefs 
 */
typedef IDirect3D8* (WINAPI* PFN_Direct3DCreate8)( UINT );
typedef HRESULT (WINAPI* PFN_D3DDevice8_Present)( IDirect3DDevice8*, CONST RECT*, CONST RECT*, CONST HWND, CONST RGNDATA* );	// 17


/*
 * Hooked Direct3D8 API functions
 */
struct Hook_Direct3D8API
{
	/* Standard D3D8 */
	PFN_Direct3DCreate8			Direct3DCreate8;
	PFN_D3DDevice8_Present		D3DDevice8_Present;		// 15

	DWORD reserved;
};

void Drv_GetDirect3D8Hooks( struct Hook_Direct3D8API* hooks, struct Hook_Direct3D8API* originals );
#endif

BOOL Drv_EnableDirect3D8Hooks();
BOOL Drv_DisableDirect3D8Hooks();

#ifdef __cplusplus
}
#endif

#endif /* Hook_Direct3D8_h */
