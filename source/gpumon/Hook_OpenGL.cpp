//
//  Hook_OpenGL.cpp
//  gpumon
//
//  Created by Aeroshogun on 4/20/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#include "../platform.h"
#include "DriverEnum.h"
#include "Hook_OpenGL.h"

#ifdef _WIN32
#include "MinHook.h"
#endif


Hook_OpenGLAPI g_hooks, g_originals, g_trampolines;


#ifdef _WIN32

extern "C" BOOL WINAPI _hook__SwapBuffers( HDC hDC )
{
    static bool first = false;
    
    return SwapBuffers( hDC );
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


void Drv_GetOpenGLHooks( Hook_OpenGLAPI* hooks, Hook_OpenGLAPI* originals )
{
    if( !hooks || !originals )
        return;
    
#ifdef __APPLE__
    hooks->aglSwapBuffers = _hook__aglSwapBuffers;
    hooks->CGLFlushDrawable = _hook__CGLFlushDrawable;
#endif

#ifdef _WIN32
    /* Get the originals so we can unhook */
    auto hDLL = LoadLibraryA( "GDI32.dll" );
    if( hDLL )
    {
        originals->SwapBuffers = (BOOL (WINAPI*)(HDC)) GetProcAddress( hDLL, "SwapBuffers" );

        FreeLibrary( hDLL );
    }

    hooks->SwapBuffers = _hook__SwapBuffers;
#endif
}

BOOL Drv_EnableOpenGLHooks()
{
#ifdef _WIN32
    Drv_GetOpenGLHooks( &g_hooks, &g_originals );

    /* Set and enable API hooks */

    auto ret = MH_CreateHook( (void*) g_originals.SwapBuffers, (void*) g_hooks.SwapBuffers, (void**) &g_trampolines.SwapBuffers );
    if( ret != MH_OK )
        return FALSE;

    ret = MH_EnableHook( g_originals.SwapBuffers );
#endif

    return TRUE;
}

BOOL Drv_DisableOpenGLHooks()
{
#ifdef _WIN32
    auto ret = MH_DisableHook( g_originals.SwapBuffers );
#endif

    return TRUE;
}