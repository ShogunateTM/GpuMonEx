#include "../platform.h"
#include "../drvdefs.h"
#include "../common/timer_t.h"
#include "../common/ProcessEnumeration.h"
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
#elif defined(X86_32)
#define CPU_ARCH_MATCH(x) (Is32BitProcess(x))
#endif




/* Process map (pid == key) */
std::unordered_map<DWORD, GMPROCESS> process_map;

/* Dynamic library handle */
HMODULE dlh = nullptr;

/* Mach injection thread (macOS) */
extern "C" void* mac_hook_thread_entry = nullptr;



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
    if( dlh )
        FreeLibrary( dlh );
}


bool perform_hooks( DWORD pid )
{
    if( pid == 0 )
        return false;
    
#ifdef __APPLE__
    mach_error_t err = mach_inject( (mach_inject_entry) mac_hook_thread_entry, NULL, 0, (pid_t) pid, 0 );
    if( err != err_none )
        return false;
#endif
    
    return true;
}




/*
 * Name: main
 * Desc: Program entry point.  Where it all begins.
 *
 * NOTES: This is not to be executed manually.  GpuMonEx will execute this process remotely
 *        and will manage it internally.  We will need a 32-bit and 64-bit version of this
 *        process running to hook processes running for the respective CPU architectures.
 *
 *        We won't initialize a window to keep it in the background.
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
    int error_code = 0;
    timer_t timer;
    
    /* Functions called at exit */
    atexit( at_exit );
    
    /* Load our dynamic link library */
    dlh = LoadLibraryA( GPUMON_DLL );
    if( !dlh )
    {
        std::cerr << "Error loading " << GPUMON_DLL << "!\nError: " << GetLastErrorAsString();
        return -1;
    }
    
#ifdef __APPLE__
    /* Get mach_inject thread function pointer */
    mac_hook_thread_entry = GetProcAddress( dlh, "mac_hook_thread_entry" );
    if( !mac_hook_thread_entry )
    {
        std::cerr << "Could not locate mac_hook_thread_entry!\nError: " << GetLastErrorAsString();
        return -1;
    }
#endif
    
    /* TODO: via program parameters, get the parent process ID (of the GpuMonEx GUI program) and remain
       open until this process is closed */
    
    while( true )
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
                    auto p = process_map[pid];
                    
                    //if( pid != 94259 )
                    //    continue;
                    
                    if( p.dwID == 0 && pid != this_pid && pid != parent_pid )
                    {
                        /* TODO: Detect any APIs that may be utilizing the GPU.  If any of them are found in this process, then
                         add it to the list. */
                        
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
                if( !ProcessIsActive(&p) )
                    it = process_map.erase(it);
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
