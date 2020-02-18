#ifndef __PLATFORM_H__
#define __PLATFORM_H__


/*
 * Platform specific includes and definitions
 */


/*
 * Standard C/C++ headers
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <conio.h>
#include <memory>
#include <io.h>
#include <stdint.h>

/* C++ only */
#ifdef __cplusplus
#include <iostream>
#include <string>
#endif

/*
 * Windows Desktop
 */
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define INITGUID                        // Initialize GUID related stuff (equivalent of including dxguid.lib)

#include <windows.h>
#include <mmsystem.h>
#include <process.h>

#endif


/*
 * MacOS
 */
#ifdef __APPLE__

#endif


#endif