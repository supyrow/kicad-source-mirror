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
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_MASK_AND_PASTE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_MASK_AND_PASTE_BASE : public wxPanel
{
	private:

	protected:
		wxStaticBitmap* m_bitmapWarning;
		wxStaticText* m_staticTextInfoMaskMinWidth;
		wxStaticText* m_staticTextInfoMaskMinWidth1;
		wxStaticLine* m_staticline1;
		wxStaticText* m_maskMarginLabel;
		wxTextCtrl* m_maskMarginCtrl;
		wxStaticText* m_maskMarginUnits;
		wxStaticText* m_maskMinWidthLabel;
		wxTextCtrl* m_maskMinWidthCtrl;
		wxStaticText* m_maskMinWidthUnits;
		wxStaticText* m_pasteMarginLabel;
		wxTextCtrl* m_pasteMarginCtrl;
		wxStaticText* m_pasteMarginUnits;
		wxStaticText* m_pasteMarginRatioLabel;
		wxTextCtrl* m_pasteMarginRatioCtrl;
		wxStaticText* m_pasteMarginRatioUnits;
		wxStaticText* m_staticTextInfoPaste;

	public:

		PANEL_SETUP_MASK_AND_PASTE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 727,280 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_SETUP_MASK_AND_PASTE_BASE();

};

