// GpuMonEx.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "GpuMonEx.h"

#include <wx/wx.h>
#include <wx/string.h>
#include <wx/utils.h>

#include "mainframe.hpp"



class GpuMonEx : public wxApp
{
public:
	virtual bool OnInit()
	{
        auto mainframe = new gpumonex::wx::main_frame( wxT( "GpuMonEx (Pre-Alpha) 0.1" ) );
		mainframe->Show();

		return true;
	}
};

IMPLEMENT_APP( GpuMonEx );
