//
//  APIHook.cpp
//  gpumon
//
//  Created by Aeroshogun on 4/29/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#include "../platform.h"
#include "../common/timer_t.h"
#include "../common/ProcessEnumeration.h"
#include "../shared_data.h"
#include <unordered_map>
#include "APIHook.h"

/* Shared data from the daemon tool */
SHARED_DATA SharedData;

#if defined(__APPLE__)

#include <mach_override.h>
#include <mach_inject.h>
#include <xnumem.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CGDirectDisplay.h>

#elif defined(_WIN32)

#include "MinHook2.h"

HMODULE hMinhook = NULL;
MH_STATUS (WINAPI *pfnMH_Initialize)();
MH_STATUS (WINAPI *pfnMH_Uninitialize)();
MH_STATUS (WINAPI *pfnMH_CreateHook)(LPVOID pTarget, LPVOID pDetour, LPVOID *ppOriginal);
MH_STATUS (WINAPI *pfnMH_RemoveHook)(LPVOID pTarget);
MH_STATUS (WINAPI *pfnMH_EnableHook)(LPVOID pTarget);
MH_STATUS (WINAPI *pfnMH_DisableHook)(LPVOID pTarget);
const char* (WINAPI *pfnMH_StatusToString)(MH_STATUS status);

#endif



#ifdef __APPLE__

#if 0
Hook_OpenGLAPI hooks;

/*
 * Name: hook_opengl
 * Desc: Attempts to initialize any hooks to OpenGL APIs (core and legacy).  Returns true if we managed to hook at
 *       least necessary API.  False if OpenGL does not exist here or isn't implemented.
 */
bool hook_opengl()
{
    bool any_success = false;
    
    /* Get our required function pointers */
    Drv_GetOpenGLHooks( &hooks );
    
    kern_return_t err;
    
    /* 
     * We're going to need at least one of these below for on-screen stuff 
     */
    printf( "Attempting to hook aglSwapBuffers...\n" );
    MACH_OVERRIDE( void, aglSwapBuffers, ( AGLContext context ), err ) {
        hooks.aglSwapBuffers( context );
    } END_MACH_OVERRIDE( aglSwapBuffers );
    
    printf( "MACH_OVERRIDE(aglSwapBuffers): returned 0x%X\n", err );
    if( !err )
        any_success = true;
    
    printf( "Attempting to hook CGLFlushDrawable...\n" );
    MACH_OVERRIDE( CGLError, CGLFlushDrawable, ( CGLContextObj context ), err ) {
        return hooks.CGLFlushDrawable( context );
    } END_MACH_OVERRIDE( CGLFlushDrawable );

    printf( "MACH_OVERRIDE(CGLFlushDrawable): returned 0x%X\n", err );
    if( !err )
        any_success = true;

    return any_success;
}
#endif

/* Get the right defintion of pthread_set_self */
//#if __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_12
#define PTHREAD_SET_SELF _pthread_set_self
//#else
//#define PTHREAD_SET_SELF __pthread_set_self
//#endif

extern "C" void PTHREAD_SET_SELF( void* );

/*
 * Name: mac_hook_thread_entry
 * Desc:
 */
extern "C" __attribute__((visibility("default"))) void mac_hook_thread_entry( ptrdiff_t code_offset, void* param_block, size_t param_size, void* dummy_pthread_data );

void mac_hook_thread_entry( ptrdiff_t code_offset, void* param_block, size_t param_size, void* dummy_pthread_data )
{
    PTHREAD_SET_SELF( dummy_pthread_data );
    
    pthread_setname_np( "hook_thread" );
    
#if 0
    /* Attempt to hook the OpenGL API */
    printf( "Hooking begins now!\n" );
    if( !hook_opengl() )
        printf( "Failed to find or hook OpenGL...\n" );
#endif
    
    /* ***** DANGER *****
     * This thread should never be allowed to return because not only will it return to a pre-determined invalid address,
     * but will cause the target process to crash.  Worst case scenario, you crash every process that task_for_pid() can
     * gain access to... if you alter enough code in this project, so be very careful.
     */
    
    auto result = thread_suspend( mach_thread_self() );
    printf( "thread_suspend(mach_thread_self) returned 0x%X", result );
    printf( "Thread SHOULD have been suspended...\n" );
}

#endif

#ifdef _WIN32

bool GetSharedDataBetweenProcesses()
{
    HANDLE hMap = OpenFileMappingA( FILE_MAP_ALL_ACCESS, FALSE, "gpumonproc_shared" );
    if( !hMap )
        return false;

    void* pBuffer = MapViewOfFile( hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
    if( !pBuffer )
    {
        CloseHandle( hMap );
        return false;
    }

    CopyMemory( &SharedData, pBuffer, sizeof( SHARED_DATA ) );

    UnmapViewOfFile( pBuffer );
    CloseHandle( hMap );

    return true;
}

bool EnableMinHookAPI()
{
    /*
     * Attempt to open the Minhook DLL via LoadLibrary.  
     */

    if( !GetSharedDataBetweenProcesses() )
    {
        MessageBoxA( NULL, "Error initialzing process shared data!", "GpuMonEx", MB_ICONERROR );
        return false;
    }

    hMinhook = LoadLibraryA( SharedData.MinHookDllPath );
    if( !hMinhook )
    {
        MessageBoxA( NULL, "Unable to load Minhook!", "GpuMonEx", MB_ICONERROR );
        return false;
    }

    pfnMH_Initialize = (MH_STATUS (WINAPI*)()) GetProcAddress( hMinhook, "MH_Initialize" );
    pfnMH_Uninitialize = (MH_STATUS (WINAPI*)()) GetProcAddress( hMinhook, "MH_Uninitialize" );
    pfnMH_CreateHook = (MH_STATUS (WINAPI*)(LPVOID, LPVOID, LPVOID*)) GetProcAddress( hMinhook, "MH_CreateHook" );
    pfnMH_RemoveHook = (MH_STATUS (WINAPI*)(LPVOID)) GetProcAddress( hMinhook, "MH_RemoveHook" );
    pfnMH_EnableHook = (MH_STATUS (WINAPI*)(LPVOID)) GetProcAddress( hMinhook, "MH_EnableHook" );
    pfnMH_DisableHook = (MH_STATUS (WINAPI*)(LPVOID)) GetProcAddress( hMinhook, "MH_DisableHook" );
    pfnMH_StatusToString = (const char* (WINAPI*)(MH_STATUS)) GetProcAddress( hMinhook, "MH_StatusToString" ); 

    /* Did we get 'em all? */
    if( !pfnMH_Initialize || !pfnMH_Uninitialize || !pfnMH_CreateHook || !pfnMH_RemoveHook || !pfnMH_EnableHook || !pfnMH_DisableHook || !pfnMH_StatusToString )
    {
        MessageBoxA( NULL, "Failed to get Minhook APIs!", "GpuMonEx", MB_ICONERROR );
    }

    auto ret = pfnMH_Initialize();
    if( ret != MH_OK )
        return false;

    Drv_EnableDirect3D11Hooks();
#ifdef X86_32
    Drv_EnableDirect3D8Hooks();
#endif
    Drv_EnableDirectDrawHooks();
    Drv_EnableOpenGLHooks();
    Drv_EnableDirect3D9Hooks();

    return true;
}

void DisableMinHookAPI()
{
    //__asm int 3;
    Drv_DisableDirect3D9Hooks();
    Drv_DisableOpenGLHooks();
    Drv_DisableDirectDrawHooks();
#ifdef X86_32
    Drv_DisableDirect3D8Hooks();
#endif
    Drv_DisableDirect3D11Hooks();

    auto ret = pfnMH_Uninitialize();

    if( hMinhook != NULL )
        FreeLibrary( hMinhook );
}

#endif
