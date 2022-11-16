///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
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
#include <wx/string.h>
#include <wx/radiobut.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/choice.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/manager.h>
#include <wx/propgrid/advprops.h>
#include <wx/panel.h>
#include <wx/stc/stc.h>
#include <wx/notebook.h>
#include <wx/grid.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SIM_MODEL_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SIM_MODEL_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_notebook;
		wxPanel* m_modelPanel;
		wxRadioButton* m_useLibraryModelRadioButton;
		wxStaticText* m_pathLabel;
		wxTextCtrl* m_tclibraryPathName;
		wxBitmapButton* m_browseButton;
		wxStaticText* m_modelNameLabel;
		wxComboBox* m_modelNameCombobox;
		wxStaticText* m_ibisPinLabel;
		wxComboBox* m_ibisPinCombobox;
		wxCheckBox* m_differentialCheckbox;
		wxStaticText* m_ibisModelLabel;
		wxComboBox* m_ibisModelCombobox;
		wxRadioButton* m_useInstanceModelRadioButton;
		wxStaticText* m_staticTextDevType;
		wxChoice* m_deviceTypeChoice;
		wxStaticText* m_staticTextSpiceType;
		wxChoice* m_typeChoice;
		wxNotebook* m_notebook4;
		wxPanel* m_parametersPanel;
		wxPropertyGridManager* m_paramGridMgr;
		wxPropertyGridPage* m_paramGrid;
		wxPanel* m_codePanel;
		wxStyledTextCtrl* m_codePreview;
		wxPanel* m_pinAssignmentsPanel;
		WX_GRID* m_pinAssignmentsGrid;
		wxCheckBox* m_excludeCheckbox;
		wxCheckBox* m_inferCheckbox;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void onRadioButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void onLibraryPathLabelUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onLibraryPathUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onBrowseButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBrowseButtonUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onModelNameLabelUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onModelNameCombobox( wxCommandEvent& event ) { event.Skip(); }
		virtual void onModelNameComboboxKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void onModelNameComboboxTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onModelNameComboboxUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onIbisPinLabelUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onIbisPinCombobox( wxCommandEvent& event ) { event.Skip(); }
		virtual void onIbisPinComboboxTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDifferentialCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOverrideCheckboxUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onIbisModelLabelUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onIbisModelCombobox( wxCommandEvent& event ) { event.Skip(); }
		virtual void onIbisModelComboboxTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDeviceTypeLabelUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onDeviceTypeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDeviceTypeChoiceUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onTypeLabelUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onTypeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void onParamGridChanged( wxPropertyGridEvent& event ) { event.Skip(); }
		virtual void onCodePreviewSetFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void onPinAssignmentsGridCellChange( wxGridEvent& event ) { event.Skip(); }
		virtual void onPinAssignmentsGridSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void onExcludeCheckbox( wxCommandEvent& event ) { event.Skip(); }
		virtual void onInferCheckbox( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SIM_MODEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Simulation Model Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_SIM_MODEL_BASE();

};

