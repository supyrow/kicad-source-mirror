/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "pcb_picker_tool.h"
#include "pcb_actions.h"
#include "pcb_grid_helper.h"
#include <view/view_controls.h>
#include <tools/zone_filler_tool.h>


PCB_PICKER_TOOL::PCB_PICKER_TOOL() :
        PCB_TOOL_BASE( "pcbnew.InteractivePicker" ),
        PICKER_TOOL_BASE()
{
}


int PCB_PICKER_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    PCB_BASE_FRAME*       frame    = getEditFrame<PCB_BASE_FRAME>();
    PCB_GRID_HELPER       grid( m_toolMgr, frame->GetMagneticItemsSettings() );
    int                   finalize_state = WAIT_CANCEL;

    if( aEvent.IsAction( &ACTIONS::pickerTool ) )
        frame->PushTool( aEvent );

    Activate();
    setControls();

    auto setCursor =
            [&]()
            {
                frame->GetCanvas()->SetCurrentCursor( m_cursor );
                controls->ShowCursor( true );
            };

    // Set initial cursor
    setCursor();
    VECTOR2D cursorPos;

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        cursorPos =  controls->GetMousePosition();

        if( m_snap )
        {
            grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
            grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
            cursorPos = grid.BestSnapAnchor( cursorPos, nullptr );
            controls->ForceCursorPosition( true, cursorPos );
        }

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( m_cancelHandler )
            {
                try
                {
                    (*m_cancelHandler)();
                }
                catch( std::exception& )
                {
                }
            }

            // Activating a new tool may have alternate finalization from canceling the current tool
            if( evt->IsActivate() )
                finalize_state = END_ACTIVATE;
            else
                finalize_state = EVT_CANCEL;

            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            bool getNext = false;

            m_picked = cursorPos;

            if( m_clickHandler )
            {
                try
                {
                    getNext = (*m_clickHandler)( *m_picked );
                }
                catch( std::exception& )
                {
                    finalize_state = EXCEPTION_CANCEL;
                    break;
                }
            }

            if( !getNext )
            {
                finalize_state = CLICK_CANCEL;
                break;
            }
            else
            {
                setControls();
            }
        }
        else if( evt->IsMotion() )
        {
            if( m_motionHandler )
            {
                try
                {
                    (*m_motionHandler)( cursorPos );
                }
                catch( std::exception& )
                {
                }
            }
        }
        else if( evt->IsDblClick( BUT_LEFT ) || evt->IsDrag( BUT_LEFT ) )
        {
            // Not currently used, but we don't want to pass them either
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            PCB_SELECTION dummy;
            m_menu.ShowContextMenu( dummy );
        }
        // TODO: It'd be nice to be able to say "don't allow any non-trivial editing actions",
        // but we don't at present have that, so we just knock out some of the egregious ones.
        else if( ZONE_FILLER_TOOL::IsZoneFillAction( evt ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    if( m_finalizeHandler )
    {
        try
        {
            (*m_finalizeHandler)( finalize_state );
        }
        catch( std::exception& )
        {
        }
    }

    reset();
    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );

    if( aEvent.IsAction( &ACTIONS::pickerTool ) )
        frame->PopTool( aEvent );

    return 0;
}


void PCB_PICKER_TOOL::reset()
{
    m_layerMask = LSET::AllLayersMask();
    PICKER_TOOL_BASE::reset();
}


void PCB_PICKER_TOOL::setControls()
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    controls->CaptureCursor( false );
    controls->SetAutoPan( false );
}


void PCB_PICKER_TOOL::setTransitions()
{
    Go( &PCB_PICKER_TOOL::Main, ACTIONS::pickerTool.MakeEvent() );
    Go( &PCB_PICKER_TOOL::Main, ACTIONS::pickerSubTool.MakeEvent() );
}


