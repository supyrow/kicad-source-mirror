/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "properties_panel.h"
#include <tool/selection.h>
#include <eda_base_frame.h>
#include <eda_item.h>
#include <import_export.h>
#include <properties/pg_cell_renderer.h>

#include <algorithm>
#include <set>

#include <wx/settings.h>
#include <wx/stattext.h>


extern APIIMPORT wxPGGlobalVarsClass* wxPGGlobalVars;

PROPERTIES_PANEL::PROPERTIES_PANEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame ) :
        wxPanel( aParent ),
        m_frame( aFrame ),
        m_splitter_key_proportion( -1 ),
        m_skipNextUpdate( false )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    // on some platforms wxPGGlobalVars is initialized automatically,
    // but others need an explicit init
    if( !wxPGGlobalVars )
        wxPGInitResourceModule();

    delete wxPGGlobalVars->m_defaultRenderer;
    wxPGGlobalVars->m_defaultRenderer = new PG_CELL_RENDERER();

    m_caption = new wxStaticText( this, wxID_ANY, _( "No objects selected" ), wxDefaultPosition,
                                  wxDefaultSize, 0 );
    mainSizer->Add( m_caption, 0, wxALL | wxEXPAND, 5 );

    m_grid = new wxPropertyGrid( this, wxID_ANY, wxDefaultPosition, wxSize( 300, 400 ),
                                 wxPG_DEFAULT_STYLE );
    m_grid->SetUnspecifiedValueAppearance( wxPGCell( wxT( "<...>" ) ) );
    m_grid->SetExtraStyle( wxPG_EX_HELP_AS_TOOLTIPS );
    mainSizer->Add( m_grid, 1, wxALL | wxEXPAND, 5 );

    m_grid->SetCellDisabledTextColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

#ifdef __WXGTK__
    // Needed for dark mode, on wx 3.0 at least.
    m_grid->SetCaptionTextColour( wxSystemSettings::GetColour( wxSYS_COLOUR_CAPTIONTEXT ) );
#endif

    SetSizer( mainSizer );
    Layout();

    m_grid->CenterSplitter();

    Connect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( PROPERTIES_PANEL::valueChanged ), NULL, this );
    Connect( wxEVT_PG_CHANGING, wxPropertyGridEventHandler( PROPERTIES_PANEL::valueChanging ), NULL, this );
    Connect( wxEVT_SHOW, wxShowEventHandler( PROPERTIES_PANEL::onShow ), NULL, this );

    Bind( wxEVT_PG_COL_END_DRAG,
          [&]( wxPropertyGridEvent& )
          {
              m_splitter_key_proportion =
                      static_cast<float>( m_grid->GetSplitterPosition() ) / m_grid->GetSize().x;
          } );

    Bind( wxEVT_SIZE,
          [&]( wxSizeEvent& aEvent )
          {
              RecalculateSplitterPos();
              aEvent.Skip();
          } );
}


void PROPERTIES_PANEL::update( const SELECTION& aSelection )
{
    if( m_skipNextUpdate )
    {
        m_skipNextUpdate = false;
        return;
    }

    if( m_grid->IsEditorFocused() )
        m_grid->CommitChangesFromEditor();

    m_grid->Clear();
    m_displayed.clear();

    if( aSelection.Empty() )
    {
        m_caption->SetLabel( _( "No objects selected" ) );
        return;
    }

    // Get all the selected types
    std::set<TYPE_ID> types;

    for( EDA_ITEM* item : aSelection )
        types.insert( TYPE_HASH( *item ) );

    wxCHECK( !types.empty(), /* void */ );

    if( aSelection.Size() > 1 )
    {
        m_caption->SetLabel( wxString::Format( _( "%d objects selected" ), aSelection.Size() ) );
    }
    else
    {
        m_caption->SetLabel( aSelection.Front()->GetFriendlyName() );
    }

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.SetUnits( m_frame->GetUserUnits() );
    propMgr.SetTransforms( &m_frame->GetOriginTransforms() );

    std::set<PROPERTY_BASE*> commonProps;
    const PROPERTY_LIST& allProperties = propMgr.GetProperties( *types.begin() );
    copy( allProperties.begin(), allProperties.end(), inserter( commonProps, commonProps.begin() ) );

    PROPERTY_DISPLAY_ORDER displayOrder = propMgr.GetDisplayOrder( *types.begin() );

    std::vector<wxString> groupDisplayOrder = propMgr.GetGroupDisplayOrder( *types.begin() );
    std::set<wxString> groups( groupDisplayOrder.begin(), groupDisplayOrder.end() );

    std::map<wxPGProperty*, int> pgPropOrders;
    std::map<wxString, std::vector<wxPGProperty*>> pgPropGroups;

    // Get all possible properties
    for( const auto& type : types )
    {
        const PROPERTY_LIST& itemProps = propMgr.GetProperties( type );

        const PROPERTY_DISPLAY_ORDER& itemDisplayOrder = propMgr.GetDisplayOrder( type );

        copy( itemDisplayOrder.begin(), itemDisplayOrder.end(),
              inserter( displayOrder, displayOrder.begin() ) );

        const std::vector<wxString>& itemGroups = propMgr.GetGroupDisplayOrder( type );

        for( const wxString& group : itemGroups )
        {
            if( !groups.count( group ) )
            {
                groupDisplayOrder.emplace_back( group );
                groups.insert( group );
            }
        }

        for( auto it = commonProps.begin(); it != commonProps.end(); /* ++it in the loop */ )
        {
            if( !binary_search( itemProps.begin(), itemProps.end(), *it ) )
                it = commonProps.erase( it );
            else
                ++it;
        }
    }

    // Find a set of properties that is common to all selected items
    for( const auto& property : commonProps )
    {
        if( property->IsInternal() )
            continue;

        if( !property->Available( aSelection.Front() ) )
            continue;

        // Either determine the common value for a property or "<...>" to indicate multiple values
        bool available = true;
        wxVariant commonVal, itemVal;

        for( EDA_ITEM* item : aSelection )
        {
            if( !property->Available( item ) )
                break; // there is an item that does not have this property, so do not display it

            wxVariant& value = commonVal.IsNull() ? commonVal : itemVal;
            const wxAny& any = item->Get( property );
            bool converted = false;

            if( property->HasChoices() )
            {
                // handle enums as ints, since there are no default conversion functions for wxAny
                int tmp;
                converted = any.GetAs<int>( &tmp );

                if( converted )
                    value = wxVariant( tmp );
            }

            if( !converted )                // all other types
                converted = any.GetAs( &value );

            if( !converted )
            {
                wxFAIL_MSG( "Could not convert wxAny to wxVariant" );
                available = false;
                break;
            }

            if( !commonVal.IsNull() && value != commonVal )
            {
                commonVal.MakeNull();       // items have different values for this property
                break;
            }
        }

        if( available )
        {
            wxPGProperty* pgProp = createPGProperty( property );

            if( pgProp )
            {
                pgProp->SetValue( commonVal );
                m_displayed.push_back( property );

                wxASSERT( displayOrder.count( property ) );
                pgPropOrders[pgProp] = displayOrder[property];
                pgPropGroups[property->Group()].emplace_back( pgProp );
            }
        }
    }

    const wxString unspecifiedGroupCaption = _( "Basic Properties" );

    for( const wxString& groupName : groupDisplayOrder )
    {
        if( !pgPropGroups.count( groupName ) )
            continue;

        std::vector<wxPGProperty*>& properties = pgPropGroups[groupName];

        auto groupItem = new wxPropertyCategory( groupName == wxEmptyString ?
                                                 unspecifiedGroupCaption : groupName );

        m_grid->Append( groupItem );

        std::sort( properties.begin(), properties.end(),
                   [&]( wxPGProperty*& aFirst, wxPGProperty*& aSecond )
                   {
                       return pgPropOrders[aFirst] < pgPropOrders[aSecond];
                   } );

        for( wxPGProperty* property : properties )
            m_grid->Append( property );
    }

    RecalculateSplitterPos();
}


void PROPERTIES_PANEL::onShow( wxShowEvent& aEvent )
{
    if( aEvent.IsShown() )
        UpdateData();
}


void PROPERTIES_PANEL::RecalculateSplitterPos()
{
    if( m_splitter_key_proportion < 0 )
        m_grid->CenterSplitter();
    else
        m_grid->SetSplitterPosition( m_splitter_key_proportion * m_grid->GetSize().x );
}
