//
//  Hook_Direct3D9.cpp
//  gpumon
//
//  Created by Aeroshogun on 4/20/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#include "../platform.h"
#include "DriverEnum.h"
#include "Hook_Direct3D9.h"
#include "TmpWnd.h"
#include "MinHook2.h"


Hook_Direct3D9API g_hooks, g_originals, g_trampolines;

/*
 * D3D VTables
 */
uint_addr_t Direct3D_vtable;
uint_addr_t D3DDevice_vtable[119];

uint_addr_t Direct3DEx_vtable;
uint_addr_t D3DDeviceEx_vtable[124];



extern "C" 
{
    extern void GetFunctionsViaVtable_D3D9( IDirect3DDevice9* pD3DDevice, struct Hook_Direct3D9API* originals );
}

extern "C" HRESULT WINAPI _hook__IDirect3DDevice9_Present( IDirect3DDevice9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion )
{
    static BOOL first = TRUE;

    if( first ) MessageBoxA( NULL, "D3D9 hooked successfully!", "Yeah boi", MB_OK ), first = FALSE;

	return g_trampolines.D3DDevice9_Present( pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion );
}

extern "C" HRESULT WINAPI _hook__IDirect3DDevice9Ex_Present( IDirect3DDevice9Ex* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion )
{
	return g_trampolines.D3DDevice9Ex_Present( pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion );
}


/*
 * Sources:
 *  https://stackoverflow.com/questions/1994676/hooking-directx-endscene-from-an-injected-dll
 *  https://guidedhacking.com/threads/get-direct3d9-and-direct3d11-devices-dummy-device-method.11867/
 */

LRESULT CALLBACK TempWndProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
    return DefWindowProcA( hWnd, uMessage, wParam, lParam );
}

HWND window = NULL;
BOOL CALLBACK EnumWindowsCallback( HWND handle, LPARAM lParam )
{
    DWORD wndProcId;
    GetWindowThreadProcessId( handle, &wndProcId );

    if( GetCurrentProcessId() != wndProcId )
        return TRUE; // skip to next window

    window = handle;
    return FALSE; // window found abort search
}

HWND GetProcessWindow()
{
    window = NULL;
    EnumWindows( EnumWindowsCallback, NULL );
    return window;
}

void Drv_GetDirect3D9Hooks( Hook_Direct3D9API* hooks, Hook_Direct3D9API* originals )
{
    /*
     * Standard Direct3D9
     */
    {
       static PFN_Direct3DCreate9 pfnDirect3DCreate9 = NULL;
        //__asm int 3;

        /* TODO: Is it better to use LoadLibraryA instead or what? Probably doesn't matter */
        HMODULE hDLL = GetModuleHandleA( "d3d9" );
        if( !hDLL )
        {
            return;
        }

        pfnDirect3DCreate9 = (PFN_Direct3DCreate9) GetProcAddress( hDLL, "Direct3DCreate9" );
        if( !pfnDirect3DCreate9 )
        {
            return;
        } 

        LPDIRECT3D9 pD3D = pfnDirect3DCreate9( D3D_SDK_VERSION );

        D3DDISPLAYMODE d3ddm;
        HRESULT hRes = pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );
        if( FAILED( hRes ) )
        {
            return;
        } 

        D3DPRESENT_PARAMETERS d3dpp; 
        ZeroMemory( &d3dpp, sizeof(d3dpp));
        d3dpp.BackBufferWidth = 0;
        d3dpp.BackBufferHeight = 0;
        d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
        d3dpp.BackBufferCount = 0;
        d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
        d3dpp.MultiSampleQuality = NULL;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.hDeviceWindow = GetForegroundWindow();
        d3dpp.Windowed = 1;
        d3dpp.EnableAutoDepthStencil = 0;
        d3dpp.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
        d3dpp.Flags = NULL;
        d3dpp.FullScreen_RefreshRateInHz = 0;
        d3dpp.PresentationInterval = 0;

        IDirect3DDevice9* pD3DDevice = NULL;
        hRes = pD3D->CreateDevice( 
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_NULLREF,
            d3dpp.hDeviceWindow,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT | D3DCREATE_MULTITHREADED,
            &d3dpp, &pD3DDevice );

        if( FAILED( hRes ) )
        {
            d3dpp.Windowed = FALSE;
            hRes = pD3D->CreateDevice( 
                D3DADAPTER_DEFAULT,
                D3DDEVTYPE_NULLREF,
                d3dpp.hDeviceWindow,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT | D3DCREATE_MULTITHREADED,
                &d3dpp, &pD3DDevice );

            if( FAILED( hRes ) )
            {
                pD3D->Release();
                //DestroyWindow( hWnd );

                char str[128];
                sprintf( str, "CreateDevice failures suck... (0x%X)", hRes );
               // MessageBoxA( NULL, str, "Dang it!", MB_OK );
                return;
            }
        }
        
        /*ULONG* lpVtbl = (unsigned long*)*((unsigned long*)pD3DDevice);
        originals->D3DDevice9_Present = (PFN_D3DDevice9_Present) (DWORD) lpVtbl[17];*/

        /*void* lpVtbl[119];
        memcpy( lpVtbl, *reinterpret_cast<void***>(pD3DDevice), sizeof( lpVtbl ) );

        originals->D3DDevice9_Present = (PFN_D3DDevice9_Present) lpVtbl[17];*/

       // GetFunctionsViaVtable_D3D9( pD3DDevice, &g_originals );

        //D3DDevice_vtable = (uint_addr_t*)::calloc(119, sizeof(uint_addr_t));
        
        ::memcpy( D3DDevice_vtable, *(uint_addr_t**) pD3DDevice, 119 * sizeof( uint_addr_t ) );

        char str[64];
        sprintf( str, "D3DDevice9::Present():= 0x%X\n", D3DDevice_vtable[17] );
        OutputDebugStringA( str );
        MessageBoxA( NULL, str, "K", MB_OK );
        
        originals->D3DDevice9_Present = (PFN_D3DDevice9_Present) ((void*) D3DDevice_vtable[17]);
        hooks->D3DDevice9_Present = _hook__IDirect3DDevice9_Present;

        pD3DDevice->Release();
        pD3D->Release();
        //DestroyWindow( hWnd );
    }

    /* TODO: Direct3D9 Ex */


    /*
     * Set hooks 
     */
    hooks->D3DDevice9_Present = _hook__IDirect3DDevice9_Present;
}

BOOL Drv_EnableDirect3D9Hooks()
{
    Drv_GetDirect3D9Hooks( &g_hooks, &g_originals );

    /* Set and enable API hooks */

    auto ret = pfnMH_CreateHook( (void*) g_originals.D3DDevice9_Present, (void*) g_hooks.D3DDevice9_Present, (void**) &g_trampolines.D3DDevice9_Present );
    if( ret != MH_OK )
    {
       // MessageBoxA( NULL, "Hooking D3D9 failed!", "DANG IT!", MB_OK );
        return FALSE;
    }

    ret = pfnMH_EnableHook( g_originals.D3DDevice9_Present );
    if( ret != MH_OK )
    {
        MessageBoxA( NULL, "Couldn't enable hook!", "DANG IT!", MB_OK );
        return FALSE;
    }

    return TRUE;
}

BOOL Drv_DisableDirect3D9Hooks()
{
    auto ret = pfnMH_DisableHook( g_originals.D3DDevice9_Present ); 

    ret = pfnMH_RemoveHook( g_originals.D3DDevice9_Present );

    return TRUE;
}