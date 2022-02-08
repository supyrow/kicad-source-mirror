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
class WX_HTML_REPORT_BOX;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/stc/stc.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/html/htmlwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

#define ID_RULES_EDITOR 2240

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_RULES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_RULES_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* m_leftMargin;
		wxBoxSizer* m_topMargin;
		wxStaticText* m_title;
		wxHyperlinkCtrl* m_syntaxHelp;
		wxStyledTextCtrl* m_textEditor;
		wxBitmapButton* m_compileButton;
		WX_HTML_REPORT_BOX* m_errorsReport;

		// Virtual event handlers, overide them in your derived class
		virtual void OnSyntaxHelp( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnContextMenu( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnCompile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorLinkClicked( wxHtmlLinkEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_RULES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_SETUP_RULES_BASE();

};

