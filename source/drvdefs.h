#pragma once

#include "platform.h"


/*
 * Error types
 */
#define GERR_DRIVERERROR    -1
#define GERR_BADPARAMS      -2
#define GERR_NOTSUPPORTED   -3


typedef struct _GPUDETAILS
{
	CHAR	DeviceDesc[128];
    CHAR    DriverDesc[128];
	DWORD	DeviceID;
	DWORD	VendorID;
} GPUDETAILS;


typedef struct _GPUSTATISTICS
{
    int     gpu_usage;              /* Overall system GPU usage */
    int     video_engine_usage;     /* Overall video engine usage (macOS: NVIDIA only?) */
    int     device_unit_usage[8];   /* Device unit N utilization (macOS: Intel only?) */
    uint64_t vram_free, vram_used;  /* Total video memory usage */
    uint64_t hw_wait_time;          /* Hardware wait time (macOS only) */
    int     fan_speed_percentage;   /* GPU fan speed capacity */
    int     fan_speed_rpms;         /* GPU fan speed in RPMs */
    int     temperature;            /* Temperature, in celsius */
    int     power_usage;            /* Power usage (in watts) */
} GPUSTATISTICS;


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
    #if defined(__i386__) /* Mojave and earlier */
        #define GPUMON_DLL "libgpumon32.dylib"
    #else
        #define GPUMON_DLL "libgpumon64.dylib"
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
