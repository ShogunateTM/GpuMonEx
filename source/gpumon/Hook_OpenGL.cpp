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

extern "C" BOOL _hook__SwapBuffers( HDC hDC )
{
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


void Drv_GetOpenGLHooks( Hook_OpenGLAPI* hooks )
{
    if( !hooks )
        return;
    
#ifdef __APPLE__
    hooks->aglSwapBuffers = _hook__aglSwapBuffers;
    hooks->CGLFlushDrawable = _hook__CGLFlushDrawable;
#endif
}
