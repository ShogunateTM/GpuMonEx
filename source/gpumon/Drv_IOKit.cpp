//
//  Drv_IOKit.cpp
//  gpumon
//
//  Created by Aeroshogun on 3/17/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//


/*
 * References
 *
 * All of the code below is largely based off of information that I have found on the following links:
 *
 * https://stackoverflow.com/questions/10110658/programmatically-get-gpu-percent-usage-in-os-x
 * https://gist.github.com/chockenberry/2afe4d0f1f9caddc81de
 * https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGLDriverMonitorUserGuide/Glossary/Glossary.html
 * https://www.tonymacx86.com/threads/smbios-19-x-imacs-2019.274686/page-49
 *
 * Much of this information is a bit dated, but it was the best I could find at the time being.  As macOS continues to evolve,
 * many of the methods by which we access such data either changes or ceases to exist, so keeping this up to date with every
 * update of the OS is quite important.
 */


#include "../platform.h"
#include "../debug.h"
#include "Drv_IOKIT.h"

#include <IOKit/IOKitLib.h>
#include <IOKit/graphics/IOAccelClientConnect.h>

#include <vector>
#include <strstream>


/* Basic description of the GPUs enumerated on this machine */
std::vector<GPUDETAILS> gpudesc;



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

bool IOKIT_GetPCIDeviceDictionaryStringValue( int adapter, const char* dictionary_class, const char* dictionary_subclass, void* value )
{
    bool return_val = false;
    int current_adapter = 0;
    
    CFMutableDictionaryRef match_dictionary = IOServiceMatching( "IOPCIDevice" );
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
    int adapter = -1;
    
    _LOG( __FUNCTION__ << "(): " << "IOKIT driver initialization started...\n" );
    
    /*char str[2048];
    GPUDETAILS gd;
    
    if( IOKIT_GetPCIDeviceDictionaryStringValue( 0, "name", NULL, str ) )
        strcpy( gd.DeviceDesc, str );
    if( IOKIT_GetAcceleratorDictionaryStringValue( 0, "CFBundleIdentifier", NULL, str ) )
        strcpy( gd.DriverDesc, str );*/
    
#if 1
    CFMutableDictionaryRef match_dictionary = IOServiceMatching( "IOPCIDevice" );
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
            
            GPUDETAILS gpudetails;
            
            /* Get the name/model of this GPU */
            const void* gpu_model = CFDictionaryGetValue( service_dictionary, __CFStringMakeConstantString( "model" ) );
            if( gpu_model )
            {
                char str[128];
            
                adapter++;
                
                if( CFGetTypeID( gpu_model ) == CFStringGetTypeID() )
                {
                    if( CFStringGetCString( (CFStringRef) gpu_model, str, 128, kCFStringEncodingASCII ) )
                    {
                        strcpy( gpudetails.DeviceDesc, str );
                        
                        /* TODO: Device and Vendor IDs */
                    }
                }
                else if( CFGetTypeID( gpu_model ) == CFDataGetTypeID() )
                {
                    size_t length = CFDataGetLength( (CFDataRef) gpu_model );
                    
                    CFDataGetBytes( (CFDataRef) gpu_model, CFRangeMake( 0, length ), (UInt8*) str );
                    strcpy( gpudetails.DeviceDesc, str );
                }
                
                /* Now attempt to get the driver name of associated with this GPU */
                if( IOKIT_GetAcceleratorDictionaryStringValue( 0, "CFBundleIdentifier", NULL, str ) )
                {
                    strcpy( gpudetails.DriverDesc, str );
                }
                
                /* Save it... */
                gpudesc.push_back( gpudetails );
            }
            
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
    gpudesc.clear();
    
    _LOG( __FUNCTION__ << "IOKIT driver uninitialization completed.\n" );
}

int IOKIT_GetGpuDetails( int AdapterNumber, GPUDETAILS* pGpuDetails )
{
    auto gd = gpudesc[AdapterNumber];
    memmove( pGpuDetails, &gd, sizeof( GPUDETAILS ) );
    
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

int IOKIT_GetProcessGpuLoad( int AdapterNumber, void* pProcess )
{
    /* TODO: No idea how this is done for macOS, but there has to be a way */
    
    return 0;
}

int IOKIT_GetGpuTemperature( int AdapterNumber )
{
    return 0;
}
