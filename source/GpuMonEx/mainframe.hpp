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
#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/notebook.h>



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

        protected:
            wxStaticText* m_text;
        };
        
        class wxGpuStatisticsPanel : public wxGpuMonPanel
        {
        public:
            wxGpuStatisticsPanel( wxNotebook* parent, wxString title ) : wxGpuMonPanel( parent, title )
            {
                m_text = new wxStaticText( this, -1, "GPU Usage: 0%", wxDefaultPosition );

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
            
        protected:
            wxStaticText* m_text;
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
