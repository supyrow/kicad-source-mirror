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
#include "kiway_player.h"
#include <wx/scrolwin.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/radiobox.h>
#include <wx/statusbr.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class BM2CMP_FRAME_BASE
///////////////////////////////////////////////////////////////////////////////
class BM2CMP_FRAME_BASE : public KIWAY_PLAYER
{
	private:

	protected:
		wxNotebook* m_Notebook;
		wxScrolledWindow* m_InitialPicturePanel;
		wxScrolledWindow* m_GreyscalePicturePanel;
		wxScrolledWindow* m_BNPicturePanel;
		wxStaticText* m_staticTextISize;
		wxStaticText* m_SizeXValue;
		wxStaticText* m_SizeYValue;
		wxStaticText* m_SizePixUnits;
		wxStaticText* m_staticTextDPI;
		wxStaticText* m_InputXValueDPI;
		wxStaticText* m_InputYValueDPI;
		wxStaticText* m_DPIUnit;
		wxStaticText* m_staticTextBPP;
		wxStaticText* m_BPPValue;
		wxStaticText* m_BPPunits;
		wxButton* m_buttonLoad;
		wxCheckBox* m_aspectRatioCheckbox;
		wxStaticText* m_staticTextOSize;
		wxTextCtrl* m_UnitSizeX;
		wxTextCtrl* m_UnitSizeY;
		wxChoice* m_PixelUnit;
		wxStaticText* m_ThresholdText;
		wxSlider* m_sliderThreshold;
		wxCheckBox* m_checkNegative;
		wxRadioBox* m_rbPCBLayer;
		wxRadioBox* m_rbOutputFormat;
		wxButton* m_buttonExportFile;
		wxButton* m_buttonExportClipboard;
		wxStatusBar* m_statusBar;

		// Virtual event handlers, override them in your derived class
		virtual void OnPaintInit( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnPaintGreyscale( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnPaintBW( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnLoadFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToggleAspectRatioLock( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSizeChangeX( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSizeChangeY( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSizeUnitChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnThresholdChange( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnNegativeClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFormatChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExportToFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExportToClipboard( wxCommandEvent& event ) { event.Skip(); }


	public:

		BM2CMP_FRAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Image Converter"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxCLOSE_BOX|wxDEFAULT_FRAME_STYLE|wxRESIZE_BORDER|wxTAB_TRAVERSAL );

		~BM2CMP_FRAME_BASE();

};

