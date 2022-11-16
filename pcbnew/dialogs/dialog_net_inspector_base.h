///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
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
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/dataview.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_NET_INSPECTOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_NET_INSPECTOR_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextFilter;
		wxTextCtrl* m_textCtrlFilter;
		wxCheckBox* m_cbShowZeroPad;
		wxCheckBox* m_groupBy;
		wxTextCtrl* m_groupByText;
		wxComboBox* m_groupByKind;
		wxDataViewCtrl* m_netsList;
		wxBitmapButton* m_addNet;
		wxBitmapButton* m_renameNet;
		wxBitmapButton* m_deleteNet;
		wxButton* m_ReportButt;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;

		// Virtual event handlers, override them in your derived class
		virtual void onFilterChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSortingChanged( wxDataViewEvent& event ) { event.Skip(); }
		virtual void onSelChanged( wxDataViewEvent& event ) { event.Skip(); }
		virtual void onListSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void onAddNet( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRenameNet( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDeleteNet( wxCommandEvent& event ) { event.Skip(); }
		virtual void onReport( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_NET_INSPECTOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Net Inspector"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 579,503 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_NET_INSPECTOR_BASE();

};

