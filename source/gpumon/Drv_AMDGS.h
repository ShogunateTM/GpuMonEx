#pragma once

#include "..\drvdefs.h"



#ifdef __cplusplus
extern "C" {
#endif

int AMDGS_Initialize();
void AMDGS_Uninitialize();
int AMDGS_GetGpuDetails( int AdapterNumber, GPUDETAILS* pGpuDetails );
int AMDGS_GetOverallGpuLoad( int AdapterNumber, GPUSTATISTICS* pGpuStatistics );
int AMDGS_GetProcessGpuLoad( void* pProcess );
int AMDGS_GetGpuTemperature();

#ifdef __cplusplus
}
#endif