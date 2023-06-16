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

#define STB_TRUETYPE_IMPLEMENTATION 

#include "../platform.h"
#include "DriverEnum.h"
#include "Hook_OpenGL.h"

#ifdef _WIN32
#include "MinHook2.h"
#endif

#include "Framerate.h"
#include "stb_truetype.h"

#include <sstream>
#include <atomic>


FrameRate fps;
Hook_OpenGLAPI      g_hooks, g_originals, g_trampolines;
std::atomic<bool>   unhooking_ogl(false);
bool                unhooked_ogl = false;

#define GUARD while(unhooking_ogl);


void BlitFPS()
{
    static bool first = false;
//    __asm int 3;
    if( !first )
        fps.StartFPSClock(), first = true;

    fps.UpdateFPS();

    std::stringstream ss;
    ss << fps.GetFPS();

    /* load font file */
    long size;
    unsigned char* fontBuffer;

    FILE* fontFile = fopen("C:\\Windows\\Fonts\\arial.ttf", "rb");
    if( !fontFile ) return;

    fseek(fontFile, 0, SEEK_END);
    size = ftell(fontFile); /* how long is the file ? */
    fseek(fontFile, 0, SEEK_SET); /* reset */

    fontBuffer = (unsigned char*)malloc(size);

    fread(fontBuffer, size, 1, fontFile);
    fclose(fontFile);

    /* prepare font */
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, fontBuffer, 0))
    {
        printf("failed\n");
    }

    int b_w = 512; /* bitmap width */
    int b_h = 128; /* bitmap height */
    int l_h = 64; /* line height */

    /* create a bitmap for the phrase */
    unsigned char* bitmap = (unsigned char*)calloc(b_w * b_h, sizeof(unsigned char));

    /* calculate font scaling */
    float scale = stbtt_ScaleForPixelHeight(&info, l_h);

    int x = 0;

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

    ascent *= scale;
    descent *= scale;

    int i;
    for (i = 0; i < ss.str().length(); ++i)
    {
        /* get bounding box for character (may be offset to account for chars that dip above or below the line */
        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(&info, ss.str()[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

        /* compute y (different characters have different heights */
        int y = ascent + c_y1;

        /* render character (stride and offset is important here) */
        int byteOffset = x + (y  * b_w);
        stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, b_w, scale, scale, ss.str()[i]);

        /* how wide is this character */
        int ax;
        stbtt_GetCodepointHMetrics(&info, ss.str()[i], &ax, 0);
        x += ax * scale;

        /* add kerning */
        int kern;
        kern = stbtt_GetCodepointKernAdvance(&info, ss.str()[i], ss.str()[i + 1]);
        x += kern * scale;
    }

    //Create a D3D9 texture and load the generated image. The text bitmap is 1 channel.
    /*D3DXCreateTexture(direct3DDevice, b_w, b_h, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, (LPDIRECT3DTEXTURE9*)&d3dTexture);
    D3DLOCKED_RECT rect;
    d3dTexture->LockRect(0, &rect, NULL, 0);
    memcpy(rect.pBits, bitmap, b_w * b_h);
    d3dTexture->UnlockRect(0);*/

    g_trampolines.glDrawPixels( 0, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, bitmap );

    free(fontBuffer);
    free(bitmap);
}

#ifdef _WIN32

extern "C" BOOL WINAPI _hook__SwapBuffers( HDC hDC )
{
    static BOOL first = TRUE;

    if( first ) MessageBoxA( NULL, "OpenGL hook successful!", "Yeah boi!", MB_OK ), first = FALSE;

    //BlitFPS();

    GUARD;

    return unhooked_ogl ? g_originals.SwapBuffers( hDC ) : g_trampolines.SwapBuffers( hDC );
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

    GUARD;

    return unhooked_ogl ? g_originals.glClearColor( r, g, b, a ) : g_trampolines.glClearColor( 0.5, 0.5, b, a );
}

extern "C" void WINAPI _hook__glDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels )
{
    GUARD;

    return unhooked_ogl ? g_originals.glDrawPixels( width, height, format, type, pixels ) : g_trampolines.glDrawPixels( width, height, format, type, pixels );
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
   // __asm int 3;
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
        originals->glDrawPixels = (void (WINAPI*)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid*)) GetProcAddress( hDLL, "glDrawPixels" );

        FreeLibrary( hDLL );
    }

    hooks->glClearColor = _hook__glClearColor;
    hooks->glDrawPixels = _hook__glDrawPixels;
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

    ret = pfnMH_CreateHook( (void*) g_originals.glDrawPixels, (void*) g_hooks.glDrawPixels, (void**) &g_trampolines.glDrawPixels );
    if( ret != MH_OK )
        return FALSE;

    ret = pfnMH_EnableHook( g_originals.glClearColor );
    ret = pfnMH_EnableHook( g_originals.glDrawPixels );
    ret = pfnMH_EnableHook( g_originals.SwapBuffers );
#endif

    return TRUE;
}

BOOL Drv_DisableOpenGLHooks()
{
    unhooking_ogl = true;

#ifdef _WIN32
    auto ret = pfnMH_DisableHook( g_originals.SwapBuffers );
    ret = pfnMH_DisableHook( g_originals.glClearColor );
    ret = pfnMH_DisableHook( g_originals.glDrawPixels );

    ret = pfnMH_RemoveHook( g_originals.SwapBuffers );
    ret = pfnMH_RemoveHook( g_originals.glClearColor );
    ret = pfnMH_RemoveHook( g_originals.glDrawPixels );
#endif

    unhooking_ogl = false;
    unhooked_ogl = true;

    return TRUE;
}