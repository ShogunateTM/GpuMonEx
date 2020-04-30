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


#ifdef __APPLE__

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
    
    /*pthread_setname_np( "hook_thread" );
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    int policy;
    pthread_attr_getschedpolicy(&attr, &policy);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    
    struct sched_param sched;
    sched.sched_priority = sched_get_priority_max(policy);
    pthread_attr_setschedparam(&attr, &sched);
    
    printf( "Begin...\n" );
    Sleep( 10000 );
    printf( "End!\n" );*/
    
    thread_suspend( mach_task_self() );
}

#endif


