/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/tokenzr.h>

#include <invoke_sch_dialog.h>
#include <sch_sheet_path.h>
#include <schematic.h>

#include "dialog_bus_manager.h"

#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/valtext.h>



DIALOG_BUS_MANAGER::DIALOG_BUS_MANAGER( SCH_EDIT_FRAME* aParent )
        : DIALOG_SHIM( aParent, wxID_ANY, _( "Bus Definitions" ),
                       wxDefaultPosition, wxDefaultSize,
                       wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
          m_parent( aParent )
{
    auto sizer = new wxBoxSizer( wxVERTICAL );
    auto buttons = new wxStdDialogButtonSizer();
    wxButton* okButton = new wxButton( this, wxID_OK );

    buttons->AddButton( okButton );
    buttons->AddButton( new wxButton( this, wxID_CANCEL ) );
    buttons->Realize();

    auto top_container = new wxBoxSizer( wxHORIZONTAL );
    auto left_pane = new wxBoxSizer( wxVERTICAL );
    auto right_pane = new wxBoxSizer( wxVERTICAL );

    // Left pane: alias list
    auto lbl_aliases = new wxStaticText( this, wxID_ANY, _( "Bus Aliases" ),
                                         wxDefaultPosition, wxDefaultSize,
                                         wxALIGN_LEFT );

    m_bus_list_view = new wxListView( this, wxID_ANY, wxDefaultPosition,
                                      wxSize( 300, 300 ), wxLC_ALIGN_LEFT |
                                      wxLC_NO_HEADER | wxLC_REPORT |
                                      wxLC_SINGLE_SEL );
    m_bus_list_view->InsertColumn( 0, "" );

    auto lbl_alias_edit = new wxStaticText( this, wxID_ANY, _( "Alias Name" ),
                                            wxDefaultPosition, wxDefaultSize,
                                            wxALIGN_LEFT );

    m_bus_edit = new wxTextCtrl( this, wxID_ANY, wxEmptyString,
            wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );

    auto left_button_sizer = new wxBoxSizer( wxHORIZONTAL );

    m_btn_add_bus = new wxButton( this, wxID_ANY, _( "Add" ) );
    m_btn_rename_bus = new wxButton( this, wxID_ANY, _( "Rename" ) );
    m_btn_remove_bus = new wxButton( this, wxID_ANY, _( "Remove" ) );

    left_button_sizer->Add( m_btn_add_bus );
    left_button_sizer->Add( m_btn_rename_bus );
    left_button_sizer->Add( m_btn_remove_bus );

    left_pane->Add( lbl_aliases, 0, wxEXPAND | wxALL, 5 );
    left_pane->Add( m_bus_list_view, 1, wxEXPAND | wxALL, 5 );
    left_pane->Add( lbl_alias_edit, 0, wxEXPAND | wxALL, 5 );
    left_pane->Add( m_bus_edit, 0, wxEXPAND | wxALL, 5 );
    left_pane->Add( left_button_sizer, 0, wxEXPAND | wxALL, 5 );

    // Right pane: signal list
    auto lbl_signals = new wxStaticText( this, wxID_ANY, _( "Alias Members" ),
                                         wxDefaultPosition, wxDefaultSize,
                                         wxALIGN_LEFT );

    m_signal_list_view = new wxListView( this, wxID_ANY, wxDefaultPosition,
                                         wxSize( 300, 300 ), wxLC_ALIGN_LEFT |
                                         wxLC_NO_HEADER | wxLC_REPORT |
                                         wxLC_SINGLE_SEL );
    m_signal_list_view->InsertColumn( 0, "" );

    auto lbl_signal_edit = new wxStaticText( this, wxID_ANY, _( "Member Name" ),
                                             wxDefaultPosition, wxDefaultSize,
                                             wxALIGN_LEFT );

    m_signal_edit = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                    wxDefaultSize, wxTE_PROCESS_ENTER );

    auto right_button_sizer = new wxBoxSizer( wxHORIZONTAL );

    m_btn_add_signal = new wxButton( this, wxID_ANY, _( "Add" ) );
    m_btn_rename_signal = new wxButton( this, wxID_ANY, _( "Rename" ) );
    m_btn_remove_signal = new wxButton( this, wxID_ANY, _( "Remove" ) );

    right_button_sizer->Add( m_btn_add_signal );
    right_button_sizer->Add( m_btn_rename_signal );
    right_button_sizer->Add( m_btn_remove_signal );

    right_pane->Add( lbl_signals, 0, wxEXPAND | wxALL, 5 );
    right_pane->Add( m_signal_list_view, 1, wxEXPAND | wxALL, 5 );
    right_pane->Add( lbl_signal_edit, 0, wxEXPAND | wxALL, 5 );
    right_pane->Add( m_signal_edit, 0, wxEXPAND | wxALL, 5 );
    right_pane->Add( right_button_sizer, 0, wxEXPAND | wxALL, 5 );

    top_container->Add( left_pane, 1, wxEXPAND );
    top_container->Add( right_pane, 1, wxEXPAND );

    sizer->Add( top_container, 1, wxEXPAND | wxALL, 5 );
    sizer->Add( buttons, 0, wxEXPAND | wxBOTTOM, 10 );
    SetSizer( sizer );

    // Setup validators

    wxTextValidator validator;
    validator.SetStyle( wxFILTER_EXCLUDE_CHAR_LIST );
    validator.SetCharExcludes( "\r\n\t " );
    m_bus_edit->SetValidator( validator );

    // Allow spaces in the signal edit, so that you can type in a list of
    // signals in the box and it can automatically split them when you add.
    validator.SetCharExcludes( "\r\n\t" );
    m_signal_edit->SetValidator( validator );

    // Setup events

    Bind( wxEVT_INIT_DIALOG, &DIALOG_BUS_MANAGER::OnInitDialog, this );
    m_bus_list_view->Connect( wxEVT_COMMAND_LIST_ITEM_DESELECTED,
            wxListEventHandler( DIALOG_BUS_MANAGER::OnSelectBus ), nullptr, this );
    m_bus_list_view->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED,
            wxListEventHandler( DIALOG_BUS_MANAGER::OnSelectBus ), nullptr, this );
    m_signal_list_view->Connect( wxEVT_COMMAND_LIST_ITEM_DESELECTED,
            wxListEventHandler( DIALOG_BUS_MANAGER::OnSelectSignal ), nullptr, this );
    m_signal_list_view->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED,
            wxListEventHandler( DIALOG_BUS_MANAGER::OnSelectSignal ), nullptr, this );

    m_btn_add_bus->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler( DIALOG_BUS_MANAGER::OnAddBus ), nullptr, this );
    m_btn_rename_bus->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler( DIALOG_BUS_MANAGER::OnRenameBus ), nullptr, this );
    m_btn_remove_bus->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler( DIALOG_BUS_MANAGER::OnRemoveBus ), nullptr, this );
    m_signal_edit->Connect( wxEVT_TEXT_ENTER,
            wxCommandEventHandler( DIALOG_BUS_MANAGER::OnAddSignal ), nullptr, this );

    m_btn_add_signal->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler( DIALOG_BUS_MANAGER::OnAddSignal ), nullptr, this );
    m_btn_rename_signal->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler( DIALOG_BUS_MANAGER::OnRenameSignal ), nullptr, this );
    m_btn_remove_signal->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler( DIALOG_BUS_MANAGER::OnRemoveSignal ), nullptr, this );
    m_bus_edit->Connect( wxEVT_TEXT_ENTER,
            wxCommandEventHandler( DIALOG_BUS_MANAGER::OnAddBus ), nullptr, this );

    // Set initial UI state

    m_btn_rename_bus->Disable();
    m_btn_remove_bus->Disable();

    m_btn_add_signal->Disable();
    m_btn_rename_signal->Disable();
    m_btn_remove_signal->Disable();

    m_bus_edit->SetHint( _( "Bus Alias Name" ) );
    m_signal_edit->SetHint( _( "Net or Bus Name" ) );

    finishDialogSettings();
    okButton->SetDefault();
}


void DIALOG_BUS_MANAGER::OnInitDialog( wxInitDialogEvent& aEvent )
{
    TransferDataToWindow();
}


bool DIALOG_BUS_MANAGER::TransferDataToWindow()
{
    m_aliases.clear();
    m_screens.clear();

    SCH_SCREENS screens( m_parent->Schematic().Root() );

    std::vector< std::shared_ptr<BUS_ALIAS> > original_aliases;

    // collect aliases from each open sheet
    for( SCH_SCREEN* screen = screens.GetFirst(); screen != nullptr; screen = screens.GetNext() )
    {
        std::unordered_set<std::shared_ptr<BUS_ALIAS>> sheet_aliases = screen->GetBusAliases();
        original_aliases.insert( original_aliases.end(), sheet_aliases.begin(),
                                 sheet_aliases.end() );
    }

    original_aliases.erase( std::unique( original_aliases.begin(),
                                         original_aliases.end() ),
                            original_aliases.end() );

    // clone into a temporary working set
    int idx = 0;

    for( const std::shared_ptr<BUS_ALIAS>& alias : original_aliases )
    {
        m_screens.insert( alias->GetParent() );
        m_aliases.push_back( alias->Clone() );
        auto text = getAliasDisplayText( alias );
        m_bus_list_view->InsertItem( idx, text );
        m_bus_list_view->SetItemPtrData( idx,  wxUIntPtr( m_aliases[idx].get() ) );
        idx++;
    }

    m_bus_list_view->SetColumnWidth( 0, -1 );

    return true;
}


bool DIALOG_BUS_MANAGER::TransferDataFromWindow()
{
    for( SCH_SCREEN* screen : m_screens )
        screen->ClearBusAliases();

    for( const std::shared_ptr<BUS_ALIAS>& alias : m_aliases )
        alias->GetParent()->AddBusAlias( alias );

    ( ( SCH_EDIT_FRAME* )GetParent() )->OnModify();
    return true;
}


void DIALOG_BUS_MANAGER::OnSelectBus( wxListEvent& event )
{
    if( event.GetEventType() == wxEVT_COMMAND_LIST_ITEM_SELECTED )
    {
        auto alias = m_aliases[ event.GetIndex() ];

        if( m_active_alias != alias )
        {
            m_active_alias = alias;

            m_bus_edit->ChangeValue( alias->GetName() );

            m_btn_rename_bus->Enable();
            m_btn_remove_bus->Enable();

            auto members = alias->Members();

            // TODO(JE) Clear() seems to be clearing the hint, contrary to
            // the wx documentation.
            m_signal_edit->Clear();
            m_signal_list_view->DeleteAllItems();

            for( unsigned i = 0; i < members.size(); i++ )
            {
                m_signal_list_view->InsertItem( i, members[i] );
            }

            m_signal_list_view->SetColumnWidth( 0, -1 );

            m_btn_add_signal->Enable();
            m_btn_rename_signal->Disable();
            m_btn_remove_signal->Disable();
        }
    }
    else
    {
        m_active_alias = nullptr;
        m_bus_edit->Clear();
        m_signal_edit->Clear();
        m_signal_list_view->DeleteAllItems();

        m_btn_rename_bus->Disable();
        m_btn_remove_bus->Disable();

        m_btn_add_signal->Disable();
        m_btn_rename_signal->Disable();
        m_btn_remove_signal->Disable();
    }
}


void DIALOG_BUS_MANAGER::OnSelectSignal( wxListEvent& event )
{
    if( event.GetEventType() == wxEVT_COMMAND_LIST_ITEM_SELECTED )
    {
        m_signal_edit->ChangeValue( event.GetText() );
        m_btn_rename_signal->Enable();
        m_btn_remove_signal->Enable();
    }
    else
    {
        m_signal_edit->Clear();
        m_btn_rename_signal->Disable();
        m_btn_remove_signal->Disable();
    }
}


void DIALOG_BUS_MANAGER::OnAddBus( wxCommandEvent& aEvent )
{
    // If there is an active alias, then check that the user actually
    // changed the text in the edit box (we can't have duplicate aliases)

    auto new_name = m_bus_edit->GetValue();

    if( new_name.Length() == 0 )
    {
        return;
    }

    for( const auto& alias : m_aliases )
    {
        if( alias->GetName() == new_name )
        {
            // TODO(JE) display error?
            return;
        }
    }

    if( !m_active_alias ||
        ( m_active_alias && m_active_alias->GetName().Cmp( new_name ) ) )
    {
        // The values are different; create a new alias
        auto alias = std::make_shared<BUS_ALIAS>();
        alias->SetName( new_name );

        // New aliases get stored on the currently visible sheet
        alias->SetParent( static_cast<SCH_EDIT_FRAME*>( GetParent() )->GetScreen() );
        auto text = getAliasDisplayText( alias );

        m_aliases.push_back( alias );
        long idx = m_bus_list_view->InsertItem( m_aliases.size() - 1, text );
        m_bus_list_view->SetColumnWidth( 0, -1 );
        m_bus_list_view->Select( idx );
        m_bus_edit->Clear();
    }
    else
    {
        // TODO(JE) Check about desired result here.
        // Maybe warn the user?  Or just do nothing
    }
}


void DIALOG_BUS_MANAGER::OnRenameBus( wxCommandEvent& aEvent )
{
    // We should only get here if there is an active alias
    wxASSERT( m_active_alias );

    m_active_alias->SetName( m_bus_edit->GetValue() );
    long idx = m_bus_list_view->FindItem( -1, wxUIntPtr( m_active_alias.get() ) );

    wxASSERT( idx >= 0 );

    m_bus_list_view->SetItemText( idx, getAliasDisplayText( m_active_alias ) );
}


void DIALOG_BUS_MANAGER::OnRemoveBus( wxCommandEvent& aEvent )
{
    // We should only get here if there is an active alias
    wxASSERT( m_active_alias );
    long i = m_bus_list_view->GetFirstSelected();
    wxASSERT(  m_active_alias == m_aliases[ i ] );

    m_bus_list_view->DeleteItem( i );
    m_bus_list_view->Update();
    m_aliases.erase( m_aliases.begin() + i );
    m_bus_edit->Clear();

    m_active_alias = nullptr;

    auto evt = wxListEvent( wxEVT_COMMAND_LIST_ITEM_DESELECTED );
    OnSelectBus( evt );
}


void DIALOG_BUS_MANAGER::OnAddSignal( wxCommandEvent& aEvent )
{
    auto name_list = m_signal_edit->GetValue();

    if( !m_active_alias || name_list.Length() == 0 )
    {
        return;
    }

    // String collecting net names that were not added to the bus
    wxString notAdded;

    // Parse a space-separated list and add each one
    wxStringTokenizer tok( name_list, " " );
    while( tok.HasMoreTokens() )
    {
        auto name = tok.GetNextToken();

        if( !m_active_alias->Contains( name ) )
        {
            m_active_alias->AddMember( name );

            long idx = m_signal_list_view->InsertItem(
                    m_active_alias->GetMemberCount() - 1, name );

            m_signal_list_view->SetColumnWidth( 0, -1 );
            m_signal_list_view->Select( idx );
        }
        else
        {
            // Some of the requested net names were not added to the list, so keep them for editing
            notAdded = notAdded.IsEmpty() ? name : notAdded + " " + name;
        }
    }

    m_signal_edit->SetValue( notAdded );
    m_signal_edit->SetInsertionPointEnd();
}


void DIALOG_BUS_MANAGER::OnRenameSignal( wxCommandEvent& aEvent )
{
    // We should only get here if there is an active alias
    wxASSERT( m_active_alias );

    auto new_name = m_signal_edit->GetValue();
    long idx = m_signal_list_view->GetFirstSelected();

    wxASSERT( idx >= 0 );

    auto old_name = m_active_alias->Members()[ idx ];

    // User could have typed a space here, so check first
    if( new_name.Find( " " ) != wxNOT_FOUND )
    {
        // TODO(JE) error feedback
        m_signal_edit->ChangeValue( old_name );
        return;
    }

    m_active_alias->Members()[ idx ] = new_name;
    m_signal_list_view->SetItemText( idx, new_name );
    m_signal_list_view->SetColumnWidth( 0, -1 );
}


void DIALOG_BUS_MANAGER::OnRemoveSignal( wxCommandEvent& aEvent )
{
    // We should only get here if there is an active alias
    wxASSERT( m_active_alias );

    long idx = m_signal_list_view->GetFirstSelected();

    wxASSERT( idx >= 0 );

    m_active_alias->Members().erase( m_active_alias->Members().begin() + idx );

    m_signal_list_view->DeleteItem( idx );
    m_signal_edit->Clear();
    m_btn_rename_signal->Disable();
    m_btn_remove_signal->Disable();
}


wxString DIALOG_BUS_MANAGER::getAliasDisplayText( std::shared_ptr< BUS_ALIAS > aAlias )
{
    wxString name = aAlias->GetName();
    wxFileName sheet_name( aAlias->GetParent()->GetFileName() );

    name += _T( " (" ) + sheet_name.GetFullName() + _T( ")" );

    return name;
}


// see invoke_sch_dialog.h
void InvokeDialogBusManager( SCH_EDIT_FRAME* aCaller )
{
    DIALOG_BUS_MANAGER dlg( aCaller );
    dlg.ShowModal();
}
