///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun 18 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FIND_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FIND_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* searchStringLabel;
		wxComboBox* m_searchCombo;
		wxCheckBox* m_matchCase;
		wxCheckBox* m_matchWords;
		wxCheckBox* m_wildcards;
		wxCheckBox* m_wrap;
		wxCheckBox* m_includeReferences;
		wxCheckBox* m_includeValues;
		wxCheckBox* m_includeTexts;
		wxCheckBox* m_includeMarkers;
		wxCheckBox* m_includeVias;
		wxButton* m_findNext;
		wxButton* m_findPrevious;
		wxButton* m_searchAgain;
		wxButton* m_closeButton;
		wxStaticText* m_status;

		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void onTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onFindNextClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onFindPreviousClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSearchAgainClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCloseButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_FIND_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
		~DIALOG_FIND_BASE();

};

