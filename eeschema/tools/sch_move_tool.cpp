/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/tool_manager.h>
#include <tools/ee_grid_helper.h>
#include <tools/ee_selection_tool.h>
#include <tools/sch_line_wire_bus_tool.h>
#include <ee_actions.h>
#include <bitmaps.h>
#include <eda_item.h>
#include <sch_item.h>
#include <sch_symbol.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_line.h>
#include <sch_edit_frame.h>
#include <eeschema_id.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include "sch_move_tool.h"


// For adding to or removing from selections
#define QUIET_MODE true


SCH_MOVE_TOOL::SCH_MOVE_TOOL() :
        EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveMove" ),
        m_moveInProgress( false ),
        m_isDrag( false ),
        m_moveOffset( 0, 0 )
{
}


bool SCH_MOVE_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    auto moveCondition =
            []( const SELECTION& aSel )
            {
                if( aSel.Empty() || SELECTION_CONDITIONS::OnlyType( SCH_MARKER_T )( aSel ) )
                    return false;

                if( SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( aSel ) )
                    return false;

                return true;
            };

    // Add move actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( EE_ACTIONS::move, moveCondition, 150 );
    selToolMenu.AddItem( EE_ACTIONS::drag, moveCondition, 150 );
    selToolMenu.AddItem( EE_ACTIONS::alignToGrid, moveCondition, 150 );

    return true;
}


int SCH_MOVE_TOOL::Main( const TOOL_EVENT& aEvent )
{
    EESCHEMA_SETTINGS*    cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    EE_GRID_HELPER        grid( m_toolMgr );
    bool                  wasDragging = m_moveInProgress && m_isDrag;

    m_anchorPos.reset();

    if( aEvent.IsAction( &EE_ACTIONS::move ) )
        m_isDrag = false;
    else if( aEvent.IsAction( &EE_ACTIONS::drag ) )
        m_isDrag = true;
    else if( aEvent.IsAction( &EE_ACTIONS::moveActivate ) )
        m_isDrag = !cfg->m_Input.drag_is_move;
    else
        return 0;

    if( m_moveInProgress )
    {
        if( m_isDrag != wasDragging )
        {
            EDA_ITEM* sel = m_selectionTool->GetSelection().Front();

            if( sel && !sel->IsNew() )
            {
                // Reset the selected items so we can start again with the current m_isDrag
                // state.
                m_frame->RollbackSchematicFromUndo();
                m_selectionTool->RemoveItemsFromSel( &m_dragAdditions, QUIET_MODE );
                m_anchorPos = m_cursor - m_moveOffset;
                m_moveInProgress = false;
                controls->SetAutoPan( false );

                // And give it a kick so it doesn't have to wait for the first mouse movement
                // to refresh.
                m_toolMgr->RunAction( EE_ACTIONS::restartMove );
            }
        }

        return 0;
    }

    // Be sure that there is at least one item that we can move. If there's no selection try
    // looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection).
    EE_SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::MovableItems );
    bool          unselect = selection.IsHover();

    // Keep an original copy of the starting points for cleanup after the move
    std::vector<DANGLING_END_ITEM> internalPoints;

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );

    if( selection.Empty() )
    {
        // Note that it's important to go through push/pop even when the selection is empty.
        // This keeps other tools from having to special-case an empty move.
        m_frame->PopTool( tool );
        return 0;
    }

    bool        restore_state = false;
    bool        chain_commands = false;
    TOOL_EVENT* evt = const_cast<TOOL_EVENT*>( &aEvent );
    VECTOR2I    prevPos;
    int         snapLayer = UNDEFINED_LAYER;

    m_cursor = controls->GetCursorPosition();

    // Main loop: keep receiving events
    do
    {
        m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        if( evt->IsAction( &EE_ACTIONS::moveActivate )
                || evt->IsAction( &EE_ACTIONS::restartMove )
                || evt->IsAction( &EE_ACTIONS::move )
                || evt->IsAction( &EE_ACTIONS::drag )
                || evt->IsMotion()
                || evt->IsDrag( BUT_LEFT )
                || evt->IsAction( &ACTIONS::refreshPreview ) )
        {
            if( !m_moveInProgress )    // Prepare to start moving/dragging
            {
                SCH_ITEM* sch_item = (SCH_ITEM*) selection.Front();
                bool      appendUndo = sch_item && sch_item->IsNew();
                bool      placingNewItems = sch_item && sch_item->IsNew();

                //------------------------------------------------------------------------
                // Setup a drag or a move
                //
                m_dragAdditions.clear();
                m_specialCaseLabels.clear();
                internalPoints.clear();

                for( SCH_ITEM* it : m_frame->GetScreen()->Items() )
                {
                    it->ClearFlags( TEMP_SELECTED );

                    if( !it->IsSelected() )
                        it->ClearFlags( STARTPOINT | ENDPOINT );

                    if( !selection.IsHover() && it->IsSelected() )
                        it->SetFlags( STARTPOINT | ENDPOINT );
                }

                if( m_isDrag )
                {
                    EDA_ITEMS connectedDragItems;

                    // Add connections to the selection for a drag.
                    //
                    for( EDA_ITEM* edaItem : selection )
                    {
                        SCH_ITEM* item = static_cast<SCH_ITEM*>( edaItem );
                        std::vector<wxPoint> connections;

                        if( item->Type() == SCH_LINE_T )
                            static_cast<SCH_LINE*>( item )->GetSelectedPoints( connections );
                        else
                            connections = item->GetConnectionPoints();

                        for( wxPoint point : connections )
                            getConnectedDragItems( item, point, connectedDragItems );
                    }

                    for( EDA_ITEM* item : connectedDragItems )
                    {
                        m_dragAdditions.push_back( item->m_Uuid );
                        m_selectionTool->AddItemToSel( item, QUIET_MODE );
                    }
                }
                else
                {
                    // Mark the edges of the block with dangling flags for a move.
                    for( EDA_ITEM* item : selection )
                        static_cast<SCH_ITEM*>( item )->GetEndPoints( internalPoints );

                    for( EDA_ITEM* item : selection )
                        static_cast<SCH_ITEM*>( item )->UpdateDanglingState( internalPoints );
                }
                // Generic setup
                //
                for( EDA_ITEM* item : selection )
                {
                    if( static_cast<SCH_ITEM*>( item )->IsConnectable() )
                    {
                        if( snapLayer == LAYER_GRAPHICS )
                            snapLayer = LAYER_ANY;
                        else
                            snapLayer = LAYER_CONNECTABLE;
                    }
                    else
                    {
                        if( snapLayer == LAYER_CONNECTABLE )
                            snapLayer = LAYER_ANY;
                        else
                            snapLayer = LAYER_GRAPHICS;
                    }

                    if( item->IsNew() )
                    {
                        if( item->HasFlag( TEMP_SELECTED ) && m_isDrag )
                        {
                            // Item was added in getConnectedDragItems
                            saveCopyInUndoList( (SCH_ITEM*) item, UNDO_REDO::NEWITEM, appendUndo );
                            appendUndo = true;
                        }
                        else
                        {
                            // Item was added in a previous command (and saved to undo by
                            // that command)
                        }
                    }
                    else if( item->GetParent() && item->GetParent()->IsSelected() )
                    {
                        // Item will be (or has been) saved to undo by parent
                    }
                    else
                    {
                        saveCopyInUndoList( (SCH_ITEM*) item, UNDO_REDO::CHANGED, appendUndo );
                        appendUndo = true;
                    }

                    SCH_ITEM* schItem = (SCH_ITEM*) item;
                    schItem->SetStoredPos( schItem->GetPosition() );
                }

                // Set up the starting position and move/drag offset
                //
                m_cursor = controls->GetCursorPosition();

                if( evt->IsAction( &EE_ACTIONS::restartMove ) )
                {
                    wxASSERT_MSG( m_anchorPos, "Should be already set from previous cmd" );
                }
                else if( placingNewItems )
                {
                    m_anchorPos = selection.GetReferencePoint();
                }

                if( m_anchorPos )
                {
                    VECTOR2I delta = m_cursor - (*m_anchorPos);
                    bool     isPasted = false;

                    // Drag items to the current cursor position
                    for( EDA_ITEM* item : selection )
                    {
                        // Don't double move pins, fields, etc.
                        if( item->GetParent() && item->GetParent()->IsSelected() )
                            continue;

                        moveItem( item, delta );
                        updateItem( item, false );

                        isPasted |= item->GetFlags() & IS_PASTED;
                        item->ClearFlags( IS_PASTED );
                    }

                    // The first time pasted items are moved we need to store
                    // the position of the cursor so that rotate while moving
                    // works as expected (instead of around the original anchor
                    // point
                    if( isPasted )
                        selection.SetReferencePoint( m_cursor );

                    m_anchorPos = m_cursor;
                }
                // For some items, moving the cursor to anchor is not good (for instance large
                // hierarchical sheets or symbols can have the anchor outside the view)
                else if( selection.Size() == 1 && !sch_item->IsMovableFromAnchorPoint() )
                {
                    m_cursor = getViewControls()->GetCursorPosition( true );
                    m_anchorPos = m_cursor;
                }
                else
                {
                    if( m_frame->GetMoveWarpsCursor() )
                    {
                        // User wants to warp the mouse
                        m_cursor = grid.BestDragOrigin( m_cursor, snapLayer, selection );
                        selection.SetReferencePoint( m_cursor );
                    }
                    else
                    {
                        // User does not want to warp the mouse
                        m_cursor = getViewControls()->GetCursorPosition( true );
                    }
                }

                controls->SetCursorPosition( m_cursor, false );
                m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );

                prevPos = m_cursor;
                controls->SetAutoPan( true );
                m_moveInProgress = true;
            }

            //------------------------------------------------------------------------
            // Follow the mouse
            //
            m_cursor = grid.BestSnapAnchor( controls->GetCursorPosition( false ),
                                            snapLayer, selection );

            VECTOR2I delta( m_cursor - prevPos );
            m_anchorPos = m_cursor;

            m_moveOffset += delta;
            prevPos = m_cursor;

            for( EDA_ITEM* item : selection )
            {
                // Don't double move pins, fields, etc.
                if( item->GetParent() && item->GetParent()->IsSelected() )
                    continue;

                moveItem( item, delta );
                updateItem( item, false );
            }

            if( selection.HasReferencePoint() )
                selection.SetReferencePoint( selection.GetReferencePoint() + delta );

            m_toolMgr->PostEvent( EVENTS::SelectedItemsMoved );
        }
        //------------------------------------------------------------------------
        // Handle cancel
        //
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( m_moveInProgress )
            {
                if( evt->IsActivate() )
                {
                    // Allowing other tools to activate during a move runs the risk of race
                    // conditions in which we try to spool up both event loops at once.

                    if( m_isDrag )
                        m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel drag." ) );
                    else
                        m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel move." ) );

                    evt->SetPassEvent( false );
                    continue;
                }

                evt->SetPassEvent( false );
                restore_state = true;
            }

            break;
        }
        //------------------------------------------------------------------------
        // Handle TOOL_ACTION special cases
        //
        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            unselect = true;
            break;
        }
        else if( evt->IsAction( &ACTIONS::doDelete ) )
        {
            evt->SetPassEvent();
            // Exit on a delete; there will no longer be anything to drag.
            break;
        }
        else if( evt->IsAction( &ACTIONS::duplicate ) )
        {
            if( selection.Front()->IsNew() )
            {
                // This doesn't really make sense; we'll just end up dragging a stack of
                // objects so we ignore the duplicate and just carry on.
                continue;
            }

            // Move original back and exit.  The duplicate will run in its own loop.
            restore_state = true;
            unselect = false;
            chain_commands = true;
            break;
        }
        else if( evt->IsAction( &EE_ACTIONS::rotateCW )
                || evt->IsAction( &EE_ACTIONS::rotateCCW )
                || evt->IsAction( &EE_ACTIONS::mirrorH )
                || evt->IsAction( &EE_ACTIONS::mirrorV ) )
        {
            evt->SetPassEvent();
        }
        else if( evt->Action() == TA_CHOICE_MENU_CHOICE )
        {
            if( evt->GetCommandId().get() >= ID_POPUP_SCH_SELECT_UNIT_CMP
                && evt->GetCommandId().get() <= ID_POPUP_SCH_SELECT_UNIT_SYM_MAX )
            {
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( selection.Front() );
                int unit = evt->GetCommandId().get() - ID_POPUP_SCH_SELECT_UNIT_CMP;

                if( symbol )
                {
                    m_frame->SelectUnit( symbol, unit );
                    m_toolMgr->RunAction( ACTIONS::refreshPreview );
                }
            }
        }
        //------------------------------------------------------------------------
        // Handle context menu
        //
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        //------------------------------------------------------------------------
        // Handle drop
        //
        else if( evt->IsMouseUp( BUT_LEFT )
                || evt->IsClick( BUT_LEFT )
                || evt->IsDblClick( BUT_LEFT ) )
        {
            break; // Finish
        }
        else
        {
            evt->SetPassEvent();
        }

        controls->SetAutoPan( m_moveInProgress );

    } while( ( evt = Wait() ) ); //Should be assignment not equality test

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetAutoPan( false );

    if( !chain_commands )
        m_moveOffset = { 0, 0 };

    m_anchorPos.reset();

    for( EDA_ITEM* item : selection )
        item->ClearEditFlags();

    if( restore_state )
    {
        m_selectionTool->RemoveItemsFromSel( &m_dragAdditions, QUIET_MODE );
        m_frame->RollbackSchematicFromUndo();
    }
    else
    {
        // One last update after exiting loop (for slower stuff, such as updating SCREEN's RTree).
        for( EDA_ITEM* item : selection )
            updateItem( item, true );

        EE_SELECTION selectionCopy( selection );
        m_selectionTool->RemoveItemsFromSel( &m_dragAdditions, QUIET_MODE );

        // If we move items away from a junction, we _may_ want to add a junction there
        // to denote the state.
        for( const DANGLING_END_ITEM& it : internalPoints )
        {
            if( m_frame->GetScreen()->IsExplicitJunctionNeeded( it.GetPosition()) )
                m_frame->AddJunction( m_frame->GetScreen(), it.GetPosition(), true, false );
        }

        m_toolMgr->RunAction( EE_ACTIONS::addNeededJunctions, true, &selectionCopy );

        m_frame->RecalculateConnections( LOCAL_CLEANUP );
        m_frame->TestDanglingEnds();

        m_frame->OnModify();
    }

    if( unselect )
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    else
        m_selectionTool->RebuildSelection();  // Schematic cleanup might have merged lines, etc.

    m_dragAdditions.clear();
    m_moveInProgress = false;
    m_frame->PopTool( tool );
    return 0;
}


void SCH_MOVE_TOOL::getConnectedDragItems( SCH_ITEM* aOriginalItem, const wxPoint& aPoint,
                                           EDA_ITEMS& aList )
{
    EE_RTREE&         items = m_frame->GetScreen()->Items();
    EE_RTREE::EE_TYPE itemsOverlapping = items.Overlapping( aOriginalItem->GetBoundingBox() );
    bool              ptHasUnselectedJunction = false;

    for( SCH_ITEM* item : itemsOverlapping )
    {
        if( item->Type() == SCH_JUNCTION_T && item->IsConnected( aPoint ) && !item->IsSelected() )
        {
            ptHasUnselectedJunction = true;
            break;
        }
    }

    for( SCH_ITEM* test : itemsOverlapping )
    {
        if( test == aOriginalItem || test->IsSelected() || !test->CanConnect( aOriginalItem ) )
            continue;

        KICAD_T testType = test->Type();

        switch( testType )
        {
        case SCH_LINE_T:
        {
            // Select the connected end of wires/bus connections that don't have an unselected
            // junction isolating them from the drag
            if( ptHasUnselectedJunction )
                break;

            SCH_LINE* line = static_cast<SCH_LINE*>( test );

            if( line->GetStartPoint() == aPoint )
            {
                if( !line->HasFlag(TEMP_SELECTED ) )
                    aList.push_back( line );

                line->SetFlags(STARTPOINT | TEMP_SELECTED );
            }
            else if( line->GetEndPoint() == aPoint )
            {
                if( !line->HasFlag(TEMP_SELECTED ) )
                    aList.push_back( line );

                line->SetFlags(ENDPOINT | TEMP_SELECTED );
            }
            else
            {
                break;
            }

            // Since only one end is going to move, the movement vector of any labels attached
            // to it is scaled by the proportion of the line length the label is from the moving
            // end.
            for( SCH_ITEM* item : itemsOverlapping )
            {
                if( item->Type() == SCH_LABEL_T )
                {
                    SCH_TEXT* label = static_cast<SCH_TEXT*>( item );

                    if( label->IsSelected() )
                        continue;   // These will be moved on their own because they're selected

                    if( label->HasFlag( TEMP_SELECTED ) )
                        continue;

                    if( label->CanConnect( line ) && line->HitTest( label->GetPosition(), 1 ) )
                    {
                        label->SetFlags( TEMP_SELECTED );
                        aList.push_back( label );

                        SPECIAL_CASE_LABEL_INFO info;
                        info.attachedLine = line;
                        info.originalLabelPos = label->GetPosition();
                        m_specialCaseLabels[label] = info;
                    }
                }
            }

            break;
        }

        case SCH_SHEET_T:
        case SCH_SYMBOL_T:
        case SCH_JUNCTION_T:
            if( test->IsConnected( aPoint ) )
            {
                // Add a new wire between the symbol or junction and the selected item so
                // the selected item can be dragged.
                SCH_LINE* newWire = nullptr;

                if( test->GetLayer() == LAYER_BUS_JUNCTION ||
                    aOriginalItem->GetLayer() == LAYER_BUS )
                {
                    newWire = new SCH_LINE( aPoint, LAYER_BUS );
                }
                else
                    newWire = new SCH_LINE( aPoint, LAYER_WIRE );

                newWire->SetFlags( IS_NEW );
                m_frame->AddToScreen( newWire, m_frame->GetScreen() );

                newWire->SetFlags( TEMP_SELECTED | STARTPOINT );
                aList.push_back( newWire );
            }
            break;

        case SCH_NO_CONNECT_T:
            // Select no-connects that are connected to items being moved.
            if( !test->HasFlag( TEMP_SELECTED ) && test->IsConnected( aPoint ) )
            {
                aList.push_back( test );
                test->SetFlags( TEMP_SELECTED );
            }

            break;

        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
            // Performance optimization:
            if( test->HasFlag( TEMP_SELECTED ) )
                break;

            // Select labels that are connected to a wire (or bus) being moved.
            if( aOriginalItem->Type() == SCH_LINE_T && test->CanConnect( aOriginalItem ) )
            {
                SCH_TEXT* label = static_cast<SCH_TEXT*>( test );
                SCH_LINE* line = static_cast<SCH_LINE*>( aOriginalItem );
                bool      oneEndFixed = !line->HasFlag( STARTPOINT ) || !line->HasFlag( ENDPOINT );

                if( line->HitTest( label->GetTextPos(), 1 ) )
                {
                    label->SetFlags( TEMP_SELECTED );
                    aList.push_back( label );

                    if( oneEndFixed )
                    {
                        SPECIAL_CASE_LABEL_INFO info;
                        info.attachedLine = line;
                        info.originalLabelPos = label->GetPosition();
                        m_specialCaseLabels[ label ] = info;
                    }
                }
            }

            break;

        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
            // Performance optimization:
            if( test->HasFlag( TEMP_SELECTED ) )
                break;

            // Select bus entries that are connected to a bus being moved.
            if( aOriginalItem->Type() == SCH_LINE_T && test->CanConnect( aOriginalItem ) )
            {
                SCH_LINE* line = static_cast<SCH_LINE*>( aOriginalItem );
                bool      oneEndFixed = !line->HasFlag( STARTPOINT ) || !line->HasFlag( ENDPOINT );

                if( oneEndFixed )
                {
                    // This is only going to end in tears, so don't go there
                    continue;
                }

                for( wxPoint& point : test->GetConnectionPoints() )
                {
                    if( line->HitTest( point, 1 ) )
                    {
                        test->SetFlags( TEMP_SELECTED );
                        aList.push_back( test );

                        // A bus entry needs its wire & label as well
                        std::vector<wxPoint> ends = test->GetConnectionPoints();
                        wxPoint              otherEnd;

                        if( ends[0] == point )
                            otherEnd = ends[1];
                        else
                            otherEnd = ends[0];

                        getConnectedDragItems( test, otherEnd, aList );

                        // No need to test the other end of the bus entry
                        break;
                    }
                }
            }

            break;

        default:
            break;
        }
    }
}


void SCH_MOVE_TOOL::moveItem( EDA_ITEM* aItem, const VECTOR2I& aDelta )
{
    switch( aItem->Type() )
    {
    case SCH_LINE_T:
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( aItem );

        if( aItem->HasFlag( STARTPOINT ) )
            line->MoveStart( (wxPoint) aDelta );

        if( aItem->HasFlag( ENDPOINT ) )
            line->MoveEnd( (wxPoint) aDelta );

    }
        break;

    case SCH_PIN_T:
    case SCH_FIELD_T:
    {
        SCH_ITEM* parent = (SCH_ITEM*) aItem->GetParent();
        wxPoint   delta( aDelta );

        if( parent && parent->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = (SCH_SYMBOL*) aItem->GetParent();
            TRANSFORM   transform = symbol->GetTransform().InverseTransform();

            delta = transform.TransformCoordinate( delta );
        }

        static_cast<SCH_ITEM*>( aItem )->Move( delta );

        // If we're moving a field with respect to its parent then it's no longer auto-placed
        if( aItem->Type() == SCH_FIELD_T && parent && !parent->IsSelected() )
            parent->ClearFieldsAutoplaced();

        break;
    }
    case SCH_SHEET_PIN_T:
    {
        SCH_SHEET_PIN* pin = (SCH_SHEET_PIN*) aItem;
        pin->SetStoredPos( pin->GetStoredPos() + (wxPoint) aDelta );
        pin->ConstrainOnEdge( pin->GetStoredPos() );
        break;
    }
    case SCH_LABEL_T:
    {
        SCH_TEXT* label = static_cast<SCH_TEXT*>( aItem );

        if( m_specialCaseLabels.count( label ) )
        {
            SPECIAL_CASE_LABEL_INFO info = m_specialCaseLabels[ label ];
            SEG currentLine( info.attachedLine->GetStartPoint(), info.attachedLine->GetEndPoint() );
            label->SetPosition( (wxPoint) currentLine.NearestPoint( info.originalLabelPos ) );
        }
        else
        {
            label->Move( (wxPoint) aDelta );
        }

        break;
    }
    default:
        static_cast<SCH_ITEM*>( aItem )->Move( (wxPoint) aDelta );
        break;
    }

    getView()->Hide( aItem, false );
    aItem->SetFlags( IS_MOVING );
}


int SCH_MOVE_TOOL::AlignElements( const TOOL_EVENT& aEvent )
{
    EE_GRID_HELPER grid( m_toolMgr);
    EE_SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::MovableItems );
    bool append_undo = false;

    for( SCH_ITEM* it : m_frame->GetScreen()->Items() )
    {
        if( !it->IsSelected() )
            it->ClearFlags( STARTPOINT | ENDPOINT );

        if( !selection.IsHover() && it->IsSelected() )
            it->SetFlags( STARTPOINT | ENDPOINT );

        it->SetStoredPos( it->GetPosition() );

        if( it->Type() == SCH_SHEET_T )
        {
            for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( it )->GetPins() )
                pin->SetStoredPos( pin->GetPosition() );
        }
    }

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );
            std::vector<int> flags{ STARTPOINT, ENDPOINT };
            std::vector<wxPoint> pts{ line->GetStartPoint(), line->GetEndPoint() };

            for( int ii = 0; ii < 2; ++ii )
            {
                EDA_ITEMS drag_items{ item };
                line->ClearFlags();
                line->SetFlags( flags[ii] );
                getConnectedDragItems( line, pts[ii], drag_items );
                std::set<EDA_ITEM*> unique_items( drag_items.begin(), drag_items.end() );

                VECTOR2I gridpt = grid.AlignGrid( pts[ii] ) - pts[ii];

                if( gridpt != VECTOR2I( 0, 0 ) )
                {
                    for( EDA_ITEM* dragItem : unique_items )
                    {
                        if( dragItem->GetParent() && dragItem->GetParent()->IsSelected() )
                            continue;

                        saveCopyInUndoList( dragItem, UNDO_REDO::CHANGED, append_undo );
                        append_undo = true;

                        moveItem( dragItem, gridpt );
                        dragItem->ClearFlags( IS_MOVING );
                        updateItem( dragItem, true );
                    }
                }
            }
        }
        else if( item->Type() == SCH_FIELD_T )
        {
            VECTOR2I gridpt = grid.AlignGrid( item->GetPosition() ) - item->GetPosition();

            if( gridpt != VECTOR2I( 0, 0 ) )
            {
                saveCopyInUndoList( item, UNDO_REDO::CHANGED, append_undo );
                append_undo = true;

                moveItem( item, gridpt );
                updateItem( item, true );
                item->ClearFlags( IS_MOVING );
            }
        }
        else
        {
            std::vector<wxPoint> connections;
            EDA_ITEMS drag_items{ item };
            connections = static_cast<SCH_ITEM*>( item )->GetConnectionPoints();

            for( const wxPoint& point : connections )
                getConnectedDragItems( static_cast<SCH_ITEM*>( item ), point, drag_items );

            std::map<VECTOR2I, int> shifts;
            VECTOR2I most_common( 0, 0 );
            int max_count = 0;

            for( const wxPoint& conn : connections )
            {
                VECTOR2I gridpt = grid.AlignGrid( conn ) - conn;

                shifts[gridpt]++;

                if( shifts[gridpt] > max_count )
                {
                    most_common = gridpt;
                    max_count = shifts[most_common];
                }
            }

            if( most_common != VECTOR2I( 0, 0 ) )
            {
                for( EDA_ITEM* dragItem : drag_items )
                {
                    if( dragItem->GetParent() && dragItem->GetParent()->IsSelected() )
                        continue;

                    saveCopyInUndoList( dragItem, UNDO_REDO::CHANGED, append_undo );
                    append_undo = true;

                    moveItem( dragItem, most_common );
                    dragItem->ClearFlags( IS_MOVING );
                    updateItem( dragItem, true );
                }
            }
        }
    }

    m_toolMgr->PostEvent( EVENTS::SelectedItemsMoved );
    m_toolMgr->RunAction( EE_ACTIONS::addNeededJunctions, true, &selection );

    m_frame->RecalculateConnections( LOCAL_CLEANUP );
    m_frame->TestDanglingEnds();

    m_frame->OnModify();
    return 0;
}


void SCH_MOVE_TOOL::setTransitions()
{
    Go( &SCH_MOVE_TOOL::Main,               EE_ACTIONS::moveActivate.MakeEvent() );
    Go( &SCH_MOVE_TOOL::Main,               EE_ACTIONS::move.MakeEvent() );
    Go( &SCH_MOVE_TOOL::Main,               EE_ACTIONS::drag.MakeEvent() );
    Go( &SCH_MOVE_TOOL::AlignElements,      EE_ACTIONS::alignToGrid.MakeEvent() );
}
