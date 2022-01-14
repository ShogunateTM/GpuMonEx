#include "../platform.h"
#include "Hook_Direct3D9.h"


void GetFunctionsViaVtable_D3D9( IDirect3DDevice9* pD3DDevice, struct Hook_Direct3D9API* originals )
{
	originals->D3DDevice9_Present = pD3DDevice->lpVtbl->Present;
}