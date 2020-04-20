#include "../platform.h"
#include "../common/ProcessEnumeration.h"
#include <unordered_map>


/* Determine the right CPU arch before doing anything */
#if defined(X86_64)
#define CPU_ARCH_MATCH(x) (!Is32BitProcess(x))
#elif defined(X86_32)
#define CPU_ARCH_MATCH(x) (Is32BitProcess(x))
#endif


std::unordered_map<DWORD, GMPROCESS> process_map;




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
    DWORD parent_pid = 0;
    int error_code = 0;
    
    /* TODO: via program parameters, get the parent process ID (of the GpuMonEx GUI program) and remain
       open until this process is closed */
    
    while( true )
    {
        GMPROCESS* processes = nullptr;
        int process_count = 0;
        
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
                    
                    if( p.dwID == 0 )
                    {
                        /* TODO: Detect any APIs that may be utilizing the GPU.  If any of them are found in this process, then
                         add it to the list. */
                        
                        p.dwID = processes[i].dwID;
                        p.hThread = processes[i].hThread;
                        p.hProcess = processes[i].hProcess;
                        process_map[pid] = p;
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
