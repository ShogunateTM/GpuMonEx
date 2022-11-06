//
//  Hook_OpenGL.cpp
//  gpumon
//
//  Created by Aeroshogun on 4/20/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

/*
 * References:
 * https://gist.github.com/rayferric/3b4eddf8d920748f5c6e09fc24c3e305
 */

#include "../platform.h"
#include "DriverEnum.h"
#include "Hook_OpenGL.h"

#ifdef _WIN32
#include "MinHook2.h"
#endif


Hook_OpenGLAPI g_hooks, g_originals, g_trampolines;


#ifdef _WIN32

extern "C" BOOL WINAPI _hook__SwapBuffers( HDC hDC )
{
    static BOOL first = TRUE;

    if( first ) MessageBoxA( NULL, "OpenGL hook successful!", "Yeah boi!", MB_OK ), first = FALSE;

    return g_trampolines.SwapBuffers( hDC );
}

#endif

#ifdef __APPLE__

extern "C" void _hook__aglSwapBuffers( AGLContext ctxt )
{
    aglSwapBuffers( ctxt );
}

extern "C" CGLError _hook__CGLFlushDrawable( CGLContextObj ctxt )
{
    return CGLFlushDrawable( ctxt );
}

#endif

extern "C" void WINAPI _hook__glClearColor( GLclampf r, GLclampf g, GLclampf b, GLclampf a )
{
    static BOOL first = TRUE;

    if( first ) MessageBoxA( NULL, "Test, hooking glClearColor", "Yeah boi!", MB_OK ), first = FALSE;

    g_trampolines.glClearColor( 0.5, 0.5, b, a );
}

void Drv_GetOpenGLHooks( Hook_OpenGLAPI* hooks, Hook_OpenGLAPI* originals )
{
    if( !hooks || !originals )
        return;
    
#ifdef __APPLE__
    hooks->aglSwapBuffers = _hook__aglSwapBuffers;
    hooks->CGLFlushDrawable = _hook__CGLFlushDrawable;
#endif

#ifdef _WIN32
    __asm int 3;
    /* Get the originals so we can unhook */
    auto hDLL = LoadLibraryA( "GDI32.dll" );
    if( hDLL )
    {
        originals->SwapBuffers = (BOOL (WINAPI*)(HDC)) GetProcAddress( hDLL, "SwapBuffers" );

        FreeLibrary( hDLL );
    }

    hDLL = LoadLibraryA( "OpenGL32.dll" );
    if( hDLL )
    {
        originals->glClearColor = (void (WINAPI*)(GLclampf, GLclampf, GLclampf, GLclampf)) GetProcAddress( hDLL, "glClearColor" );

        FreeLibrary( hDLL );
    }

    hooks->glClearColor = _hook__glClearColor;
    hooks->SwapBuffers = _hook__SwapBuffers;
#endif
}

BOOL Drv_EnableOpenGLHooks()
{
#ifdef _WIN32
    Drv_GetOpenGLHooks( &g_hooks, &g_originals );

    /* Set and enable API hooks */

    auto ret = pfnMH_CreateHook( (void*) g_originals.SwapBuffers, (void*) g_hooks.SwapBuffers, (void**) &g_trampolines.SwapBuffers );
    if( ret != MH_OK )
        return FALSE;

    ret = pfnMH_CreateHook( (void*) g_originals.glClearColor, (void*) g_hooks.glClearColor, (void**) &g_trampolines.glClearColor );
    if( ret != MH_OK )
        return FALSE;

    ret = pfnMH_EnableHook( g_originals.glClearColor );
    ret = pfnMH_EnableHook( g_originals.SwapBuffers );
#endif

    return TRUE;
}

BOOL Drv_DisableOpenGLHooks()
{
#ifdef _WIN32
    auto ret = pfnMH_DisableHook( g_originals.SwapBuffers );
    ret = pfnMH_DisableHook( g_originals.glClearColor );
#endif

    return TRUE;
}