#undef UNICODE

#include "../platform.h"
#include "../drvdefs.h"
#include "../common/timer_t.h"
#include "../common/ProcessEnumeration.h"
#include "../shared_data.h"
#include <unordered_map>
#include <sstream>

#ifdef __APPLE__
//extern "C" {
#include <mach_override.h>
#include <mach_inject.h>
#include <xnumem.h>
//}
#endif

/* Determine the right CPU arch before doing anything */
#if defined(X86_64)
#define CPU_ARCH_MATCH(x) (!Is32BitProcess(x))
#define GPUMON_DLL "gpumon64.dll"
#define MINHOOK_MODULE "Minhook.x64.dll"
#elif defined(X86_32)
#define CPU_ARCH_MATCH(x) (Is32BitProcess(x))
#define GPUMON_DLL "gpumon32.dll"
#define MINHOOK_MODULE "Minhook.x86.dll"
#endif




/* Process map (pid == key) */
std::unordered_map<DWORD, GMPROCESS> process_map;

/* Dynamic library handle */
HMODULE dlh = nullptr;

/* Shared Data structure and friends */
SHARED_DATA SharedData;
void* pSharedBufferData = NULL;
HANDLE hSharedDataMap = NULL;

/* Mach injection thread (macOS) */
extern "C" void* mac_hook_thread_entry = nullptr;


BOOL InitializeSharedDataBetweenProcesses();
BOOL UninitializeSharedDataBetweenProcesses();



/*
 * Win32 specific 
 */
#ifdef _WIN32

#define NT_SUCCESS(x) ((x) >= 0)
#define NTAPI  __stdcall
#define NTSTATUS ULONG

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef NTSTATUS (NTAPI* pfnNtCreateThreadEx)
(
    OUT PHANDLE hThread,
    IN ACCESS_MASK DesiredAccess,
    IN PVOID ObjectAttributes,
    IN HANDLE ProcessHandle,
    IN PVOID lpStartAddress,
    IN PVOID lpParameter,
    IN ULONG Flags,
    IN SIZE_T StackZeroBits,
    IN SIZE_T SizeOfStackCommit,
    IN SIZE_T SizeOfStackReserve,
    OUT PVOID lpBytesBuffer
);

typedef NTSTATUS (NTAPI* pfnRtlCreateUserThread)
(
    IN HANDLE ProcessHandle,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    IN BOOLEAN CreateSuspended,
    IN ULONG StackZeroBits OPTIONAL,
    IN SIZE_T StackReserve OPTIONAL,
    IN SIZE_T StackCommit OPTIONAL,
    IN PTHREAD_START_ROUTINE StartAddress,
    IN PVOID Parameter OPTIONAL,
    OUT PHANDLE ThreadHandle OPTIONAL,
    OUT PCLIENT_ID ClientId OPTIONAL
);

#endif




#ifdef _WIN32

/* Win32 equivalent */
#define getpid GetCurrentProcessId

/*
 * Name: getppid
 * Desc: Gets the parent process ID (Windows version of the UNIX equivalent).
 * SOURCE: https://gist.github.com/mattn/253013/d47b90159cf8ffa4d92448614b748aa1d235ebe4
 */
DWORD getppid()
{
    HANDLE hSnapshot;
    PROCESSENTRY32 pe32;
    DWORD ppid = 0, pid = GetCurrentProcessId();

    hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

    __try
    {
        if( hSnapshot == INVALID_HANDLE_VALUE ) 
            __leave;

        ZeroMemory( &pe32, sizeof( pe32 ) );
        pe32.dwSize = sizeof( pe32 );
        if( !Process32First( hSnapshot, &pe32 ) ) 
            __leave;

        do
        {
            if( pe32.th32ProcessID == pid )
            {
                ppid = pe32.th32ParentProcessID;
                break;
            }
        } while( Process32Next( hSnapshot, &pe32 ) );

    }

    __finally
    {
        if( hSnapshot != INVALID_HANDLE_VALUE )
            CloseHandle( hSnapshot );
    }

    return ppid;
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
 * Name: at_exit
 * Desc: Called by atexit() when the program exits, do uninitialization here.
 */
void at_exit()
{
    UninitializeSharedDataBetweenProcesses();

    if( dlh )
        FreeLibrary( dlh );
}


/*
 * Name: QuitSignalReceived
 * Desc: Returns true when the parent program (GpuMonEx.exe) signals for all the child process to quit.
 */
bool QuitSignalReceived()
{
    bool ShutdownThisProcess = false;

#ifdef _WIN32               /* Windows */
#elif defined(__APPLE__)    /* MacOS */
#else                       /* Linux */
#endif

    return ShutdownThisProcess;
}

/*
 * Name: InitializeSharedDataBetweenProcesses
 * Desc: Allocates memory used to share information between the host and target processes we are
 *       trying to inject.  Some data unique to this daemon sometimes need to be shared with the 
 *       "victim".
 */
BOOL InitializeSharedDataBetweenProcesses()
{
#ifdef _WIN32

    /*
     * Get the necessary DLL paths that are specific to the installation.  We will need this because if
     * not, we won't know where to find them otherwise.
     * 
     * TODO: Use the registry key instead?  Probably not, especially if we want this to run from anywhere.
     */

    if( !GetFullPathNameA( MINHOOK_MODULE, MAX_PATH, SharedData.MinHookDllPath, nullptr ) )
        return FALSE;

    if( !GetCurrentDirectoryA( MAX_PATH, SharedData.GpuMonExPath ) )
        return FALSE;

    /* 
     * Create a file mapping object, map the memory to a buffer and write the data to it.
     */

    hSharedDataMap = CreateFileMappingA(
        INVALID_HANDLE_VALUE, 
        NULL,
        PAGE_READWRITE,
        0,
        sizeof( SHARED_DATA ),
        "gpumonproc_shared" );
    if( !hSharedDataMap )
        return FALSE;

    pSharedBufferData = MapViewOfFile( hSharedDataMap, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
    if( !pSharedBufferData )
    {
        CloseHandle( hSharedDataMap );
        return FALSE;
    }

    CopyMemory( pSharedBufferData, &SharedData, sizeof( SHARED_DATA ) );

    return TRUE;
#endif

    return FALSE;
}

BOOL UninitializeSharedDataBetweenProcesses()
{
#ifdef _WIN32
    BOOL error = FALSE;

    if( !UnmapViewOfFile( pSharedBufferData ) )
        error = TRUE;

    CloseHandle( hSharedDataMap );

    return error;
#endif

    return FALSE;
}

/*
 * Name: windows_inject
 * Desc: Attempt to inject the gpumon DLL into the target process
 * Source: https://github.com/Arvanaghi/Windows-DLL-Injector/blob/master/Source/DLL_Injector.c
 */
#ifdef _WIN32
bool windows_inject( DWORD pid )
{
    HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS | SYNCHRONIZE, FALSE, pid );
    if( !hProcess )
        return false;

    // Allocate memory for DLL's path name to remote process
	LPVOID dllPathAddressInRemoteMemory = VirtualAllocEx( hProcess, NULL, strlen( GPUMON_DLL ), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE /*PAGE_READWRITE*/ );
	if( dllPathAddressInRemoteMemory == NULL ) 
    {
		return FALSE;
	}

	// Write DLL's path name to remote process
	BOOL succeededWriting = WriteProcessMemory( hProcess, dllPathAddressInRemoteMemory, GPUMON_DLL, strlen( GPUMON_DLL ), NULL );

	if( !succeededWriting ) 
    {
		return FALSE;
	} 
    else 
    {
		// Returns a pointer to the LoadLibrary address. This will be the same on the remote process as in our current process.
		LPVOID loadLibraryAddress = (LPVOID) GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "LoadLibraryA" );
		if( loadLibraryAddress == NULL ) 
        {
			return FALSE;
		}
        else 
        {
			HANDLE remoteThread = CreateRemoteThread( hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE) loadLibraryAddress, dllPathAddressInRemoteMemory, /*CREATE_SUSPENDED*/ 0, NULL );
			if( remoteThread == NULL )
            {
				return FALSE;
			}
            
            std::string error  = GetLastErrorAsString();

           // ResumeThread( remoteThread );
            auto ret = WaitForSingleObject( remoteThread, INFINITE );

            CloseHandle( hProcess );
		}
	}

    return true;
}
#endif


BOOL InjectModule( DWORD pid )
{
#ifdef _WIN32
    HANDLE ProcessHandle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
    if( ProcessHandle == NULL )
        return FALSE;

    char DllFullPath[MAX_PATH];

    /* If we don't use the FULL path to the DLL we are injecting, it won't work and this whole thing is for nothing! */
    if( !GetFullPathNameA( GPUMON_DLL, MAX_PATH, DllFullPath, nullptr ) )
    {
        CloseHandle( ProcessHandle );
        return FALSE;
    }

    UINT32 DllFullPathLength = ( strlen( DllFullPath ) + 1 );
    PVOID DllFullPathBufferData = VirtualAllocEx( ProcessHandle, NULL, DllFullPathLength, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
    if( DllFullPathBufferData == NULL )
    {
        CloseHandle( ProcessHandle );
        return FALSE;
    }

    SIZE_T ReturnLength;
    BOOL bOK = WriteProcessMemory( ProcessHandle, DllFullPathBufferData, DllFullPath, strlen( DllFullPath ) + 1, &ReturnLength );

    LPTHREAD_START_ROUTINE LoadLibraryAddress = NULL;
    HMODULE Kernel32Module = GetModuleHandleA( "Kernel32" );

    LoadLibraryAddress = (LPTHREAD_START_ROUTINE) GetProcAddress( Kernel32Module, "LoadLibraryA" );
    pfnNtCreateThreadEx NtCreateThreadEx = (pfnNtCreateThreadEx) GetProcAddress( GetModuleHandleA( "ntdll.dll" ), "NtCreateThreadEx" );
    if( NtCreateThreadEx == NULL )
    {
        CloseHandle( ProcessHandle );
        return FALSE;
    }

    HANDLE ThreadHandle = NULL;
    NtCreateThreadEx( &ThreadHandle, 0x1FFFF, NULL, ProcessHandle, (LPTHREAD_START_ROUTINE) LoadLibraryAddress, DllFullPathBufferData, FALSE, NULL, NULL, NULL, NULL );
    //ThreadHandle = CreateRemoteThread( ProcessHandle, NULL, NULL, (LPTHREAD_START_ROUTINE) LoadLibraryAddress, DllFullPathBufferData, /*CREATE_SUSPENDED*/ 0, NULL );
    if( ThreadHandle == NULL )
    {
        CloseHandle( ProcessHandle );
        return FALSE;
    }

    std::string error = GetLastErrorAsString();

    WaitForSingleObjectEx( ThreadHandle, INFINITE, FALSE );

    CloseHandle( ProcessHandle );
    CloseHandle( ThreadHandle );

    return TRUE;
#elif defined(__APPLE__)
#else
#endif

    return FALSE;
}

BOOL UndoInjectModule( DWORD pid )
{
#ifdef _WIN32   /* Windows desktop */
    BOOL bMore = FALSE, bFound = FALSE;
    HANDLE hSnapshot;
    HMODULE hModule = NULL;
    MODULEENTRY32 me = { sizeof(me) };
    BOOL bSuccess = FALSE;

    hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, pid );
    bMore = Module32First( hSnapshot, &me );
    for(; bMore; bMore = Module32Next( hSnapshot, &me ) ) 
    {
        if( !_stricmp( (LPCTSTR)me.szModule, GPUMON_DLL ) || !_stricmp( (LPCTSTR)me.szExePath, GPUMON_DLL ) )
        {
            bFound = TRUE;
            break;
        }
    }
    if( !bFound ) 
    {
        CloseHandle( hSnapshot );
        return FALSE;
    }

    HANDLE ProcessHandle = NULL;

    ProcessHandle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );

    if( ProcessHandle == NULL )
    {
        return FALSE;
    }

    LPTHREAD_START_ROUTINE FreeLibraryAddress = NULL;
    HMODULE Kernel32Module = GetModuleHandleA( "Kernel32" );

    FreeLibraryAddress = (LPTHREAD_START_ROUTINE) GetProcAddress( Kernel32Module, "FreeLibrary" );
    pfnNtCreateThreadEx NtCreateThreadEx = (pfnNtCreateThreadEx) GetProcAddress( GetModuleHandleA( "ntdll.dll" ), "NtCreateThreadEx" );
    if( NtCreateThreadEx == NULL )
    {
        CloseHandle( ProcessHandle );
        return FALSE;
    }

    HANDLE ThreadHandle = NULL;

    NtCreateThreadEx( &ThreadHandle, 0x1FFFFF, NULL, ProcessHandle, (LPTHREAD_START_ROUTINE) FreeLibraryAddress, me.modBaseAddr, FALSE, NULL, NULL, NULL, NULL );
    if( ThreadHandle == NULL )
    {
        CloseHandle(ProcessHandle);
        return FALSE;
    }
    if( WaitForSingleObject( ThreadHandle, INFINITE ) == WAIT_FAILED )
    {
        return FALSE;
    }

    CloseHandle( ProcessHandle );
    CloseHandle( ThreadHandle );

    return TRUE;
#elif defined(__APPLE__)    /* macOS */
#else   /* Linux */
#endif
}

bool iequals( const std::string& a, const std::string& b )
{
    unsigned int sz = a.size();
    if( b.size() != sz )
        return false;
    for( unsigned int i = 0; i < sz; ++i )
        if( tolower( a[i] ) != tolower( b[i] ) )
            return false;
    return true;
}

/*
* Name: ContainsGpuRelatedDependencies
* Desc: Searches the list of dependencies for this process.  If there's any external library that contains
*       APIs that manipulate the GPU (i.e. Direct3D, OpenGL, Vulkan, etc.), then we will attempt to hook it.
*/
bool ContainsGpuRelatedDependencies( DWORD pid, DWORD* out_flags )
{
    struct dependency_t
    {
        std::string module;
        DWORD flag;
    };

    struct dependency_t target_dependencies[] 
#ifdef _WIN32
        = { { "ddraw.dll",      GMPROCESS_USES_DDRAW },
            { "d3d8.dll",       GMPROCESS_USES_D3D8 },
            { "d3d9.dll",       GMPROCESS_USES_D3D9 },
            { "d3d9ex.dll",     GMPROCESS_USES_D3D9 },
            { "d3d10.dll",      GMPROCESS_USES_D3D10 },
            { "d3d11.dll",      GMPROCESS_USES_D3D11 },
            { "d3d12.dll",      GMPROCESS_USES_D3D12 },
            { "opengl32.dll",   GMPROCESS_USES_OPENGL } }
#endif
    ;

    std::vector<std::string> dependencies;
    bool dependency_found = false;
    
    /* Get the list of dependencies, and then compare it to the list of target dependencies. If we find
       a module we desire to hook, go ahead add the flag so we can hook it. */
    if( GetProcessDependencies( pid, dependencies, TRUE ) )
    {
        for( auto d = dependencies.begin(); d != dependencies.end(); ++d )
        {
            for( int i = 0; i < 8; i++ )
            {
                if( iequals( (*d), target_dependencies[i].module ) )
                {
                    *out_flags |= target_dependencies[i].flag;
                    dependency_found = true;
                }
            }
        }
    }

    return dependency_found;
}


bool perform_hooks( DWORD pid )
{
    if( pid == 0 )
        return false;

#ifdef _WIN32
    auto noerr = InjectModule( pid ); //windows_inject( pid );
    if( !noerr )
    {
        OutputDebugStringA( GetLastErrorAsString().c_str() );
        return false;
    }
#endif
    
#ifdef __APPLE__
    mach_error_t err = mach_inject( (mach_inject_entry) mac_hook_thread_entry, NULL, 0, (pid_t) pid, 0 );
    if( err != err_none )
        return false;
#endif
    
    return true;
}

bool undo_hooks( DWORD pid )
{
    if( pid == 0 )
        return false;

#ifdef _WIN32
    auto noerr = UndoInjectModule( pid );
    if( !noerr )
    {
        OutputDebugStringA( GetLastErrorAsString().c_str() );
        return false;
    }
#endif

    return true;
}


/*
 * Name: main
 * Desc: Program entry point.  Where it all begins.
 *
 * NOTES: This is not to be executed manually (*).  GpuMonEx will execute this process remotely
 *        and will manage it internally.  We will need a 32-bit and 64-bit version of this
 *        process running to hook processes running for the respective CPU architectures.
 *
 *        We won't initialize a window to keep it in the background.
 * 
 *        (*) For debugging purposes, we'll allow command line usage to target a specific process
 *        rather than inject into every process if necessary to avoid it.
 *
 *       Windows: We need this process to run in the background yet portable, so we still ned
 *       to call the WinMain entry point.  We then call int main after converting the parameters
 *       to the appropriate format.
 *
 *       MacOS and Linux: We can still use the standard int main here.
 */
int main( int argc, char** argv )
{
    DWORD parent_pid = getppid();
    DWORD this_pid = getpid();
    DWORD target_pid = -1;
    int error_code = 0;
    timer_t timer;
    
    /* Are we targeting a specific process? */
    if( argc == 2 )
        target_pid = atoi( argv[1] );

    /* Functions called at exit */
    atexit( at_exit );

#ifdef _WIN32
    /* Enable debug privileges */
    if( !EnableDebugPrivileges() )
        OutputDebugStringA( GetLastErrorAsString().c_str() );
#endif
    
    InitializeSharedDataBetweenProcesses();
    
#ifdef __APPLE__
    /* Get mach_inject thread function pointer */
    mac_hook_thread_entry = GetProcAddress( dlh, "mac_hook_thread_entry" );
    if( !mac_hook_thread_entry )
    {
        std::cerr << "Could not locate mac_hook_thread_entry!\nError: " << GetLastErrorAsString();
        return -1;
    }
#endif
    
    process_map.clear();

    /* TODO: via program parameters, get the parent process ID (of the GpuMonEx GUI program) and remain
       open until this process is closed */
    
    while( !QuitSignalReceived() )
    {
        GMPROCESS* processes = nullptr;
        int process_count = 0;
        
        timer.start();
        
        /* Attempt to get a list of currently active processes */
        if( EnumerateProcesses( &processes, &process_count ) )
        {
            /* Interate through this list of existing processes */
            for( int i = 0; i < process_count; i++ )
            {
                DWORD pid = processes[i].dwID;
                
                if( CPU_ARCH_MATCH( pid ) )
                {
                    /* Do we already have this process in the list? */
                    /*auto p = process_map[pid];
                    if( p.hProcess != nullptr )
                        continue;*/

                    std::unordered_map<DWORD, GMPROCESS>::iterator it = process_map.find(pid);
                    if( it != process_map.end() )
                        continue;

                    /* Are we targeting only ONE specific process? */
                    if( target_pid != -1 )
                    {
                        if( pid != target_pid )
                            continue;
                    }

                    auto p = process_map[pid];

                    if( p.dwID == 0 && pid != this_pid && pid != parent_pid )
                    {
                        /* TODO: Detect any APIs that may be utilizing the GPU.  If any of them are found in this process, then
                         add it to the list. */
                        DWORD flags = 0;
                        ContainsGpuRelatedDependencies( pid, &flags );

                        if( perform_hooks( pid ) )
                        {
                            p.dwID = processes[i].dwID;
                            p.hThread = processes[i].hThread;
                            p.hProcess = processes[i].hProcess;
                            process_map[pid] = p;
                        }
                    }
                }
            }
            
            delete [] processes;
            
            /* Search list for dead processes */
            auto it = process_map.begin();
            while( it != process_map.end() )
            {
                auto p = it->second;
                if( !ProcessIsActive(&p) && p.dwID != 0 )
                {
                    //undo_hooks( p.dwID );
                    it = process_map.erase(it);
                }
                else
                    it++;
            }
        }
        
        timer.stop();
        
        std::cout << "Elapsed time: " << timer.elapsed() << " ms\n";
        std::cout << "Process count: " << process_map.size() << std::endl;
        
        /* TODO: Deltas based on execution time? */
        Sleep(1000);
    }
    
    /* 
     * We need to undo all the hooks after this process is signaled to close.  If not, then
     * we will not be able to re-hook any processes that were already hooked until they are 
     * closed and re-opened.
     */

    auto it = process_map.begin();
    while( it != process_map.end() )
    {
        auto p = it->second;
        undo_hooks( p.dwID );
        it = process_map.erase(it);
    }

    return error_code;
}

#ifdef _WIN32

/*
 * Name: WinMain
 * Desc: Windows program entry point.  Simply calls the int main implementation above.
 */
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    return main( __argc, __argv );
}

#endif
