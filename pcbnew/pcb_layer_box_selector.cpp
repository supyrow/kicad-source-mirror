/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 Jean-Pierre Charras <jean-pierre.charras@ujf-grenoble.fr>
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>
#include <layer_ids.h>
#include <settings/color_settings.h>

#include <board.h>
#include <pcb_layer_box_selector.h>
#include <tools/pcb_actions.h>

// translate aLayer to its action
static TOOL_ACTION* layer2action( PCB_LAYER_ID aLayer )
{
    switch( aLayer )
    {
    case F_Cu:      return &PCB_ACTIONS::layerTop;
    case In1_Cu:    return &PCB_ACTIONS::layerInner1;
    case In2_Cu:    return &PCB_ACTIONS::layerInner2;
    case In3_Cu:    return &PCB_ACTIONS::layerInner3;
    case In4_Cu:    return &PCB_ACTIONS::layerInner4;
    case In5_Cu:    return &PCB_ACTIONS::layerInner5;
    case In6_Cu:    return &PCB_ACTIONS::layerInner6;
    case In7_Cu:    return &PCB_ACTIONS::layerInner7;
    case In8_Cu:    return &PCB_ACTIONS::layerInner8;
    case In9_Cu:    return &PCB_ACTIONS::layerInner9;
    case In10_Cu:   return &PCB_ACTIONS::layerInner10;
    case In11_Cu:   return &PCB_ACTIONS::layerInner11;
    case In12_Cu:   return &PCB_ACTIONS::layerInner12;
    case In13_Cu:   return &PCB_ACTIONS::layerInner13;
    case In14_Cu:   return &PCB_ACTIONS::layerInner14;
    case In15_Cu:   return &PCB_ACTIONS::layerInner15;
    case In16_Cu:   return &PCB_ACTIONS::layerInner16;
    case In17_Cu:   return &PCB_ACTIONS::layerInner17;
    case In18_Cu:   return &PCB_ACTIONS::layerInner18;
    case In19_Cu:   return &PCB_ACTIONS::layerInner19;
    case In20_Cu:   return &PCB_ACTIONS::layerInner20;
    case In21_Cu:   return &PCB_ACTIONS::layerInner21;
    case In22_Cu:   return &PCB_ACTIONS::layerInner22;
    case In23_Cu:   return &PCB_ACTIONS::layerInner23;
    case In24_Cu:   return &PCB_ACTIONS::layerInner24;
    case In25_Cu:   return &PCB_ACTIONS::layerInner25;
    case In26_Cu:   return &PCB_ACTIONS::layerInner26;
    case In27_Cu:   return &PCB_ACTIONS::layerInner27;
    case In28_Cu:   return &PCB_ACTIONS::layerInner28;
    case In29_Cu:   return &PCB_ACTIONS::layerInner29;
    case In30_Cu:   return &PCB_ACTIONS::layerInner30;
    case B_Cu:      return &PCB_ACTIONS::layerBottom;
    default:        return nullptr;
    }
}


// class to display a layer list in a wxBitmapComboBox.

// Reload the Layers
void PCB_LAYER_BOX_SELECTOR::Resync()
{
    Freeze();
    Clear();

    const int BM_SIZE = 14;

    LSET show = LSET::AllLayersMask() & ~m_layerMaskDisable;
    LSET activated = getEnabledLayers() & ~m_layerMaskDisable;
    wxString layerstatus;

    for( PCB_LAYER_ID layerid : show.UIOrder() )
    {
        if( !m_showNotEnabledBrdlayers && !activated[layerid] )
            continue;
        else if( !activated[layerid] )
            layerstatus = wxT( " " ) + _( "(not activated)" );
        else
            layerstatus.Empty();

        wxBitmap bmp( BM_SIZE, BM_SIZE );
        DrawColorSwatch( bmp, getLayerColor( LAYER_PCB_BACKGROUND ), getLayerColor( layerid ) );

        wxString layername = getLayerName( layerid ) + layerstatus;

        if( m_layerhotkeys )
        {
            TOOL_ACTION* action = layer2action( layerid );

            if( action )
                layername = AddHotkeyName( layername, action->GetHotKey(), IS_COMMENT );
        }

        Append( layername, bmp, (void*)(intptr_t) layerid );
    }

    if( !m_undefinedLayerName.IsEmpty() )
        Append( m_undefinedLayerName, wxNullBitmap, (void*)(intptr_t)UNDEFINED_LAYER );

    // Ensure the size of the widget is enough to show the text and the icon
    // We have to have a selected item when doing this, because otherwise GTK
    // will just choose a random size that might not fit the actual data
    // (such as in cases where the font size is very large). So we select
    // the first item, get the size of the control and make that the minimum size,
    // then remove the selection (which was the initial state).
    SetSelection( 0 );

    SetMinSize( wxSize( -1, -1 ) );
    wxSize bestSize = GetBestSize();

    bestSize.x = GetBestSize().x + BM_SIZE + 10;
    SetMinSize( bestSize );

    SetSelection( wxNOT_FOUND );
    Thaw();
}


// Returns true if the layer id is enabled (i.e. is it should be displayed)
bool PCB_LAYER_BOX_SELECTOR::isLayerEnabled( LAYER_NUM aLayer ) const
{
    BOARD* board = m_boardFrame->GetBoard();

    return board->IsLayerEnabled( ToLAYER_ID( aLayer ) );
}


LSET PCB_LAYER_BOX_SELECTOR::getEnabledLayers() const
{
    BOARD* board = m_boardFrame->GetBoard();

    return board->GetEnabledLayers();
}


// Returns a color index from the layer id
COLOR4D PCB_LAYER_BOX_SELECTOR::getLayerColor( LAYER_NUM aLayer ) const
{
    wxASSERT( m_boardFrame );

    return m_boardFrame->GetColorSettings()->GetColor( aLayer );
}


// Returns the name of the layer id
wxString PCB_LAYER_BOX_SELECTOR::getLayerName( LAYER_NUM aLayer ) const
{
    BOARD* board = m_boardFrame->GetBoard();

    return board->GetLayerName( ToLAYER_ID( aLayer ) );
}
