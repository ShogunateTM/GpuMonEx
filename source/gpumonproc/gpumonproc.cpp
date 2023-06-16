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

#ifdef _WIN32
#include <Shlobj.h>
#include <AclAPI.h>
#include <sddl.h>
#endif

/* Determine the right CPU arch before doing anything */
#if defined(X86_64)
#define CPU_ARCH_MATCH(x) (!Is32BitProcess(x))
#define GPUMON_DLL "gpumon64.dll"
#define wGPUMON_DLL L"gpumon64.dll"
#define MINHOOK_MODULE "Minhook.x64.dll"
#define PEGGY_DLL "peggy64.dll"
#elif defined(X86_32)
#define CPU_ARCH_MATCH(x) (Is32BitProcess(x))
#define GPUMON_DLL "gpumon32.dll"
#define wGPUMON_DLL L"gpumon32.dll"
#define MINHOOK_MODULE "Minhook.x86.dll"
#define PEGGY_DLL "peggy32.dll"
#endif




/* Process map (pid == key) */
std::unordered_map<DWORD, GMPROCESS> process_map;

/* Dynamic library handle */
HMODULE dlh = nullptr;

/* Shared Data structure and friends */
SHARED_DATA SharedData;
void* pSharedBufferData = NULL;
HANDLE hSharedDataMap = NULL;

bool bShutdownThisProcess = false;


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

/*
 * Name: EnableReadExecutePermissions
 * Desc: To avoid the need to manually set read and read/execute permissions to the gpumon DLL we
 *       inject into any UWP or WinRT processes.
 * 
 * Source: https://www.unknowncheats.me/forum/general-programming-and-reversing/177183-basic-intermediate-techniques-uwp-app-modding.html
 */
DWORD EnableReadExecutePermissionsW( std::wstring wstrFilePath )
{
    PACL pOldDACL = NULL, pNewDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESSW eaAccess;
    SECURITY_INFORMATION siInfo = DACL_SECURITY_INFORMATION;
    DWORD dwResult = ERROR_SUCCESS;
    PSID pSID;
 
    // Get a pointer to the existing DACL
    dwResult = GetNamedSecurityInfoW( wstrFilePath.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDACL, NULL, &pSD );
    if( dwResult != ERROR_SUCCESS )
        goto Cleanup;
 
    // Get the SID for ALL APPLICATION PACKAGES using its SID string
    ConvertStringSidToSidW( L"S-1-15-2-1", &pSID );
    if( pSID == NULL )
        goto Cleanup;
 
    ZeroMemory( &eaAccess, sizeof(EXPLICIT_ACCESSW) );
    eaAccess.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
    eaAccess.grfAccessMode = SET_ACCESS;
    eaAccess.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    eaAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    eaAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    eaAccess.Trustee.ptstrName = (LPWSTR) pSID;
 
    // Create a new ACL that merges the new ACE into the existing DACL
    dwResult = SetEntriesInAclW( 1, &eaAccess, pOldDACL, &pNewDACL );
    if( ERROR_SUCCESS != dwResult )
        goto Cleanup;
 
    // Attach the new ACL as the object's DACL
    dwResult = SetNamedSecurityInfoW( (LPWSTR)wstrFilePath.c_str(), SE_FILE_OBJECT, siInfo, NULL, NULL, pNewDACL, NULL );
    if( ERROR_SUCCESS != dwResult )
        goto Cleanup;
 
Cleanup:
    if( pSD != NULL )
        LocalFree( (HLOCAL) pSD );
    if( pNewDACL != NULL )
        LocalFree( (HLOCAL) pNewDACL );
 
    return dwResult;
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
#ifdef _WIN32               /* Windows */
#elif defined(__APPLE__)    /* MacOS */
#else                       /* Linux */
#endif

    return bShutdownThisProcess;
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

#if 0
bool callFunction (char * functionName, char * format, ...) 
{
	// Used for building code reference
	BYTE	codeFormat[] =	"\xE8\x01\x00\x00\x00"	// call codeStart;
							"\xC3"					// ret (returnAddress)

							// codeStart :
							"\xB8\x00\x00\x00\x00"	// mov eax, functionAddress

							//"\x68\x00\x00\x00\x00"// push arguments : will be increasing
													// according to total arguments
							// 11
							"\xFF\xD0"				// call eax (MessageBoxA) or the function called
							"\x68\x00\x00\x00\x00"	// push returnAddress
							"\xC3";					// ret;

	// Used to build the code
	BYTE*	codeBuild;								// Stores the finished code
	size_t	codeLength = 0;							// Calculate code total length
	int		codeAddLength = 0;						// Calculate additional length like string length, etc
	int		codePos = 0;							// The current position to insert byte;
	int		codeDataPos = 0;						// The current position to insert data;

	// Used to detect the arguments and build the code
	va_list argumentsList;
	char*	strArgumentParsed;						// Parse all string related arguments
	int		totalArguments = 0;						// the total argument found. Used for calculating code length

	// For finding all data about the function called by the user
	DWORD	functionAddress = 0;

	// For code injection
	BYTE*	codeAlloc;								// Will be used for the address to inject the finished code
	HANDLE	threadHandle = 0;						// Handling the thread that will execute the code
	DWORD	threadExitCode = 0;						// Stores the end of the thread

    HINSTANCE hLibrary = LoadLibrary (dll.c_str());
    if (!hLibrary) {

        log._stream << "Attempted to load a dll but failed" << std::endl;
        return false;
    }

    FARPROC pPrototype = GetProcAddress (hLibrary, functionName);
    if (!pPrototype) {

        log._stream << "Could not find function '" << functionName << "' in library - error code: " << GetLastError() << std::endl;
        return false;
    }

    FARPROC pRelative = (FARPROC)((DWORD)pPrototype - (DWORD)hLibrary);

    LPVOID pFunction = (LPVOID)((DWORD)pRelative + dwBaseAddress);

    log._stream << "Attempting to call '" << functionName << "' at location:" << pFunction << std::endl;

	// Check only if got anything in the format
	if(format != "")
	{
		// Check the total arguments and calculate the total additional data size
		va_start(argumentsList, format);
		for(int i = 0; format[i] != '\0'; i++)
		{
			// Calculate the total arguments found
			if(format[i] == '%')
			{
				totalArguments++;
				i++;
			}

			// Calculate all the additional data size
			switch(format[i])
			{
				case 'd':
				case 'x':
					// Skip because DWORD directly use the asm PUSH statement
					va_arg(argumentsList, DWORD);
					break;
				case 's':
					// Add the size of arguments to additional code length
					codeAddLength += lstrlen(va_arg(argumentsList, char*)) + 1;
					break;
			}
		}
		va_end(argumentsList);
	}

	// Start building the code
	codeLength = (totalArguments * 5) + sizeof(codeFormat) + codeAddLength;
	codeBuild = new BYTE[codeLength];
	
	// Initialize the position
	codePos = 0;
	codeDataPos = codeLength - codeAddLength - 1;

	// Allocate memory address for code injection here since we are using it for building
	// the code

    //pRemote = VirtualAllocEx (hProcess, NULL, numBytes, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	codeAlloc = (BYTE*)VirtualAllocEx(hProcess, NULL, codeLength, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if(!codeAlloc)
	{
		log._stream << "Failed to allocate memory address" << std::endl;
		return false;
	}

	// Add the first 7 bytes from the code reference
	for(int i = 0; i < 7; i++, codePos++)
		codeBuild[codePos] = codeFormat[i];

	//printf("\n\n %.02X \n\n", codePos);

	// Skip the 7th byte and move to the next one
	// codePos++;

	// For the 8th byte to 11th byte, add the function address
	*(DWORD*)&codeBuild[codePos] = functionAddress;

	// Skip 4 bytes and move to the next one
	codePos += 4;

	//for(int i = 0; i < totalArguments * 5;i++, codePos++)
	//	codeBuild[codePos] = 0x00;

	// Skip all the push statement and move to the next one
	codePos += (totalArguments * 5);

	// Again, add the next 3 bytes from the code reference
	for(int i = 11; i < 14; i++, codePos++)
		codeBuild[codePos] = codeFormat[i];

	// Add the address of the ret to push statement
	*(DWORD*)&codeBuild[codePos] = (DWORD)codeAlloc + 5;

	// Skip 4 bytes and move to the next one
	codePos += 4;

	// Add the last 1 bytes from the code reference
	//for(int i = 18; i < 20; i++, codePos++)
	codeBuild[codePos] = codeFormat[18];

	// Step backward 8 bytes to build all the push statement
	// Since asm is backward ex: (..., arg3, arg2, arg1, arg0)function
	// We should also going backwards
	codePos -= 7;

	if(format != "")
	{
		// Build all the push statements and also insert the arguments
		va_start(argumentsList, format);
		for(int i = 0; format[i] != '\0'; i++)
		{
			// This is to insert the push statement
			codePos -= 5;

			if(format[i] == '%')
			{
				// insert the push statement and skip 1 byte;
				codeBuild[codePos] = 0x68;
				codePos++;

				// move to the character after %
				i++;

				switch(format[i])
				{
					case 'd':
					case 'x':
						// Add the DWORD into the code
						*(DWORD*)&codeBuild[codePos] = va_arg(argumentsList, DWORD);
						break;
					case 's':
						// Parse the string to our string parser
						strArgumentParsed = va_arg(argumentsList, char*);

						// Add the address of the string to the push statement
						*(DWORD*)&codeBuild[codePos] = (DWORD)(codeAlloc + codeDataPos);

						// For every character in the string add to the code in the data section
						for(int j = 0; j < lstrlen(strArgumentParsed); j++, codeDataPos++)
							codeBuild[codeDataPos] = strArgumentParsed[j];

						// Add the next byte as the string ending
						codeBuild[codeDataPos] = 0x00;

						// Move the data position by 1 skipping the string end
						codeDataPos++;
						break;
				}

				// Move backwards 1 bytes to build the next push statement
				codePos--;
			}
		}
		va_end(argumentsList);
	}

	if(!WriteProcessMemory(hProcess, (LPVOID)codeAlloc, codeBuild, codeLength, NULL))
	{
		log._stream << "Failed to write process memory" << std::endl;
		return false;
	}

	threadHandle = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)codeAlloc, 0, 0, NULL);
	if(!threadHandle)
	{
		log._stream << "Failed to create remote thread" << std::endl;
		return false;
	}

	if(WaitForMultipleObjects(1, &threadHandle, TRUE, 5000) != 0)
	{
		log._stream << "Failed on waiting \n" << std::endl;
		return false;
	}

	if(!GetExitCodeThread(threadHandle, &threadExitCode))
	{
		log._stream << "Failed to retrieve thread exit code \n\n" << std::endl;
		return false;
	}

    if (codeAlloc) {
	    if(!VirtualFreeEx(hProcess, codeAlloc, 0, MEM_RELEASE))
	    {
		    log._stream << "Failed to free virtual allocation. " << std::endl;
		    return false;
	    }

    }
	return true;
}
#endif

struct ExitParams
{
    HMODULE hModule;
    int exitcode;
};

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

    char DllFullPath[MAX_PATH];

    /* If we don't use the FULL path to the DLL we are injecting, it won't work and this whole thing is for nothing! */
    /*if( !GetFullPathNameA( PEGGY_DLL, MAX_PATH, DllFullPath, nullptr ) )
    {
        CloseHandle( ProcessHandle );
        return FALSE;
    }*/

    /*PVOID BufferData = VirtualAllocEx( ProcessHandle, NULL, sizeof( struct ExitParams ), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
    if( BufferData == NULL )
    {
        CloseHandle( ProcessHandle );
        return FALSE;
    }

    SIZE_T ReturnLength;
    struct ExitParams exit_params = { me.hModule, 0 };
    BOOL bOK = WriteProcessMemory( ProcessHandle, BufferData, &exit_params, sizeof( struct ExitParams ), &ReturnLength );*/

    LPTHREAD_START_ROUTINE FreeLibraryAddress = NULL;
    HMODULE Kernel32Module = LoadLibraryA( /*DllFullPath*/ "Kernel32.dll" );

    FreeLibraryAddress = (LPTHREAD_START_ROUTINE) GetProcAddress( Kernel32Module, /*"_FreeLibraryRemote@4"*/ "FreeLibrary" );
    pfnNtCreateThreadEx NtCreateThreadEx = (pfnNtCreateThreadEx) GetProcAddress( GetModuleHandleA( "ntdll.dll" ), "NtCreateThreadEx" );
    if( NtCreateThreadEx == NULL )
    {
        CloseHandle( ProcessHandle );
        return FALSE;
    }

    HANDLE ThreadHandle = NULL;

    NtCreateThreadEx( &ThreadHandle, 0x1FFFFF, NULL, ProcessHandle, (LPTHREAD_START_ROUTINE) FreeLibraryAddress, /*me.modBaseAddr*/me.hModule /*&exit_params*/, FALSE, NULL, NULL, NULL, NULL );
    if( ThreadHandle == NULL )
    {
        CloseHandle( ProcessHandle );
        return FALSE;
    }
    if( WaitForSingleObjectEx( ThreadHandle, INFINITE, FALSE ) == WAIT_FAILED )
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
        = { /*{ "ddraw.dll",      GMPROCESS_USES_DDRAW },
            { "d3d8.dll",       GMPROCESS_USES_D3D8 },
            { "d3d9.dll",       GMPROCESS_USES_D3D9 },
            { "d3d9ex.dll",     GMPROCESS_USES_D3D9 },
            { "d3d10.dll",      GMPROCESS_USES_D3D10 },
            { "d3d11.dll",      GMPROCESS_USES_D3D11 },
            { "d3d12.dll",      GMPROCESS_USES_D3D12 },*/
            { "opengl32.dll",   GMPROCESS_USES_OPENGL } }
#endif
    ;

    int target_dependencies_count = sizeof( target_dependencies ) / sizeof( struct dependency_t );

    std::vector<std::string> dependencies;
    bool dependency_found = false;
    
    /* Get the list of dependencies, and then compare it to the list of target dependencies. If we find
       a module we desire to hook, go ahead add the flag so we can hook it. */
    if( GetProcessDependencies( pid, dependencies, TRUE ) )
    {
        for( auto d = dependencies.begin(); d != dependencies.end(); ++d )
        {
            for( int i = 0; i < target_dependencies_count; i++ )
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
 *       Windows: We need this process to run in the background yet portable, so we still need
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
    if( argc > 2 )
    {
        if( !strcmp( argv[1], "-pid" ) )
            target_pid = atoi( argv[2] );
        else if( !strcmp( argv[1], "-pname" ) )
            target_pid = GetProcessIdByName( argv[2] );

        if( target_pid == 0 )   /* Process is not running and therefore does not exist */
        {
            std::stringstream ss;

            ss << "main(): Process ID '" << argv[2] << "' not found!" << std::endl;
            OutputDebugStringA( ss.str().c_str() );
            target_pid = -1;
        }
    }
    
    /* Functions called at exit */
    atexit( at_exit );

#ifdef _WIN32
    /* Programatically enable read and read/execute privileges for the "ALL APPLICATION PACKAGES" group for WinRT/UWP apps */
    DWORD result = EnableReadExecutePermissionsW( wGPUMON_DLL );
    if( result != ERROR_SUCCESS )
        OutputDebugStringA( GetLastErrorAsString().c_str() );

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
