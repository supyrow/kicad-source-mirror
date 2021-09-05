///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "panel_setup_netclasses_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_NETCLASSES_BASE::PANEL_SETUP_NETCLASSES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bpanelNetClassesSizer;
	bpanelNetClassesSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxSP_NO_XP_THEME );
	m_splitter->SetMinimumPaneSize( 80 );

	m_netclassesPane = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	m_netclassGrid = new WX_GRID( m_netclassesPane, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_DEFAULT|wxHSCROLL|wxTAB_TRAVERSAL|wxVSCROLL );

	// Grid
	m_netclassGrid->CreateGrid( 1, 13 );
	m_netclassGrid->EnableEditing( true );
	m_netclassGrid->EnableGridLines( true );
	m_netclassGrid->EnableDragGridSize( false );
	m_netclassGrid->SetMargins( 0, 0 );

	// Columns
	m_netclassGrid->EnableDragColMove( false );
	m_netclassGrid->EnableDragColSize( true );
	m_netclassGrid->SetColLabelSize( 24 );
	m_netclassGrid->SetColLabelValue( 0, _("Net Class") );
	m_netclassGrid->SetColLabelValue( 1, _("Clearance") );
	m_netclassGrid->SetColLabelValue( 2, _("Track Width") );
	m_netclassGrid->SetColLabelValue( 3, _("Via Size") );
	m_netclassGrid->SetColLabelValue( 4, _("Via Hole") );
	m_netclassGrid->SetColLabelValue( 5, _("uVia Size") );
	m_netclassGrid->SetColLabelValue( 6, _("uVia Hole") );
	m_netclassGrid->SetColLabelValue( 7, _("DP Width") );
	m_netclassGrid->SetColLabelValue( 8, _("DP Gap") );
	m_netclassGrid->SetColLabelValue( 9, _("Wire Thickness") );
	m_netclassGrid->SetColLabelValue( 10, _("Bus Thickness") );
	m_netclassGrid->SetColLabelValue( 11, _("Color") );
	m_netclassGrid->SetColLabelValue( 12, _("Line Style") );
	m_netclassGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_netclassGrid->EnableDragRowSize( true );
	m_netclassGrid->SetRowLabelSize( 0 );
	m_netclassGrid->SetRowLabelValue( 0, _("Default") );
	m_netclassGrid->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_netclassGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bUpperSizer->Add( m_netclassGrid, 1, wxEXPAND|wxLEFT, 2 );

	wxBoxSizer* buttonBoxSizer;
	buttonBoxSizer = new wxBoxSizer( wxHORIZONTAL );

	m_addButton = new wxBitmapButton( m_netclassesPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	buttonBoxSizer->Add( m_addButton, 0, wxLEFT, 2 );


	buttonBoxSizer->Add( 5, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_removeButton = new wxBitmapButton( m_netclassesPane, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	buttonBoxSizer->Add( m_removeButton, 0, wxRIGHT|wxLEFT, 5 );


	buttonBoxSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_colorDefaultHelpText = new wxStaticText( m_netclassesPane, wxID_ANY, _("Set color to transparent to use Kicad default color."), wxDefaultPosition, wxDefaultSize, 0 );
	m_colorDefaultHelpText->Wrap( -1 );
	buttonBoxSizer->Add( m_colorDefaultHelpText, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bUpperSizer->Add( buttonBoxSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	m_netclassesPane->SetSizer( bUpperSizer );
	m_netclassesPane->Layout();
	bUpperSizer->Fit( m_netclassesPane );
	m_membershipPane = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeft;
	bLeft = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbFilters;
	sbFilters = new wxStaticBoxSizer( new wxStaticBox( m_membershipPane, wxID_ANY, _("Filter Nets") ), wxVERTICAL );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	m_ncfilterLabel = new wxStaticText( sbFilters->GetStaticBox(), wxID_ANY, _("Net class filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ncfilterLabel->Wrap( -1 );
	m_ncfilterLabel->SetMinSize( wxSize( 120,-1 ) );

	bSizer9->Add( m_ncfilterLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_netClassFilterChoices;
	m_netClassFilter = new wxChoice( sbFilters->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_netClassFilterChoices, 0 );
	m_netClassFilter->SetSelection( 0 );
	bSizer9->Add( m_netClassFilter, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	sbFilters->Add( bSizer9, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer101;
	bSizer101 = new wxBoxSizer( wxHORIZONTAL );

	m_filterLabel = new wxStaticText( sbFilters->GetStaticBox(), wxID_ANY, _("Net name filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filterLabel->Wrap( -1 );
	m_filterLabel->SetMinSize( wxSize( 120,-1 ) );

	bSizer101->Add( m_filterLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_netNameFilter = new wxTextCtrl( sbFilters->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	bSizer101->Add( m_netNameFilter, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbFilters->Add( bSizer101, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer131;
	bSizer131 = new wxBoxSizer( wxHORIZONTAL );

	m_showAllButton = new wxButton( sbFilters->GetStaticBox(), wxID_ANY, _("Show All Nets"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer131->Add( m_showAllButton, 1, wxALL, 5 );


	bSizer131->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_filterNetsButton = new wxButton( sbFilters->GetStaticBox(), wxID_ANY, _("Apply Filters"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer131->Add( m_filterNetsButton, 1, wxALL, 5 );


	sbFilters->Add( bSizer131, 1, wxEXPAND|wxTOP|wxBOTTOM, 6 );


	bLeft->Add( sbFilters, 0, wxEXPAND|wxBOTTOM, 5 );

	wxStaticBoxSizer* sbEdit;
	sbEdit = new wxStaticBoxSizer( new wxStaticBox( m_membershipPane, wxID_ANY, _("Assign Net Class") ), wxVERTICAL );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );

	m_assignLabel = new wxStaticText( sbEdit->GetStaticBox(), wxID_ANY, _("New net class:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_assignLabel->Wrap( -1 );
	m_assignLabel->SetMinSize( wxSize( 120,-1 ) );

	bSizer11->Add( m_assignLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_assignNetClassChoices;
	m_assignNetClass = new wxChoice( sbEdit->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_assignNetClassChoices, 0 );
	m_assignNetClass->SetSelection( 0 );
	bSizer11->Add( m_assignNetClass, 1, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	sbEdit->Add( bSizer11, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );

	m_assignAllButton = new wxButton( sbEdit->GetStaticBox(), wxID_ANY, _("Assign To All Listed Nets"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_assignAllButton, 1, wxALL, 5 );


	bSizer12->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_assignSelectedButton = new wxButton( sbEdit->GetStaticBox(), wxID_ANY, _("Assign To Selected Nets"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_assignSelectedButton, 1, wxALL, 5 );


	sbEdit->Add( bSizer12, 0, wxEXPAND|wxTOP, 6 );


	bLeft->Add( sbEdit, 1, wxEXPAND|wxTOP, 8 );


	bLowerSizer->Add( bLeft, 1, wxEXPAND|wxTOP|wxRIGHT, 5 );

	wxBoxSizer* bRight;
	bRight = new wxBoxSizer( wxVERTICAL );

	m_membershipGrid = new WX_GRID( m_membershipPane, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_DEFAULT );

	// Grid
	m_membershipGrid->CreateGrid( 0, 2 );
	m_membershipGrid->EnableEditing( true );
	m_membershipGrid->EnableGridLines( true );
	m_membershipGrid->EnableDragGridSize( false );
	m_membershipGrid->SetMargins( 0, 0 );

	// Columns
	m_membershipGrid->EnableDragColMove( false );
	m_membershipGrid->EnableDragColSize( true );
	m_membershipGrid->SetColLabelSize( 24 );
	m_membershipGrid->SetColLabelValue( 0, _("Net") );
	m_membershipGrid->SetColLabelValue( 1, _("Net Class") );
	m_membershipGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_membershipGrid->EnableDragRowSize( true );
	m_membershipGrid->SetRowLabelSize( 0 );
	m_membershipGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_membershipGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bRight->Add( m_membershipGrid, 1, wxEXPAND|wxBOTTOM|wxLEFT, 5 );


	bLowerSizer->Add( bRight, 1, wxEXPAND|wxTOP|wxLEFT, 5 );


	m_membershipPane->SetSizer( bLowerSizer );
	m_membershipPane->Layout();
	bLowerSizer->Fit( m_membershipPane );
	m_splitter->SplitHorizontally( m_netclassesPane, m_membershipPane, -1 );
	bMargins->Add( m_splitter, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	bpanelNetClassesSizer->Add( bMargins, 1, wxEXPAND|wxTOP, 2 );


	this->SetSizer( bpanelNetClassesSizer );
	this->Layout();
	bpanelNetClassesSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnUpdateUI ) );
	m_netclassGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeNetclassGrid ), NULL, this );
	m_addButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAddNetclassClick ), NULL, this );
	m_removeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnRemoveNetclassClick ), NULL, this );
	m_membershipPane->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::onmembershipPanelSize ), NULL, this );
	m_netNameFilter->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnApplyFilters ), NULL, this );
	m_showAllButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnShowAll ), NULL, this );
	m_filterNetsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnApplyFilters ), NULL, this );
	m_assignAllButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAssignAll ), NULL, this );
	m_assignSelectedButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAssignSelected ), NULL, this );
	m_membershipGrid->Connect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeMembershipGrid ), NULL, this );
}

PANEL_SETUP_NETCLASSES_BASE::~PANEL_SETUP_NETCLASSES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnUpdateUI ) );
	m_netclassGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeNetclassGrid ), NULL, this );
	m_addButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAddNetclassClick ), NULL, this );
	m_removeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnRemoveNetclassClick ), NULL, this );
	m_membershipPane->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::onmembershipPanelSize ), NULL, this );
	m_netNameFilter->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnApplyFilters ), NULL, this );
	m_showAllButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnShowAll ), NULL, this );
	m_filterNetsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnApplyFilters ), NULL, this );
	m_assignAllButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAssignAll ), NULL, this );
	m_assignSelectedButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnAssignSelected ), NULL, this );
	m_membershipGrid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( PANEL_SETUP_NETCLASSES_BASE::OnSizeMembershipGrid ), NULL, this );

}
