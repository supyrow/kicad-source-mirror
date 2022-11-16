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
class BITMAP_BUTTON;
class COLOR_SWATCH;
class FONT_CHOICE;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stc/stc.h>
#include <wx/choice.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/hyperlink.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/bmpcbox.h>
#include <wx/gbsizer.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_TEXTBOX_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_TEXTBOX_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxGridBagSizer* m_textEntrySizer;
		wxStaticText* m_textLabel;
		wxStyledTextCtrl* m_textCtrl;
		wxStaticText* m_fontLabel;
		FONT_CHOICE* m_fontCtrl;
		BITMAP_BUTTON* m_separator1;
		BITMAP_BUTTON* m_bold;
		BITMAP_BUTTON* m_italic;
		BITMAP_BUTTON* m_separator2;
		BITMAP_BUTTON* m_hAlignLeft;
		BITMAP_BUTTON* m_hAlignCenter;
		BITMAP_BUTTON* m_hAlignRight;
		BITMAP_BUTTON* m_separator3;
		BITMAP_BUTTON* m_vAlignTop;
		BITMAP_BUTTON* m_vAlignCenter;
		BITMAP_BUTTON* m_vAlignBottom;
		BITMAP_BUTTON* m_separator4;
		BITMAP_BUTTON* m_horizontal;
		BITMAP_BUTTON* m_vertical;
		BITMAP_BUTTON* m_separator5;
		wxHyperlinkCtrl* m_syntaxHelp;
		wxStaticText* m_textSizeLabel;
		wxTextCtrl* m_textSizeCtrl;
		wxStaticText* m_textSizeUnits;
		wxStaticText* m_textColorLabel;
		wxPanel* m_panelBorderColor1;
		COLOR_SWATCH* m_textColorSwatch;
		wxCheckBox* m_borderCheckbox;
		wxStaticText* m_borderWidthLabel;
		wxTextCtrl* m_borderWidthCtrl;
		wxStaticText* m_borderWidthUnits;
		wxStaticText* m_borderColorLabel;
		wxPanel* m_panelBorderColor;
		COLOR_SWATCH* m_borderColorSwatch;
		wxStaticText* m_borderStyleLabel;
		wxBitmapComboBox* m_borderStyleCombo;
		wxCheckBox* m_filledCtrl;
		wxStaticText* m_fillColorLabel;
		wxPanel* m_panelFillColor;
		COLOR_SWATCH* m_fillColorSwatch;
		wxCheckBox* m_privateCheckbox;
		wxCheckBox* m_CommonUnit;
		wxCheckBox* m_CommonConvert;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void onMultiLineTCLostFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnFormattingHelp( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void onBorderChecked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onFillChecked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_LIB_TEXTBOX_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Text Box Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_LIB_TEXTBOX_PROPERTIES_BASE();

};

