//
//  mainframe.hpp
//  GpuMonEx
//
//  Created by Aeroshogun on 4/4/20.
//  Copyright © 2020 Shogunate Technologies. All rights reserved.
//

#ifndef mainframe_hpp
#define mainframe_hpp

#include <stdio.h>
#include <sstream>
#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include "statschart.hpp"

#include "../gpumon/DriverEnum.h"



namespace gpumonex
{
    namespace wx
    {
        class wxGpuMonThread;

        /*
         * wxGpuMonPanel
         * Base panel class, making life a little easier...
         */
        class wxGpuMonPanel : public wxPanel
        {
        public:
            wxGpuMonPanel( wxNotebook* parent, wxString title ) : wxPanel( parent, wxID_ANY ), m_title( title ) {}

            /* Return the title of the tab */
            virtual wxString GetPanelTitle() { return m_title; }
            /* Called within thread class below */
            virtual void Update() = 0;

        protected:
            wxString        m_title;
            wxGpuMonThread* m_thread;
        };

        
        class wxGpuMonThread : public wxThread
        {
        public:
            wxGpuMonThread( wxGpuMonPanel* panel ) : wxThread(), m_panel( panel ) {}

            wxThread::ExitCode Entry();

        protected:
            wxGpuMonPanel* m_panel;
        };


        class wxGeneralPanel : public wxGpuMonPanel
        {
        public:
            wxGeneralPanel( wxNotebook* parent, wxString title ) : wxGpuMonPanel( parent, title )
            {
                m_thread = new wxGpuMonThread( this );
                if( !m_thread )
                    return;

                /* Driver handle instance*/
                extern GPUDRIVER driver[Drv_MAX];
                GPUDETAILS gpudetails;
                wxArrayString   m_arrItems;
                int i = 0;

                while(true)
                {
                    if( !driver[Drv_BASE].GetGpuDetails( i++, &gpudetails ) )
                        break;

                    std::stringstream ss;
                    ss << gpudetails.DeviceDesc /*<< " (Vendor ID: 0x" << gpudetails.VendorID << " Device ID: 0x" << gpudetails.DeviceID << ")"*/;
                    
                    // Create common wxArrayString array
                    m_arrItems.Add( wxString::FromUTF8( ss.str() ) );
                }

                /* 
                 * Display details of the primary driver version first 
                 */

                m_combobox = new wxComboBox(this, -1, m_arrItems[0], wxDefaultPosition+wxPoint( 20, 0 ), wxDefaultSize, m_arrItems, 0, wxDefaultValidator, _T("ID_COMBOBOX1"));

                i = 2;

                extern bool GetDriverVersion( std::string& strVersionNumber, int GpuNumber );

                std::stringstream ss, ss2, ss3;
                std::string vn;

                m_text[0] = new wxStaticText( this, -1, "Display Adapter", wxPoint(0, 20));

                GetDriverVersion( vn, 0 );
                driver[Drv_BASE].GetGpuDetails( 0, &gpudetails );
                ss3 << "Driver version: " << vn;
                m_text[1] = new wxStaticText( this, -1, ss3.str(), wxPoint(0, 20 * i)); i++;
                ss << "Vendor ID: 0x" << gpudetails.VendorID;
                m_text[2] = new wxStaticText( this, -1, ss.str(), wxPoint(0, 20 * i)); i++;
                ss2 << "Device ID: 0x" << gpudetails.DeviceID;
                m_text[3] = new wxStaticText( this, -1, ss2.str(), wxPoint(0, 20 * i)); i++;

                auto error = m_thread->Create();
                if( error != wxTHREAD_NO_ERROR )
                {
                    wxMessageBox( "Error creating wxGeneralPanel thread!" );
                    return;
                }

                error = m_thread->Run();
                if( error != wxTHREAD_NO_ERROR )
                    wxMessageBox( "Error running wxGeneralPanel thread!" );
            }
            
            virtual ~wxGeneralPanel()
            {
                if( m_thread )
                {
                    m_thread->Kill();
                }
            }


            virtual void Update();

        private:
            wxComboBox* m_combobox;

        protected:
            wxStaticText* m_text[4];
        };
        
        class wxGpuStatisticsPanel : public wxGpuMonPanel
        {
        public:
            wxGpuStatisticsPanel( wxNotebook* parent, wxString title ) : wxGpuMonPanel( parent, title )
            {
                int i = 0;
                m_text_gpu_usage = new wxStaticText( this, -1, "Overall GPU Usage: 0%", wxPoint( 0, 20*i ) ); i++;
                m_text_video_engine_usage = new wxStaticText( this, -1, "Video Engine Usage: 0%", wxPoint( 0, 20*i ) ); i++;
                m_text_vmem_free = new wxStaticText( this, -1, "Video Memory Free: 0", wxPoint( 0, 20*i ) ); i++;
                m_text_vmem_used = new wxStaticText( this, -1, "Video Memory Used: 0", wxPoint( 0, 20*i ) ); i++;
                m_text_hw_wait_time = new wxStaticText( this, -1, "Hardware Wait Time (ns): 0", wxPoint( 0, 20*i ) ); i++;
                m_text_fan_speed_percent = new wxStaticText( this, -1, "Fan Speed (%): 0%", wxPoint( 0, 20*i ) ); i++;
                m_text_fan_speed_rpms = new wxStaticText( this, -1, "Fan Speed (RPM): 0", wxPoint( 0, 20*i ) ); i++;
                m_text_temperature = new wxStaticText( this, -1, "Temperature: 0c", wxPoint( 0, 20*i ) ); i++;
                m_text_power_usage = new wxStaticText( this, -1, "Power Usage: 0W", wxPoint( 0, 20*i ) ); i++;
                
                /* m_plot = new mpWindow( this, -1, wxPoint(0,0), wxSize(100,100), wxSUNKEN_BORDER );
                 m_plot->AddLayer(     new mpScaleX( wxT("Ganzzahl N")));
                 m_plot->AddLayer(     new mpScaleY( wxT("Kosten K(N) in Bits")));
                 m_plot->AddLayer(     new FixedBitwidth(32) );
                 m_plot->AddLayer(     new Optimum() );
                 m_plot->AddLayer( e = new Elias() );
                 m_plot->AddLayer( f = new Fibonacci() );
                 
                 e->SetPen( wxPen(*wxRED, 1, wxSOLID));
                 f->SetPen( wxPen(*wxGREEN, 1, wxSOLID));
                 
                 wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
                 topsizer->Add( m_plot, 1, wxEXPAND );
                 SetAutoLayout( TRUE );
                 SetSizer( topsizer );*/
                
                m_gpu_usage_histogram = new wxGpuUsage(0);
                m_gpu_usage_histogram->SetContinuity( true );
                m_gpu_usage_histogram->SetName( wxT( "GPU Usage" ) );
                m_gpu_usage_histogram->ShowName( true );

                m_plot = new mpWindow( this, -1, wxPoint( 0, (20*i)+20 ), wxSize( 620, 100 ), wxSUNKEN_BORDER );
                m_plot->AddLayer( m_gpu_usage_histogram /* new MySIN( 10.0, 220.0 )*/ );
                
                /*wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
                topsizer->Add( m_plot, 1, wxEXPAND );
                SetAutoLayout( TRUE );
                SetSizer( topsizer );*/
                
                
                m_thread = new wxGpuMonThread( this );
                if( !m_thread )
                    return;

                auto error = m_thread->Create();
                if( error != wxTHREAD_NO_ERROR )
                {
                    wxMessageBox( "Error creating wxGpuStatisticsPanel thread!" );
                    return;
                }

                error = m_thread->Run();
                if( error != wxTHREAD_NO_ERROR )
                    wxMessageBox( "Error running wxGpuStatisticsPanel thread!" );
            }
            
            virtual ~wxGpuStatisticsPanel()
            {
                if( m_thread )
                {
                    m_thread->Kill();
                }
            }


            virtual void Update();

            void ResetHistogram()
            {
                m_gpu_usage_histogram->Clear();

                std::vector<double> vx, vy;

                for( int i = 0; i < 300; i++ )
                    vx.push_back(0), vy.push_back(0);

                m_gpu_usage_histogram->SetData( vx, vy );
                m_plot->Fit();
            }
            
        protected:
            wxStaticText* m_text_gpu_usage;
            wxStaticText* m_text_video_engine_usage;
            wxStaticText* m_text_device_unit_usage[8];
            wxStaticText* m_text_vmem_free;
            wxStaticText* m_text_vmem_used;
            wxStaticText* m_text_hw_wait_time;
            wxStaticText* m_text_fan_speed_percent;
            wxStaticText* m_text_fan_speed_rpms;
            wxStaticText* m_text_temperature;
            wxStaticText* m_text_power_usage;
            
            wxGpuUsage* m_gpu_usage_histogram;

            mpWindow* m_plot;
        };
        
        class wxProcessMonitorPanel : public wxGpuMonPanel
        {
        public:
            wxProcessMonitorPanel( wxNotebook* parent, wxString title ) : wxGpuMonPanel( parent, title )
            {
                m_text = new wxStaticText( this, -1, "TODO", wxDefaultPosition );

                m_thread = new wxGpuMonThread( this );
                if( !m_thread )
                    return;

                auto error = m_thread->Create();
                if( error != wxTHREAD_NO_ERROR )
                {
                    wxMessageBox( "Error creating wxProcessMonitorPanel thread!" );
                    return;
                }

                error = m_thread->Run();
                if( error != wxTHREAD_NO_ERROR )
                    wxMessageBox( "Error running wxProcessMonitorPanel thread!" );
            }

            virtual ~wxProcessMonitorPanel()
            {
                if( m_thread )
                {
                    m_thread->Kill();
                }
            }

            
            virtual void Update();

        protected:
            wxStaticText* m_text;
        };
        
        
        /*
         * main_frame: Represents the main window 
         */
        class main_frame : public wxFrame
        {
        public:
            main_frame( const wxString& title ) : wxFrame( NULL, wxID_ANY, title, wxDefaultPosition, wxSize( 640, 480 ) )
            {
                Centre();
                
                m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxSize( 640, 480 ) );
                
                m_window1 = new wxGeneralPanel( m_notebook, "General" );
                m_window2 = new wxGpuStatisticsPanel( m_notebook, "GPU Statistics" );
                m_window3 = new wxProcessMonitorPanel( m_notebook, "Process Monitor" );
                
                m_notebook->AddPage( m_window1, m_window1->GetPanelTitle(), true, 0 );
                m_notebook->AddPage( m_window2, m_window2->GetPanelTitle(), false, 1 );
                m_notebook->AddPage( m_window3, m_window3->GetPanelTitle(), false, 2 );

                m_window2->ResetHistogram();
            }

        protected:
            wxNotebook*             m_notebook;
            wxGeneralPanel*         m_window1;
            wxGpuStatisticsPanel*   m_window2;
            wxProcessMonitorPanel*  m_window3;
        };
    }
}

#endif /* mainframe_hpp */
