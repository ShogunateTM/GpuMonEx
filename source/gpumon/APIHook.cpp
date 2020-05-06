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
#include <unordered_map>
#include "APIHook.h"

#if defined(__APPLE__)

#include <mach_override.h>
#include <mach_inject.h>
#include <xnumem.h>

#endif

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CGDirectDisplay.h>


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


