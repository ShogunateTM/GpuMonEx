#include "DriverEnum.h"
#include "Drv_D3DKMT.h"
#include "Drv_NVAPI.h"
#include "Drv_AMDGS.h"



/*
 * Name: Drv_GetGpuDriver
 * Desc: Returns the details and driver functions to interact with a particular GPU.
 *
 * TODO: This currently accounts for the primary display adapter.  Must be able to enumerate
 *       display adapters and have the drivers match each appropriate one.
 */
void Drv_GetGpuDriver( int DriverType, GPUDRIVER* pDriver )
{
    if( !pDriver )
        return;

    pDriver->DriverType = DriverType;

    /* Is this an autodetect scenario? */
    if( DriverType == Drv_Default )
    {
        /* Let's make our best guess based on this.. */
       /* DISPLAY_DEVICEA device;
        ZeroMemory( &device, sizeof( DISPLAY_DEVICEA ) );
        device.cb = sizeof( DISPLAY_DEVICEA );

        EnumDisplayDevicesA( NULL, 0, &device, EDD_GET_DEVICE_INTERFACE_NAME ); */

        /* TODO: Properly enumerate display adapters (without DirectX) here and choose 
           the best driver to use. 
           LINK: https://docs.microsoft.com/en-us/windows/win32/wmisdk/example--getting-wmi-data-from-the-local-computer */

        pDriver->Initialize = D3DKMT_Initialize;
        pDriver->Uninitialize = D3DKMT_Uninitialize;
        pDriver->GetGpuDetails = D3DKMT_GetGpuDetails;
        pDriver->GetOverallGpuLoad = D3DKMT_GetOverallGpuLoad;
        pDriver->GetProcessGpuLoad = D3DKMT_GetProcessGpuLoad;
        pDriver->GetGpuTemperature = D3DKMT_GetGpuTemperature;
    }
    else
    {
        /* NVIDIA driver */
        if( DriverType == Drv_NVAPI )
        {
            pDriver->Initialize = NVAPI_Initialize;
            pDriver->Uninitialize = NVAPI_Uninitialize;
            pDriver->GetGpuDetails = NVAPI_GetGpuDetails;
            pDriver->GetOverallGpuLoad = NVAPI_GetOverallGpuLoad;
            pDriver->GetProcessGpuLoad = NVAPI_GetProcessGpuLoad;
            pDriver->GetGpuTemperature = NVAPI_GetGpuTemperature;
        }
        /* AMD driver */
        else if( DriverType == Drv_AMDGS )
        {
            pDriver->Initialize = AMDGS_Initialize;
            pDriver->Uninitialize = AMDGS_Uninitialize;
            pDriver->GetGpuDetails = AMDGS_GetGpuDetails;
            pDriver->GetOverallGpuLoad = AMDGS_GetOverallGpuLoad;
            pDriver->GetProcessGpuLoad = AMDGS_GetProcessGpuLoad;
            pDriver->GetGpuTemperature = AMDGS_GetGpuTemperature;
        }
        /* D3DKMT driver (Intel and everything else) */
        else 
        { 
            pDriver->Initialize = D3DKMT_Initialize;
            pDriver->Uninitialize = D3DKMT_Uninitialize;
            pDriver->GetGpuDetails = D3DKMT_GetGpuDetails;
            pDriver->GetOverallGpuLoad = D3DKMT_GetOverallGpuLoad;
            pDriver->GetProcessGpuLoad = D3DKMT_GetProcessGpuLoad;
            pDriver->GetGpuTemperature = D3DKMT_GetGpuTemperature;
        }
    }
}