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


#ifdef _WIN32
/* This will get moved later... */
#define _ComPtr(_interface)     _com_ptr_t<_com_IIID<_interface, &__uuidof(_interface)>>

/* A cheap way of ensuring that we don't accidentally uninitalize COM if it's somehow already in use
* somewhere else within this software. 
*/
class ComGuard
{
public:
    ComGuard() : hr(E_FAIL) 
    {
        hr = CoInitialize( nullptr );
    };

    ComGuard(UINT Flags)
    {
        hr = CoInitializeEx( nullptr, Flags );
    }

    ~ComGuard()
    {
        if( hr != CO_E_ALREADYINITIALIZED || SUCCEEDED( hr ) )
            CoUninitialize();
    }

    HRESULT hr;
};
#endif

bool GetDriverVersion( std::string& strVersionNumber, int GpuNumber )
{
    /*
    * COM, and the appropriate security flags need to be set in order for this method to work. 
    */

    ComGuard guard = ComGuard( COINIT_MULTITHREADED );
//    if( FAILED( guard.hr ) )
//        return false;

    auto hr = CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, 
        RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL );
    if( FAILED( hr ) )
        return false;

    /* Initialize WMI */
    _ComPtr(IWbemLocator) pLoc;
    hr = CoCreateInstance( CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*) &pLoc );
    if( FAILED( hr ) )
        return false;

    _ComPtr(IWbemServices) pSvc;
    hr = pLoc->ConnectServer( _bstr_t( L"ROOT\\CIMV2" ), NULL, NULL, 0, NULL, 0, 0, &pSvc );
    if( FAILED( hr ) )
        return false;

    hr = CoSetProxyBlanket( pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, 
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );
    if( FAILED( hr ) )
        return false;

    _ComPtr(IEnumWbemClassObject) pEnumerator;
    hr = pSvc->ExecQuery( bstr_t("WQL"), bstr_t("SELECT * FROM Win32_VideoController"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator );
    if( FAILED( hr ) )
        return false;

    /* Now we're actually going to get the data we came here for. 
    TODO: Enumerate the actual adapter we want to use? */

    IWbemClassObject* pclsObj = nullptr;
    ULONG Return = 0;
    int i = 0;

    while( pEnumerator )
    {
        hr = pEnumerator->Next( WBEM_INFINITE, 1, &pclsObj, &Return );

        if( !Return )
            break;

        if( i++ != GpuNumber )
        {
            pclsObj->Release();
            continue;
        }

        VARIANT vtProp;

        hr = pclsObj->Get( L"DriverVersion", 0, &vtProp, 0, 0 );

        std::wstring wstr( vtProp.bstrVal );
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
        strVersionNumber = cv.to_bytes( wstr );

        VariantClear( &vtProp );

        pclsObj->Release();
    }

    /* Cleanup should be automatic */

    return true;
}

/*
 * Name: GetOSVersionString
 * Desc: Returns the OS name and version.
 */
std::string GetOSVersionString()
{
    std::stringstream osver;

#if defined(_WIN32) /* Windows */

    /* Get the build version */
    const auto system = L"kernel32.dll";
    DWORD dummy;
    const auto cbInfo = ::GetFileVersionInfoSizeExW( FILE_VER_GET_NEUTRAL, system, &dummy );
    
    std::vector<char> buffer( cbInfo );
    ::GetFileVersionInfoExW( FILE_VER_GET_NEUTRAL, system, dummy, buffer.size(), &buffer[0] );

    void* p = nullptr;
    UINT size = 0;
    ::VerQueryValueW( buffer.data(), L"\\", &p, &size );

    assert( size >= sizeof( VS_FIXEDFILEINFO ) );
    assert( p != nullptr );

    auto fixed = static_cast<const VS_FIXEDFILEINFO*>(p);

    /* Determine the OS name (i.e. Win7, Server 2008, etc.) */

    std::stringstream ss;

    ss << HIWORD(fixed->dwFileVersionMS) << '.'
        << LOWORD(fixed->dwFileVersionMS) << '.'
        << HIWORD(fixed->dwFileVersionLS) << '.'
        << LOWORD(fixed->dwFileVersionLS);

    return ss.str();

#elif defined(__APPLE__) /* MacOS */
#else /* Linux */
#endif
}

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

    std::string strv;
    GetOSVersionString();
    GetDriverVersion( strv, 0 );
    
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
