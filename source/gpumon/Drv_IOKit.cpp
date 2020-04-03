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

#include <IOKit/IOKitLib.h>
#include <IOKit/graphics/IOAccelClientConnect.h>

#include <vector>
#include <strstream>



bool IOKIT_GetAcceleratorDictionaryNumberValue( int adapter, const char* dictionary_class, const char* dictionary_subclass, void* value )
{
    bool return_val = false;
    int current_adapter = 0;
    
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
            
            if( IORegistryEntryCreateCFProperties( registry_entry, &service_dictionary, kCFAllocatorDefault, kNilOptions ) != kIOReturnSuccess || current_adapter++ != adapter )
            {
                /* Service dictionary creation failed, move on to the next device */
                IOObjectRelease( registry_entry );
                continue;
            }
            
            CFMutableDictionaryRef perf_properties = (CFMutableDictionaryRef) CFDictionaryGetValue( service_dictionary, __CFStringMakeConstantString( dictionary_class ) );
            if( perf_properties )
            {
                if( dictionary_subclass )
                {
                    auto subclass = CFDictionaryGetValue( perf_properties, __CFStringMakeConstantString( dictionary_subclass ) );
                    
                    if( subclass != nullptr )
                        return_val = CFNumberGetValue( (CFNumberRef) subclass, kCFNumberSInt64Type, value );
                }
                else
                {
                    return_val = CFNumberGetValue( (CFNumberRef) perf_properties, kCFNumberSInt64Type, value );
                }
            }
            
            /* If we get this far, then this device is not a GPU, most likely (i.e. Kairo software renderer). */
            CFRelease( service_dictionary );
            IOObjectRelease( registry_entry );
        }
        
        /* We're finished enumerating devices */
        IOObjectRelease( iterator );
    }
    
    return return_val;
}


bool IOKIT_GetAcceleratorDictionaryStringValue( int adapter, const char* dictionary_class, const char* dictionary_subclass, void* value )
{
    bool return_val = false;
    int current_adapter = 0;
    
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
            
            if( IORegistryEntryCreateCFProperties( registry_entry, &service_dictionary, kCFAllocatorDefault, kNilOptions ) != kIOReturnSuccess || current_adapter++ != adapter )
            {
                /* Service dictionary creation failed, move on to the next device */
                IOObjectRelease( registry_entry );
                continue;
            }
            
            CFMutableDictionaryRef perf_properties = (CFMutableDictionaryRef) CFDictionaryGetValue( service_dictionary, __CFStringMakeConstantString( dictionary_class ) );
            if( perf_properties )
            {
                char str[2048];
                 
                if( dictionary_subclass )
                {
                    auto subclass = CFDictionaryGetValue( perf_properties, __CFStringMakeConstantString( dictionary_subclass ) );
                    
                    if( subclass != nullptr )
                        return_val = CFStringGetCString( (CFStringRef) subclass, str, 2048, kCFStringEncodingASCII ); //CFNumberGetValue( (CFNumberRef) subclass, kCFNumberSInt64Type, value );
                }
                else
                {
                    return_val = CFStringGetCString( (CFStringRef) perf_properties, str, 2048, kCFStringEncodingASCII );
                }
                
                strcpy( (char*) value, str );
            }
            
            /* If we get this far, then this device is not a GPU, most likely (i.e. Kairo software renderer). */
            CFRelease( service_dictionary );
            IOObjectRelease( registry_entry );
        }
        
        /* We're finished enumerating devices */
        IOObjectRelease( iterator );
    }
    
    return return_val;
}


/* Logging file */
std::ofstream logfi;


int IOKIT_Initialize()
{
    _LOG( __FUNCTION__ << "(): " << "IOKIT driver initialization started...\n" );
    
#if 0
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
                gpudevice.performance_statistics = perf_properties;
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
            
                /* Save this device description */
                gpudevices.push_back( gpudevice );
                
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
#endif
    
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

int IOKIT_GetOverallGpuLoad( int AdapterNumber, GPUSTATISTICS* pGpuStatistics )
{
    ssize_t utilization = -1;
    
    /* Reset statistics */
    if( !pGpuStatistics )
        return GERR_BADPARAMS;
    
    memset( pGpuStatistics, 0, sizeof( GPUSTATISTICS ) );
    
    /* NOTE: It appears that GPU usage can be queried from either "PerformanceStatistics" or "PerformanceStatisticsAccum", only one
     * sometimes has slightly different information.  For the time being, for overall GPU usage, we will simply stick with the former.
     */
    
    /* Attempt to query the GPU usage.
     *
     * NOTE: As macOS evolves, the key names within the PerformanceStatistics dictionary become subject to change.  Known key names
     * include:
     *  - "GPU Core Utilization"
     *  - "Device Utilization"
     *  - "GPU Activity"
     *
     * What we're going to do is check for each one in ascending order.  If we get positive return value, then we'll go ahead and stick with
     * that.  If not, then it means that this driver does not report GPU usage directly.
     *
     * Now, if NONE of these work, it's theoretically not impossible to get the GPU usage.  Hypothetically speaking, by querying the key
     * "hardwareWaitTime", we can divide this number by the GPU's frequency in nanoherz.  So far, I've only seen this scenario come up once
     * with the Intel GMA 950.  No modern revisions of macOS support that GPU anyway, but this is meant to work on older versions of macOS.
     * The next challenge becomes getting the GPU clock frequency programatically.
     *
     * So until this scenario actually comes up, I won't worry about it too much, but it would make for an interesting test case down the road.
     * So for now, we will just return -1.
     */
    
    pGpuStatistics->gpu_usage = -1;
    
    for( int i = 0; i < 3; i++ )
    {
        const char* keys[] = { "GPU Activity", "Device Utilization", "GPU Core Utilization" };
        
        if( IOKIT_GetAcceleratorDictionaryNumberValue( AdapterNumber, "PerformanceStatistics", keys[i], &utilization ) )
        {
            /* Get a proper percentage if it isn't giving us one already */
            if( utilization > 100 )
                utilization /= 10000000;
            
            pGpuStatistics->gpu_usage = int(utilization);
            
            break;
        }
    }
    
    /* Get GPU Video Engine usage */
    if( IOKIT_GetAcceleratorDictionaryNumberValue( AdapterNumber, "PerformanceStatistics", "GPU Video Engine Utilization", &utilization ) )
    {
        /* Get a proper percentage if it isn't giving us one already */
        if( utilization > 100 )
            utilization /= 10000000;
        
        pGpuStatistics->video_engine_usage = int(utilization);
    }
    else
        pGpuStatistics->video_engine_usage = -1;
    
    /* Get GPU Device Unit N Utilization.  The number of units varies and so far, I've seen this go up to 4. 
     *
     * TODO: I've only seen this attribute with Intel GPUs so far.  Not sure how many there are in modern hardware revisions either...
     */
    
    for( int i = 0; i < 8; i++ )
    {
        std::strstream ss;
        
        ss << "Device Unit " << i << " Utilization";
        
        if( IOKIT_GetAcceleratorDictionaryNumberValue( AdapterNumber, "PerformanceStatistics", ss.str(), &utilization ) )
        {
            /* Get a proper percentage if it isn't giving us one already */
            if( utilization > 100 )
                utilization /= 10000000;
            
            pGpuStatistics->device_unit_usage[i] = int(utilization);
        }
        else
        {
            pGpuStatistics->device_unit_usage[i] = -1;
        }
    }
    
    /* Get Video Memory usage statistics
     *
     * NOTE: Like above with GPU usage/utilization, the names changed in later revisions of macOS.  Known values:
     * - vramFreeBytes
     * - vramUsedBytes
     * - "VRAM,totalMB"
     * 
     * Now, vramFreeBytes should be available for all GPUs, but vramUsedBytes was removed somewhere down the line.  If the latter does
     * not exist in this driver, we will have to query the total VRAM from either "PerformanceStatistics[Accum]" or IOPCIDevice.  Intel
     * stores the information in the former and AMD in the latter.  Not sure about NV as of yet since at the time of writing, my Mac Pro
     * doesn't support any OS beyond El Capitan (10.11.6) and NV support in the latest version(s) of macOS have been sorely lacking any,
     * and NV has no comments last I checked.
     */
    
    if( IOKIT_GetAcceleratorDictionaryNumberValue( AdapterNumber, "PerformanceStatistics", "vramFreeBytes", &utilization ) )
    {
        pGpuStatistics->vram_free = utilization;
        
        if( IOKIT_GetAcceleratorDictionaryNumberValue( AdapterNumber, "PerformanceStatistics", "vramUsedBytes", &utilization ) )
        {
            pGpuStatistics->vram_used = utilization;
        }
        else
        {
            ssize_t vram = 0;
            
            if( IOKIT_GetAcceleratorDictionaryNumberValue( AdapterNumber, "PerformanceStatistics", "VRAM,totalMB", &vram ) )
            {
                pGpuStatistics->vram_used = ( vram * 1000 * 1000 ) - pGpuStatistics->vram_free;
            }
            else
            {
                /* TODO: Get "VRAM,totalMB" from IOPCIDevice instead... */
            }
        }
    }
    
    /* Get GPU hardware wait time (in nanoseconds) */
    if( IOKIT_GetAcceleratorDictionaryNumberValue( AdapterNumber, "PerformanceStatistics", "hardwareWaitTime", &utilization ) )
        pGpuStatistics->hw_wait_time = utilization;
    else
        pGpuStatistics->hw_wait_time = -1;
    
    /* These don't appear to be available on El Capitan (10.11.6), and I don't have a newer mac to test these on ... */
    /*"Fan Speed(%)"=13
     "GPU Activity(%)"=0
     "Fan Speed(RPM)"=577
     "Temperature(C)"=51
     "Total Power(W)"=107*/
    
    /* Get GPU fan speed (percentage of max speed and current RPMs) */
    if( IOKIT_GetAcceleratorDictionaryNumberValue( AdapterNumber, "PerformanceStatistics", "Fan Speed(%)", &utilization ) )
        pGpuStatistics->fan_speed_percentage = int(utilization);
    else
        pGpuStatistics->fan_speed_percentage = -1;
    
    if( IOKIT_GetAcceleratorDictionaryNumberValue( AdapterNumber, "PerformanceStatistics", "Fan Speed(RPM)", &utilization ) )
        pGpuStatistics->fan_speed_rpms = int(utilization);
    else
        pGpuStatistics->fan_speed_rpms = -1;
    
    /* Get GPU temperature (in celsius) */
    if( IOKIT_GetAcceleratorDictionaryNumberValue( AdapterNumber, "PerformanceStatistics", "Temperature", &utilization ) )
        pGpuStatistics->temperature = int(utilization);
    else
        pGpuStatistics->temperature = -1;
    
    /* Get current power consumption level (in watts) */
    if( IOKIT_GetAcceleratorDictionaryNumberValue( AdapterNumber, "PerformanceStatistics", "Total Power", &utilization ) )
        pGpuStatistics->power_usage = int(utilization);
    else
        pGpuStatistics->power_usage = -1;
    
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
// https://www.tonymacx86.com/threads/smbios-19-x-imacs-2019.274686/page-49

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
