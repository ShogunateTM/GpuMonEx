// GPUMonEx.Driver.D3DKMT.cpp : Defines the exported functions for the DLL application.
//

#include "..\platform.h"
#include "Drv_D3DKMT.h"

#undef NTSTATUS
#define NTSTATUS ULONG


#include <Windows.h>
#include <iostream>
#include "d3dkmt.h"
#include <vector>
#include "..\debug.h"
#include <d3d11_2.h>
#include <comip.h>
#include <comdef.h>
#include <string.h>
#include <wbemidl.h>
//#include <ntdef.h>


#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

#define UpdateDelta( DltMgr, NewValue ) 	\
	((DltMgr)->Delta = (NewValue) - (DltMgr)->Value, \
    (DltMgr)->Value = (NewValue), (DltMgr)->Delta)


struct UINT64_DELTA
{
	ULONG64 Value;
	ULONG64 Delta;
};

typedef _com_ptr_t<_com_IIID<ID3D11Device, &IID_ID3D11Device>> CD3D11Device;
typedef _com_ptr_t<_com_IIID<IDXGIDevice, &IID_IDXGIDevice>> CDXGIDevice;
typedef _com_ptr_t<_com_IIID<IDXGIAdapter, &IID_IDXGIAdapter>> CDXGIAdapter;
typedef _com_ptr_t<_com_IIID<IDXGIAdapter1, &IID_IDXGIAdapter1>> CDXGIAdapter1;
typedef _com_ptr_t<_com_IIID<IDXGIAdapter2, &IID_IDXGIAdapter2>> CDXGIAdapter2;
typedef _com_ptr_t<_com_IIID<ID3D11DeviceContext, &IID_ID3D11DeviceContext>> CD3D11DeviceContext;
typedef _com_ptr_t<_com_IIID<ID3D11Device2, &IID_ID3D11Device2>> CD3D11Device2;
typedef _com_ptr_t<_com_IIID<IDXGIAdapter2, &IID_IDXGIAdapter2>> CDXGIAdapter2;
typedef _com_ptr_t<_com_IIID<IDXGIFactory, &IID_IDXGIFactory>> CDXGIFactory;
typedef _com_ptr_t<_com_IIID<IDXGIFactory1, &IID_IDXGIFactory1>> CDXGIFactory1;
typedef _com_ptr_t<_com_IIID<IDXGIFactory2, &IID_IDXGIFactory2>> CDXGIFactory2;


PFND3DKMT_QUERYSTATISTICS	pfnD3DKMTQueryStatistics = nullptr;
HMODULE						hGdi = nullptr;
DXGI_ADAPTER_DESC2			DxgiDesc;
UINT						AdapterNumber = 0;
std::vector<float>			GpuUsage;
UINT64_DELTA				ClockTotalRunningTimeDelta = {0};
LARGE_INTEGER				ClockTotalRunningTimeFrequency = {0};
UINT64_DELTA				GpuTotalRunningTimeDelta = {0};
UINT64_DELTA				GpuSystemRunningTimeDelta = {0};
UINT64_DELTA				GpuNodesTotalRunningTimeDelta[16];


/*
 * Name: KmtGetAdapter
 * Desc: Returns the adapter description for the specified display adapter index.
 *		 Relies on DXGI to get the information needed.
 */
HRESULT KmtGetAdapter( UINT Index, DXGI_ADAPTER_DESC2* Desc )
{
	CDXGIFactory2 DXGIFactory;
	CDXGIAdapter1 Adapter1;
	CDXGIAdapter2 Adapter2;

	HRESULT hr = CreateDXGIFactory2( 0, __uuidof( CDXGIFactory2 ), (void**) &DXGIFactory );
	if( FAILED( hr ) )
		return hr;

	hr = DXGIFactory->EnumAdapters1( Index, &Adapter1 );
	if( FAILED( hr ) )
		return hr;

	hr = Adapter1->QueryInterface( __uuidof( Adapter1 ), (void**) &Adapter2 );

	return Adapter2->GetDesc2( Desc );
}


/*
 * Name: KmtGetNodeCount
 * Desc: Returns the number of GPU nodes associated with this GPU.
 */
UINT KmtGetNodeCount()
{
	D3DKMT_QUERYSTATISTICS QueryStatistics;
	memset( &QueryStatistics, 0, sizeof( D3DKMT_QUERYSTATISTICS ) );
	QueryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
	QueryStatistics.AdapterLuid = DxgiDesc.AdapterLuid;

	NTSTATUS status = pfnD3DKMTQueryStatistics( &QueryStatistics );
	if( !NT_SUCCESS( status ) )
		return 0;

	return QueryStatistics.QueryResult.AdapterInformation.NodeCount;
}


/*
 * Name: KmtDetectGpu
 * Desc: Uses Direct3D KMT to detect an Intel GPU for Windows devices.  If the Intel
 *		 GPU hardware is not found, then this function returns a failure.  Once the
 *		 expected GPU is found, we open up GDI32.dll and get a function pointer to the
 *		 function required to monitor GPU specific stats.
 */
BOOL KmtDetectGpu()
{
#if 0
	CD3D11Device Device;
	CD3D11Device2 Device2;
	CDXGIAdapter Adapter;
	CDXGIAdapter2 Adapter2;
	CD3D11DeviceContext DeviceContext;
	CDXGIDevice DXGIDevice;

	/* In order to get the LUID of the display adapter, create a Direct3D11 device and get
	   a copy of the descriptor which contains the information we need. */

	HRESULT hr = D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr, 0, nullptr, 0,
		D3D11_SDK_VERSION, &Device, nullptr, &DeviceContext );

	hr = Device->QueryInterface( __uuidof(ID3D11Device2), (void**) &Device2 );
	hr = Device->QueryInterface( __uuidof(IDXGIDevice), (void**) &DXGIDevice );
	hr = DXGIDevice->GetAdapter( &Adapter );
	hr = Adapter->QueryInterface( __uuidof(IDXGIAdapter2), (void**) &Adapter2 );

	Adapter2->GetDesc2( &DxgiDesc );
#endif

	UINT i = 0;

	/* Search for an Intel GPU.  If we don't find one, exit. */

	do {
		if( SUCCEEDED( KmtGetAdapter( i, &DxgiDesc ) ) )
		{
			if( wcsstr( DxgiDesc.Description, L"Intel(R)" ) != NULL )
				break;
		}
		else 
		{
			_ERROR( "Intel GPU not found!" << std::endl );
			return FALSE;
		}

		i++;
	} while(true);

	/* Load the gdi32.dll directly, and get a function pointer to the necessary function. */

	hGdi = ::LoadLibrary( TEXT( "gdi32.dll" ) );
	if( !hGdi )
	{
		_ERROR( "Unable to open gdi32.dll!" << std::endl );
		return FALSE;
	}

	pfnD3DKMTQueryStatistics = (PFND3DKMT_QUERYSTATISTICS) GetProcAddress( hGdi, "D3DKMTQueryStatistics" );
	if( !pfnD3DKMTQueryStatistics )
	{
		_ERROR( "Unable to locate D3DKMTQueryStatistics()!" << std::endl );
		return FALSE;
	}

	FreeLibrary( hGdi );

	return TRUE;
}

/*
 * Name: KmtGetGpuUsage
 * Desc: Returns the GPU usage in the form of a percentage.  
 */
int  KmtGetGpuUsage()
{
	UINT64 SharedBytesUsed = 0, DedicatedBytesUsed = 0, CommittedBytesUsed = 0;
	D3DKMT_QUERYSTATISTICS QueryStatistics;
	ULONG i;
	ULONG64 TotalRunningTime = 0, SystemRunningTime = 0;
	UINT NodeCount = KmtGetNodeCount();
	DOUBLE ElapsedTime;

	/* Query the statistics of each node and determine the level of running time for each one. */

	for( i = 0; i < NodeCount; i++ )
	{
		memset( &QueryStatistics, 0, sizeof( D3DKMT_QUERYSTATISTICS ) );
		QueryStatistics.Type = D3DKMT_QUERYSTATISTICS_NODE;
		QueryStatistics.AdapterLuid = DxgiDesc.AdapterLuid;
		QueryStatistics.QueryNode.NodeId = i;

		NTSTATUS status = pfnD3DKMTQueryStatistics( &QueryStatistics );
		if( NT_SUCCESS( status ) )
		{
			UpdateDelta( &GpuNodesTotalRunningTimeDelta[i], QueryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart );
			TotalRunningTime += QueryStatistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
			SystemRunningTime += QueryStatistics.QueryResult.NodeInformation.SystemInformation.RunningTime.QuadPart;
		}
	}

	/* Update timing */

	LARGE_INTEGER PerformanceCounter;

	QueryPerformanceCounter( &PerformanceCounter );
	QueryPerformanceFrequency( &ClockTotalRunningTimeFrequency );
	UpdateDelta( &ClockTotalRunningTimeDelta, PerformanceCounter.QuadPart );
	UpdateDelta( &GpuTotalRunningTimeDelta, TotalRunningTime );
	UpdateDelta( &GpuSystemRunningTimeDelta, SystemRunningTime );

	ElapsedTime = (DOUBLE) ClockTotalRunningTimeDelta.Delta * 10000000 / ClockTotalRunningTimeFrequency.QuadPart;

	float GpuUsage = 0;
	int PositiveNodes = 0;

	/* At the moment, we are only returning the GPU usage for the first node.  I noticed that 
	   with Intel GPUs, the first node is the one that is used primarily, and the other nodes
	   don't seem to get used much, if at all.  In the future, we should probably take them all
	   into account and average the results together, but for now, the first node tends to give
	   rather consistent results so far. */

	//for( i = 0; i < NodeCount; i++ )
	//{
		if( ElapsedTime != 0 )
		{
			GpuUsage = (float) ( GpuTotalRunningTimeDelta.Delta / ElapsedTime );
		}
	//}

	if( GpuUsage > 1.0f )
		GpuUsage = 1.0f;

	return int( GpuUsage * 100.0f );
}


/*
 * Name: KmtGetGpuTemperature
 * Desc: Returns the temperature of the GPU.
 * NOTE: Since the intel GPU is integrated with the same die as the CPU itself, this will
 *		 probably be the same as CPU temperature itself.
 */
int  KmtGetGpuTemperature()
{
#if 0	/* TODO: Find out why this implementation does not work */
	ULONG Temperature = -1;
    
    HRESULT ci = CoInitialize(NULL); // needs comdef.h
    HRESULT hr = CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL );

    if( SUCCEEDED( hr ) )
    {
        IWbemLocator *pLocator; // needs Wbemidl.h & Wbemuuid.lib
        hr = CoCreateInstance( CLSID_WbemAdministrativeLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*) &pLocator );
        if( SUCCEEDED( hr ) )
        {
            IWbemServices *pServices;
            BSTR ns = SysAllocString( L"root\\WMI" );
            hr = pLocator->ConnectServer( ns, NULL, NULL, NULL, 0, NULL, NULL, &pServices );
            pLocator->Release();
            SysFreeString(ns);
            if( SUCCEEDED( hr ) )
            {
                BSTR query = SysAllocString( L"SELECT * FROM MSAcpi_ThermalZoneTemperature" );
                BSTR wql = SysAllocString( L"WQL" );
                IEnumWbemClassObject *pEnum;
                hr = pServices->ExecQuery( wql, query, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum );
                SysFreeString(wql);
                SysFreeString(query);
                pServices->Release();
                if( SUCCEEDED( hr ) )
                {
                    IWbemClassObject *pObject;
                    ULONG returned;
                    hr = pEnum->Next( WBEM_INFINITE, 1, &pObject, &returned );
                    pEnum->Release();
                    if( SUCCEEDED( hr ) )
                    {
                        BSTR temp = SysAllocString( L"CurrentTemperature" );
                        VARIANT v;
                        VariantInit(&v);
                        hr = pObject->Get( temp, 0, &v, NULL, NULL );
                        pObject->Release();
                        SysFreeString(temp);
                        if( SUCCEEDED( hr ) )
                        {
                            Temperature = V_I4(&v);
                        }
                        VariantClear(&v);
                    }
                }
            }
            if( ci == S_OK )
            {
                CoUninitialize();
            }
        }
    }

	return Temperature;
#else
	return 0;
#endif
}


/* Logging file */
//std::ofstream logfi;


int D3DKMT_Initialize()
{
	//_LOG( __FUNCTION__ << "(): " << "D3DKMT driver (for Intel) initialization started...\n" );
	return KmtDetectGpu();
}

void D3DKMT_Uninitialize()
{
	//_LOG( __FUNCTION__ << "D3DKMT driver uninitialization completed.\n" );
}

int D3DKMT_GetGpuDetails( int AdapterNumber, GPUDETAILS* pGpuDetails )
{
	//_LOG( __FUNCTION__ << "TODO: Implement...\n" );

	if( !pGpuDetails )
	{
		//_ERROR( "Invalid parameter!" << std::endl );
		return 0;
	}

	DXGI_ADAPTER_DESC2 desc;
	HRESULT hr = KmtGetAdapter( 0, &desc );
	if( FAILED( hr ) )
		return 0;

	wcstombs( pGpuDetails->DeviceDesc, desc.Description, 128 );
	pGpuDetails->DeviceID = desc.DeviceId;
	pGpuDetails->VendorID = desc.VendorId;

	return 1;
}

int D3DKMT_GetOverallGpuLoad()
{
	return KmtGetGpuUsage();
}

int D3DKMT_GetGpuTemperature()
{
	return KmtGetGpuTemperature();
}
