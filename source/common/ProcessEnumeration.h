#ifndef __PROCESS_ENUMERATION_H__
#define __PROCESS_ENUMERATION_H__

/*
 * System Process structure
 */
typedef struct _GMPROCESS
{
    void* hProcess;
    void* hThread;
    DWORD dwID;
};

void* CreateNewProcess( const CHAR* szProcessName );
void* OpenProcessByName( const CHAR* szProcessName );
void* OpenProcessByID( DWORD dwProcessID );
bool Is32BitProcess( DWORD dwProcessID );
bool ProcessIsActive( void* pProcess );

#endif // __PROCESS_ENUMERATION_H__