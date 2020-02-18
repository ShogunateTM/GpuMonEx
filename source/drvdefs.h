#pragma once

#include <Windows.h>

typedef struct _GPUDETAILS
{
	CHAR	DeviceDesc[128];
	DWORD	DeviceID;
	DWORD	VendorID;
} GPUDETAILS;