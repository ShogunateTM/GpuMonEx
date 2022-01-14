#include "../platform.h"
#include "ProcessEnumeration.h"

#pragma warning (disable:4996)


/*
 * Globals
 */

#ifdef _WIN32
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)( HANDLE, PBOOL );

LPFN_ISWOW64PROCESS pfnIsWow64Process;

BOOL DebugPrivilegesEnabled = FALSE;
#endif



/*
 * Windows only
 */

#ifdef _WIN32
/*
 * Name: IsWow64
 * Desc: Determines if the process calling this function is either 32-bit or 64-bit.
 *       Returns TRUE if this is a Wow64 process (32-bit), FALSE if 64-bit.
 */
BOOL IsWow64( void )
{
    BOOL bIsWow64 = FALSE;

    /* If we cannot locate the function IsWow64Process within kernel32.dll, then it's
       generally safe to assume that this is a 32-bit process/OS */

    HMODULE kernel32 = GetModuleHandle( TEXT( "kernel32" ) );
    if( !kernel32 )
        return TRUE;

    pfnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress( kernel32, "IsWow64Process" );

    if( !pfnIsWow64Process )
        return FALSE;   /* Function not found/supported */

    if( !pfnIsWow64Process( GetCurrentProcess(), &bIsWow64 ) )
    {
        /* TODO */
    }

    return bIsWow64;
}


/*
 * Name: EnableDebugPrivileges
 * Desc: Enables the process to use the PROCESS_ALL_ACCESS flag in OpenProcess()
 */
BOOL EnableDebugPrivileges()
{
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tkp;

    if( !OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
        return FALSE;

    LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &luid );
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = luid;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL ret = AdjustTokenPrivileges( hToken, FALSE, &tkp, sizeof( tkp ), NULL, NULL );

    CloseHandle( hToken );

    return ret;
}
#endif


/*
 * macOS only
 */
#ifdef __APPLE__
/*
 * Name: GetBSDProcessList
 * Desc: Gets a list of all actively running process, including Carbon, Cocoa, and Classic.  Even includes
 *       non-application (daemon) processes.
 *
 * SOURCE: https://developer.apple.com/library/archive/qa/qa2001/qa1123.html
 */
int GetBSDProcessList( kinfo_proc** proc_list, size_t* proc_count )
{
    int err = 0;
    kinfo_proc* result = NULL;
    bool done = false;
    static const int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    size_t length = 0;
    
    assert( proc_list != NULL );
    assert( *proc_list == NULL );
    assert( proc_count != NULL );
    
    *proc_count = 0;
    
    do
    {
        assert( result == NULL );
        
        err = sysctl( (int*) name, ( sizeof( name ) / sizeof( *name ) ) - 1, NULL, &length, NULL, 0 );
        if( err == -1 )
            err = errno;
        
        if( !err )
        {
            result = (kinfo_proc*) malloc( length );
            if( result == NULL )
                err = ENOMEM;
        }
        
        if( !err )
        {
            err = sysctl( (int*) name, ( sizeof( name ) / sizeof( *name ) ) - 1, result, &length, NULL, 0 );
            if( err == -1 )
                err = errno;
            if( err == 0 )
                done = true;
            else if( err == ENOMEM )
            {
                assert( result != NULL );
                free( result );
                result = NULL;
                err = 0;
            }
        }
    } while( err == 0 && !done );
    
    if( err != 0 && result != NULL )
    {
        free( result );
        result = NULL;
    }
    
    *proc_list = result;
    
    if( err == 0 )
        *proc_count = length / sizeof( kinfo_proc );
    
    assert( (err == 0) == (*proc_list != NULL) );
    
    return err;
}

/*
 * This was a lazy copy-pasta from below...
 * SOURCES: http://osxbook.com/book/bonus/chapter8/core/download/gcore.c
 *          https://stackoverflow.com/questions/7983962/is-there-a-way-to-check-if-process-is-64-bit-or-32-bit/27929872#27929872
 */
static int
B_get_process_info(pid_t pid, struct kinfo_proc *kp)
{
    size_t bufsize      = 0;
    size_t orig_bufsize = 0;
    int    retry_count  = 0;
    int    local_error  = 0;
    int    mib[4]       = { CTL_KERN, KERN_PROC, KERN_PROC_PID, 0 };
    
    mib[3] = pid;
    orig_bufsize = bufsize = sizeof(struct kinfo_proc);
    
    for (retry_count = 0; ; retry_count++) {
        local_error = 0;
        bufsize = orig_bufsize;
        if ((local_error = sysctl(mib, 4, kp, &bufsize, NULL, 0)) < 0) {
            if (retry_count < 1000) {
                sleep(1);
                continue;
            }
            return local_error;
        } else if (local_error == 0) {
            break;
        }
        sleep(1);
    }
    
    return local_error;
}
#endif


/*
 * Name: CreateNewProcess
 * Desc: Launches a new process from an executable file on disk.
 */
bool CreateNewProcess( const CHAR* szProcessName, GMPROCESS* pProcess )
{
#if _WIN32
    if( !DebugPrivilegesEnabled )
        DebugPrivilegesEnabled = EnableDebugPrivileges();

#if 0
    /* Normally we would use CreateProcess but that would return both a process and a thread
       handle to manage.  With ShellExecuteEx, there's only the handle to the process to worry
       about.  Plus, you can either wait for the process to exit, or end it yourself. 
       
       If using ShellExecuteEx becomes a problem, then we can change it if necessary */

    size_t cSize = strlen( szProcessName );
    std::wstring wc( cSize, L'#' );
    mbstowcs( &wc[0], szProcessName, cSize );

    wchar_t drive[_MAX_DRIVE], dir[_MAX_DIR], file[_MAX_FNAME], ext[_MAX_EXT];
    _wsplitpath( wc.c_str(), drive, dir, file, ext );

    std::wstring fn = file; fn.append(ext);
    std::wstring param = L"/k cd /d "; param.append( drive ); param.append( dir );

    SHELLEXECUTEINFO sei;
    ::ZeroMemory( &sei, sizeof( sei ) );
    sei.cbSize = sizeof( sei );
    sei.lpVerb = L"open";
    sei.lpFile = wc.c_str(); //wc.c_str();
    sei.lpParameters = param.c_str();
    sei.nShow = SW_SHOW;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    if( !ShellExecuteEx( &sei ) )
        return NULL;

    /* TODO: Since it doesn't seem like we can set the access permissions (that I know of),
       See if it can be changed using OpenProcess() ... */

    hProcess = (void*) sei.hProcess;
#else
    /* Before starting a new process, get the directory first */
    char drive[_MAX_DRIVE], dir[_MAX_DIR], file[_MAX_FNAME], ext[_MAX_EXT];
    _splitpath( szProcessName, drive, dir, file, ext );
    std::string pdir = drive; pdir.append(dir);

    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFOA        si = { 0 };

    si.cb = sizeof( si );
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;

    BOOL ret = CreateProcessA( szProcessName,
                NULL,           /* TODO: User params */
                NULL, NULL,
                TRUE,           /* Do we need to inherit handles or no? */
                0,              /* Probably should set some flags... */
                NULL,
                pdir.c_str(),   /* Set current directory to .exe location */
                &si,
                &pi );

    pProcess->hProcess = pi.hProcess;
    pProcess->hThread = pi.hThread;
    pProcess->dwID = GetProcessId( pi.hProcess );
#endif

#endif

    return true;
}


/*
 * Name: OpenProcessByName
 * Desc: Attempts to open any process with the given name.
 *
 * NOTES: See notes below per OS.
 *  
 *  Windows: This works, but with a couple of flaws.
 *      1. There can be MANY processes with the same name, this code will just
 *         give you the first process it finds with the given name, if it exists.
 *
 *      2. This code pulls from a snapshot of processes, and that snapshot could be
 *         out of date at the time it was called, so there's race condition.  See link below:
 *         https://stackoverflow.com/questions/865152/how-can-i-get-a-process-handle-by-its-name-in-c
 */
bool OpenProcessByName( const CHAR* szProcessName, GMPROCESS* pProcess )
{
#ifdef _WIN32
    if( !DebugPrivilegesEnabled )
        DebugPrivilegesEnabled = EnableDebugPrivileges();

    DWORD dwProcessID = 0;
    HANDLE Snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    PROCESSENTRY32 Process;
    ZeroMemory( &Process, sizeof( Process ) );
    Process.dwSize = sizeof( Process );

    if( Process32First( Snapshot, &Process ) )
    {
        size_t cSize = strlen( szProcessName );
        
        do
        {
            std::wstring wc( cSize, L'#' );
            mbstowcs( &wc[0], szProcessName, cSize );

            if( std::wstring( Process.szExeFile ) == wc )
            {
                dwProcessID = Process.th32ProcessID;
                break;
            }
        } while( Process32Next( Snapshot, &Process ) );
    }
    else
    {
        DWORD error = GetLastError();
    }

    CloseHandle( Snapshot );

    if( dwProcessID != 0 )
    {
        pProcess->hProcess = (HANDLE)OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwProcessID );
        if( !pProcess->hProcess )
            pProcess->hProcess = (HANDLE)OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID );

        pProcess->dwID = dwProcessID;
        pProcess->hThread = NULL;
    }
#endif

    return true;
}


/*
 * Name: OpenProcessByID
 * Desc: Opens a given process using the process's ID number.
 */
bool OpenProcessByID( DWORD dwProcessID, GMPROCESS* pProcess )
{
#ifdef _WIN32
    if( !DebugPrivilegesEnabled )
        DebugPrivilegesEnabled = EnableDebugPrivileges();

    /* Attempt to open this process with all access.  If we fail, use read-only flags */
    pProcess->hProcess = (HANDLE) OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwProcessID );
    if( !pProcess->hProcess )
        pProcess->hProcess = (HANDLE) OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID );

    pProcess->dwID = GetProcessId( pProcess->hProcess );
    pProcess->hThread = NULL;
#endif
    
#ifdef __APPLE__
    struct kinfo_proc kp;
    
    if( !B_get_process_info( dwProcessID, &kp ) )
    {
        pProcess->dwID = dwProcessID;
        pProcess->hThread = nullptr;
        pProcess->hProcess = nullptr; /* TODO: Stop being lazy... */
    }
    else
        return false;
#endif

    return true;
}


/*
 * Name: CloseProcess
 * Desc: Closes the process handle and the thread (if a handle to that exists).
 */
void CloseProcess( GMPROCESS* pProcess )
{
    if( !pProcess )
        return;

#if _WIN32
    if( pProcess->hThread )
        CloseHandle( pProcess->hThread );
    if( pProcess->hProcess )
        CloseHandle( pProcess->hProcess );
#endif
}


/*
 * Name: TerminateProcess
 * Desc: Closes the process handle and the thread (if a handle to that exists).
 */
void TerminateProcess( GMPROCESS* pProcess )
{
    if( !pProcess )
        return;

#if _WIN32
    if( pProcess->hThread )
        CloseHandle( pProcess->hThread );
    if( pProcess->hProcess )
        TerminateProcess( pProcess->hProcess, 0 );
#endif
}


/*
 * Name: Is32BitProcess
 * Desc: Returns true if this process is running in 32-bit mode.
 */
bool Is32BitProcess( DWORD dwProcessID )
{
#ifdef _WIN32
    HANDLE hProcess = (HANDLE) OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID );
    BOOL bIsWow = TRUE;
    BOOL ret;

    if( hProcess )
    {
        ret = IsWow64Process( hProcess, &bIsWow );
        CloseHandle( hProcess );

        if( !ret )
            return false;
    }

    return bIsWow == TRUE;
#endif
    
#ifdef __APPLE__
    /* TODO: This is not very efficient just to query one little flag.  Use the GMPROCESS structure instead. */
    struct kinfo_proc kp;
    
    if( !B_get_process_info( dwProcessID, &kp ) )
    {
        /* If this flag is set, then this process is 64-bit */
        if( kp.kp_proc.p_flag & P_LP64 )
            return false;
    }
#endif

    return true;
}


/*
 * Name: ProcessIsActive
 * Desc: Returns true if the process handle is valid and running.
 */
bool ProcessIsActive( GMPROCESS* pProcess )
{
#ifdef _WIN32
    DWORD dwResult = WaitForSingleObject( pProcess->hProcess, 0 );

    return ( dwResult == WAIT_TIMEOUT );
#endif
    
#ifdef __APPLE__
    /* Call kill() with 0 as the signal so we can do error checking without actually killing the process */
    if( !kill( pProcess->dwID, 0 ) )
        return true;    /* Check errno for actual error */
#endif

    return false;
}


/*
 * Name: EnumerateProcesses
 * Desc: Returns a list of processes actively running on this machine.
 */
bool EnumerateProcesses( GMPROCESS** ppProcesses, int* ProcessCount )
{
#ifdef _WIN32
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if( EnumProcesses( aProcesses, sizeof( aProcesses ), &cbNeeded ) )
    {
        cProcesses = cbNeeded / sizeof(DWORD);
        *ProcessCount = (int) cProcesses;
    
        (*ppProcesses) = new GMPROCESS[cProcesses];

        for( i = 0; i < cProcesses; i++ )
        {
            (*ppProcesses)[i].dwID = aProcesses[i];
            (*ppProcesses)[i].hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, FALSE, aProcesses[i] );
            (*ppProcesses)[i].hThread = nullptr;
        }

        return true;
    }
   
#endif

    /* 
     * macOS 
     */
#ifdef __APPLE__
    struct kinfo_proc* plist = nullptr;
    size_t count = 0;
    
    /* Remember to deallocate the list that was returned if this function succeeds */
    if( !GetBSDProcessList( &plist, &count ) )
    {
        (*ppProcesses) = new GMPROCESS[count];
        *ProcessCount = (int) count;
        
        for( int i = 0; i < count; i++ )
        {
            (*ppProcesses)[i].dwID = plist[i].kp_proc.p_pid;
            (*ppProcesses)[i].hThread = nullptr;
            (*ppProcesses)[i].hProcess = nullptr; /* I'll probably fill that in later... */
        }
        
        free( plist );
        return true;
    }
    
#endif
    
    return false;
}

/*
 * Name: GetCurrentProcessName
 * Desc: Returns the full path and name of the currently running process.
 */
bool GetCurrentProcessName( std::string& strProcessName )
{
#ifdef _WIN32
    char strBuf[MAX_PATH];

    if( !GetModuleFileNameExA( GetCurrentProcess(), NULL, strBuf, MAX_PATH ) )
        return false;   // TODO: GetLastError()

    strProcessName = strBuf;

    return true;
#endif

    // TODO: MacOS and Linux

    return false;
}


/*
 * Name: GetProcessDependencies
 * Desc: Returns a list of modules used by this process
 */
bool GetProcessDependencies( DWORD process_id, std::vector<std::string>& dependencies, BOOL strip_module_path )
{
#ifdef _WIN32
    HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;

    /* Get a read only handle to this process */
    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id );
    if( !hProcess )
        return false;

    /* Get a list of all the modules used by this process */
    if( EnumProcessModules( hProcess, hMods, sizeof( hMods ), &cbNeeded ) )
    {
        for( UINT i = 0; i < ( cbNeeded / sizeof( HMODULE ) ); i++ )
        {
            char szModName[MAX_PATH];

            /* This will get the full path to the module's file */
            if( GetModuleFileNameExA( hProcess, hMods[i], szModName, sizeof( szModName ) / sizeof( char ) ) )
            {
                std::string strMod = szModName;

                /* Are we keeping the full path of the module or just keeping the module name itself? */
                if( strip_module_path )
                {
                    // Remove directory if present.
                    // Do this before extension removal incase directory has a period character.
                    const size_t last_slash_idx = strMod.find_last_of("\\/");
                    if( std::string::npos != last_slash_idx )
                    {
                        strMod.erase(0, last_slash_idx + 1);
                    }

                    // Remove extension if present.
                    /*const size_t period_idx = strMod.rfind('.');
                    if( std::string::npos != period_idx )
                    {
                        strMod.erase(period_idx);
                    }*/
                }

                dependencies.push_back( strMod );
            }
        }
    }

    /* Close process handle */
    CloseHandle( hProcess );

    return true;
#endif

    return false;
}