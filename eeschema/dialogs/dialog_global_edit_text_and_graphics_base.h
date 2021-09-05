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
#include "widgets/color_swatch.h"
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxCheckBox* m_references;
		wxCheckBox* m_values;
		wxCheckBox* m_otherFields;
		wxCheckBox* m_wires;
		wxCheckBox* m_buses;
		wxCheckBox* m_globalLabels;
		wxCheckBox* m_hierLabels;
		wxCheckBox* m_sheetTitles;
		wxCheckBox* m_sheetFields;
		wxCheckBox* m_sheetPins;
		wxCheckBox* m_sheetBorders;
		wxCheckBox* m_schTextAndGraphics;
		wxCheckBox* m_fieldnameFilterOpt;
		wxTextCtrl* m_fieldnameFilter;
		wxCheckBox* m_referenceFilterOpt;
		wxTextCtrl* m_referenceFilter;
		wxCheckBox* m_symbolFilterOpt;
		wxTextCtrl* m_symbolFilter;
		wxCheckBox* m_typeFilterOpt;
		wxChoice* m_typeFilter;
		wxCheckBox* m_netFilterOpt;
		wxTextCtrl* m_netFilter;
		wxCheckBox* m_selectedFilterOpt;
		wxPanel* m_specifiedValues;
		wxStaticText* m_textSizeLabel;
		wxTextCtrl* m_textSizeCtrl;
		wxStaticText* m_textSizeUnits;
		wxCheckBox* m_Bold;
		wxStaticText* orientationLabel;
		wxChoice* m_orientation;
		wxCheckBox* m_Italic;
		wxStaticText* hAlignLabel;
		wxChoice* m_hAlign;
		wxCheckBox* m_Visible;
		wxStaticText* vAlignLabel;
		wxChoice* m_vAlign;
		wxStaticLine* m_staticline1;
		wxStaticLine* m_staticline2;
		wxStaticLine* m_staticline21;
		wxStaticLine* m_staticline3;
		wxStaticLine* m_staticline4;
		wxStaticLine* m_staticline5;
		wxStaticText* m_lineWidthLabel;
		wxTextCtrl* m_LineWidthCtrl;
		wxStaticText* m_lineWidthUnits;
		wxCheckBox* m_setColor;
		COLOR_SWATCH* m_colorSwatch;
		wxStaticText* lineStyleLabel;
		wxChoice* m_lineStyle;
		wxCheckBox* m_setBgColor;
		COLOR_SWATCH* m_bgColorSwatch;
		wxStaticText* m_dotSizeLabel;
		wxTextCtrl* m_dotSizeCtrl;
		wxStaticText* m_dotSizeUnits;
		wxCheckBox* m_setDotColor;
		COLOR_SWATCH* m_dotColorSwatch;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnReferenceFilterText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSymbolFilterText( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Edit Text and Graphic Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE();

};

