//
//  timer_t.h
//  gpumonproc
//
//  Created by Aeroshogun on 4/28/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#ifndef timer_t_h
#define timer_t_h

/*
 * Basic timer class
 */
class timer_t
{
public:
    /* Initialize high resolution timing functionality */
    timer_t()
    {
#ifdef __APPLE__
        mach_timebase_info( &timebase_info );       /* Determine timing scale */
#endif
    }
    
    /* Get starting time */
    void start()
    {
#ifdef __APPLE__
        starttime = mach_absolute_time();
#endif
    }
    
    /* Get stop time */
    void stop()
    {
#ifdef __APPLE__
        stoptime = mach_absolute_time();
#endif
    }
    
    /* Get time elapsed (return in miliseconds) */
    uint64_t elapsed()
    {
        uint64_t elapsedtime = 0;
        
#ifdef __APPLE__
        elapsedtime = (stoptime - starttime) * timebase_info.numer / timebase_info.denom;
        elapsedtime /= 1000000;
#endif
        
        return elapsedtime;
    }
    
    uint64_t starttime;
    uint64_t stoptime;
    
#ifdef __APPLE__
    mach_timebase_info_data_t timebase_info;
#endif
};

#endif /* timer_t_h */
