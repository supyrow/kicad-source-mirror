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
#include "widgets/wx_grid.h"
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/grid.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/splitter.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PACKAGES_VIEW_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PACKAGES_VIEW_BASE : public wxPanel
{
	private:

	protected:
		wxStaticBitmap* m_searchBitmap;
		wxTextCtrl* m_searchCtrl;
		wxSplitterWindow* m_splitter1;
		wxScrolledWindow* m_packageListWindow;
		wxPanel* m_panelDetails;
		wxNotebook* m_notebook;
		wxPanel* m_panelDescription;
		wxRichTextCtrl* m_infoText;
		wxPanel* m_panelVersions;
		wxCheckBox* m_showAllVersions;
		WX_GRID* m_gridVersions;
		wxButton* m_buttonDownload;
		wxButton* m_buttonInstall;

		// Virtual event handlers, overide them in your derived class
		virtual void OnSearchTextChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowAllVersionsClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnVersionsCellClicked( wxGridEvent& event ) { event.Skip(); }
		virtual void OnDownloadVersionClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnInstallVersionClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_PACKAGES_VIEW_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_PACKAGES_VIEW_BASE();

		void m_splitter1OnIdle( wxIdleEvent& )
		{
			m_splitter1->SetSashPosition( 0 );
			m_splitter1->Disconnect( wxEVT_IDLE, wxIdleEventHandler( PANEL_PACKAGES_VIEW_BASE::m_splitter1OnIdle ), NULL, this );
		}

};

