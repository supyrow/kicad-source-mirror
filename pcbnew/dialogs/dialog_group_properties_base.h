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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/listbox.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GROUP_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GROUP_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_nameLabel;
		wxTextCtrl* m_nameCtrl;
		wxCheckBox* m_locked;
		wxStaticText* m_membersLabel;
		wxListBox* m_membersList;
		wxBitmapButton* m_bpAddMember;
		wxBitmapButton* m_bpRemoveMember;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void onClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnMemberSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddMember( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveMember( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_GROUP_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Group Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_GROUP_PROPERTIES_BASE();

};

