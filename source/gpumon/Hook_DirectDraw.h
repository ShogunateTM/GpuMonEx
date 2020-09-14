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


struct Hook_DirectDraw
{
	HRESULT (WINAPI* IDirectDrawSurface_Flip)( IDirectDrawSurface*, IDirectDrawSurface*, DWORD );
	HRESULT (WINAPI* IDirectDrawSurface_Blt)( IDirectDrawSurface*, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX );

	HRESULT (WINAPI* IDirectDrawSurface2_Flip)( IDirectDrawSurface2*, IDirectDrawSurface2*, DWORD );
	HRESULT (WINAPI* IDirectDrawSurface2_Blt)( IDirectDrawSurface2*, LPRECT, LPDIRECTDRAWSURFACE2, LPRECT, DWORD, LPDDBLTFX );

	HRESULT (WINAPI* IDirectDrawSurface3_Flip)( IDirectDrawSurface3*, IDirectDrawSurface3*, DWORD );
	HRESULT (WINAPI* IDirectDrawSurface3_Blt)( IDirectDrawSurface3*, LPRECT, LPDIRECTDRAWSURFACE3, LPRECT, DWORD, LPDDBLTFX );

	HRESULT (WINAPI* IDirectDrawSurface4_Flip)( IDirectDrawSurface4*, IDirectDrawSurface4*, DWORD );
	HRESULT (WINAPI* IDirectDrawSurface4_Blt)( IDirectDrawSurface4*, LPRECT, LPDIRECTDRAWSURFACE4, LPRECT, DWORD, LPDDBLTFX );

	HRESULT (WINAPI* IDirectDrawSurface7_Flip)( IDirectDrawSurface7*, IDirectDrawSurface7*, DWORD );
	HRESULT (WINAPI* IDirectDrawSurface7_Blt)( IDirectDrawSurface7*, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX );
};

#endif /* Hook_DirectDraw_h */
