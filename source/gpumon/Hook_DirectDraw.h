//
//  Hook_DirectDraw.h
//  gpumon
//
//  Created by Aeroshogun on 4/20/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#ifndef Hook_DirectDraw_h
#define Hook_DirectDraw_h

/* DirectDraw and Direct3D 7 and earlier... */

#define INITGUID
#include <Windows.h>
#include <ddraw.h>
#include <d3d.h>


/*
 * Typedefs
 */
typedef HRESULT (WINAPI* PFN_DirectDrawCreate)( GUID FAR*, LPDIRECTDRAW FAR*, IUnknown FAR* );
typedef HRESULT (WINAPI* PFN_DirectDrawCreateEx)( GUID FAR*, LPVOID*, REFIID  iid,IUnknown FAR *pUnkOuter );

typedef HRESULT (WINAPI* PFN_DDrawSurface_Flip)( IDirectDrawSurface*, IDirectDrawSurface*, DWORD );
typedef HRESULT (WINAPI* PFN_DDrawSurface_Blt)( IDirectDrawSurface*, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX );

typedef HRESULT (WINAPI* PFN_DDrawSurface2_Flip)( IDirectDrawSurface2*, IDirectDrawSurface2*, DWORD );
typedef HRESULT (WINAPI* PFN_DDrawSurface2_Blt)( IDirectDrawSurface2*, LPRECT, LPDIRECTDRAWSURFACE2, LPRECT, DWORD, LPDDBLTFX );

typedef HRESULT (WINAPI* PFN_DDrawSurface3_Flip)( IDirectDrawSurface3*, IDirectDrawSurface3*, DWORD );
typedef HRESULT (WINAPI* PFN_DDrawSurface3_Blt)( IDirectDrawSurface3*, LPRECT, LPDIRECTDRAWSURFACE3, LPRECT, DWORD, LPDDBLTFX );

typedef HRESULT (WINAPI* PFN_DDrawSurface4_Flip)( IDirectDrawSurface4*, IDirectDrawSurface4*, DWORD );
typedef HRESULT (WINAPI* PFN_DDrawSurface4_Blt)( IDirectDrawSurface4*, LPRECT, LPDIRECTDRAWSURFACE4, LPRECT, DWORD, LPDDBLTFX );

typedef HRESULT (WINAPI* PFN_DDrawSurface7_Flip)( IDirectDrawSurface7*, IDirectDrawSurface7*, DWORD );
typedef HRESULT (WINAPI* PFN_DDrawSurface7_Blt)( IDirectDrawSurface7*, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX );


/*
 * Hooked DirectDraw API functions
 */
struct Hook_DirectDrawAPI
{
	PFN_DDrawSurface_Flip DDrawSurface_Flip;
	PFN_DDrawSurface_Blt DDrawSurface_Blt;

	PFN_DDrawSurface2_Flip DDrawSurface2_Flip;
	PFN_DDrawSurface2_Blt DDrawSurface2_Blt;

	PFN_DDrawSurface3_Flip DDrawSurface3_Flip;
	PFN_DDrawSurface3_Blt DDrawSurface3_Blt;

	PFN_DDrawSurface4_Flip DDrawSurface4_Flip;
	PFN_DDrawSurface4_Blt DDrawSurface4_Blt;

	PFN_DDrawSurface7_Flip DDrawSurface7_Flip;
	PFN_DDrawSurface7_Blt DDrawSurface7_Blt;
};

void Drv_GetDirectDrawHooks( Hook_DirectDrawAPI* hooks, Hook_DirectDrawAPI* originals );
BOOL Drv_EnableDirectDrawHooks();
BOOL Drv_DisableDirectDrawHooks();

#endif /* Hook_DirectDraw_h */
