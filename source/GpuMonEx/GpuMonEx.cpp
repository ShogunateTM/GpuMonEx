// GpuMonEx.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "GpuMonEx.h"
#include "mainframe.hpp"

#include "../platform.h"
#include "../gmdebug.h"
#include "../gpumon/DriverEnum.h"



/* GpuMon core function */
void (*pfnDrv_GetGpuDriver)( int DriverType, GPUDRIVER* pDriver );

/* Handle to the actual GpuMon core */
HMODULE hGpuMonDll = nullptr;

/* Driver handle instance*/
GPUDRIVER driver[Drv_MAX];




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



class GpuMonEx : public wxApp
{
public:
	virtual bool OnInit()
	{
        if( Initialize() != 0 )
            return false;
        
        auto mainframe = new gpumonex::wx::main_frame( wxT( "GpuMonEx (Pre-Alpha) 0.1" ) );
		mainframe->Show();

		return true;
	}
    
    virtual int OnExit()
    {
        Uninitialize();
        return 0;
    }
    
private:
    int Initialize()
    {
        if( !InitializeGpuMon() )
        {
            wxMessageBox( wxT( "Error loading driver communication library!" ), wxT( "GpuMonEx" ), wxICON_EXCLAMATION );
            return ERR_DRVFAIL;
        }
        
#if defined(_WIN32)
        /* The D3DKMT driver hook is manditory */
        if( !InitializeDriverHook( Drv_D3DKMT, 0, &driver[Drv_D3DKMT] ) )
        {
            GERROR( "Unable to initialize D3DKMT driver hooks!" );
            wxMessageBox( wxT( "Unable to initialize D3DKMT driver hooks!" ), wxT("GpuMonEx"), wxICON_EXCLAMATION );
            return ERR_DRVFAIL;
        }
        
        /* These two are purely optional */
        if( !InitializeDriverHook( Drv_NVAPI, 0, &driver[Drv_NVAPI] ) )
        {
            GLOG( 3, "NVAPI driver not available..." );
        }
        if( !InitializeDriverHook( Drv_AMDGS, 0, &driver[Drv_AMDGS] ) )
        {
            GLOG( 3, "AMDGS driver not available..." );
        }
#elif defined(__APPLE__)
        /* Initialize IOKit hooks */
        if( !InitializeDriverHook( Drv_IOKIT, 0, &driver[Drv_IOKIT] ) )
        {
            GERROR( "Unable to initialize IOKit APIs!" );
            wxMessageBox( wxT( "Unable to initialize IOKit APIs!" ), wxT("GpuMonEx"), wxICON_EXCLAMATION );
            return ERR_DRVFAIL;
        }
#endif
        
        return 0;
    }
    
    void Uninitialize()
    {
        UninitializeGpuMon();
    }
};

IMPLEMENT_APP( GpuMonEx );
