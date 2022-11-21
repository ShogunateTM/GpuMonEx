//
//  Hook_Direct3D8.cpp
//  gpumon
//
//  Created by Aeroshogun on 4/20/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//


#define INCLUDE_D3D8

#include "../platform.h"
#include "DriverEnum.h"
#include "Hook_Direct3D8.h"
#include "TmpWnd.h"
#include "MinHook2.h"

#ifdef X86_32   // x64 not supported

Hook_Direct3D8API g_hooks, g_originals, g_trampolines;

/*
* D3D VTables
*/
uint_addr_t Direct3D8_vtable;
uint_addr_t D3DDevice8_vtable[97];


extern "C" 
{
    extern void GetFunctionsViaVtable_D3D8( IDirect3DDevice8* pD3DDevice, struct Hook_Direct3D8API* originals );
}

extern "C" HRESULT WINAPI _hook__IDirect3DDevice8_Present( IDirect3DDevice8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion )
{
    static BOOL first = TRUE;

    if( first ) MessageBoxA( NULL, "D3D8 hooked successfully!", "Yeah boi", MB_OK ), first = FALSE;

    return g_trampolines.D3DDevice8_Present( pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion );
}




/*
* Sources:
*  https://stackoverflow.com/questions/1884676/hooking-directx-endscene-from-an-injected-dll
*  https://guidedhacking.com/threads/get-direct3d8-and-direct3d11-devices-dummy-device-method.11867/
*/


void Drv_GetDirect3D8Hooks( Hook_Direct3D8API* hooks, Hook_Direct3D8API* originals )
{
    /*
    * Standard Direct3D8
    */
    {
        CTmpWnd TmpWnd;
        static PFN_Direct3DCreate8 pfnDirect3DCreate8 = NULL;
        //__asm int 3;

        /* TODO: Is it better to use LoadLibraryA instead or what? Probably doesn't matter */
        HMODULE hDLL = GetModuleHandleA( "d3d8" );
        if( !hDLL )
        {
            return;
        }

        pfnDirect3DCreate8 = (PFN_Direct3DCreate8) GetProcAddress( hDLL, "Direct3DCreate8" );
        if( !pfnDirect3DCreate8 )
        {
            return;
        } 

        LPDIRECT3D8 pD3D = pfnDirect3DCreate8( D3D_SDK_VERSION );

        D3DDISPLAYMODE d3ddm;
        HRESULT hRes = pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );
        if( FAILED( hRes ) )
        {
            return;
        } 

        D3DPRESENT_PARAMETERS d3dpp; 
        ZeroMemory( &d3dpp, sizeof(d3dpp));
        d3dpp.Windowed = TRUE;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.BackBufferFormat = d3ddm.Format;
        d3dpp.hDeviceWindow = GetForegroundWindow();

        IDirect3DDevice8* pD3DDevice = NULL;
        hRes = pD3D->CreateDevice( 
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            d3dpp.hDeviceWindow,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING |/* D3DCREATE_DISABLE_DRIVER_MANAGEMENT |*/ D3DCREATE_MULTITHREADED,
            &d3dpp, &pD3DDevice );

        if( FAILED( hRes ) )
        {
            d3dpp.Windowed = FALSE;
            
            hRes = pD3D->CreateDevice( 
                D3DADAPTER_DEFAULT,
                D3DDEVTYPE_HAL,
                d3dpp.hDeviceWindow,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING | /*D3DCREATE_DISABLE_DRIVER_MANAGEMENT |*/ D3DCREATE_MULTITHREADED,
                &d3dpp, &pD3DDevice );

            if( FAILED( hRes ) )
            {
                pD3D->Release();
                //DestroyWindow( hWnd );

                char str[128];
                sprintf( str, "CreateDevice failures suck... (0x%X)", hRes );
                MessageBoxA( NULL, str, "Dang it!", MB_OK );
                return;
            }
        }

        /*ULONG* lpVtbl = (unsigned long*)*((unsigned long*)pD3DDevice);
        originals->D3DDevice8_Present = (PFN_D3DDevice8_Present) (DWORD) lpVtbl[17];*/

        /*void* lpVtbl[118];
        memcpy( lpVtbl, *reinterpret_cast<void***>(pD3DDevice), sizeof( lpVtbl ) );

        originals->D3DDevice8_Present = (PFN_D3DDevice8_Present) lpVtbl[17];*/

        // GetFunctionsViaVtable_D3D8( pD3DDevice, &g_originals );

        //D3DDevice_vtable = (uint_addr_t*)::calloc(118, sizeof(uint_addr_t));

        ::memcpy( D3DDevice8_vtable, *(uint_addr_t**) pD3DDevice, 97 * sizeof( uint_addr_t ) );

        char str[64];
        sprintf( str, "D3DDevice8::Present():= 0x%X\n", D3DDevice8_vtable[15] );
        OutputDebugStringA( str );
        MessageBoxA( NULL, str, "K", MB_OK );

        originals->D3DDevice8_Present = (PFN_D3DDevice8_Present) ((void*) D3DDevice8_vtable[15]);
        hooks->D3DDevice8_Present = _hook__IDirect3DDevice8_Present;

        pD3DDevice->Release();
        pD3D->Release();
        //DestroyWindow( hWnd );
    }

    /*
     * Set hooks 
     */
    hooks->D3DDevice8_Present = _hook__IDirect3DDevice8_Present;
}

BOOL Drv_EnableDirect3D8Hooks()
{
    Drv_GetDirect3D8Hooks( &g_hooks, &g_originals );

    /* Set and enable API hooks */

    auto ret = pfnMH_CreateHook( (void*) g_originals.D3DDevice8_Present, (void*) g_hooks.D3DDevice8_Present, (void**) &g_trampolines.D3DDevice8_Present );
    if( ret != MH_OK )
    {
        //MessageBoxA( NULL, "Hooking D3D8 failed!", "DANG IT!", MB_OK );
        return FALSE;
    }

    ret = pfnMH_EnableHook( g_originals.D3DDevice8_Present );
    if( ret != MH_OK )
    {
        MessageBoxA( NULL, "Couldn't enable hook!", "DANG IT!", MB_OK );
        return FALSE;
    }

    return TRUE;
}

BOOL Drv_DisableDirect3D8Hooks()
{
    auto ret = pfnMH_DisableHook( g_originals.D3DDevice8_Present );

    ret = pfnMH_RemoveHook( g_originals.D3DDevice8_Present );

    return TRUE;
}

#endif