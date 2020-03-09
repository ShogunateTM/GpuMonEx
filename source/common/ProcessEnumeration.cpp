#include "../platform.h"


/*
 * Globals
 */

#ifdef _WIN32
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)( HANDLE, PBOOL );

LPFN_ISWOW64PROCESS pfnIsWow64Process;
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

    pfnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress( GetModuleHandle( TEXT("kernel32" ) ), "IsWow64Process" );

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
void* OpenProcessByName( const TCHAR* szProcessName )
{
    void* hProcess = NULL;

#ifdef _WIN32
    DWORD dwProcessID = 0;
    HANDLE Snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    PROCESSENTRY32 Process;
    ZeroMemory( &Process, sizeof( Process ) );
    Process.dwFlags = sizeof( Process );

    if( Process32First( &Snapshot, &Process ) )
    {
        do
        {
            if( std::wstring( Process.szExeFile ) == std::wstring( szProcessName ) )
            {
                dwProcessID = Process.th32ProcessID;
                break;
            }
        } while( Process32Next( &Snapshot, &Process ) );
    }

    CloseHandle( Snapshot );

    if( dwProcessID != 0 )
        hProcess = (HANDLE) OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwProcessID );
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
    /* Attempt to open this process with all access.  If we fail, use read-only flags */
    hProcess = (HANDLE) OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwProcessID );
    if( !hProcess )
        hProcess = (HANDLE) OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID );
#endif

    return hProcess;
}


/*
 * Name: 
 * Desc: 
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