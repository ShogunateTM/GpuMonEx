//
//  Drv_IOKit.cpp
//  gpumon
//
//  Created by Aeroshogun on 3/17/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//


#include "../platform.h"
#include "../debug.h"
#include "Drv_IOKIT.h"

//#include <Cocoa/Cocoa.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/graphics/IOAccelClientConnect.h>




/*
 * IOKit GPU device management class
 */
class iokit_gpudevice_t
{
public:
    iokit_gpudevice_t() : registry_entry(0),
                            service_dictionary(nullptr),
                            performance_statistics(nullptr),
                            performance_statistics_accum(nullptr),
                            bundle_identifier(""), io_class("") {}
    ~iokit_gpudevice_t()
    {
        CFRelease( service_dictionary );
        IOObjectRelease( registry_entry );
    }
    
public:
    double gpu_usage();
    double video_engine_usage();
    double vram_usage();
    long vram_free();
    long vram_used();
    
public:
    io_registry_entry_t registry_entry;
    CFMutableDictionaryRef service_dictionary;
    CFMutableDictionaryRef performance_statistics;
    CFMutableDictionaryRef performance_statistics_accum;
    std::string bundle_identifier;
    std::string io_class;
};





/*
 * Name: iokit_gpudevice_t::gpu_usage
 * Desc: Returns the GPU usage as reported by macOS.  Returns -1 if there is an error.
 */
double iokit_gpudevice_t::gpu_usage()
{
    double usage = -1;
    
    /* NOTE: It appears that GPU usage can be queried from either "PerformanceStatistics" or "PerformanceStatisticsAccum", only one
     * sometimes has slightly different information.  For the time being, for overall GPU usage, we will simply stick with the former.
     */
    
    /* CFMutableDictionaryRef perf_properties = (CFMutableDictionaryRef) CFDictionaryGetValue( serviceDictionary, CFSTR("PerformanceStatistics") );
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
     
     }*/
    
    /* Sanity check, this should have been initialized during enumeration in IOKIT_Initialize */
    if( !this->service_dictionary )
        return -1;
    
    /* Query the GPU's current performance statistics */
    performance_statistics = (CFMutableDictionaryRef) CFDictionaryGetValue( service_dictionary, CFSTR( "PerformanceStatistics" ) );
    if( !performance_statistics )
        return -1;
    
    /* Attempt to query the GPU usage.
     *
     * NOTE: As macOS evolves, the key names within the PerformanceStatistics dictionary become subject to change.  Known key names
     * include:
     *  - "GPU Core Utilization"
     *  - "Device Utilization"
     *  - "GPU Activity"
     *
     * What we're going to do is check for each one in ascending order.  If we get a non-null pointer, then we'll go ahead and stick with
     * that.  If not, then it means that this driver does not report GPU usage directly.
     */
    
    for( int i = 0; i < 3; i++ )
    {
        const char* keys[] = { "GPU Activity", "Device Utilization", "GPU Core Utilization" };
        ssize_t utilization = 0;
        
        auto gpu_core_utilization = CFDictionaryGetValue( performance_statistics, __CFStringMakeConstantString( keys[i] ) );
        if( gpu_core_utilization )
        {
            CFNumberGetValue( (CFNumberRef) gpu_core_utilization, kCFNumberSInt64Type, &utilization );
            
            usage = utilization;
            
            /* GPU Core Utilization requires us to scale it down to a proper percentage */
            if( usage > 100 )
                usage /= 10000000;
            
            return usage;
        }
    }
    
    /* Now, if we reach this point, it's theoretically not impossible to get the GPU usage.  Hypothetically speaking, by querying the key
     * "hardwareWaitTime", we can divide this number by the GPU's frequency in nanoherz.  So far, I've only seen this scenario come up once
     * with the Intel GMA 950.  No modern revisions of macOS support that GPU anyway, but this is meant to work on older versions of macOS.
     * The next challenge becomes getting the GPU clock frequency programatically.  
     *
     * So until this scenario actually comes up, I won't worry about it too much, but it would make for an interesting test case down the road.
     * So for now, we will just return -1.
     */
    
    return usage;
}

//std::vector<iokit_gpudevice_t*> gpudevices;

/* Logging file */
//std::ofstream logfi;


int IOKIT_Initialize()
{
    _LOG( __FUNCTION__ << "(): " << "IOKIT driver initialization started...\n" );
    
    /* We're going to start by enumerating the available devices hooked up to the
       PCI address space.  We're going to be searching for devices that expose the 
       "PerformanceStatistics" property and we can get the GPU statistics from this.
     
       TODO: Get all GPUs and sort by adapter number, get GPU name, device/vendor ID,
       etc. */
    
    CFMutableDictionaryRef match_dictionary = IOServiceMatching( kIOAcceleratorClassName );
    if( !match_dictionary )
        return 0;
    
    /* Begin device enumeration */
    io_iterator_t iterator;
    
    if( IOServiceGetMatchingServices( kIOMasterPortDefault, match_dictionary, &iterator ) == kIOReturnSuccess )
    {
        io_registry_entry_t registry_entry;
        
        /* Enumerate every available device */
        while( ( registry_entry = IOIteratorNext( iterator ) ) )
        {
            /* Put this services object into a dictionary object */
            CFMutableDictionaryRef service_dictionary;
            
            if( IORegistryEntryCreateCFProperties( registry_entry, &service_dictionary, kCFAllocatorDefault, kNilOptions ) != kIOReturnSuccess )
            {
                /* Service dictionary creation failed, move on to the next device */
                IOObjectRelease( registry_entry );
                continue;
            }
            
            /* Either one of these will contain the information necessary to get the overall system GPU usage and in some
               cases, GPU video engine usage also. If we get at least one, then we have a valid GPU of some sort.  Occasionally,
               you will get a display adapter that does not expose this but if it doesn't, chances are it's not a hardware
               accelerated implementation anyway (such as that or it must be really old). */
            
            iokit_gpudevice_t gpudevice;
            
            CFMutableDictionaryRef perf_properties = (CFMutableDictionaryRef) CFDictionaryGetValue( service_dictionary, CFSTR( "PerformanceStatistics" ) );
            CFMutableDictionaryRef perf_properties_accum = (CFMutableDictionaryRef) CFDictionaryGetValue( service_dictionary, CFSTR( "PerformanceStatisticsAccum" ) );
            
            if( perf_properties || perf_properties_accum )
            {
                /* Save these details */
                gpudevice.registry_entry = registry_entry;
                gpudevice.service_dictionary = service_dictionary;
                gpudevice.performance_statistics = perf_properties_accum;
                gpudevice.performance_statistics_accum = perf_properties_accum;
                
                /* Save the accelerator name */
                const void* ioclass = CFDictionaryGetValue( service_dictionary, CFSTR( "IOClass" ) );
                if( ioclass )
                {
                    char ioclass_str[2048];
                    
                    if( CFStringGetCString( (CFStringRef) ioclass, ioclass_str, 2048, kCFStringEncodingASCII ) )
                        gpudevice.io_class.append( ioclass_str );
                    else
                        gpudevice.io_class = "N/A";
                }
                
                
                /* Get bundle identifier */
                const void* bundle_id = CFDictionaryGetValue( service_dictionary, CFSTR( "CFBundleIdentifier" ) );
                if( bundle_id )
                {
                    char bundle_id_str[2048];
                    
                    if( CFStringGetCString( (CFStringRef) bundle_id, bundle_id_str, 2048, kCFStringEncodingASCII ) )
                        gpudevice.bundle_identifier.append( bundle_id_str );
                    else
                        gpudevice.bundle_identifier = "N/A";
                }
            
                /* Continue with hardware enumeration */
                continue;
            }
            
            /* If we get this far, then this device is not a GPU, most likely (i.e. Kairo software renderer). */
            CFRelease( service_dictionary );
            IOObjectRelease( registry_entry );
        }
        
        /* We're finished enumerating devices */
        IOObjectRelease( iterator );
    }
    else
    {
        /* TODO: Report failure */
        return 0;
    }
    
    return 1;
}

void IOKIT_Uninitialize()
{
    _LOG( __FUNCTION__ << "IOKIT driver uninitialization completed.\n" );
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
// https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGLDriverMonitorUserGuide/Glossary/Glossary.html

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
