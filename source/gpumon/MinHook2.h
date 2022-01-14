#pragma once

#include <MinHook.h>


#if defined (X86_32) 
#define MINHOOK_MODULE "Minhook.x86.dll"
#elif defined (X86_64)
#define MINHOOK_MODULE "Minhook.x64.dll"
#else	/* TODO: Support ARM64 directly? */
#error "CPU Architecture not supported!"
#endif


/*
 * MinHook function pointers (same as the originals) 
 */
extern MH_STATUS (WINAPI *pfnMH_Initialize)();
extern MH_STATUS (WINAPI *pfnMH_Uninitialize)();
extern MH_STATUS (WINAPI *pfnMH_CreateHook)(LPVOID pTarget, LPVOID pDetour, LPVOID *ppOriginal);
extern MH_STATUS (WINAPI *pfnMH_RemoveHook)(LPVOID pTarget);
extern MH_STATUS (WINAPI *pfnMH_EnableHook)(LPVOID pTarget);
extern MH_STATUS (WINAPI *pfnMH_DisableHook)(LPVOID pTarget);
extern const char* (WINAPI *pfnMH_StatusToString)(MH_STATUS status);
