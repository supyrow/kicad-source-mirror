///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/unit_selector.h"

#include "panel_fusing_current_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_FUSING_CURRENT_BASE::PANEL_FUSING_CURRENT_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* m_parametersSizer;
	m_parametersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Parameters") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer11;
	fgSizer11 = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizer11->SetFlexibleDirection( wxBOTH );
	fgSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_dummy1 = new wxStaticText( m_parametersSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_dummy1->Wrap( -1 );
	fgSizer11->Add( m_dummy1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_ambientText = new wxStaticText( m_parametersSizer->GetStaticBox(), wxID_ANY, _("Ambient temperature:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ambientText->Wrap( -1 );
	fgSizer11->Add( m_ambientText, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_ambientValue = new wxTextCtrl( m_parametersSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_ambientValue, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_ambientUnit = new wxStaticText( m_parametersSizer->GetStaticBox(), wxID_ANY, _("deg C"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ambientUnit->Wrap( -1 );
	fgSizer11->Add( m_ambientUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_dummy2 = new wxStaticText( m_parametersSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_dummy2->Wrap( -1 );
	fgSizer11->Add( m_dummy2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_meltingText = new wxStaticText( m_parametersSizer->GetStaticBox(), wxID_ANY, _("Melting point:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_meltingText->Wrap( -1 );
	fgSizer11->Add( m_meltingText, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_meltingValue = new wxTextCtrl( m_parametersSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_meltingValue, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_meltingUnit = new wxStaticText( m_parametersSizer->GetStaticBox(), wxID_ANY, _("deg C"), wxDefaultPosition, wxDefaultSize, 0 );
	m_meltingUnit->Wrap( -1 );
	fgSizer11->Add( m_meltingUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_widthRadio = new wxRadioButton( m_parametersSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_widthRadio, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_widthText = new wxStaticText( m_parametersSizer->GetStaticBox(), wxID_ANY, _("Track width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthText->Wrap( -1 );
	fgSizer11->Add( m_widthText, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_widthValue = new wxTextCtrl( m_parametersSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_widthValue, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_widthUnitChoices;
	m_widthUnit = new UNIT_SELECTOR_LEN( m_parametersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_widthUnitChoices, 0 );
	m_widthUnit->SetSelection( 0 );
	fgSizer11->Add( m_widthUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_thicknessRadio = new wxRadioButton( m_parametersSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_thicknessRadio, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_thicknessText = new wxStaticText( m_parametersSizer->GetStaticBox(), wxID_ANY, _("Track thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessText->Wrap( -1 );
	fgSizer11->Add( m_thicknessText, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_thicknessValue = new wxTextCtrl( m_parametersSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_thicknessValue, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString m_thicknessUnitChoices;
	m_thicknessUnit = new UNIT_SELECTOR_THICKNESS( m_parametersSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_thicknessUnitChoices, 0 );
	m_thicknessUnit->SetSelection( 0 );
	fgSizer11->Add( m_thicknessUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_currentRadio = new wxRadioButton( m_parametersSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_currentRadio, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_currentText = new wxStaticText( m_parametersSizer->GetStaticBox(), wxID_ANY, _("Current:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_currentText->Wrap( -1 );
	fgSizer11->Add( m_currentText, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_currentValue = new wxTextCtrl( m_parametersSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_currentValue, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_currentUnit = new wxStaticText( m_parametersSizer->GetStaticBox(), wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_currentUnit->Wrap( -1 );
	fgSizer11->Add( m_currentUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_timeRadio = new wxRadioButton( m_parametersSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_timeRadio, 0, wxALL, 5 );

	m_timeText = new wxStaticText( m_parametersSizer->GetStaticBox(), wxID_ANY, _("Time to fuse:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_timeText->Wrap( -1 );
	fgSizer11->Add( m_timeText, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_timeValue = new wxTextCtrl( m_parametersSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer11->Add( m_timeValue, 0, wxALL, 5 );

	m_timeUnit = new wxStaticText( m_parametersSizer->GetStaticBox(), wxID_ANY, _("s"), wxDefaultPosition, wxDefaultSize, 0 );
	m_timeUnit->Wrap( -1 );
	fgSizer11->Add( m_timeUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_parametersSizer->Add( fgSizer11, 2, wxEXPAND, 5 );


	bSizer8->Add( m_parametersSizer, 0, wxALL, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_calculateButton = new wxButton( this, wxID_ANY, _("Calculate"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_calculateButton, 0, wxALL, 5 );

	m_comment = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_comment->Wrap( -1 );
	bSizer3->Add( m_comment, 0, wxALIGN_CENTER|wxALL, 5 );


	bSizer8->Add( bSizer3, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* m_helpSizer;
	m_helpSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Help") ), wxVERTICAL );

	m_htmlHelp = new HTML_WINDOW( m_helpSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	m_htmlHelp->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
	m_htmlHelp->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	m_helpSizer->Add( m_htmlHelp, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	bSizer8->Add( m_helpSizer, 1, wxALL|wxEXPAND, 5 );


	bSizer7->Add( bSizer8, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer7 );
	this->Layout();

	// Connect Events
	m_calculateButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FUSING_CURRENT_BASE::m_onCalculateClick ), NULL, this );
}

PANEL_FUSING_CURRENT_BASE::~PANEL_FUSING_CURRENT_BASE()
{
	// Disconnect Events
	m_calculateButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FUSING_CURRENT_BASE::m_onCalculateClick ), NULL, this );

}
