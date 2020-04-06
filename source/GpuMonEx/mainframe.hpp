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

namespace gpumonex
{
    namespace wx
    {
        /*
         * main_frame: Represents the main window 
         */
        class main_frame : public wxFrame
        {
        public:
            main_frame( const wxString& title ) : wxFrame( NULL, wxID_ANY, title, wxDefaultPosition, wxSize( 960, 540 ) )
            {
                Centre();
            }
        };
    }
}

#endif /* mainframe_hpp */
