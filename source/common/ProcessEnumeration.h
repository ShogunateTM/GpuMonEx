#ifndef __PROCESS_ENUMERATION_H__
#define __PROCESS_ENUMERATION_H__

void* OpenProcessByName( const char* szProcessName );
void* OpenProcessByID( DWORD dwProcessID );
bool Is32BitProcess( DWORD dwProcessID );

#endif // __PROCESS_ENUMERATION_H__