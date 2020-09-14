// gpucmd.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "../platform.h"
#include "../gmdebug.h"
#include "../gpumon/DriverEnum.h"
#include "../common/ProcessEnumeration.h"

#include <memory>
#include <cstdlib>

#ifdef _WIN32
#pragma warning (disable:4996)
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
//std::unordered_map<GPUDRIVER> driver;


#ifndef _WIN32
int _stricmp(const char* str1, const char* str2)
{
    if (str1 == str2)
        return 0;
    else if (str1 == NULL)
        return -1;
    else if (str2 == NULL)
        return 1;
    else {
        while (tolower(*str1) == tolower(*str2) && *str1 != 0 && *str2 != 0)
        {
            ++str1;
            ++str2;
        }
        if (*str1 < *str2)
            return -1;
        else if (*str1 > *str2)
            return 1;
        else
            return 0;
    }
}
#endif

/*
 * Name: GetLastErrorAsString
 * Desc: Returns the last error, in string format. Returns an empty string if there is no error.
 */
std::string GetLastErrorAsString()
{
#if defined(_WIN32)
    //Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0)
        return std::string(); //No error message has been recorded

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string msg(messageBuffer, size);
	std::stringstream ss;

	ss << "\nGetLastError(0x" << errorMessageID << "): " << msg << std::endl;

	std::string message = ss.str();

    //Free the buffer.
    LocalFree(messageBuffer);
#else	// macOS and Linux
	//std::string message( strerror( errno ) );
	std::stringstream ss;
	int e = errno;

	ss << "\nerrno(" << e << "): " << strerror( e ) << std::endl;

	std::string message = ss.str();

	char* dlerr = dlerror();
	if( dlerr )
	{
		message.append( "\ndlerror(): " );
		message.append( dlerr );
		message.append( "\n" );
	}
#endif

    return message;
}


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

		/* << "\tHelp:        -h or -?" << std::endl
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
bool InitializeGpuMon()
{
    GLOG( 3, "Begin..." );
    GLOG( 4, "Loading dynamic library \"" << GPUMON_DLL << "\"..." );
    
	/* Start by loading the appropriate DLL*/
    hGpuMonDll = LoadLibraryA( GPUMON_DLL );
	if( !hGpuMonDll )
	{
		GLOG( GPUMON_ERROR, "ERROR: Could not open " << GPUMON_DLL << GetLastErrorAsString() );
             
		return false;
	}
	
    GLOG( 4, "Querying procedure address: Drv_GetGpuDriver..." );
    
	/* Get the primary function that returns our driver retreival function */
	pfnDrv_GetGpuDriver = (void (*)(int, GPUDRIVER*)) GetProcAddress( hGpuMonDll, "Drv_GetGpuDriver" );
	if( !pfnDrv_GetGpuDriver )
	{
		GLOG( GPUMON_ERROR, "ERROR: Could not locate Drv_GetGpuDriver() within module " << GPUMON_DLL << GetLastErrorAsString() );

		return false;
	}
    
    GLOG( 3, "Success" );

	return true;
}


/*
 * Name: UninitializeGpuMon
 * Desc: Uninitializes the GpuMon DLL and anything else that needs uninitialization
 *		 done above us.
 */
void UninitializeGpuMon()
{
    GLOG( 3, "Begin..." );
    
	if( hGpuMonDll )
    {
        GLOG( 4, "Freeing dynamic library handle..." );
		FreeLibrary( hGpuMonDll );
        hGpuMonDll = NULL;
    }
    else
        GLOG( 4, "GpuMon dynamic library not loaded!" );
    
    GLOG( 3, "Done" );
}


/*
 * Name: IntializeDriverHook
 * Desc: Initalizes driver hook for a specific driver vendor.
 */
bool InitializeDriverHook( int DriverType, int Adapter, GPUDRIVER* driver )
{
    GLOG( 3, "Begin..." );
    
    /* Sanity check */
    if( !driver )
    {
        GERROR( "Invalid driver structure pointer!" );
        return false;
    }
    
    GLOG( 4, "Initializing driver structure..." );
    
    /* Get the driver and attempt to initialize it */
    pfnDrv_GetGpuDriver( DriverType, driver );
    
    GLOG( 4, "Initializing driver hooks (driver type: " << DriverType << ", adapter #: " << Adapter << ")" );
    
    if( !driver->Initialize() )
    {
        GLOG( 4, "Could not initialize driver hooks, hardware not present or internal API failure." );
        return false;
    }
    
    GLOG( 3, "Success" );
    
    return true;
}


int main( int argc, char** argv )
{
    int Seconds = 100;
    int DriverType;
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
#ifdef _WIN32
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
#endif

	std::atexit( []()
		{
			if( driver[Drv_D3DKMT].Uninitialize ) driver[Drv_D3DKMT].Uninitialize(); 
			if( driver[Drv_NVAPI].Uninitialize ) driver[Drv_NVAPI].Uninitialize();
			if( driver[Drv_AMDGS].Uninitialize ) driver[Drv_AMDGS].Uninitialize();
            if( driver[Drv_IOKIT].Uninitialize ) driver[Drv_IOKIT].Uninitialize();
            UninitializeGpuMon();
		} );

#ifdef _WIN32
	SetConsoleCtrlHandler( HandlerRoutine, TRUE );
#endif

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
					GERRORP( "Error creating new process!" );
					return ERR_PROCNOEXIST;
				}
			}

			/* Open existing process */
			if( ARGUMENT( "/e" ) )
			{
				i++;
				if( !OpenProcessByName( argv[i], &Process ) )
				{
					GERRORP( "Error opening process!" );
					return ERR_PROCNOEXIST;
				}
			}

			/* Open exisint process */
			if( ARGUMENT( "/id" ) )
			{
				i++;
				if( !OpenProcessByID( atoi( argv[i] ), &Process ) )
				{
					GERRORP( "Error opening process!" );
					return ERR_PROCNOEXIST;
				}
			}
		}
	}
    
#ifdef __i386__
    std::cout << "32-bit mode...\n\n";
#else
    std::cout << "64-bit mode...\n\n";
#endif

	if( DebugEnabled )
		dbg = std::make_unique<NVDebug>( DebugLevel, DebugFile );
	
	/*
	 * Start by loading the appropriate DLL
	 */
	
	if( !InitializeGpuMon() )
		return ERR_DRVFAIL;

#if defined(_WIN32)
    /* The D3DKMT driver hook is manditory */
    if( !InitializeDriverHook( Drv_D3DKMT, Adapter, &driver[Drv_D3DKMT] ) )
    {
        GERRORP( "Unable to initialize D3DKMT driver hooks!" );
        return ERR_DRVFAIL;
    }
    
    DriverType = Drv_D3DKMT;
    
	/* These two are purely optional */
	if( !InitializeDriverHook( Drv_NVAPI, Adapter, &driver[Drv_NVAPI] ) )
	{
		GLOGP( 3, "NVAPI driver not available..." );
	}
	if( !InitializeDriverHook( Drv_AMDGS, Adapter, &driver[Drv_AMDGS] ) )
	{
		GLOGP( 3, "AMDGS driver not available..." );
	}
#elif defined(__APPLE__)
    /* Initialize IOKit hooks */
    if( !InitializeDriverHook( Drv_IOKIT, Adapter, &driver[Drv_IOKIT] ) )
    {
        GERRORP( "Unable to initialize IOKit APIs!" );
        return ERR_DRVFAIL;
    }
    
    DriverType = Drv_IOKIT;
#endif

	/* If desired, print out the adapter info of the detected GPU */
	if( ShowAdapterInfo )
	{
		GPUDETAILS details;

		if( !driver[DriverType].GetGpuDetails( 0, &details ) )
			return ERR_DRVFAIL;

		GLOGP( 3, details.DeviceDesc );
		GLOGP( 3, std::showbase << std::internal << std::setfill('0') << std::hex );
		GLOGP( 3, "Device ID: " << details.DeviceID );
		GLOGP( 3, "Vendor ID: " << details.VendorID );
		GLOGP( 3, std::dec );

//		return 0;
	}

#ifdef _WIN32   /* TODO: Laziness... */
	/* Show Start time */
	SYSTEMTIME Time;

	if( Timestamp )
	{
		GetSystemTime( &Time );

		GLOGP( 3, "Start time: " << Time.wMonth << "/" <<
			Time.wDay << "/" << Time.wYear << 
			" (" << Time.wHour << ":" << Time.wMinute << ":" <<
			Time.wSecond << ")" << std::endl );
	}
#endif

	/* Print GPU usage every second */

	if( ShowAdapterUsage || ShowProcessUsage )
	{
		for( int i = 0; i < Seconds; i++ )
		{
			if( ShowAdapterUsage )
			{
                GPUSTATISTICS stats;
				int ret = driver[DriverType].GetOverallGpuLoad( Adapter, &stats );
				GLOGP( 3, "Adapter GPU Usage: " << stats.gpu_usage << "%" );
			}
			if( ShowProcessUsage )
			{
				int GpuUsage = driver[DriverType].GetProcessGpuLoad( Adapter, Process.hProcess );
				GLOGP( 3, "Process GPU Usage: " << GpuUsage << "%" );
			}

			Sleep( 1000 );
		}
	}

	if( Timestamp )
	{
#ifdef _WIN32
		GetSystemTime( &Time );

		GLOGP( 3, "Stop time: " << Time.wMonth << "/" <<
			Time.wDay << "/" << Time.wYear << 
			" (" << Time.wHour << ":" << Time.wMinute << ":" <<
			Time.wSecond << ")" << std::endl );
#endif
	}

	/*if( logfi.is_open() )
		logfi.close();*/

	TerminateProcess( &Process );
    
    UninitializeGpuMon();
    
    return 0;
}
