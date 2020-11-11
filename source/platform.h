#ifndef __PLATFORM_H__
#define __PLATFORM_H__

/* 
 * CPU arch defines (only Intel x86/x64 are supported) 
 */
#ifdef _WIN32
    #if defined(_M_X64) || defined(__amd64__)
        #define X86_64
    #else
        #define X86_32
    #endif
#elif defined(__APPLE__)
    #if defined(__i386__) /* Mojave and earlier */
        #define X86_32
    #else
        #define X86_64
    #endif
#endif


/*
 * Platform specific includes and definitions
 */


/*
 * Standard C/C++ headers
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

/* C++ only */
#ifdef __cplusplus
#include <iostream>
#include <string>
#include <memory>
#include <locale>
#include <codecvt>
#endif


/*
 * Windows Desktop
 */
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define INITGUID                        // Initialize GUID related stuff (equivalent of including dxguid.lib)
#define _WIN32_DCOM

#include <windows.h>
#include <mmsystem.h>
#include <process.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <shellapi.h>

#include <conio.h>
#include <io.h>
#include <stdint.h>

#include <comip.h>
#include <comdef.h>
#include <WbemIdl.h>
//#include <atlbase.h>

#endif


/*
 * MacOS
 */
#ifdef __APPLE__

#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>

#include <errno.h>
#include <stdbool.h>
#include <sys/sysctl.h>

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>

typedef struct kinfo_proc kinfo_proc;

#define DWORD   unsigned long
#define WORD    unsigned short
#define BYTE    unsigned char
#define CHAR    char

#define HANDLE  void*
#define HMODULE void*

#define Sleep(x) usleep((x)*1000)

#endif


#endif
