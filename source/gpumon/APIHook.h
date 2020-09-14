//
//  APIHook.h
//  gpumon
//
//  Created by Aeroshogun on 4/29/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#ifndef APIHook_h
#define APIHook_h

#include "../drvdefs.h"
#include "Hook_OpenGL.h"

#ifdef _WIN32
#include "Hook_Direct3D12.h"
#include "Hook_Direct3D11.h"
#include "Hook_Direct3D10.h"
#include "Hook_Direct3D9.h"
#include "Hook_Direct3D8.h"
#include "Hook_DirectDraw.h"    /* Includes all DDraw and D3D verisons 7 and earlier */
#endif

#ifdef __APPLE__
#include "Hook_Metal.h"
#endif

#ifdef _WIN32
bool EnableMinHookAPI();
void DisableMinHookAPI();
#endif

#endif /* APIHook_h */
