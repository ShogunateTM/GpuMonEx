//
//  Drv_IOKit.h
//  gpumon
//
//  Created by Aeroshogun on 3/17/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//


#pragma once

#include "../drvdefs.h"



#ifdef __cplusplus
extern "C" {
#endif
    
    int IOKIT_Initialize();
    void IOKIT_Uninitialize();
    int IOKIT_GetGpuDetails( int AdapterNumber, GPUDETAILS* pGpuDetails );
    int IOKIT_GetOverallGpuLoad( int AdapterNumber, GPUSTATISTICS* pGpuStatistics );
    int IOKIT_GetProcessGpuLoad( void* pProcess );
    int IOKIT_GetGpuTemperature();
    
#ifdef __cplusplus
}
#endif
