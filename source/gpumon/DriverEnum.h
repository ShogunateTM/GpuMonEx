#ifndef __DRIVERENUM_H__
#define __DRIVERENUM_H__

#include "../drvdefs.h"

/* Windows DLL exports */
#ifdef _WIN32
 #ifdef DLL_EXPORT
  #define GPUMON_API __declspec(dllexport)
 #else
  #define GPUMON_API __declspec(dllimport)
 #endif
#endif


/* MacOS dylib exports */
#ifdef __APPLE__
 #ifdef DYLIB_EXPORT
  #define GPUMON_API __attribute__((visibility("default")))
 #else
  #define GPUMON_API
 #endif
#endif


/* 
 * GpuMon dynamic library file to load (per platform)
 */
#ifdef _WIN32
    #if defined(_M_X64) || defined(__amd64__)
        #define GPUMON_DLL "gpumon64.dll"
    #else
        #define GPUMON_DLL "gpumon32.dll"
    #endif
#elif defined(__APPLE__)
    #if defined(__386__) /* Mojave and earlier */
        #define GPUMON_DLL "libgpumon.dylib"
    #else
        #define GPUMON_DLL "libgpumon.dylib"
    #endif
#endif

/*
 * LoadLibraryA, GetProcAddress and FreeLibrary for UNIX
 */
#ifndef _WIN32
#define LoadLibraryA(x)         dlopen( x, RTLD_NOW )
#define GetProcAddress(x,y)     dlsym( x, y )
#define FreeLibrary(x)          dlclose( x )
#endif


/*
 * Driver types
 */
enum
{
    Drv_Default = -1,   /* Returns the default driver for the primary display adapter */
    
    Drv_D3DKMT = 0,     /* Windows, works for all GPUs under windows (in theory), including Intel HD */
    Drv_NVAPI,          /* NVIDIA only */
    Drv_AMDGS,          /* AMD only */
    
    Drv_IOKIT,          /* MacOS, all GPUs */
    
    Drv_MAX
};


/*
 * Driver structure 
 */
typedef struct _GPUDRIVER
{
    int DriverType;

    /* Driver ineraction functions */

    int (*Initialize)();
    void (*Uninitialize)();
    int (*GetGpuDetails)( int, GPUDETAILS* );
    int (*GetOverallGpuLoad)( int, GPUSTATISTICS* );
    int (*GetProcessGpuLoad)( void* );
    int (*GetGpuTemperature)();
} GPUDRIVER;


#ifdef __cplusplus
extern "C" {
#endif

void GPUMON_API Drv_GetGpuDriver( int DriverType, GPUDRIVER* pDriver );

#ifdef __cplusplus
}
#endif


#endif
