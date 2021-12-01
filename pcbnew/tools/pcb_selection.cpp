/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <functional>
using namespace std::placeholders;

#include <board.h>
#include <board_item.h>
#include <footprint.h>
#include <pcb_edit_frame.h>
#include <pcb_group.h>
#include <class_draw_panel_gal.h>
#include <view/view_controls.h>
#include <view/view_group.h>
#include <painter.h>
#include <bitmaps.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection.h>
#include <connectivity/connectivity_data.h>
#include "pcb_selection_tool.h"
#include "pcb_actions.h"

#include "plugins/kicad/pcb_plugin.h"



EDA_ITEM* PCB_SELECTION::GetTopLeftItem( bool aFootprintsOnly ) const
{
    EDA_ITEM* topLeftItem = nullptr;

    wxPoint pnt;

    // find the leftmost (smallest x coord) and highest (smallest y with the smallest x) item in the selection
    for( EDA_ITEM* item : m_items )
    {
        pnt = item->GetPosition();

        if( ( item->Type() != PCB_FOOTPRINT_T ) && aFootprintsOnly )
        {
            continue;
        }
        else
        {
            if( topLeftItem == nullptr )
            {
                topLeftItem = item;
            }
            else if( ( pnt.x < topLeftItem->GetPosition().x ) ||
                     ( ( topLeftItem->GetPosition().x == pnt.x ) &&
                     ( pnt.y < topLeftItem->GetPosition().y ) ) )
            {
                topLeftItem = item;
            }
        }
    }

    return topLeftItem;
}


const std::vector<KIGFX::VIEW_ITEM*> PCB_SELECTION::updateDrawList() const
{
    std::vector<VIEW_ITEM*> items;

    std::function<void ( EDA_ITEM* )> addItem;
    addItem = [&]( EDA_ITEM* item )
            {
                items.push_back( item );

                if( item->Type() == PCB_FOOTPRINT_T )
                {
                    FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );
                    footprint->RunOnChildren( [&]( BOARD_ITEM* bitem )
                                              {
                                                  addItem( bitem );
                                              } );
                }
                else if( item->Type() == PCB_GROUP_T )
                {
                    PCB_GROUP* group = static_cast<PCB_GROUP*>( item );
                    group->RunOnChildren( [&]( BOARD_ITEM* bitem )
                                          {
                                              addItem( bitem );
                                          } );
                }
            };

    for( EDA_ITEM* item : m_items )
        addItem( item );

    return items;
}


const LSET PCB_SELECTION::GetSelectionLayers()
{
    LSET retval;

    for( EDA_ITEM* item : m_items )
    {
        if( BOARD_ITEM* board_item = dynamic_cast<BOARD_ITEM*>( item ) )
            retval |= board_item->GetLayerSet();
    }

    return retval;
}
