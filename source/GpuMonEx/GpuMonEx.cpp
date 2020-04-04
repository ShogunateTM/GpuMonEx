// GpuMonEx.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "GpuMonEx.h"

#include <wx/wx.h>
#include <wx/string.h>
#include <wx/utils.h>


class GpuMonEx_MainFrame : public wxFrame
{
public:
	GpuMonEx_MainFrame( const wxString& title ) : wxFrame( NULL, wxID_ANY, title, wxDefaultPosition, wxSize( 960, 540 ) )
	{
		Centre();
	}
};

class GpuMonEx : public wxApp
{
public:
	virtual bool OnInit()
	{
		auto mainframe = new GpuMonEx_MainFrame( wxT( "GpuMonEx (Pre-Alpha)" ) );
		mainframe->Show();

		return true;
	}
};

IMPLEMENT_APP( GpuMonEx );