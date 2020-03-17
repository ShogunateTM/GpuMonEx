#ifndef __DRIVERENUM_H__
#define __DRIVERENUM_H__

#include "..\drvdefs.h"

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
  #define GPUMON_API __attribute__((visibility("default"))
 #else
  #define GPUMON_API
 #endif
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
    int (*GetOverallGpuLoad)();
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
