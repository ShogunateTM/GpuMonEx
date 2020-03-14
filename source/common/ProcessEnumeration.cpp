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
 * Name: CreateNewProcess
 * Desc: Launches a new process from an executable file on disk.
 */
void* CreateNewProcess( const CHAR* szProcessName )
{
    void* hProcess = NULL;

#if _WIN32
    if( !DebugPrivilegesEnabled )
        DebugPrivilegesEnabled = EnableDebugPrivileges();

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
#endif

    return hProcess;
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
void* OpenProcessByName( const CHAR* szProcessName )
{
    void* hProcess = NULL;

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
        hProcess = (HANDLE)OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwProcessID );
        if( !hProcess )
            hProcess = (HANDLE)OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID );
    }
#endif

    return hProcess;
}


/*
 * Name: OpenProcessByID
 * Desc: Opens a given process using the process's ID number.
 */
void* OpenProcessByID( DWORD dwProcessID )
{
    void* hProcess = NULL;

#ifdef _WIN32
    if( !DebugPrivilegesEnabled )
        DebugPrivilegesEnabled = EnableDebugPrivileges();

    /* Attempt to open this process with all access.  If we fail, use read-only flags */
    hProcess = (HANDLE) OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwProcessID );
    if( !hProcess )
        hProcess = (HANDLE) OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID );
#endif

    return hProcess;
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

    return true;
}


/*
 * Name: ProcessIsActive
 * Desc: Returns true if the process handle is valid and running.
 */
bool ProcessIsActive( void* pProcess )
{
#ifdef _WIN32
    DWORD dwResult = WaitForSingleObject( pProcess, 0 );

    if( dwResult == WAIT_OBJECT_0 )
        return true;
#endif

    return false;
}