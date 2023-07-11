// dllmain.cpp : Defines the entry point for the DLL application.
#include "../platform.h"
#include "../common/ProcessEnumeration.h"
#include "APIHook.h"

#include <processthreadsapi.h>
#include <atomic>


std::atomic<bool> unhooking(false);
HMODULE hThis = nullptr;

//Foo foo;

#if 0 /* Can cause race conditions, forget it... */
typedef BOOL (WINAPI *PFreeLibrary)( _In_ HMODULE hModule );  
typedef VOID (WINAPI *PExitThread)( _In_ DWORD dwExitCode );
typedef unsigned int (WINAPI *PTHREADPROC)( LPVOID lParam );  
 
typedef struct _DLLUNLOADINFO
{  
	PFreeLibrary	m_fpFreeLibrary;  
	PExitThread		m_fpExitThread;  
	HMODULE		    m_hFreeModule;   
}DLLUNLOADINFO, *PDLLUNLOADINFO;


/*
 * Name: DllUnloadThreadProc
 * Desc: Self explanitory
 * Source: https://www.unknowncheats.me/forum/c-and-c-/146497-release-source-self-unloading-dll.html
 */
unsigned int WINAPI DllUnloadThreadProc( LPVOID lParam )  
{  
	PDLLUNLOADINFO pDllUnloadInfo = (PDLLUNLOADINFO)lParam ;  
 
	//  
	// FreeLibrary dll  
	//  
	( pDllUnloadInfo->m_fpFreeLibrary )( pDllUnloadInfo->m_hFreeModule );  
 
	//
	// Exit Thread
	// This thread return value is freed memory.
	// So you don't have to return this thread.
	//
	pDllUnloadInfo->m_fpExitThread( 0 );
    return 0;  
}  
 

/*
 * Name: DllSelfUnloading
 * Desc: We call this if the DLL has no supported APIs that access the GPU so we don't have an instance of this DLL in the existing process.
 * Source: https://www.unknowncheats.me/forum/c-and-c-/146497-release-source-self-unloading-dll.html
 */
VOID DllSelfUnloading( _In_ const HMODULE hModule )  
{  
	PVOID pMemory = NULL ;  
	ULONG ulFuncSize ;  
	unsigned int uintThreadId = 0 ;  
 
	pMemory = VirtualAlloc( NULL, 0x1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE );  
	if( pMemory != NULL )  
	{  
		ulFuncSize = (ULONG_PTR)DllUnloadThreadProc - (ULONG_PTR)DllSelfUnloading ;  
		if( ( ulFuncSize >> 31 ) & 0x01 )  
		{
			ulFuncSize = (ULONG_PTR)DllSelfUnloading - (ULONG_PTR)DllUnloadThreadProc ;  
		}
 
		memcpy( pMemory, DllUnloadThreadProc, ulFuncSize ) ;  
 
		( (PDLLUNLOADINFO)( ( (ULONG_PTR)pMemory ) + 0x500 ) )->m_fpFreeLibrary =   
			(PFreeLibrary)GetProcAddress( GetModuleHandle( L"kernel32.dll"  ), "FreeLibrary" ) ;  
 
		( (PDLLUNLOADINFO)( ( (ULONG_PTR)pMemory ) + 0x500 ) )->m_fpExitThread =   
			(PExitThread)GetProcAddress( GetModuleHandle( L"kernel32.dll" ), "ExitThread" ) ;  
 
		( (PDLLUNLOADINFO)( ( (ULONG_PTR)pMemory ) + 0x500 ) )->m_hFreeModule = hModule ;  
 
		_beginthreadex( NULL, 0, (PTHREADPROC)pMemory, (PVOID)( ( (ULONG_PTR)pMemory ) + 0x500 ), 0, &uintThreadId ) ;  
	}  
}  
#endif


/* Strip the filename from a process's full path */
std::string GetFileName( std::string filePath, bool withExtension = true, char seperator = '/' )
{
    // Get last dot position
    std::size_t dotPos = filePath.rfind('.');
    std::size_t sepPos = filePath.rfind(seperator);

    if( sepPos != std::string::npos )
        return filePath.substr( sepPos + 1, ( withExtension ? filePath.size() : dotPos - 1 ) - sepPos ); 

    return "";
}

/*
 * Name: DoNotHook
 * Desc: Returns TRUE if the current running process is part of the GpuMonEx software package (i.e. GpuMonEx.exe, gpumonproc.exe, gpucmd.exe, etc.)
 *       Returns FALSE if this is anything else that is running.
 */
BOOL DoNotHook()
{
    std::string strProcessPath;

    if( GetCurrentProcessName( strProcessPath ) )
    {
        std::string strProcessName = GetFileName( strProcessPath, false, '\\' );

        if( strProcessName != "GpuMonEx" && strProcessName != "fraps" &&
            strProcessName != "gpucmd" && strProcessName != "gpucmd64" &&
            strProcessName != "gpumonproc32" && strProcessName != "gpumonproc64" &&
            strProcessName != "gpumonproc32d" && strProcessName != "gpumonproc64d" )
        {
            return FALSE;
        }
    }

    return TRUE;
}

void Initialize( void* param )
{
# if 0 /* 
        * TODO: This is only supported on Win10 and later.  Yes, 8.x and earlier are not supported by M$ anymore, but idc.
        *       Either use the alternative solution below for OSes that don't support it, detect the appropriate function to use based on the OS, or don't use it at all.
        * Source(s): https://stackoverflow.com/questions/36543301/detecting-windows-10-version/36545162#36545162
        *            https://stackoverflow.com/questions/905876/how-to-set-name-to-a-win32-thread
        */
    SetThreadDescription( GetCurrentThread(), L"Hook init thread" );
#endif
    EnableMinHookAPI(), _endthread();
}

void Uninitialize( void* param )
{
    DisableMinHookAPI(), _endthread();
}

unsigned __stdcall UninitializeEx( void* param )
{
    DisableMinHookAPI(), _endthreadex(0);
    return 0;
}

extern "C" BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    //__asm int 3;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
       // MessageBoxA( NULL, "Initiating hook..", "HOOK!", MB_OK );   // For a while, this wasn't getting called, and now it is all of a sudden.
                                                                    // Gee, I wonder what changed...
        /* Avoid hooking ourselves */
        if( DoNotHook() )
            break;
        //{
            //FreeLibrary( hModule );
        //}

        hThis = hModule;

        DisableThreadLibraryCalls( hModule );

        /* Enable API hooking */
        //EnableMinHookAPI();
        _beginthread( Initialize, 0, NULL );

        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        /* Uninitialize MinHook and stuff before we go */
       //MessageBoxA( NULL, "Initiating UN-hook..", "HOOK!", MB_OK );
        if( !DoNotHook() )
        {
            //DisableThreadLibraryCalls( hModule );

            //unsigned int thread_id = 0;
            //HANDLE hThread = (HANDLE) _beginthreadex( NULL, 0, UninitializeEx, NULL, 0, &thread_id );//_beginthread( Uninitialize, 0, NULL );
            //WaitForSingleObject( hThread, INFINITE );
            unhooking = true;
            DisableMinHookAPI();
            unhooking = false;
            //FreeLibrary( hThis );
            //FreeLibraryAndExitThread( hThis, 0 );
        }
        break;
    }
    return TRUE;
}

