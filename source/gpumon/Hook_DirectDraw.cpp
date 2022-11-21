//
//  Hook_DirectDraw.cpp
//  gpumon
//
//  Created by Aeroshogun on 4/20/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#undef DIRECT3D_VERSION
#include "../platform.h"
#include "DriverEnum.h"
#include "Hook_DirectDraw.h"
#include "TmpWnd.h"
#include "MinHook2.h"

#ifdef X86_32
#include <d3dx.h>
#endif


Hook_DirectDrawAPI g_hooks, g_originals, g_trampolines;

/*
 * DirectDraw/Direct3D VTables
 */
uint_addr_t DDrawSurface_vtable[36];




extern "C" HRESULT WINAPI _hook__IDirectDrawSurface_Flip( IDirectDrawSurface* pThis,  LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags )
{
    static BOOL first = TRUE;

    if( first ) MessageBoxA( NULL, "DDraw hooked successfully!", "Yeah boi", MB_OK ), first = FALSE;

    return g_trampolines.DDrawSurface_Flip( pThis, lpDDSurfaceTargetOverride, dwFlags );
}

extern "C" HRESULT WINAPI _hook__IDirectDrawSurface_Blt( IDirectDrawSurface* pThis, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx )
{
    return g_trampolines.DDrawSurface_Blt( pThis, lpDestRect, lpDDSrcSurface, lpSrcRect, dwFlags, lpDDBltFx );
}


extern HWND GetProcessWindow();

void Drv_GetDirectDrawHooks( Hook_DirectDrawAPI* hooks, Hook_DirectDrawAPI* originals )
{
    /*
     * DirectDraw (all versions)
     */
    static PFN_DirectDrawCreate pfnDirectDrawCreate = NULL;
    //__asm int 3;

    /* TODO: Is it better to use LoadLibraryA instead or what? Probably doesn't matter */
    HMODULE hDLL = GetModuleHandleA( "ddraw" );
    if( !hDLL )
    {
        return;
    }

    pfnDirectDrawCreate = (PFN_DirectDrawCreate) GetProcAddress( hDLL, "DirectDrawCreate" );
    if( !pfnDirectDrawCreate )
    {
        return;
    }

    IDirectDraw* lpDD = NULL;   IDirectDrawSurface* lpDDSurface = NULL, *lpDDSPrimary = NULL;
    IDirectDraw2* lpDD2 = NULL; IDirectDrawSurface2* lpDDSurface2 = NULL;
    IDirectDraw4* lpDD4 = NULL; IDirectDrawSurface4* lpDDSurface4 = NULL;
    IDirectDraw7* lpDD7 = NULL; IDirectDrawSurface7* lpDDSurface7 = NULL;

    HRESULT hr = pfnDirectDrawCreate( NULL, &lpDD, NULL );

    hr = lpDD->SetCooperativeLevel( GetForegroundWindow(), DDSCL_NORMAL | DDSCL_NOWINDOWCHANGES | DDSCL_MULTITHREADED );
    if( FAILED( hr ) )
    {
        lpDD->Release();
        MessageBoxA( NULL, "Error Setting cooperative level!", "CRAP", MB_ICONEXCLAMATION );
        return;
    }

    DDSURFACEDESC ddsd;
    ::ZeroMemory( &ddsd, sizeof( ddsd ) );
    // Create the primary surface with 1 back buffer
    ddsd.dwSize = sizeof( ddsd );
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    ddsd.dwBackBufferCount = 1;
    hr = lpDD->CreateSurface( &ddsd, &lpDDSPrimary, NULL );
    if( FAILED( hr ) )
    {
        lpDD->Release();
        MessageBoxA( NULL, "Error creating surface!!", "CRAP", MB_ICONEXCLAMATION );
        return;
    }

#if 0
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN /*| DDSCAPS_SYSTEMMEMORY*/;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;

    hr = lpDD->CreateSurface( &ddsd, &lpDDSurface, NULL );
    if( FAILED( hr ) )
    {
        lpDDSPrimary->Release();
        lpDD->Release();
        MessageBoxA( NULL, "Error creating dummy surface!", "CRAP", MB_ICONEXCLAMATION );
        return;
    }
#endif

    /*ULONG* lpVtbl = (unsigned long*)*((unsigned long*)pD3DDevice);
    originals->D3DDevice9_Present = (PFN_D3DDevice9_Present) (DWORD) lpVtbl[17];*/

    /*void* lpVtbl[119];
    memcpy( lpVtbl, *reinterpret_cast<void***>(pD3DDevice), sizeof( lpVtbl ) );

    originals->D3DDevice9_Present = (PFN_D3DDevice9_Present) lpVtbl[17];*/

    // GetFunctionsViaVtable_D3D9( pD3DDevice, &g_originals );

    //D3DDevice_vtable = (uint_addr_t*)::calloc(119, sizeof(uint_addr_t));
   
    ::memcpy( DDrawSurface_vtable, *(uint_addr_t**) lpDDSPrimary, 36 * sizeof( uint_addr_t ) );

    /*char str[64];
    sprintf( str, "DDrawSurface::Blt():= 0x%X\n", DDrawSurface_vtable[5] );
    OutputDebugStringA( str );
    MessageBoxA( NULL, str, "K", MB_OK );*/

    originals->DDrawSurface_Blt = (PFN_DDrawSurface_Blt) ((void*) DDrawSurface_vtable[5]);
    hooks->DDrawSurface_Blt = _hook__IDirectDrawSurface_Blt;

    //lpDDSurface->Release();
    lpDDSPrimary->Release();
    lpDD->Release();
}

BOOL Drv_EnableDirectDrawHooks()
{
    Drv_GetDirectDrawHooks( &g_hooks, &g_originals );

    /* Set and enable API hooks */

    auto ret = pfnMH_CreateHook( (void*) g_originals.DDrawSurface_Blt, (void*) g_hooks.DDrawSurface_Blt, (void**) &g_trampolines.DDrawSurface_Blt );
    if( ret != MH_OK )
    {
        MessageBoxA( NULL, "Hooking DirectDraw failed!", "DANG IT!", MB_OK );
        return FALSE;
    }

    ret = pfnMH_EnableHook( g_originals.DDrawSurface_Blt );
    if( ret != MH_OK )
    {
        MessageBoxA( NULL, "Couldn't enable hook!", "DANG IT!", MB_OK );
        return FALSE;
    }

    return TRUE;
}

BOOL Drv_DisableDirectDrawHooks()
{
    auto ret = pfnMH_DisableHook( g_originals.DDrawSurface_Blt );

    ret = pfnMH_RemoveHook( g_originals.DDrawSurface_Blt );

    return TRUE;
}