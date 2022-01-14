#pragma once

/*
 * Inter-process data structure
 */
typedef struct _SHARED_DATA
{
	char GpuMonExPath[MAX_PATH];		/* Path to the GpuMonEx install directory */
	char MinHookDllPath[MAX_PATH];		/* Path to the MinHook.xXX.dll file within GpuMonEx's installation directory */
}SHARED_DATA;