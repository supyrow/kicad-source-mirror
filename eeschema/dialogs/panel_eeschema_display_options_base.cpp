///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_eeschema_display_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE::PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	m_galOptionsSizer = new wxBoxSizer( wxVERTICAL );


	bPanelSizer->Add( m_galOptionsSizer, 1, wxEXPAND|wxLEFT, 5 );

	wxBoxSizer* bRightColumn;
	bRightColumn = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Appearance") ), wxVERTICAL );

	m_checkShowHiddenPins = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("S&how hidden pins"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_checkShowHiddenPins, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkShowHiddenFields = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Show hidden fields"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_checkShowHiddenFields, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_checkShowERCErrors = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Show ERC errors"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_checkShowERCErrors, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkShowERCWarnings = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Show ERC warnings"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_checkShowERCWarnings, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkShowERCExclusions = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Show ERC exclusions"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_checkShowERCExclusions, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkPageLimits = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Show page limi&ts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkPageLimits->SetValue(true);
	sbSizer1->Add( m_checkPageLimits, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bRightColumn->Add( sbSizer1, 0, wxEXPAND|wxTOP, 5 );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Selection && Highlighting") ), wxVERTICAL );

	m_checkSelTextBox = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Draw selected text items as box"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer3->Add( m_checkSelTextBox, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkSelDrawChildItems = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Draw selected child items"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer3->Add( m_checkSelDrawChildItems, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkSelFillShapes = new wxCheckBox( sbSizer3->GetStaticBox(), wxID_ANY, _("Fill selected shapes"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer3->Add( m_checkSelFillShapes, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,10 ) );

	m_selWidthLabel = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, _("Selection thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_selWidthLabel->Wrap( -1 );
	gbSizer1->Add( m_selWidthLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_selWidthCtrl = new wxSpinCtrlDouble( sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT|wxSP_ARROW_KEYS, 0, 50, 0, 1 );
	m_selWidthCtrl->SetDigits( 0 );
	gbSizer1->Add( m_selWidthCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_highlightColorNote = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, _("(selection color can be edited in the \"Colors\" page)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_highlightColorNote->Wrap( -1 );
	gbSizer1->Add( m_highlightColorNote, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxALL, 5 );

	m_highlightWidthLabel = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, _("Highlight thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_highlightWidthLabel->Wrap( -1 );
	gbSizer1->Add( m_highlightWidthLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_highlightWidthCtrl = new wxSpinCtrlDouble( sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT|wxSP_ARROW_KEYS, 0, 50, 0, 1 );
	m_highlightWidthCtrl->SetDigits( 0 );
	gbSizer1->Add( m_highlightWidthCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	sbSizer3->Add( gbSizer1, 1, wxEXPAND, 5 );


	bRightColumn->Add( sbSizer3, 0, wxTOP|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer31;
	sbSizer31 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Cross-probing") ), wxVERTICAL );

	m_checkCrossProbeCenter = new wxCheckBox( sbSizer31->GetStaticBox(), wxID_ANY, _("Center view on cross-probed items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeCenter->SetValue(true);
	sbSizer31->Add( m_checkCrossProbeCenter, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeZoom = new wxCheckBox( sbSizer31->GetStaticBox(), wxID_ANY, _("Zoom to fit cross-probed items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeZoom->SetValue(true);
	sbSizer31->Add( m_checkCrossProbeZoom, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_checkCrossProbeAutoHighlight = new wxCheckBox( sbSizer31->GetStaticBox(), wxID_ANY, _("Highlight cross-probed nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkCrossProbeAutoHighlight->SetValue(true);
	m_checkCrossProbeAutoHighlight->SetToolTip( _("Highlight nets when they are highlighted in the PCB editor") );

	sbSizer31->Add( m_checkCrossProbeAutoHighlight, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bRightColumn->Add( sbSizer31, 1, wxEXPAND|wxTOP, 5 );


	bPanelSizer->Add( bRightColumn, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );
}

PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE::~PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE()
{
}
