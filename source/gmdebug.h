#ifndef __GMDEBUG_H__
#define __GMDEBUG_H__


/* 
 * Classic NVSDK helper headers 
 */
#include <nvdebug.h>


/* 
 * GpuMon debugging macros 
 */
#define GPUMON_DEBUG_LVL	3

#define GPUMON_ERROR	1
#define GPUMON_WARNING	2
#define GPUMON_DEBUG(l)	(l+3)

#define GLOG( a, b ) \
	std::cout << b << std::endl; \
	DISPDBG( a, b )

#define GWARN( b ) \
	std::cout << "GWARN: " << b << std::endl; \
	DISPDBG( GPUMON_WARNING, "GWARN: " << b )

#define GERROR( b ) \
	std::cout << "GERROR: " << b << std::endl; \
	DISPDBG( GPUMON_GERROR, "GERROR: " << b )

/*
 * gpucmd error codes
 */
#define ERR_OK			0
#define ERR_NOPARAMS	1
#define ERR_MISSINGHW	2
#define ERR_BADPARAMS	3
#define ERR_DRVFAIL		4

#endif // __GMDEBUG_H__