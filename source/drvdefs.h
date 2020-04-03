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
