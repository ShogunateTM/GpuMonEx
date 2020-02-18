#pragma once

#include <fstream>

extern std::ofstream logfi;

/* Logging defines */
#define _LOG(x) std::cout << x; \
	if(logfi.is_open()) \
	{ \
		logfi << x; \
	}

#define _LOGW(x) std::wcout << x; \
	if(logfi.is_open()) \
	{ \
		logfi << x; \
	}

#define _ERROR(x) std::cerr << x; \
	if(logfi.is_open()) \
	{ \
		logfi << x; \
	}