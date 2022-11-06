//
//  Hook_Direct3D11.h
//  gpumon
//
//  Created by Aeroshogun on 4/20/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#ifndef Hook_Direct3D11_h
#define Hook_Direct3D11_h

#include <d3d11_4.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Typedefs 
 */



/*
* Hooked Direct3D9 API functions
*/
struct Hook_Direct3D11API;

void Drv_GetDirect3D11Hooks( struct Hook_Direct3D11API* hooks, struct Hook_Direct3D11API* originals );
BOOL Drv_EnableDirect3D11Hooks();
BOOL Drv_DisableDirect3D11Hooks();

#ifdef __cplusplus
}
#endif

#endif /* Hook_Direct3D11_h */
