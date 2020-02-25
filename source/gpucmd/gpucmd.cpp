// gpucmd.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "..\platform.h"
#include "..\gpumon\DriverEnum.h"

#include <memory>
#include <cstdlib>

/* 
 * Classic NVSDK helper headers 
 */
#include <nvdebug.h>

/* 
 * GpuMon debugging macros 
 */
#define GPUMON_DEBUG_LVL	3

#define GPUMON_ERROR	1
#define GPUMON_WARNING	2
#define GPUMON_DEBUG	3

#define GLOG( a, b ) \
	std::cout << b << std::endl; \
	DISPDBG( a, b )


/*
 * gpucmd error codes
 */
#define ERR_OK			0
#define ERR_NOPARAMS	1
#define ERR_MISSINGHW	2
#define ERR_BADPARAMS	3
#define ERR_DRVFAIL		4


#if defined(_M_X64) || defined(__amd64__)
#define GPUMON_DLL "gpumon64.dll"
#else
#define GPUMON_DLL "gpumon32.dll"
#endif

/*
 * Globals
 */

/* GpuMon core function */
void (*pfnDrv_GetGpuDriver)( int DriverType, GPUDRIVER* pDriver );

/* Handle to the actual GpuMon core */
HMODULE hGpuMonDll = nullptr;

/* Driver handle instance*/
GPUDRIVER driver;



/*
 * Name: ShowHelpMenu
 * Desc: Prints the below instructions as well as a list of error codes so if
 *		 something goes wrong, you have an idea as to what it is!
 */
void ShowHelpMenu()
{
	std::cout 
		<< "Usage: gpucmd [arguments]" << std::endl
		<< "\tHelp:        -h or -?" << std::endl
		<< "\tDuration:    -d [seconds] (optional; default = 100)" << std::endl
		<< "\tIntel Only:  -intel" << std::endl
		<< "\tNVIDIA Only: -nvidia" << std::endl
        << "\tAMD Only     -amd" << std::endl
		<< "\tLogging:     -l [filename+directory]" << std::endl
		<< "\tTimestamp:   -t (log start/end time)" << std::endl
		<< "\tAdapterInfo: -a (prints adapter info)" << std::endl;

	std::cout
		<< "\nError codes:" << std::endl
		<< "\tNo Error              (ERR_OK):        " << ERR_OK << std::endl
		<< "\tNo Parameters         (ERR_NOPARAMS):  " << ERR_NOPARAMS << std::endl
		<< "\tMissing Hardware      (ERR_MISSINGHW): " << ERR_MISSINGHW << std::endl
		<< "\tBad Parameters        (ERR_BADPARAMS): " << ERR_BADPARAMS << std::endl
		<< "\tGpuMon Internal Error (ERR_DRVFAIL):   " << ERR_DRVFAIL << std::endl;
}


/*
 * Name: InitializeGpuMon
 * Desc: Initializes the GpuMon DLL for the appropriate architecture.
 */
bool InitializeGpuMon( int DriverType, int Adapter,  GPUDRIVER* driver )
{
	/* Start by loading the appropriate DLL*/
    hGpuMonDll = LoadLibraryA( GPUMON_DLL );
	if( !hGpuMonDll )
	{
		DISPDBG( GPUMON_ERROR, "ERROR: Could not open " << GPUMON_DLL << "!\nGetLastError(): " << GetLastError() << "\n\n" );
		return false;
	}
	
	/* Get the primary function that returns our driver retreival function */
	pfnDrv_GetGpuDriver = (void (*)(int, GPUDRIVER*)) GetProcAddress( hGpuMonDll, "Drv_GetGpuDriver" );
	if( !pfnDrv_GetGpuDriver )
	{
		DISPDBG( GPUMON_ERROR, "ERROR: Could not locate Drv_GetGpuDriver() within module " << GPUMON_DLL << "!\nGetLastError(): " << GetLastError() << "\n\n" );
		return false;
	}

	/* TODO: Laziness: Actually enumerate the GPUs when Drv_Default is passed in */

	/* Get the driver and attempt to initialize it */
	pfnDrv_GetGpuDriver( Drv_D3DKMT, driver );
            
	if( !driver->Initialize() )
	{
		return false;
	}

	return true;
}


/*
 * Name: UninitializeGpuMon
 * Desc: Uninitializes the GpuMon DLL and anything else that needs uninitialization
 *		 done above us.
 */
void UninitializeGpuMon()
{
	if( hGpuMonDll )
		FreeLibrary( hGpuMonDll );
}


int main( int argc, char** argv )
{
    int Seconds = 100;
	bool AutoDetect = true;
	int GpuType = Drv_Default;
	bool Timestamp = false;
	bool ShowAdapterInfo = false;

	std::unique_ptr<NVDebug> dbg;
	
	auto HandlerRoutine = []( _In_ DWORD dwCtrlType ) {
		switch( dwCtrlType )
		{
		case CTRL_C_EVENT:
			exit(-1);
			// Signal is handled - don't pass it on to the next handler
			return TRUE;
		default:
			// Pass signal on to the next handler
			return FALSE;
		}
	};

	std::atexit( []() { driver.Uninitialize(); } );
	SetConsoleCtrlHandler( HandlerRoutine, TRUE );

	/*
	 * Copyright notice
	 */
    std::cout << "gpucmd (C) 2018-20 by Shogunate (TM), all rights reserved.\n\n";

	/* 
	 * Parse command line arguments 
	 */

	if( argc < 2 )
	{
		std::cerr << "Usage: gpucmd [arguments]" << std::endl
			<< "\tHelp: -h or -?" << std::endl;

		return ERR_NOPARAMS;
	}

    for( int i = 1; i < argc; i++ )
	{
		/* Help */
		if( !strcmp( "-h", argv[i] ) || !strcmp( "-?", argv[i] ) )
		{
			ShowHelpMenu();
			return ERR_OK;
		}

		/* Manual GPU selection */
		if( !strcmp( "-intel", argv[i] ) )
		{
			AutoDetect = false;

			if( GpuType != Drv_Default )
			{ 
				std::cerr << "Mutliple GPU vendor flags are not allowed." << std::endl;
				return ERR_BADPARAMS;
			}

			GpuType = Drv_D3DKMT;
		}

		if( !strcmp( "-nvidia", argv[i] ) )
		{
			AutoDetect = false;

			if( GpuType != Drv_Default )
			{ 
				std::cerr << "Mutliple GPU vendor flags are not allowed." << std::endl;
				return ERR_BADPARAMS;
			}

			GpuType = Drv_NVAPI;
		}

		/* Duration (in seconds ) */
		if( !strcmp( "-d", argv[i] ) )
		{
			i++;
			Seconds = atoi(argv[i]);
		}

		/* Logging */
		if( !strcmp( "-l", argv[i] ) )
		{
			i++;
			
			dbg = std::make_unique<NVDebug>( 3, argv[i] );
		}

		/* Timestamp */
		if( !strcmp( "-t", argv[i] ) )
			Timestamp = true;

		/* Display LUID */
		//if( !strcmp( "-luid", argv[i] ) )
		//	ShowLUID = true;

		/* Display adapter info */
		if( !strcmp( "-a", argv[i] ) )
			ShowAdapterInfo = true;
	}

	/*
	 * Start by loading the appropriate DLL
	 */
	
	if( !InitializeGpuMon( Drv_D3DKMT, 0, &driver ) )
		return ERR_DRVFAIL;

	/* If desired, print out the adapter info of the detected GPU */
	if( ShowAdapterInfo )
	{
		GPUDETAILS details;

		if( !driver.GetGpuDetails( 0, &details ) )
			return ERR_DRVFAIL;

		GLOG( 3, details.DeviceDesc );
		DISPDBG( 3, std::showbase << std::internal << std::setfill('0') << std::hex );
		DISPDBG( 3, "Device ID: " << details.DeviceID );
		DISPDBG( 3, "Vendor ID: " << details.VendorID );
		DISPDBG( 3, std::dec );

//		return 0;
	}

	/* Show Start time */
	SYSTEMTIME Time;

	if( Timestamp )
	{
		GetSystemTime( &Time );

		DISPDBG( 3, "Start time: " << Time.wMonth << "/" <<
			Time.wDay << "/" << Time.wYear << 
			" (" << Time.wHour << ":" << Time.wMinute << ":" <<
			Time.wSecond << ")" << std::endl );
	}

	/* Print GPU usage every second */

    for( int i = 0; i < Seconds; i++ )
    {
        GLOG( 3, "GPU Usage: " << driver.GetOverallGpuLoad() << "%" << std::endl );

        Sleep(1000);
    }

	if( Timestamp )
	{
		GetSystemTime( &Time );

		DISPDBG( 3, "Stop time: " << Time.wMonth << "/" <<
			Time.wDay << "/" << Time.wYear << 
			" (" << Time.wHour << ":" << Time.wMinute << ":" <<
			Time.wSecond << ")" << std::endl );
	}

	/*if( logfi.is_open() )
		logfi.close();*/
    
    return 0;
}
