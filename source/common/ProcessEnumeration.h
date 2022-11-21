#ifndef __PROCESS_ENUMERATION_H__
#define __PROCESS_ENUMERATION_H__

/*
 * Enumerated process API usage flags
 * (Upper 16-bits: Major version, Lower 16-bits: Minor version)
 */
#define GMPROCESS_USES_DDRAW    0x00010000  /* Any version */
#define GMPROCESS_USES_D3DIM    0x00020000  /* Version 7 and earlier */
#define GMPROCESS_USES_D3D8     0x00040000
#define GMPROCESS_USES_D3D9     0x00080000
#define GMPROCESS_USES_D3D10    0x00100000
#define GMPROCESS_USES_D3D11    0x00200000
#define GMPROCESS_USES_D3D12    0x00400000
#define GMPROCESS_USES_OPENGL   0x00800000  /* Any version */
#define GMPROCESS_USES_VULKAN   0x01000000  /* Any version */
#define GMPROCESS_USES_METAL    0x02000000
#define GMPROCESS_USES_OPENCL   0x04000000
#define GMPROCESS_USES_CUDA     0x08000000


/*
 * System Process structure
 */
typedef struct _GMPROCESS
{
    void* hProcess;
    void* hThread;
    DWORD dwID;
    DWORD dwFlags;
} GMPROCESS;


BOOL EnableDebugPrivileges();
bool CreateNewProcess( const CHAR* szProcessName, GMPROCESS* pProcess );
bool OpenProcessByName( const CHAR* szProcessName, GMPROCESS* pProcess );
DWORD GetProcessIdByName( const CHAR* szProcessName );
bool OpenProcessByID( DWORD dwProcessID, GMPROCESS* pProcess );
void CloseProcess( GMPROCESS* pProcess );
void TerminateProcess( GMPROCESS* pProcess );
bool Is32BitProcess( DWORD dwProcessID );
bool ProcessIsActive( GMPROCESS* pProcess );
bool EnumerateProcesses( GMPROCESS** ppProcesses, int* ProcessCount );
bool GetCurrentProcessName( std::string& strProcessName );
bool GetProcessDependencies( DWORD process_id, std::vector<std::string>& dependencies, BOOL strip_module_path );

#endif // __PROCESS_ENUMERATION_H__
