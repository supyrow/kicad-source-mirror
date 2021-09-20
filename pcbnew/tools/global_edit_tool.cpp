/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <footprint.h>
#include <pcb_track.h>
#include <zone.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/edit_tool.h>
#include <dialogs/dialog_exchange_footprints.h>
#include <dialogs/dialog_cleanup_tracks_and_vias.h>
#include <dialogs/dialog_swap_layers.h>
#include <dialogs/dialog_unused_pad_layers.h>
#include <tools/global_edit_tool.h>
#include <board_commit.h>
#include <dialogs/dialog_cleanup_graphics.h>

GLOBAL_EDIT_TOOL::GLOBAL_EDIT_TOOL() :
        PCB_TOOL_BASE( "pcbnew.GlobalEdit" ),
        m_selectionTool( nullptr )
{
}


void GLOBAL_EDIT_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason != RUN )
        m_commit = std::make_unique<BOARD_COMMIT>( this );
}


bool GLOBAL_EDIT_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    return true;
}


int GLOBAL_EDIT_TOOL::ExchangeFootprints( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION& selection = m_selectionTool->GetSelection();
    FOOTPRINT*     footprint = nullptr;
    bool           updateMode = false;
    bool           currentMode = false;

    if( aEvent.HasPosition() )
        selection = m_selectionTool->RequestSelection( EDIT_TOOL::FootprintFilter );

    if( !selection.Empty() )
        footprint = selection.FirstOfKind<FOOTPRINT>();

    if( aEvent.IsAction( &PCB_ACTIONS::updateFootprint ) )
    {
        updateMode = true;
        currentMode = true;
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::updateFootprints ) )
    {
        updateMode = true;
        currentMode = false;
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::changeFootprint ) )
    {
        updateMode = false;
        currentMode = true;
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::changeFootprints ) )
    {
        updateMode = false;
        currentMode = false;
    }
    else
    {
        wxFAIL_MSG( "ExchangeFootprints: unexpected action" );
    }

    // invoke the exchange dialog process
    {
        PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
        DIALOG_EXCHANGE_FOOTPRINTS dialog( editFrame, footprint, updateMode, currentMode );
        dialog.ShowQuasiModal();
    }

    return 0;
}


bool GLOBAL_EDIT_TOOL::swapBoardItem( BOARD_ITEM* aItem, PCB_LAYER_ID* aLayerMap )
{
    if( aLayerMap[ aItem->GetLayer() ] != aItem->GetLayer() )
    {
        m_commit->Modify( aItem );
        aItem->SetLayer( aLayerMap[ aItem->GetLayer() ] );
        frame()->GetCanvas()->GetView()->Update( aItem, KIGFX::GEOMETRY );
        return true;
    }

    return false;
}


int GLOBAL_EDIT_TOOL::SwapLayers( const TOOL_EVENT& aEvent )
{
    PCB_LAYER_ID layerMap[PCB_LAYER_ID_COUNT];

    DIALOG_SWAP_LAYERS dlg( frame(), layerMap );

    if( dlg.ShowModal() != wxID_OK )
        return 0;

    bool hasChanges = false;

    // Change tracks.
    for( PCB_TRACK* segm : frame()->GetBoard()->Tracks() )
    {
        if( segm->Type() == PCB_VIA_T )
        {
            PCB_VIA*     via = static_cast<PCB_VIA*>( segm );
            PCB_LAYER_ID top_layer, bottom_layer;

            if( via->GetViaType() == VIATYPE::THROUGH )
                continue;

            via->LayerPair( &top_layer, &bottom_layer );

            if( layerMap[bottom_layer] != bottom_layer || layerMap[top_layer] != top_layer )
            {
                m_commit->Modify( via );
                via->SetLayerPair( layerMap[top_layer], layerMap[bottom_layer] );
                frame()->GetCanvas()->GetView()->Update( via, KIGFX::GEOMETRY );
                hasChanges = true;
            }
        }
        else
        {
            hasChanges |= swapBoardItem( segm, layerMap );
        }
    }

    for( BOARD_ITEM* zone : frame()->GetBoard()->Zones() )
        hasChanges |= swapBoardItem( zone, layerMap );

    for( BOARD_ITEM* drawing : frame()->GetBoard()->Drawings() )
        hasChanges |= swapBoardItem( drawing, layerMap );

    if( hasChanges )
    {
        frame()->OnModify();
        m_commit->Push( "Layers moved" );
        frame()->GetCanvas()->Refresh();
    }

    return 0;
}


int GLOBAL_EDIT_TOOL::CleanupTracksAndVias( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    DIALOG_CLEANUP_TRACKS_AND_VIAS dlg( editFrame );

    dlg.ShowModal();
    return 0;
}


int GLOBAL_EDIT_TOOL::CleanupGraphics( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    DIALOG_CLEANUP_GRAPHICS dlg( editFrame, false );

    dlg.ShowModal();
    return 0;
}


int GLOBAL_EDIT_TOOL::RemoveUnusedPads( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    PCB_SELECTION&  selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForHierarchy( aCollector, true );
            } );
    DIALOG_UNUSED_PAD_LAYERS dlg( editFrame, selection, *m_commit );

    dlg.ShowModal();

    return 0;
}


void GLOBAL_EDIT_TOOL::setTransitions()
{
    Go( &GLOBAL_EDIT_TOOL::ExchangeFootprints,   PCB_ACTIONS::updateFootprint.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::ExchangeFootprints,   PCB_ACTIONS::updateFootprints.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::ExchangeFootprints,   PCB_ACTIONS::changeFootprint.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::ExchangeFootprints,   PCB_ACTIONS::changeFootprints.MakeEvent() );

    Go( &GLOBAL_EDIT_TOOL::SwapLayers,           PCB_ACTIONS::swapLayers.MakeEvent() );

    Go( &GLOBAL_EDIT_TOOL::EditTracksAndVias,    PCB_ACTIONS::editTracksAndVias.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::EditTextAndGraphics,  PCB_ACTIONS::editTextAndGraphics.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::GlobalDeletions,      PCB_ACTIONS::globalDeletions.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::CleanupTracksAndVias, PCB_ACTIONS::cleanupTracksAndVias.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::CleanupGraphics,      PCB_ACTIONS::cleanupGraphics.MakeEvent() );
    Go( &GLOBAL_EDIT_TOOL::RemoveUnusedPads,     PCB_ACTIONS::removeUnusedPads.MakeEvent() );
}


