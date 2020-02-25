// GPUMonEx.Driver.NVAPI.cpp : Defines the exported functions for the DLL application.
//

#include "../platform.h"
#include "Drv_NVAPI.h"


//
// Getting Nvidia GPU Usage
//
// Reference: Open Hardware Monitor (http://code.google.com/p/open-hardware-monitor)
//

//#include "nvidia.h"
#include "../debug.h"
#include <d3d11_1.h>

// Need to get the appropriate driver for the CPU arch this binary was written for
#if defined(_M_X64) || defined(__amd64__)
#define NVAPI_DLL "nvapi64.dll"
#else
#define NVAPI_DLL "nvapi.dll"
#endif


// magic numbers, do not change them
#define NVAPI_MAX_PHYSICAL_GPUS   64
#define NVAPI_MAX_USAGES_PER_GPU  34
     
// function pointer types
typedef int *(*NvAPI_QueryInterface_t)(unsigned int offset);
typedef int (*NvAPI_Initialize_t)();
typedef int (*NvAPI_EnumPhysicalGPUs_t)(int **handles, int *count);
typedef int (*NvAPI_GPU_GetUsages_t)(int *handle, unsigned int *usages);

// nvapi.dll internal function pointers
NvAPI_QueryInterface_t      _NvAPI_QueryInterface     = NULL;
NvAPI_Initialize_t          _NvAPI_Initialize         = NULL;
NvAPI_EnumPhysicalGPUs_t    _NvAPI_EnumPhysicalGPUs   = NULL;
NvAPI_GPU_GetUsages_t       _NvAPI_GPU_GetUsages      = NULL;



/*
 * Name: NvGpuDetected
 * Desc: Returns true if an NVIDIA Gpu has been detected on this system.
 * NOTE: This function depends on whether a valid NVIDIA driver is installed
 *       on the target machine.  Since the Surface Hub does not include an
 *		 nvapi[64].dll in it's strippified driver, we need to load it directly 
 *		 in order for the required APIs to work.
 */
BOOL NvGpuDetected()
{
    HMODULE hmod = LoadLibraryA( NVAPI_DLL );

	if( hmod == NULL )
    {
        _ERROR( "Couldn't find " << NVAPI_DLL << std::endl );
        return FALSE;
    }

	// nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
    _NvAPI_QueryInterface = (NvAPI_QueryInterface_t) GetProcAddress( hmod, "nvapi_QueryInterface" );
     
    // some useful internal functions that aren't exported by nvapi.dll
    _NvAPI_Initialize = (NvAPI_Initialize_t) (*_NvAPI_QueryInterface)(0x0150E828);
    _NvAPI_EnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t) (*_NvAPI_QueryInterface)(0xE5AC921F);
    _NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t) (*_NvAPI_QueryInterface)(0x189A1FDF);
     
    if( _NvAPI_Initialize == NULL || _NvAPI_EnumPhysicalGPUs == NULL ||
        _NvAPI_QueryInterface == NULL || _NvAPI_GPU_GetUsages == NULL )
    {
        _ERROR( "Couldn't get functions in nvapi.dll" << std::endl );
        return FALSE;
    }
     
    // initialize NvAPI library, call it once before calling any other NvAPI functions
    if( (*_NvAPI_Initialize)() != 0 )
	{
		_ERROR( "Could not initialize nvapi!" << std::endl );
		return FALSE;
	}

	_LOG( "An NVIDIA Gpu has been detected!" << std::endl );

	return TRUE;
}

/*
 * Name: NvGetGpuLoad
 * Desc: Returns the NVIDIA Gpu's current load percentage.
 */
int  NvGetGpuLoad()
{
	int          gpuCount = 0;
    int         *gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
    unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };
     
    // gpuUsages[0] must be this value, otherwise NvAPI_GPU_GetUsages won't work
    gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;
     
    (*_NvAPI_EnumPhysicalGPUs)( gpuHandles, &gpuCount );
     
    (*_NvAPI_GPU_GetUsages)( gpuHandles[0], gpuUsages );
    int usage = gpuUsages[3];

	return usage;
}

/*
 * Name: NvGetGpuTemperature
 * Desc: Returns the current temperature of an NVIDIA Gpu.
 */
int  NvGetGpuTemperature()
{
#if 0
	// Array of physical GPU handle
	NvPhysicalGpuHandle nvGPUHandles[ NVAPI_MAX_PHYSICAL_GPUS];
	// Thermal settings
	NV_GPU_THERMAL_SETTINGS temperature;
	// Error string
	NvAPI_ShortString string;

	// ---- Retrive the handle for the GPU ----

	NvU32 gpuCount = 0;
	NvAPI_Status status = NvAPI_EnumPhysicalGPUs( nvGPUHandles, &gpuCount );
	if( status != NVAPI_OK ) // !TODO: cache the table for drivers >= 105.00
	{
		NvAPI_GetErrorMessage( status, string );
		//_ERROR( "NvAPI_EnumPhysicalGPUs(): " << string << std::endl );
		printf( "%s\n", string );
		return -1;
	}

	// Ensure the index is correct
//	if( dwGPUIndex > gpuCount )
//		return -1;

	// Retrive the temperature
	ZeroMemory( &temperature, sizeof( NV_GPU_THERMAL_SETTINGS ) );
	temperature.version = NV_GPU_THERMAL_SETTINGS_VER;
	status = NvAPI_GPU_GetThermalSettings( nvGPUHandles[0], NVAPI_THERMAL_TARGET_ALL, &temperature );
	if( status != NVAPI_OK )
	{
		NvAPI_GetErrorMessage( status, string );
		_ERROR( "NvAPI_GPU_GetThermalSettings(): " << string << std::endl );
		return -1;
	}

	if( temperature.count == 0 )
	{
		_ERROR( "temperature.count == 0" << std::endl );
		return -1;
	}

	return temperature.sensor[0].currentTemp;
#else
	return -1;
#endif
}


#include <d3d11_2.h>

/* Logging file */
std::ofstream logfi;

extern HRESULT KmtGetAdapter( UINT Index, DXGI_ADAPTER_DESC2* Desc );


int NVAPI_Initialize()
{
	_LOG( __FUNCTION__ << "(): " << "NVAPI driver initialization started...\n" );
	return NvGpuDetected();
}

void NVAPI_Uninitialize()
{
	_LOG( __FUNCTION__ << "(): NVAPI driver uninitialization completed.\n" );
}

int NVAPI_GetGpuDetails( int AdapterNumber, GPUDETAILS* pGpuDetails )
{
	_LOG( __FUNCTION__ << "(): TODO: Implement...\n" );

	if( !pGpuDetails )
	{
		_ERROR( "Invalid parameter!" << std::endl );
		return 0;
	}

	_LOG( __FUNCTION__ << "(): Gathering GPU details...\n" );

#if 0
	strcpy( pGpuDetails->DeviceDesc, "NVIDIA Something..." );
	pGpuDetails->DeviceID = 0xFFFF;	/* TODO */
	pGpuDetails->VendorID = 0x10DE;	/* This is always a given */
#else
	DXGI_ADAPTER_DESC2 desc;
	HRESULT hr = KmtGetAdapter( AdapterNumber, &desc );
	if( FAILED( hr ) )
		return 0;

	wcstombs( pGpuDetails->DeviceDesc, desc.Description, 128 );
	pGpuDetails->DeviceID = desc.DeviceId;
	pGpuDetails->VendorID = desc.VendorId;
#endif

	return 1;
}

int NVAPI_GetOverallGpuLoad()
{
	return NvGetGpuLoad();
}

int NVAPI_GetGpuTemperature()
{
	return NvGetGpuTemperature();
}
