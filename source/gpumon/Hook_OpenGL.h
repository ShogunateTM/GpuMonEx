//
//  Hook_OpenGL.h
//  gpumon
//
//  Created by Aeroshogun on 4/20/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#ifndef Hook_OpenGL_h
#define Hook_OpenGL_h

/*
 * OpenGL headers per OS
 */

#ifdef _WIN32
#include <gl/gl.h>
#endif

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <AGL/agl.h>
#endif


/*
 * Hooked OpenGL API functions
 */
struct Hook_OpenGLAPI
{
#ifdef __APPLE__
    void (*aglSwapBuffers)( AGLContext );
    CGLError (*CGLFlushDrawable)( CGLContextObj );
#endif
};

void GPUMON_API Drv_GetOpenGLHooks( Hook_OpenGLAPI* hooks );

#endif /* Hook_OpenGL_h */
