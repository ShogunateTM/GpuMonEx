#pragma once

#include "..\drvdefs.h"


#ifdef __cplusplus
extern "C" {
#endif

int D3DKMT_Initialize();
void D3DKMT_Uninitialize();
int D3DKMT_GetGpuDetails( int AdapterNumber, GPUDETAILS* pGpuDetails );
int D3DKMT_GetOverallGpuLoad( int AdapterNumber, GPUSTATISTICS* pGpuStatistics );
int D3DKMT_GetProcessGpuLoad( void* pProcess );
int D3DKMT_GetGpuTemperature();

#ifdef __cplusplus
}
#endif