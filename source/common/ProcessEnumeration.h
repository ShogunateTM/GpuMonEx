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
} GMPROCESS;


bool CreateNewProcess( const CHAR* szProcessName, GMPROCESS* pProcess );
bool OpenProcessByName( const CHAR* szProcessName, GMPROCESS* pProcess );
bool OpenProcessByID( DWORD dwProcessID, GMPROCESS* pProcess );
void CloseProcess( GMPROCESS* pProcess );
void TerminateProcess( GMPROCESS* pProcess );
bool Is32BitProcess( DWORD dwProcessID );
bool ProcessIsActive( GMPROCESS* pProcess );

#endif // __PROCESS_ENUMERATION_H__