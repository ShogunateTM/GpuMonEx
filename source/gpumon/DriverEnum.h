#ifndef __DRIVERENUM_H__
#define __DRIVERENUM_H__

#include "..\drvdefs.h"


#ifdef DLL_EXPORT
#define GPUMON_API __declspec(dllexport)
#else
#define GPUMON_API __declspec(dllimport)
#endif

/*
 * Driver types
 */
enum
{
    Drv_Default = -1,   /* Returns the default driver for the primary display adapter */
    Drv_D3DKMT = 0,     /* Works for all GPUs under windows (in theory), including Intel HD */
    Drv_NVAPI,          /* NVIDIA only */
    Drv_AMDGS           /* AMD only */
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