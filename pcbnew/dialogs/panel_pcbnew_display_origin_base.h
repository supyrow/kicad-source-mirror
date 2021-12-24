///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PCBNEW_DISPLAY_ORIGIN_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PCBNEW_DISPLAY_ORIGIN_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxRadioBox* m_DisplayOrigin;
		wxRadioBox* m_XAxisDirection;
		wxRadioBox* m_YAxisDirection;

	public:

		PANEL_PCBNEW_DISPLAY_ORIGIN_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_PCBNEW_DISPLAY_ORIGIN_BASE();

};

