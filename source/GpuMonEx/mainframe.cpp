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

    driver[Drv_D3DKMT].GetOverallGpuLoad( 0, &stats );

    m_text->SetLabel( wxString::Format( wxT( "GPU Usage: %d%%" ), stats.gpu_usage ) );
}

void gpumonex::wx::wxProcessMonitorPanel::Update()
{

}