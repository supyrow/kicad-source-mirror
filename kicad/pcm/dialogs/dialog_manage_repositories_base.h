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
class WX_GRID;

#include "dialog_shim.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MANAGE_REPOSITORIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MANAGE_REPOSITORIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_GRID* m_grid;
		wxBitmapButton* m_buttonAdd;
		wxBitmapButton* m_buttonMoveUp;
		wxBitmapButton* m_buttonMoveDown;
		wxBitmapButton* m_buttonRemove;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1Save;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnGridCellClicked( wxGridEvent& event ) { event.Skip(); }
		virtual void OnAddButtonClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveUpButtonClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveDownButtonClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveButtonClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_MANAGE_REPOSITORIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Manage Repositories"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_MANAGE_REPOSITORIES_BASE();

};

