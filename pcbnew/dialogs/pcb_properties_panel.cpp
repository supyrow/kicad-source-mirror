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

#include "pcb_properties_panel.h"

#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <properties/property_mgr.h>
#include <properties/pg_editors.h>
#include <board_commit.h>
#include <board_connected_item.h>
#include <properties/pg_properties.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <settings/color_settings.h>


PCB_PROPERTIES_PANEL::PCB_PROPERTIES_PANEL( wxWindow* aParent, PCB_EDIT_FRAME* aFrame )
    : PROPERTIES_PANEL( aParent, aFrame ), m_frame( aFrame ), m_propMgr( PROPERTY_MANAGER::Instance() )
{
    m_propMgr.Rebuild();

    m_editor = wxPropertyGrid::RegisterEditorClass( new PG_UNIT_EDITOR( m_frame ),
                                                    wxT( "UnitEditor" ) );
}


void PCB_PROPERTIES_PANEL::UpdateData()
{
    PCB_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();

    // TODO perhaps it could be called less often? use PROPERTIES_TOOL and catch MODEL_RELOAD?
    updateLists( static_cast<PCB_EDIT_FRAME*>( m_frame )->GetBoard() );
    update( selection );
}


wxPGProperty* PCB_PROPERTIES_PANEL::createPGProperty( const PROPERTY_BASE* aProperty ) const
{
    if( aProperty->TypeHash() == TYPE_HASH( PCB_LAYER_ID ) )
    {
        wxASSERT( aProperty->HasChoices() );

        PGPROPERTY_COLORENUM* ret = new PGPROPERTY_COLORENUM( wxPG_LABEL, wxPG_LABEL,
                const_cast<wxPGChoices&>( aProperty->Choices() ) );

        ret->SetColorFunc(
                [&]( const wxString& aChoice ) -> wxColour
                {
                    PCB_LAYER_ID l = ENUM_MAP<PCB_LAYER_ID>::Instance().ToEnum( aChoice );
                    wxASSERT( IsPcbLayer( l ) );
                    return m_frame->GetColorSettings()->GetColor( l ).ToColour();
                } );

        ret->SetLabel( aProperty->Name() );
        ret->SetName( aProperty->Name() );
        ret->Enable( !aProperty->IsReadOnly() );
        ret->SetClientData( const_cast<PROPERTY_BASE*>( aProperty ) );

        return ret;
    }

    return PGPropertyFactory( aProperty );
}


void PCB_PROPERTIES_PANEL::valueChanged( wxPropertyGridEvent& aEvent )
{
    PCB_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();
    BOARD_ITEM* firstItem = static_cast<BOARD_ITEM*>( selection.Front() );
    PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *firstItem ), aEvent.GetPropertyName() );
    wxVariant newValue = aEvent.GetPropertyValue();
    BOARD_COMMIT changes( m_frame );

    for( EDA_ITEM* edaItem : selection )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( edaItem );
        changes.Modify( item );
        item->Set( property, newValue );
    }

    changes.Push( _( "Change property" ) );
    m_frame->Refresh();
}


void PCB_PROPERTIES_PANEL::updateLists( const BOARD* aBoard )
{
    wxPGChoices layersAll, layersCu, nets;

    // Regenerate all layers
    for( LSEQ seq = aBoard->GetEnabledLayers().UIOrder(); seq; ++seq )
        layersAll.Add( LSET::Name( *seq ), *seq );

    for( LSEQ seq = LSET( aBoard->GetEnabledLayers() & LSET::AllCuMask() ).UIOrder(); seq; ++seq )
        layersCu.Add( LSET::Name( *seq ), *seq );

    m_propMgr.GetProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ) )->SetChoices( layersAll );
    m_propMgr.GetProperty( TYPE_HASH( PCB_SHAPE ), _HKI( "Layer" ) )->SetChoices( layersAll );

    // Copper only properties
    m_propMgr.GetProperty( TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Layer" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_VIA ), _HKI( "Layer Top" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_VIA ), _HKI( "Layer Bottom" ) )->SetChoices( layersCu );

    // Regenerate nets
    for( const auto& netinfo : aBoard->GetNetInfo().NetsByNetcode() )
    {
        nets.Add( netinfo.second->GetNetname(), netinfo.first );
    }

    auto netProperty = m_propMgr.GetProperty( TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Net" ) );
    netProperty->SetChoices( nets );
}
