//
//  main.m
//  gpumontest
//
//  Created by Aeroshogun on 3/29/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

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
                
                const void* bundle_id = CFDictionaryGetValue( serviceDictionary, CFSTR( "CFBundleIdentifier" ) );
                if( bundle_id )
                {
                    char bundle_id_str[2048];
                    
                    if( CFStringGetCString( (CFStringRef) bundle_id, bundle_id_str, 2048, kCFStringEncodingASCII ) )
                    {
                        NSLog( @"Bundle ID: %s", bundle_id_str );
                    }
                }
                
                CFMutableDictionaryRef perf_properties = (CFMutableDictionaryRef) CFDictionaryGetValue( serviceDictionary, CFSTR("PerformanceStatistics") );
                if (perf_properties) {
                    
                    static ssize_t gpuCoreUse=0;
                    static ssize_t freeVramCount=0;
                    static ssize_t usedVramCount=0;
                    static ssize_t gpuVideoEngineUse=0;
                    
                    const void* gpuCoreUtilization = CFDictionaryGetValue(perf_properties, CFSTR("GPU Core Utilization"));
                    const void* gpuVideoEngineUtilization = CFDictionaryGetValue(perf_properties, CFSTR("GPU Video Engine Utilization"));
                    const void* freeVram = CFDictionaryGetValue(perf_properties, CFSTR("vramFreeBytes"));
                    const void* usedVram = CFDictionaryGetValue(perf_properties, CFSTR("vramUsedBytes"));
                    if (gpuCoreUtilization && freeVram && usedVram)
                    {
                        CFNumberGetValue( (CFNumberRef) gpuCoreUtilization, kCFNumberSInt64Type, &gpuCoreUse);
                        CFNumberGetValue( (CFNumberRef) freeVram, kCFNumberSInt64Type, &freeVramCount);
                        CFNumberGetValue( (CFNumberRef) usedVram, kCFNumberSInt64Type, &usedVramCount);
                        NSLog(@"GPU: %.3f%% VRAM: %.3f%%",gpuCoreUse/(double)10000000,usedVramCount/(double)(freeVramCount+usedVramCount)*100.0);
                        
                    }
                    
                    if( gpuVideoEngineUtilization )
                    {
                        CFNumberGetValue( (CFNumberRef) gpuVideoEngineUtilization, kCFNumberSInt64Type, &gpuVideoEngineUse );
                        NSLog(@"Video Engine: %.3f%%", gpuVideoEngineUse > 100 ? gpuVideoEngineUse/(double)10000000 : gpuVideoEngineUse );
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
