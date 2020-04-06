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


/*
 import wx
 
 # Define the tab content as classes:
 class TabOne(wx.Panel):
 def __init__(self, parent):
 wx.Panel.__init__(self, parent)
 t = wx.StaticText(self, -1, "This is the first tab", (20,20))
 
 class TabTwo(wx.Panel):
 def __init__(self, parent):
 wx.Panel.__init__(self, parent)
 t = wx.StaticText(self, -1, "This is the second tab", (20,20))
 
 class TabThree(wx.Panel):
 def __init__(self, parent):
 wx.Panel.__init__(self, parent)
 t = wx.StaticText(self, -1, "This is the third tab", (20,20))
 
 class TabFour(wx.Panel):
 def __init__(self, parent):
 wx.Panel.__init__(self, parent)
 t = wx.StaticText(self, -1, "This is the last tab", (20,20))
 
 
 class MainFrame(wx.Frame):
 def __init__(self):
 wx.Frame.__init__(self, None, title="wxPython tabs example @pythonspot.com")
 
 # Create a panel and notebook (tabs holder)
 p = wx.Panel(self)
 nb = wx.Notebook(p)
 
 # Create the tab windows
 tab1 = TabOne(nb)
 tab2 = TabTwo(nb)
 tab3 = TabThree(nb)
 tab4 = TabFour(nb)
 
 # Add the windows to tabs and name them.
 nb.AddPage(tab1, "Tab 1")
 nb.AddPage(tab2, "Tab 2")
 nb.AddPage(tab3, "Tab 3")
 nb.AddPage(tab4, "Tab 4")
 
 # Set noteboook in a sizer to create the layout
 sizer = wx.BoxSizer()
 sizer.Add(nb, 1, wx.EXPAND)
 p.SetSizer(sizer)
 
 
 if __name__ == "__main__":
 app = wx.App()
 MainFrame().Show()
 app.MainLoop()*/


/*
 #include "wx/notebook.h" 
 #include "copy.xpm" 
 #include "cut.xpm" 
 #include "paste.xpm" // Create the notebook
 wxNotebook* notebook = new wxNotebook(parent, wxID_ANY,   wxDefaultPosition, wxSize(300, 200)); 
 // Create the image list 
 wxImageList* imageList = new wxImageList(16, 16, true, 3); 
 imageList->Add(wxIcon(copy_xpm)); 
 imageList->Add(wxIcon(paste_xpm)); 
 imageList->Add(wxIcon(cut_xpm)); 
 // Create and add the pages 
 wxPanel1* window1 = new wxPanel(notebook, wxID_ANY); 
 wxPanel2* window2 = new wxPanel(notebook, wxID_ANY); 
 wxPanel3* window3 = new wxPanel(notebook, wxID_ANY); 
 notebook->AddPage(window1, wxT("Tab one"), true, 0); 
 notebook->AddPage(window2, wxT("Tab two"), false, 1); 
 notebook->AddPage(window3, wxT("Tab three"), false 2);
 */

namespace gpumonex
{
    namespace wx
    {
        class wxGeneralPanel : public wxPanel
        {
        public:
            wxGeneralPanel( wxNotebook* parent, wxString title ) : wxPanel( parent, wxID_ANY ), m_title( title )
            {
                
            }
            
            wxString GetPanelTitle() { return m_title; }
            
        protected:
            wxString m_title;
            wxStaticText* m_text;
        };
        
        class wxGpuStatisticsPanel : public wxPanel
        {
        public:
            wxGpuStatisticsPanel( wxNotebook* parent, wxString title ) : wxPanel( parent, wxID_ANY ), m_title( title )
            {
                m_text = new wxStaticText( this, -1, "GPU Usage: 0%", wxDefaultPosition );
            }
            
            wxString GetPanelTitle() { return m_title; }
            
        protected:
            wxString m_title;
            wxStaticText* m_text;
        };
        
        class wxProcessMonitorPanel : public wxPanel
        {
        public:
            wxProcessMonitorPanel( wxNotebook* parent, wxString title ) : wxPanel( parent, wxID_ANY ), m_title( title )
            {
                m_text = new wxStaticText( this, -1, "TODO", wxDefaultPosition );
            }
            
            wxString GetPanelTitle() { return m_title; }
            
        protected:
            wxString m_title;
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
                
                wxNotebook* notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxSize( 640, 480 ) );
                
                wxGeneralPanel* window1 = new wxGeneralPanel( notebook, "General" );
                wxGpuStatisticsPanel* window2 = new wxGpuStatisticsPanel( notebook, "GPU Statistics" );
                wxProcessMonitorPanel* window3 = new wxProcessMonitorPanel( notebook, "Process Monitor" );
                
                notebook->AddPage( window1, window1->GetPanelTitle(), true, 0 );
                notebook->AddPage( window2, window2->GetPanelTitle(), false, 1 );
                notebook->AddPage( window3, window3->GetPanelTitle(), false, 2 );
            }
        };
    }
}

#endif /* mainframe_hpp */
