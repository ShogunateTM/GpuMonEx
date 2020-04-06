#pragma once

#ifdef _WIN32
#include "resource.h"
#endif

/*
 * wxWidgets includes
 */
#include <wx/wx.h>
#include <wx/string.h>
#include <wx/utils.h>
#include <wx/msgdlg.h>


namespace gpumonex
{
    namespace wx
    {
        /*
         * wxGpuMonThread base class
         */
        class wxGpuMonThread : public wxThread
        {
        public:
            wxGpuMonThread() : wxThread( wxTHREAD_JOINABLE ) {}
            
        protected:
            wxMutex m_mutex;
        };
        
        /*
         * wxGpuMonHardwareMonitoringThread
         * Periodically checks for changes in GPU hardware
         */
        class wxHardwareMonitoringThread : public wxGpuMonThread
        {
        private:
            wxHardwareMonitoringThread( const wxHardwareMonitoringThread& copy );
            
        public:
            wxHardwareMonitoringThread();
            
        };
    }
}




