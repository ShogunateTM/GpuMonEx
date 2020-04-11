//
//  mainframe.cpp
//  GpuMonEx
//
//  Created by Aeroshogun on 4/4/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#include "mainframe.hpp"

#include "../platform.h"
#include "../gmdebug.h"
#include "../gpumon/DriverEnum.h"

/* Driver handle instance*/
extern GPUDRIVER driver[Drv_MAX];


wxThread::ExitCode gpumonex::wx::wxGpuMonThread::Entry()
{
    wxThread::ExitCode exit_code = nullptr;

    while( true )
    {
        /* Update every second */
        m_panel->Update();
        wxSleep(1);
    }

    return exit_code;
}

            
void gpumonex::wx::wxGeneralPanel::Update()
{

}

void gpumonex::wx::wxGpuStatisticsPanel::Update()
{
    GPUSTATISTICS stats;

    /* This is VERY lazy, I know.  So I'll fix it... later... */
#ifdef _WIN32
    int drvtype = Drv_D3DKMT;
#elif defined(__APPLE__)
    int drvtype = Drv_IOKIT;
#endif
    driver[drvtype].GetOverallGpuLoad( 0, &stats );

    if( stats.gpu_usage != -1 )
        m_text_gpu_usage->SetLabel( wxString::Format( wxT( "GPU Usage: %d%%" ), stats.gpu_usage ) );
    else
        m_text_gpu_usage->SetLabel( wxString::Format( wxT( "GPU Usage: ?" ) ) );
    
    if( stats.video_engine_usage != -1 )
        m_text_video_engine_usage->SetLabel( wxString::Format( wxT( "Video Engine Usage: %d%%" ), stats.video_engine_usage ) );
    else
        m_text_video_engine_usage->SetLabel( wxString::Format( wxT( "Video Engine Usage: ?" ) ) );
    
    if( stats.vram_free != -1 )
        m_text_vmem_free->SetLabel( wxString::Format( wxT( "Video Memory Free: %ld MB" ), stats.vram_free/(1000*1000) ) );
    else
        m_text_vmem_free->SetLabel( wxString::Format( wxT( "Video Memory Free: ? MB" ) ) );
    
    if( stats.vram_used != -1 )
        m_text_vmem_used->SetLabel( wxString::Format( wxT( "Video Memory Used: %ld MB" ), stats.vram_used/(1000*1000) ) );
    else
        m_text_vmem_used->SetLabel( wxString::Format( wxT( "Video Memory Used: ? MB" ) ) );
    
    if( stats.hw_wait_time != -1 )
        m_text_hw_wait_time->SetLabel( wxString::Format( wxT( "Hardware Wait Time (ns): %ld" ), stats.hw_wait_time ) );
    else
        m_text_hw_wait_time->SetLabel( wxString::Format( wxT( "Hardware Wait Time (ns): ?" ) ) );
    
    if( stats.fan_speed_percentage != -1 )
        m_text_fan_speed_percent->SetLabel( wxString::Format( wxT( "Fan Speed (%%): %d%%" ), stats.fan_speed_percentage ) );
    else
        m_text_fan_speed_percent->SetLabel( wxString::Format( wxT( "Fan Speed (%%): ?" ) ) );
    
    if( stats.fan_speed_rpms != -1)
        m_text_fan_speed_rpms->SetLabel( wxString::Format( wxT( "Fan Speed (RPM): %d" ), stats.fan_speed_rpms ) );
    else
        m_text_fan_speed_rpms->SetLabel( wxString::Format( wxT( "Fan Speed (RPM): ?" ) ) );
    
    if( stats.temperature != -1 )
        m_text_temperature->SetLabel( wxString::Format( wxT( "Temperature: %dc" ), stats.temperature ) );
    else
        m_text_temperature->SetLabel( wxString::Format( wxT( "Temperature: ?" ) ) );
    
    if( stats.power_usage != -1 )
        m_text_power_usage->SetLabel( wxString::Format( wxT( "Power Usage: %dW" ), stats.power_usage ) );
    else
        m_text_power_usage->SetLabel( wxString::Format( wxT( "Power Usage: ?" ) ) );
    
}

void gpumonex::wx::wxProcessMonitorPanel::Update()
{

}
