//
//  Hook_Direct3D9.h
//  gpumon
//
//  Created by Aeroshogun on 4/20/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#ifndef Hook_Direct3D9_h
#define Hook_Direct3D9_h

/* 
 * Direct3D9 header 
 */

#include <d3d9.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Typedefs 
 */
typedef IDirect3D9* (WINAPI* PFN_Direct3DCreate9)( UINT );
typedef HRESULT (WINAPI* PFN_D3DDevice9_Present)( IDirect3DDevice9*, CONST RECT*, CONST RECT*, CONST HWND, CONST RGNDATA* );	// 17

typedef IDirect3D9Ex* (WINAPI* PFN_Direct3DCreate9Ex)( UINT );
typedef HRESULT (WINAPI* PFN_D3DDevice9Ex_Present)( IDirect3DDevice9Ex*, CONST RECT*, CONST RECT*, CONST HWND, CONST RGNDATA* );


 /*
  * Hooked Direct3D9 API functions
  */
struct Hook_Direct3D9API
{
	/* Standard D3D9 */
	PFN_Direct3DCreate9			Direct3DCreate9;
	PFN_D3DDevice9_Present		D3DDevice9_Present;		// 17

	/* D3D9 Ex*/
	PFN_Direct3DCreate9Ex		Direct3DCreate9Ex;
	PFN_D3DDevice9Ex_Present	D3DDevice9Ex_Present;

	DWORD reserved;
};

void Drv_GetDirect3D9Hooks( struct Hook_Direct3D9API* hooks, struct Hook_Direct3D9API* originals );
BOOL Drv_EnableDirect3D9Hooks();
BOOL Drv_DisableDirect3D9Hooks();

#ifdef __cplusplus
}
#endif

#endif /* Hook_Direct3D9_h */
