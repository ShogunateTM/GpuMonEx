/*********************************************************************NVMH2****
Path:  D:\Dev\devrel\Nvsdk\Common\include
File:  NV_Error.h

Copyright (C) 1999, 2000 NVIDIA Corporation
This file is provided without support, instruction, or implied warranty of any
kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
not liable under any circumstances for any damages or loss whatsoever arising
from the use or inability to use this file or items derived from it.

Comments:

No longer requires separate .cpp file.  Functions defined in the .h with the
magic of the static keyword.



******************************************************************************/



#ifndef	__MYERROR__
#define	__MYERROR__


#include    <stdlib.h>          // for exit()
#include    <stdio.h>
#include    <windows.h>


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000








void OkMsgBox( char * szCaption, char * szFormat, ... );

void FDebug( char * szFormat, ... );
void FMsg( char * szFormat, ... );



#ifndef NULLCHECK
#define NULLCHECK(q, msg,quit) {if(q==NULL) { DoError(msg, quit); }}
#endif

#ifndef IFNULLRET
#define IFNULLRET(q, msg)	   {if(q==NULL) { FDebug(msg); return;}}
#endif

#ifndef FAILRET
#define FAILRET(hres, msg) {if(FAILED(hres)){FDebug("*** %s   HRESULT: %d\n",msg, hres);return hres;}}
#endif

#ifndef HRESCHECK
#define HRESCHECK(q, msg)	 {if(FAILED(q)) { FDebug(msg); return;}}
#endif

#ifndef NULLASSERT
#define NULLASSERT( q, msg,quit )   {if(q==NULL) { FDebug(msg); assert(false); if(quit) exit(0); }}
#endif

/////////////////////////////////////////////////////////////////


static void OkMsgBox( char * szCaption, char * szFormat, ... )
{
	char szBuffer[256];
	char *pArguments;

	pArguments = (char *) &szFormat + sizeof( szFormat );
	vsprintf( szBuffer, szFormat, pArguments );
	MessageBox( NULL, szBuffer, szCaption, MB_OK );
}


#ifdef _DEBUG
	static void FDebug ( char * szFormat, ... )
	{	
		static char buffer[2048];
		char *pArgs;

		pArgs = (char*) &szFormat + sizeof( szFormat );
		vsprintf( buffer, szFormat, pArgs );

		OutputDebugString ( buffer );

		Sleep( 2 );		// can miss some if calling too fast!
	}

	static void NullFunc( char * szFormat, ... ) {}

	#if 0
		#define WMDIAG(str) { OutputDebugString(str); }
	#else
		#define WMDIAG(str) {}
	#endif
#else
	static void FDebug( char * szFormat, ... )		{}
	static void NullFunc( char * szFormat, ... )	{}

	#define WMDIAG(str) {}
#endif


static void FMsg( char * szFormat, ... )
{	
	static char buffer[2048];
	char *pArgs;

	pArgs = (char*) &szFormat + sizeof( szFormat );
	vsprintf( buffer, szFormat, pArgs );

	OutputDebugString ( buffer );

	Sleep( 2 );		// can miss some if calling too fast!
}






#endif
