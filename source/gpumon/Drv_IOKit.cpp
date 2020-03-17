//
//  Drv_IOKit.cpp
//  gpumon
//
//  Created by Aeroshogun on 3/17/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//


#include "..\platform.h"
#include "..\debug.h"
#include "Drv_IOKIT.h"

#include <IOKit/IOKitLib.h>


/* Logging file */
//std::ofstream logfi;


int IOKIT_Initialize()
{
    //_LOG( __FUNCTION__ << "(): " << "AMD driver initialization started...\n" );
    return 0;
}

void IOKIT_Uninitialize()
{
    //_LOG( __FUNCTION__ << "AMD driver uninitialization completed.\n" );
}

int IOKIT_GetGpuDetails( int AdapterNumber, GPUDETAILS* pGpuDetails )
{
    //_LOG( __FUNCTION__ << "TODO: Implement...\n" );
    
    return 0;
}

int IOKIT_GetOverallGpuLoad()
{
    return 0;
}

int IOKIT_GetProcessGpuLoad( void* pProcess )
{
    return 0;
}

int IOKIT_GetGpuTemperature()
{
    return 0;
}



#if 0

// TODO: work on the MacOS version, basing the code off of this:
// https://stackoverflow.com/questions/10110658/programmatically-get-gpu-percent-usage-in-os-x
// https://gist.github.com/chockenberry/2afe4d0f1f9caddc81de

#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>
#include <IOKit/IOKitLib.h>

int main(int argc, const char * argv[])
{

while (1) {

    // Get dictionary of all the PCI Devicces
    CFMutableDictionaryRef matchDict = IOServiceMatching(kIOAcceleratorClassName);

    // Create an iterator
    io_iterator_t iterator;

    if (IOServiceGetMatchingServices(kIOMasterPortDefault,matchDict,
                                     &iterator) == kIOReturnSuccess)
    {
        // Iterator for devices found
        io_registry_entry_t regEntry;

        while ((regEntry = IOIteratorNext(iterator))) {
            // Put this services object into a dictionary object.
            CFMutableDictionaryRef serviceDictionary;
            if (IORegistryEntryCreateCFProperties(regEntry,
                                                  &serviceDictionary,
                                                  kCFAllocatorDefault,
                                                  kNilOptions) != kIOReturnSuccess)
            {
                // Service dictionary creation failed.
                IOObjectRelease(regEntry);
                continue;
            }

            CFMutableDictionaryRef perf_properties = (CFMutableDictionaryRef) CFDictionaryGetValue( serviceDictionary, CFSTR("PerformanceStatistics") );
            if (perf_properties) {

                static ssize_t gpuCoreUse=0;
                static ssize_t freeVramCount=0;
                static ssize_t usedVramCount=0;

                const void* gpuCoreUtilization = CFDictionaryGetValue(perf_properties, CFSTR("GPU Core Utilization"));
                const void* freeVram = CFDictionaryGetValue(perf_properties, CFSTR("vramFreeBytes"));
                const void* usedVram = CFDictionaryGetValue(perf_properties, CFSTR("vramUsedBytes"));
                if (gpuCoreUtilization && freeVram && usedVram)
                {
                    CFNumberGetValue( (CFNumberRef) gpuCoreUtilization, kCFNumberSInt64Type, &gpuCoreUse);
                    CFNumberGetValue( (CFNumberRef) freeVram, kCFNumberSInt64Type, &freeVramCount);
                    CFNumberGetValue( (CFNumberRef) usedVram, kCFNumberSInt64Type, &usedVramCount);
                    NSLog(@"GPU: %.3f%% VRAM: %.3f%%",gpuCoreUse/(double)10000000,usedVramCount/(double)(freeVramCount+usedVramCount)*100.0);

                }

            }

            CFRelease(serviceDictionary);
            IOObjectRelease(regEntry);
        }
        IOObjectRelease(iterator);
    }

   sleep(1);
}
return 0;
}

#endif
