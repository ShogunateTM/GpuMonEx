/******************************************************************************

  Copyright (C) 1999, 2000 NVIDIA Corporation
  This file is provided without support, instruction, or implied warranty of any
  kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
  not liable under any circumstances for any damages or loss whatsoever arising
  from the use or inability to use this file or items derived from it.
  
    Comments:
    
	Simple debug support with stream input for complex arguments.
	Also writes to a debug log style file.

	Declare this somewhere in your .cpp file:

	NVDebug dbg(DebugLevel, "outfile.txt")
	
	// Output Like this 
	DISPDBG(0, "This" << " is " << "a useful debug class");

  cmaughan@nvidia.com
      
******************************************************************************/
#ifndef __NVDEBUG_H
#define __NVDEBUG_H

#ifdef _WIN32
#include <windows.h>
#pragma warning(disable: 4786)
#include <tchar.h>
#endif
#pragma warning(disable: 4786)
#include <iostream>
#pragma warning(disable: 4786)
#include <sstream>
#pragma warning(disable: 4786)
#include <iomanip>
#pragma warning(disable: 4786)
#include <strstream>
#pragma warning(disable: 4786)
#include <fstream>
#pragma warning(disable: 4786)
#include <assert.h>
#include "singleton.h"
#ifdef _WIN32
#include <crtdbg.h>
#endif

/* Non-Windows platforms */
#ifndef _WIN32
#define OutputDebugString printf //std::cout << s
#define OutputDebugStringA OutputDebugString
#define TEXT(s) s
#define DWORD unsigned long
#endif

/* Trap functionality */
#ifdef _MSC_VER
#define __trap  _asm int 3
#else
#define __trap  __asm__("int $3")
#endif

#ifndef _WIN32
#define __KE_FUNCTION__ __PRETTY_FUNCTION__
#else
#define __KE_FUNCTION__ __FUNCSIG__
#endif


static const DWORD MaxDebugFileSize = 100000;

class NVDebug : public Singleton<NVDebug>
{
public:

	NVDebug(unsigned int GlobalDebugLevel, const char* pszFileName)
		: m_GlobalDebugLevel(GlobalDebugLevel),
		m_dbgLog(pszFileName, std::ios::out | std::ios::trunc)	// Open a log file for debug messages
	{
		OutputDebugString(TEXT("NVDebug::NVDebug\n\n"));
	}
	
	virtual ~NVDebug()
	{
		m_dbgLog.flush();
		m_dbgLog.close();

		m_strStream.flush();
		m_strStream.clear();
	}

	// Flush the current output
#ifdef _WIN32
	void NVDebug::EndOutput()
#else
    void EndOutput()
#endif
	{
		m_strStream << std::endl << std::ends;

		// Don't make a huge debug file.
		if (m_dbgLog.tellp() < MaxDebugFileSize)
		{
			m_dbgLog << m_strStream.str();
			FlushLog();
		}

		OutputDebugStringA(m_strStream.str().c_str());

		//m_strStream.freeze(false);
		//m_strStream.seekp(0);
		m_strStream.str("");
	}

	unsigned int GetLevel() const { return m_GlobalDebugLevel; };
	std::ostringstream& GetStream() { return m_strStream; }
	std::stringstream& GetLastMessage() { return m_strLastMessage; }
	void FlushLog() { m_dbgLog.flush(); }
	void FlushLastMessage() { m_strLastMessage.str(""); m_strLastMessage.clear(); }
	
private:
	std::ostringstream m_strStream;
	std::stringstream m_strLastMessage;
	std::ofstream m_dbgLog;
	unsigned int m_GlobalDebugLevel;
};

#if 1
#define DISPDBG(a, b)											\
do																\
{																\
	if (NVDebug::GetSingletonPtr() != NULL)						\
	if (a <= NVDebug::GetSingleton().GetLevel()) {	\
		NVDebug::GetSingleton().FlushLastMessage();	\
		NVDebug::GetSingleton().GetLastMessage() << __KE_FUNCTION__ << ": " << b << "\n";	\
		NVDebug::GetSingleton().GetStream() << __KE_FUNCTION__ << ": " << b << "\n";			\
		NVDebug::GetSingleton().EndOutput(); }				\
} while(0)

#define NVASSERT(a, b)														\
do																			\
{																			\
	static bool bIgnore = false;											\
	if (!bIgnore && ((int)(a) == 0))										\
	{																		\
		std::ostringstream strOut;											\
		strOut << b << "\nAt: " << __FILE__ << ", " << __LINE__;			\
        OutputDebugString(strOut.str().c_str() );                           \
        __trap;                                                             \
		/*int Ret = MessageBoxEx(NULL, strOut.str().c_str(), "NVASSERT", MB_ABORTRETRYIGNORE, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ));	\
		if (Ret == IDIGNORE)			\
		{								\
			bIgnore = true;				\
		}								\
		else if (Ret == IDABORT)		\
		{								\
            __trap;                     \
		}*/								\
	}									\
} while (0)

#else
#define DISPDBG(a, b)	\
do																\
{																\
	if (NVDebug::GetSingletonPtr() != NULL)						\
	if (a <= NVDebug::GetSingleton().GetLevel()) {	\
		NVDebug::GetSingleton().FlushLastMessage();	\
		NVDebug::GetSingleton().GetLastMessage() << __KE_FUNCTION__ << "(): " << b;	\
	}				\
} while(0)
#define NVASSERT(a, b)
#endif

// Return the last message sent to the debug log
#define GETLASTMSG() NVDebug::GetSingletonPtr() != NULL ? NVDebug::GetSingleton().GetLastMessage().str() : "(nil)"

// Note that the cast ensures that the stream
// doesn't try to interpret pointers to objects in different ways.
// All we want is the object's address in memory.
#define BASE_DBG_PTR(a)					\
"0x" << std::hex << (DWORD)(a) << std::dec
	
#endif // __NVDEBUG_H
