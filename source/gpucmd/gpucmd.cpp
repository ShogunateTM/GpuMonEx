// gpucmd.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "..\platform.h"
#include "..\gpumon\DriverEnum.h"

#if defined(_M_X64) || defined(__amd64__)
#define GPUMON_DLL "gpumon64.dll"
#else
#define GPUMON_DLL "gpumon32.dll"
#endif


void (*pfnDrv_GetGpuDriver)( int DriverType, GPUDRIVER* pDriver );

int main( int argc, char** argv )
{
    /* Start by loading the appropriate DLL*/
    HMODULE hGpuMonDll = LoadLibraryA( GPUMON_DLL );
    if( hGpuMonDll )
    {
        pfnDrv_GetGpuDriver = (void (*)(int, GPUDRIVER*)) GetProcAddress( hGpuMonDll, "Drv_GetGpuDriver" );
        if( pfnDrv_GetGpuDriver )
        {
            GPUDRIVER driver;

            pfnDrv_GetGpuDriver( Drv_D3DKMT, &driver );
            
            if( driver.Initialize() )
            {
                GPUDETAILS details;

                driver.GetGpuDetails( 0, &details );
                printf( "%s\n", details.DeviceDesc );
                driver.Uninitialize();
            }
        }

        FreeLibrary( hGpuMonDll );
    }
    
    return 0;
}
