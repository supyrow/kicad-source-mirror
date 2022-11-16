///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/valtext.h>
#include <wx/statbox.h>
#include <wx/bmpcbox.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_IMPORT_GFX_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_IMPORT_GFX_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextFile;
		wxTextCtrl* m_textCtrlFileName;
		wxBitmapButton* m_browseButton;
		wxRadioButton* m_rbInteractivePlacement;
		wxRadioButton* m_rbAbsolutePlacement;
		wxStaticText* m_xLabel;
		wxTextCtrl* m_xCtrl;
		wxStaticText* m_xUnits;
		wxStaticText* m_yLabel;
		wxTextCtrl* m_yCtrl;
		wxStaticText* m_yUnits;
		wxStaticText* m_staticTextBrdlayer;
		PCB_LAYER_BOX_SELECTOR* m_SelLayerBox;
		wxStaticText* m_importScaleLabel;
		wxTextCtrl* m_importScaleCtrl;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_groupItems;
		wxStaticText* m_lineWidthLabel;
		wxTextCtrl* m_lineWidthCtrl;
		wxStaticText* m_lineWidthUnits;
		wxStaticText* m_staticTextLineWidth1;
		wxChoice* m_choiceDxfUnits;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onBrowseFiles( wxCommandEvent& event ) { event.Skip(); }
		virtual void onInteractivePlacement( wxCommandEvent& event ) { event.Skip(); }
		virtual void originOptionOnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onAbsolutePlacement( wxCommandEvent& event ) { event.Skip(); }
		virtual void onGroupItems( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_IMPORT_GFX_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Import Vector Graphics File"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_IMPORT_GFX_BASE();

};

