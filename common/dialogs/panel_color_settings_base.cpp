///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_color_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_COLOR_SETTINGS_BASE::PANEL_COLOR_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bControlSizer;
	bControlSizer = new wxBoxSizer( wxHORIZONTAL );

	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Theme:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	bControlSizer->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_cbThemeChoices;
	m_cbTheme = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbThemeChoices, 0 );
	m_cbTheme->SetSelection( 0 );
	m_cbTheme->SetMinSize( wxSize( 150,-1 ) );

	bControlSizer->Add( m_cbTheme, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );


	bControlSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_optOverrideColors = new wxCheckBox( this, wxID_ANY, _("Override individual item colors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_optOverrideColors->SetToolTip( _("Show all items in their default color even if they have specific colors set in their properties.") );

	bControlSizer->Add( m_optOverrideColors, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bControlSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_btnOpenFolder = new wxButton( this, wxID_ANY, _("Open Theme Folder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnOpenFolder->SetToolTip( _("Open the folder containing color themes") );

	bControlSizer->Add( m_btnOpenFolder, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	m_mainSizer->Add( bControlSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_mainSizer->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );

	m_colorsMainSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bottomMargin;
	bottomMargin = new wxBoxSizer( wxVERTICAL );

	m_colorsListWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE|wxVSCROLL );
	m_colorsListWindow->SetScrollRate( 5, 5 );
	m_colorsListWindow->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_colorsListWindow->SetMinSize( wxSize( 240,-1 ) );

	m_colorsGridSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
	m_colorsGridSizer->AddGrowableCol( 0 );
	m_colorsGridSizer->SetFlexibleDirection( wxHORIZONTAL );
	m_colorsGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );

	m_colorsGridSizer->SetMinSize( wxSize( 100,-1 ) );

	m_colorsListWindow->SetSizer( m_colorsGridSizer );
	m_colorsListWindow->Layout();
	m_colorsGridSizer->Fit( m_colorsListWindow );
	bottomMargin->Add( m_colorsListWindow, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	m_colorsMainSizer->Add( bottomMargin, 0, wxEXPAND|wxBOTTOM, 3 );

	m_previewPanelSizer = new wxBoxSizer( wxVERTICAL );


	m_colorsMainSizer->Add( m_previewPanelSizer, 1, wxEXPAND, 5 );


	m_mainSizer->Add( m_colorsMainSizer, 1, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_COLOR_SETTINGS_BASE::OnSize ) );
	m_cbTheme->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnThemeChanged ), NULL, this );
	m_cbTheme->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( PANEL_COLOR_SETTINGS_BASE::OnLeftDownTheme ), NULL, this );
	m_optOverrideColors->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnOverrideItemColorsClicked ), NULL, this );
	m_btnOpenFolder->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnBtnOpenThemeFolderClicked ), NULL, this );
}

PANEL_COLOR_SETTINGS_BASE::~PANEL_COLOR_SETTINGS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_COLOR_SETTINGS_BASE::OnSize ) );
	m_cbTheme->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnThemeChanged ), NULL, this );
	m_cbTheme->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( PANEL_COLOR_SETTINGS_BASE::OnLeftDownTheme ), NULL, this );
	m_optOverrideColors->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnOverrideItemColorsClicked ), NULL, this );
	m_btnOpenFolder->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COLOR_SETTINGS_BASE::OnBtnOpenThemeFolderClicked ), NULL, this );

}
