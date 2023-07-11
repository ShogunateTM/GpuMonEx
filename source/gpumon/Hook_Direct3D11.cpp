//
//  Hook_Direct3D11.cpp
//  gpumon
//
//  Created by Aeroshogun on 4/20/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#include "../platform.h"
#include "DriverEnum.h"
#include "Hook_Direct3D11.h"
#include "MinHook2.h"



typedef HRESULT (WINAPI* PFN_IDXGISwapChain_Present)( IDXGISwapChain*, UINT SyncInterval, UINT Flags );
typedef HRESULT (WINAPI* PFN_IDXGISwapChain1_Present1)( IDXGISwapChain1*, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS *pPresentParameters );


struct Hook_Direct3D11API
{
    PFN_IDXGISwapChain_Present DXGISwapChain_Present;       // vtbl[8]

    PFN_IDXGISwapChain1_Present1 DXGISwapChain1_Present1;   // vtbl[22]
};

Hook_Direct3D11API g_hooks, g_originals, g_trampolines;

/*
 * D3D VTables
 */
uint_addr_t DXGISwapChain_vtable[18];



extern "C" HRESULT WINAPI _hook__IDXGISwapChain_Present( IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags )
{
    static BOOL first = TRUE;

    if( first ) MessageBoxA( NULL, "D3D11 hooked successfully!", "Yeah boi", MB_OK ), first = FALSE;

    return g_trampolines.DXGISwapChain_Present( pThis, SyncInterval, Flags );
}

extern "C" HRESULT WINAPI _hook__IDXGISwapChain1_Present1( IDXGISwapChain1* pThis, UINT SyncInterval, UINT Flags, const DXGI_PRESENT_PARAMETERS *pPresentParameters )
{
    static BOOL first = TRUE;

    if( first ) MessageBoxA( NULL, "D3D11 hooked successfully!", "Yeah boi", MB_OK ), first = FALSE;

    return g_trampolines.DXGISwapChain1_Present1( pThis, SyncInterval, Flags, pPresentParameters );
}


/*
 * Sources:
 * https://github.com/guided-hacking/GH_D3D11_Hook/tree/master/GH_D3D11_Hook
 */



void Drv_GetDirect3D11Hooks( Hook_Direct3D11API* hooks, Hook_Direct3D11API* originals )
{
    static PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN pfnD3D11CreateDeviceAndSwapChain = NULL;
    //__asm int 3;

    /* TODO: Is it better to use LoadLibraryA instead or what? Probably doesn't matter */
    HMODULE hDLL = GetModuleHandleA( "d3d11" );
    if( !hDLL )
    {
        return;
    }

    pfnD3D11CreateDeviceAndSwapChain = (PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN) GetProcAddress( hDLL, "D3D11CreateDeviceAndSwapChain" );
    if( !pfnD3D11CreateDeviceAndSwapChain )
    {
        return;
    }

   // __asm int 3;

    ID3D11Device* pDevice = nullptr;
    IDXGISwapChain* pSwapchain = nullptr;
    ID3D11DeviceContext* pContext = nullptr;

    D3D_FEATURE_LEVEL featLevel;
    DXGI_SWAP_CHAIN_DESC sd{ 0 };
    sd.BufferCount = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.Height = 800;
    sd.BufferDesc.Width = 600;
    sd.BufferDesc.RefreshRate = { 60, 1 };
    sd.OutputWindow = GetForegroundWindow();
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    HRESULT hr = pfnD3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_REFERENCE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &pSwapchain, &pDevice, &featLevel, nullptr );
        
    if( FAILED( hr ) )
    {
        char str[128];
        sprintf( str, "CreateDevice failures suck... (0x%X)", hr );
        MessageBoxA( NULL, str, "Dang it!", MB_OK );
        return;
    }
   

    /*ULONG* lpVtbl = (unsigned long*)*((unsigned long*)pD3DDevice);
    originals->D3DDevice9_Present = (PFN_D3DDevice9_Present) (DWORD) lpVtbl[17];*/

    /*void* lpVtbl[119];
    memcpy( lpVtbl, *reinterpret_cast<void***>(pD3DDevice), sizeof( lpVtbl ) );

    originals->D3DDevice9_Present = (PFN_D3DDevice9_Present) lpVtbl[17];*/

    // GetFunctionsViaVtable_D3D9( pD3DDevice, &g_originals );

    //D3DDevice_vtable = (uint_addr_t*)::calloc(119, sizeof(uint_addr_t));

    /*::memcpy( DXGISwapChain_vtable, *(uint_addr_t**) pSwapchain, 18 * sizeof( uint_addr_t ) );

    char str[64];
    sprintf( str, "IDXGISwapChain::Present():= 0x%X\n", DXGISwapChain_vtable[8] );
    OutputDebugStringA( str );
    MessageBoxA( NULL, str, "K", MB_OK );*/

    void** pVMT = *(void***)pSwapchain;
    char str[64];
    sprintf( str, "IDXGISwapChain::Present():= 0x%X\n", pVMT[8] );
    OutputDebugStringA( str );
    MessageBoxA( NULL, str, "K", MB_ICONEXCLAMATION );

    originals->DXGISwapChain_Present = (PFN_IDXGISwapChain_Present) (pVMT[8]);


    /*
     * Set hooks 
     */
    hooks->DXGISwapChain_Present = _hook__IDXGISwapChain_Present;
}

BOOL Drv_EnableDirect3D11Hooks()
{
    Drv_GetDirect3D11Hooks( &g_hooks, &g_originals );

    /* Set and enable API hooks */

    auto ret = pfnMH_CreateHook( (void*) g_originals.DXGISwapChain_Present, (void*) g_hooks.DXGISwapChain_Present, (void**) &g_trampolines.DXGISwapChain_Present );
    if( ret != MH_OK )
    {
        //MessageBoxA( NULL, "Hooking failed!", "DANG IT!", MB_OK );
        return FALSE;
    }

    ret = pfnMH_EnableHook( g_originals.DXGISwapChain_Present );
    if( ret != MH_OK )
    {
        MessageBoxA( NULL, "Couldn't enable hook!", "DANG IT!", MB_OK );
        return FALSE;
    }

    return TRUE;
}

BOOL Drv_DisableDirect3D11Hooks()
{
    auto ret = MH_UNKNOWN;
    
    if( pfnMH_DisableHook ) ret = pfnMH_DisableHook( g_originals.DXGISwapChain_Present );

    if( pfnMH_RemoveHook ) ret = pfnMH_RemoveHook( g_originals.DXGISwapChain_Present );

    return TRUE;
}