// gpucmd.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "..\platform.h"
#include "..\gmdebug.h"
#include "..\gpumon\DriverEnum.h"
#include "..\common\ProcessEnumeration.h"

#include <memory>
#include <cstdlib>

#ifdef _WIN32
#pragma warning (disable:4996)
#endif

#if defined(_M_X64) || defined(__amd64__)
#define GPUMON_DLL "gpumon64.dll"
#else
#define GPUMON_DLL "gpumon32.dll"
#endif

#define ARGUMENT(x) (!_stricmp( x, argv[i] ))


/*
 * Globals
 */

/* GpuMon core function */
void (*pfnDrv_GetGpuDriver)( int DriverType, GPUDRIVER* pDriver );

/* Handle to the actual GpuMon core */
HMODULE hGpuMonDll = nullptr;

/* Driver handle instance*/
GPUDRIVER driver[Drv_MAX];



/*
 * Name: ShowHelpMenu
 * Desc: Prints the below instructions as well as a list of error codes so if
 *		 something goes wrong, you have an idea as to what it is!
 */
void ShowHelpMenu()
{
	std::cout 
		<< "Usage: gpucmd [arguments]" << std::endl
		<< "--help (-h or -?):                   < Help" << std::endl
		<< "--duration (-d) [seconds]:           < Program's execution duration (in seconds)" << std::endl
	//	<< "--driver [any|nvapi|amdgs|d3dkmt]:   < Select desired driver types (optional)" << std::endl
		<< "--logging (-l) [directory+filename]: < Log results to text file" << std::endl
		<< "--debug-level (-dbglvl) [0|1|2|3]:   < Debug output level (to debug file)" << std::endl
		<< "--timestamp (-t):                    < Show timestamps" << std::endl
		<< "--select-adapter [0|1|2|n]:          < Select video adapter number to use (0=deafult; optional)" << std::endl
		<< "--adapter-info (-ai):                < Show display adapter info" << std::endl
		<< "--adapter-usage (-au):               < GPU usage for this display adapter" << std::endl
		<< "--process-usage (-pu) [arguments]    < GPU usage for the selected process (see options below)" << std::endl
		<< "\t/o [directory+exe]:                < Monitor new process (open executable and terminate when done)" << std::endl
		<< "\t/e [process-name]:                 < Monitor existing process using the process name" << std::endl
		<< "\t/id [process-id]:                  < Monitor existing process using the process ID" << std::endl;

	/*	<< "--open-process-name (-opn) [directory+exe]:" << std::endl
		<< "--existing-process-name (-epn) [pname]:" << std::endl
		<< "--process-id (-pid) [processID]:" << std::endl; */

		/*<< "\tHelp:        -h or -?" << std::endl
		<< "\tDuration:    -d [seconds] (optional; default = 100)" << std::endl
		<< "\tIntel Only:  -intel" << std::endl
		<< "\tNVIDIA Only: -nvidia" << std::endl
        << "\tAMD Only     -amd" << std::endl
		<< "\tLogging:     -l [filename+directory]" << std::endl
		<< "\tTimestamp:   -t (log start/end time)" << std::endl
		<< "\tAdapterInfo: -a (prints adapter info)" << std::endl;*/

	std::cout
		<< "\nError codes:" << std::endl
		<< "\tNo Error              (ERR_OK):          " << ERR_OK << std::endl
		<< "\tNo Parameters         (ERR_NOPARAMS):    " << ERR_NOPARAMS << std::endl
		<< "\tMissing Hardware      (ERR_MISSINGHW):   " << ERR_MISSINGHW << std::endl
		<< "\tBad Parameters        (ERR_BADPARAMS):   " << ERR_BADPARAMS << std::endl
		<< "\tGpuMon Internal Error (ERR_DRVFAIL):     " << ERR_DRVFAIL << std::endl
		<< "\tNon-existant process  (ERR_PROCNOEXIST): " << ERR_PROCNOEXIST << std::endl;
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
		GLOG( GPUMON_ERROR, "ERROR: Could not open " << GPUMON_DLL << "!\nGetLastError(): " << GetLastError() << "\n\n" );
		return false;
	}
	
	/* Get the primary function that returns our driver retreival function */
	pfnDrv_GetGpuDriver = (void (*)(int, GPUDRIVER*)) GetProcAddress( hGpuMonDll, "Drv_GetGpuDriver" );
	if( !pfnDrv_GetGpuDriver )
	{
		GLOG( GPUMON_ERROR, "ERROR: Could not locate Drv_GetGpuDriver() within module " << GPUMON_DLL << "!\nGetLastError(): " << GetLastError() << "\n\n" );
		return false;
	}

	/* TODO: Laziness: Actually enumerate the GPUs when Drv_Default is passed in */

	/* Get the driver and attempt to initialize it */
	pfnDrv_GetGpuDriver( DriverType, driver );
            
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
	int Adapter = 0;
	bool ShowAdapterUsage = false;
	bool ShowProcessUsage = false;

	/* Debug output settings */
	int DebugLevel = 0;
	bool DebugEnabled = false;
	char DebugFile[2048];

	GMPROCESS Process;

	std::unique_ptr<NVDebug> dbg;
	
	/* 
	 * Attempt to setup a few fail-safe methods to fall back on in case of a premature exit 
	 */
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

	std::atexit( []() 
		{ 
			if( driver[Drv_D3DKMT].Uninitialize ) driver[Drv_D3DKMT].Uninitialize(); 
			if( driver[Drv_NVAPI].Uninitialize ) driver[Drv_NVAPI].Uninitialize();
			if( driver[Drv_AMDGS].Uninitialize ) driver[Drv_AMDGS].Uninitialize(); 
		} );

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
		if( ARGUMENT( "-h" ) || ARGUMENT( "-?" ) || ARGUMENT( "--help" ) )
		{
			ShowHelpMenu();
			return ERR_OK;
		}

#if 0
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
#endif

		/* Duration (in seconds ) */
		if( ARGUMENT( "-d" ) || ARGUMENT( "--duration" ) )
		{
			i++;
			Seconds = atoi(argv[i]);
		}

		/* Logging */
		if( ARGUMENT( "-l" ) || ARGUMENT( "--logging" ) )
		{
			i++;
			
			DebugEnabled = true;
			strcpy( DebugFile, argv[i] );
		}

		/* Debug level */
		if( ARGUMENT( "-dbglvl" ) || ARGUMENT( "--debug-level" ) )
		{
			i++;
			DebugLevel = atoi( argv[i] );
		}

		/* Timestamp */
		if( ARGUMENT( "-t" ) || ARGUMENT( "--timestamp" ) )
			Timestamp = true;

		/* Display LUID */
		//if( !strcmp( "-luid", argv[i] ) )
		//	ShowLUID = true;

		/* Select adapter */
		if( ARGUMENT( "-sa" ) || ARGUMENT( "--select-adapter" ) )
		{
			i++;
			Adapter = atoi( argv[i] );
		}

		/* Display adapter info */
		if( ARGUMENT( "-ai" ) || ARGUMENT( "--adapter-info" ) )
			ShowAdapterInfo = true;

		/* Display adapter GPU usage */
		if( ARGUMENT( "-au" ) || ARGUMENT( "--adapter-usage" ) )
			ShowAdapterUsage = true;

		/* Process GPU usage */
		if( ARGUMENT( "-pu" ) || ARGUMENT( "--process-usage" ) )
		{
			i++;
			ShowProcessUsage = true;

			/* Open a new process */
			if( ARGUMENT( "/o" ) )
			{
				i++;
				if( !CreateNewProcess( argv[i], &Process ) )				
				{
					GERROR( "Error creating new process!" );
					return ERR_PROCNOEXIST;
				}
			}

			/* Open existing process */
			if( ARGUMENT( "/e" ) )
			{
				i++;
				if( !OpenProcessByName( argv[i], &Process ) )
				{
					GERROR( "Error opening process!" );
					return ERR_PROCNOEXIST;
				}
			}

			/* Open exisint process */
			if( ARGUMENT( "/id" ) )
			{
				i++;
				if( !OpenProcessByID( atoi( argv[i] ), &Process ) )
				{
					GERROR( "Error opening process!" );
					return ERR_PROCNOEXIST;
				}
			}
		}
	}

	if( DebugEnabled )
		dbg = std::make_unique<NVDebug>( DebugLevel, DebugFile );
	
	/*
	 * Start by loading the appropriate DLL
	 */
	
	if( !InitializeGpuMon( Drv_D3DKMT, Adapter, &driver[Drv_D3DKMT] ) )
		return ERR_DRVFAIL;

	/* These two are purely optional */
	if( !InitializeGpuMon( Drv_NVAPI, Adapter, &driver[Drv_NVAPI] ) )
	{
		GLOG( 3, "NVAPI driver not available..." );
	}
	if( !InitializeGpuMon( Drv_AMDGS, Adapter, &driver[Drv_AMDGS] ) )
	{
		GLOG( 3, "AMDGS driver not available..." );
	}

	/* If desired, print out the adapter info of the detected GPU */
	if( ShowAdapterInfo )
	{
		GPUDETAILS details;

		if( !driver[Drv_D3DKMT].GetGpuDetails( 0, &details ) )
			return ERR_DRVFAIL;

		GLOG( 3, details.DeviceDesc );
		GLOG( 3, std::showbase << std::internal << std::setfill('0') << std::hex );
		GLOG( 3, "Device ID: " << details.DeviceID );
		GLOG( 3, "Vendor ID: " << details.VendorID );
		GLOG( 3, std::dec );

//		return 0;
	}

	/* Show Start time */
	SYSTEMTIME Time;

	if( Timestamp )
	{
		GetSystemTime( &Time );

		GLOG( 3, "Start time: " << Time.wMonth << "/" <<
			Time.wDay << "/" << Time.wYear << 
			" (" << Time.wHour << ":" << Time.wMinute << ":" <<
			Time.wSecond << ")" << std::endl );
	}

	/* Print GPU usage every second */

	if( ShowAdapterUsage || ShowProcessUsage )
	{
		for( int i = 0; i < Seconds; i++ )
		{
			if( ShowAdapterUsage )
			{
				int GpuUsage = driver[Drv_D3DKMT].GetOverallGpuLoad();
				GLOG( 3, "Adapter GPU Usage: " << GpuUsage << "%" );
			}
			if( ShowProcessUsage )
			{
				int GpuUsage = driver[Drv_D3DKMT].GetProcessGpuLoad( Process.hProcess );
				GLOG( 3, "Process GPU Usage: " << GpuUsage << "%" );
			}

			Sleep( 1000 );
		}
	}

	if( Timestamp )
	{
		GetSystemTime( &Time );

		GLOG( 3, "Stop time: " << Time.wMonth << "/" <<
			Time.wDay << "/" << Time.wYear << 
			" (" << Time.wHour << ":" << Time.wMinute << ":" <<
			Time.wSecond << ")" << std::endl );
	}

	/*if( logfi.is_open() )
		logfi.close();*/

	TerminateProcess( &Process );
    
    return 0;
}
