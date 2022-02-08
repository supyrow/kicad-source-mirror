/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <symbol_edit_frame.h>
#include <lib_pin.h>
#include <settings/settings_manager.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <trigo.h>
#include <string_utils.h>
#include "sch_painter.h"

// small margin in internal units between the pin text and the pin line
#define PIN_TEXT_MARGIN 4

const wxString LIB_PIN::GetCanonicalElectricalTypeName( ELECTRICAL_PINTYPE aType )
{
    // These strings are the canonical name of the electrictal type
    // Not translated, no space in name, only ASCII chars.
    // to use when the string name must be known and well defined
    // must have same order than enum ELECTRICAL_PINTYPE (see lib_pin.h)
    static const wxChar* msgPinElectricType[] =
    {
        wxT( "input" ),
        wxT( "output" ),
        wxT( "bidirectional" ),
        wxT( "tri_state" ),
        wxT( "passive" ),
        wxT( "free" ),
        wxT( "unspecified" ),
        wxT( "power_in" ),
        wxT( "power_out" ),
        wxT( "open_collector" ),
        wxT( "open_emitter" ),
        wxT( "no_connect" )
    };

    return msgPinElectricType[static_cast<int>( aType )];
}


/// Utility for getting the size of the 'internal' pin decorators (as a radius)
// i.e. the clock symbols (falling clock is actually external but is of
// the same kind)

static int internalPinDecoSize( const RENDER_SETTINGS* aSettings, const LIB_PIN &aPin )
{
    const KIGFX::SCH_RENDER_SETTINGS* settings = static_cast<const KIGFX::SCH_RENDER_SETTINGS*>( aSettings );

    if( settings && settings->m_PinSymbolSize )
        return settings->m_PinSymbolSize;

    return aPin.GetNameTextSize() != 0 ? aPin.GetNameTextSize() / 2 : aPin.GetNumberTextSize() / 2;
}

/// Utility for getting the size of the 'external' pin decorators (as a radius)
// i.e. the negation circle, the polarity 'slopes' and the nonlogic
// marker
static int externalPinDecoSize( const RENDER_SETTINGS* aSettings, const LIB_PIN &aPin )
{
    const KIGFX::SCH_RENDER_SETTINGS* settings = static_cast<const KIGFX::SCH_RENDER_SETTINGS*>( aSettings );

    if( settings && settings->m_PinSymbolSize )
        return settings->m_PinSymbolSize;

    return aPin.GetNumberTextSize() / 2;
}


LIB_PIN::LIB_PIN( LIB_SYMBOL* aParent ) :
        LIB_ITEM( LIB_PIN_T, aParent ),
        m_orientation( PIN_RIGHT ),
        m_shape( GRAPHIC_PINSHAPE::LINE ),
        m_type( ELECTRICAL_PINTYPE::PT_UNSPECIFIED ),
        m_attributes( 0 )
{
    // Use the application settings for pin sizes if exists.
    // pgm can be nullptr when running a shared lib from a script, not from a kicad appl
    PGM_BASE* pgm  = PgmOrNull();

    if( pgm )
    {
        auto* settings = pgm->GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
        m_length       = Mils2iu( settings->m_Defaults.pin_length );
        m_numTextSize  = Mils2iu( settings->m_Defaults.pin_num_size );
        m_nameTextSize = Mils2iu( settings->m_Defaults.pin_name_size );
    }
    else    // Use hardcoded eeschema defaults: symbol_editor settings are not existing.
    {
        m_length       = Mils2iu( DEFAULT_PIN_LENGTH );
        m_numTextSize  = Mils2iu( DEFAULT_PINNUM_SIZE );
        m_nameTextSize = Mils2iu( DEFAULT_PINNAME_SIZE );
    }
}


LIB_PIN::LIB_PIN( LIB_SYMBOL* aParent, const wxString& aName, const wxString& aNumber,
                  int aOrientation, ELECTRICAL_PINTYPE aPinType, int aLength, int aNameTextSize,
                  int aNumTextSize, int aConvert, const VECTOR2I& aPos, int aUnit ) :
        LIB_ITEM( LIB_PIN_T, aParent ),
        m_position( aPos ),
        m_length( aLength ),
        m_orientation( aOrientation ),
        m_shape( GRAPHIC_PINSHAPE::LINE ),
        m_type( aPinType ),
        m_attributes( 0 ),
        m_numTextSize( aNumTextSize ),
        m_nameTextSize( aNameTextSize )
{
    SetName( aName );
    SetNumber( aNumber );
    SetUnit( aUnit );
    SetConvert( aConvert );
}


bool LIB_PIN::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    EDA_RECT rect = GetBoundingBox();

    return rect.Inflate( aAccuracy ).Contains( aPosition );
}


bool LIB_PIN::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    EDA_RECT sel = aRect;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox( false, true ) );

    return sel.Intersects( GetBoundingBox( false, true ) );
}


int LIB_PIN::GetPenWidth() const
{
    return 0;
}


KIFONT::FONT* LIB_PIN::GetDrawFont() const
{
    return KIFONT::FONT::GetFont( GetDefaultFont(), false, false );
}


wxString LIB_PIN::GetShownName() const
{
    if( m_name == "~" )
        return wxEmptyString;
    else
        return m_name;
}


VECTOR2I LIB_PIN::GetPinRoot() const
{
    switch( m_orientation )
    {
    default:
    case PIN_RIGHT: return VECTOR2I( m_position.x + m_length, -( m_position.y ) );
    case PIN_LEFT:  return VECTOR2I( m_position.x - m_length, -( m_position.y ) );
    case PIN_UP:    return VECTOR2I( m_position.x, -( m_position.y + m_length ) );
    case PIN_DOWN:  return VECTOR2I( m_position.x, -( m_position.y - m_length ) );
    }
}


void LIB_PIN::print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset, void* aData,
                     const TRANSFORM& aTransform )
{
    LIB_SYMBOL_OPTIONS* opts = (LIB_SYMBOL_OPTIONS*) aData;
    bool                drawHiddenFields   = opts ? opts->draw_hidden_fields : false;
    bool                showPinType        = opts ? opts->show_elec_type     : false;
    bool                show_connect_point = opts ? opts->show_connect_point : false;

    LIB_SYMBOL* part = GetParent();

    /* Calculate pin orient taking in account the symbol orientation. */
    int orient = PinDrawOrient( aTransform );

    /* Calculate the pin position */
    VECTOR2I pos1 = aTransform.TransformCoordinate( m_position ) + aOffset;

    if( IsVisible() || drawHiddenFields )
    {
        printPinSymbol( aSettings, pos1, orient );

        printPinTexts( aSettings, pos1, orient, part->GetPinNameOffset(),
                       opts->force_draw_pin_text || part->ShowPinNumbers(),
                       opts->force_draw_pin_text || part->ShowPinNames() );

        if( showPinType )
            printPinElectricalTypeName( aSettings, pos1, orient );

        if( show_connect_point
                && m_type != ELECTRICAL_PINTYPE::PT_NC
                && m_type != ELECTRICAL_PINTYPE::PT_NIC )
        {
            wxDC* DC = aSettings->GetPrintDC();
            COLOR4D color = aSettings->GetLayerColor( IsVisible() ? LAYER_PIN : LAYER_HIDDEN );
            GRCircle( DC, pos1, TARGET_PIN_RADIUS, 0, color );
        }
    }
}


void LIB_PIN::printPinSymbol( const RENDER_SETTINGS* aSettings, const VECTOR2I& aPos, int aOrient )
{
    wxDC*   DC = aSettings->GetPrintDC();
    int     MapX1, MapY1, x1, y1;
    int     width = GetEffectivePenWidth( aSettings );
    int     posX = aPos.x, posY = aPos.y, len = m_length;
    COLOR4D color = aSettings->GetLayerColor( IsVisible() ? LAYER_PIN : LAYER_HIDDEN );

    MapX1 = MapY1 = 0;
    x1    = posX;
    y1    = posY;

    switch( aOrient )
    {
    case PIN_UP:     y1 = posY - len;  MapY1 = 1;   break;
    case PIN_DOWN:   y1 = posY + len;  MapY1 = -1;  break;
    case PIN_LEFT:   x1 = posX - len;  MapX1 = 1;   break;
    case PIN_RIGHT:  x1 = posX + len;  MapX1 = -1;  break;
    }

    if( m_shape == GRAPHIC_PINSHAPE::INVERTED || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK )
    {
        const int radius = externalPinDecoSize( aSettings, *this );
        GRCircle( DC, VECTOR2I( MapX1 * radius + x1, MapY1 * radius + y1 ), radius, width, color );

        GRMoveTo( MapX1 * radius * 2 + x1, MapY1 * radius * 2 + y1 );
        GRLineTo( DC, posX, posY, width, color );
    }
    else
    {
        GRMoveTo( x1, y1 );
        GRLineTo( DC, posX, posY, width, color );
    }

    // Draw the clock shape (>)inside the symbol
    if( m_shape == GRAPHIC_PINSHAPE::CLOCK
            || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK
            || m_shape == GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK
            || m_shape == GRAPHIC_PINSHAPE::CLOCK_LOW )
    {
        const int clock_size = internalPinDecoSize( aSettings, *this );
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            GRMoveTo( x1, y1 + clock_size );
            GRLineTo( DC, x1 - MapX1 * clock_size * 2, y1, width, color );
            GRLineTo( DC, x1, y1 - clock_size, width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1 + clock_size, y1 );
            GRLineTo( DC, x1, y1 - MapY1 * clock_size * 2, width, color );
            GRLineTo( DC, x1 - clock_size, y1, width, color );
        }
    }

    // Draw the active low (or H to L active transition)
    if( m_shape == GRAPHIC_PINSHAPE::INPUT_LOW
            || m_shape == GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK
            || m_shape == GRAPHIC_PINSHAPE::CLOCK_LOW )
    {
        const int deco_size = externalPinDecoSize( aSettings, *this );
        if( MapY1 == 0 )            /* MapX1 = +- 1 */
        {
            GRMoveTo( x1 + MapX1 * deco_size * 2, y1 );
            GRLineTo( DC, x1 + MapX1 * deco_size * 2, y1 - deco_size * 2, width, color );
            GRLineTo( DC, x1, y1, width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1, y1 + MapY1 * deco_size * 2 );
            GRLineTo( DC, x1 - deco_size * 2, y1 + MapY1 * deco_size * 2, width, color );
            GRLineTo( DC, x1, y1, width, color );
        }
    }

    if( m_shape == GRAPHIC_PINSHAPE::OUTPUT_LOW ) /* IEEE symbol "Active Low Output" */
    {
        const int deco_size = externalPinDecoSize( aSettings, *this );
        if( MapY1 == 0 )            /* MapX1 = +- 1 */
        {
            GRMoveTo( x1, y1 - deco_size * 2 );
            GRLineTo( DC, x1 + MapX1 * deco_size * 2, y1, width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1 - deco_size * 2, y1 );
            GRLineTo( DC, x1, y1 + MapY1 * deco_size * 2, width, color );
        }
    }
    else if( m_shape == GRAPHIC_PINSHAPE::NONLOGIC ) /* NonLogic pin symbol */
    {
        const int deco_size = externalPinDecoSize( aSettings, *this );
        GRMoveTo( x1 - (MapX1 + MapY1) * deco_size, y1 - (MapY1 - MapX1) * deco_size );
        GRLineTo( DC, x1 + (MapX1 + MapY1) * deco_size, y1 + ( MapY1 - MapX1 ) * deco_size, width,
                  color );
        GRMoveTo( x1 - (MapX1 - MapY1) * deco_size, y1 - (MapY1 + MapX1) * deco_size );
        GRLineTo( DC, x1 + (MapX1 - MapY1) * deco_size, y1 + ( MapY1 + MapX1 ) * deco_size, width,
                  color );
    }

    if( m_type == ELECTRICAL_PINTYPE::PT_NC ) // Draw a N.C. symbol
    {
        const int deco_size = TARGET_PIN_RADIUS;
        GRLine( DC, posX - deco_size, posY - deco_size, posX + deco_size, posY + deco_size, width,
                color );
        GRLine( DC, posX + deco_size, posY - deco_size, posX - deco_size, posY + deco_size, width,
                color );
    }
}


void LIB_PIN::printPinTexts( const RENDER_SETTINGS* aSettings, VECTOR2I& aPinPos, int aPinOrient,
                             int aTextInside, bool aDrawPinNum, bool aDrawPinName )
{
    if( !aDrawPinName && !aDrawPinNum )
        return;

    int           x, y;
    wxDC*         DC = aSettings->GetPrintDC();
    KIFONT::FONT* font = GetDrawFont();

    wxSize pinNameSize( m_nameTextSize, m_nameTextSize );
    wxSize pinNumSize( m_numTextSize, m_numTextSize );

    int    namePenWidth = std::max( Clamp_Text_PenSize( GetPenWidth(), m_nameTextSize, false ),
                                    aSettings->GetDefaultPenWidth() );
    int    numPenWidth = std::max( Clamp_Text_PenSize( GetPenWidth(), m_numTextSize, false ),
                                   aSettings->GetDefaultPenWidth() );

    int    name_offset = Mils2iu( PIN_TEXT_MARGIN ) + namePenWidth;
    int    num_offset = Mils2iu( PIN_TEXT_MARGIN ) + numPenWidth;

    /* Get the num and name colors */
    COLOR4D NameColor = aSettings->GetLayerColor( IsVisible() ? LAYER_PINNAM : LAYER_HIDDEN );
    COLOR4D NumColor  = aSettings->GetLayerColor( IsVisible() ? LAYER_PINNUM : LAYER_HIDDEN );

    int x1 = aPinPos.x;
    int y1 = aPinPos.y;

    switch( aPinOrient )
    {
    case PIN_UP:    y1 -= m_length; break;
    case PIN_DOWN:  y1 += m_length; break;
    case PIN_LEFT:  x1 -= m_length; break;
    case PIN_RIGHT: x1 += m_length; break;
    }

    wxString name = GetShownName();
    wxString number = GetShownNumber();

    if( name.IsEmpty() )
        aDrawPinName = false;

    if( number.IsEmpty() )
        aDrawPinNum = false;

    if( aTextInside )  // Draw the text inside, but the pin numbers outside.
    {
        if(( aPinOrient == PIN_LEFT) || ( aPinOrient == PIN_RIGHT) )
        {
            // It is an horizontal line
            if( aDrawPinName )
            {
                if( aPinOrient == PIN_RIGHT )
                {
                    x = x1 + aTextInside;
                    GRPrintText( DC, VECTOR2I( x, y1 ), NameColor, name, ANGLE_HORIZONTAL,
                                 pinNameSize, GR_TEXT_H_ALIGN_LEFT, GR_TEXT_V_ALIGN_CENTER,
                                 namePenWidth, false, false, font );
                }
                else    // Orient == PIN_LEFT
                {
                    x = x1 - aTextInside;
                    GRPrintText( DC, VECTOR2I( x, y1 ), NameColor, name, ANGLE_HORIZONTAL,
                                 pinNameSize, GR_TEXT_H_ALIGN_RIGHT, GR_TEXT_V_ALIGN_CENTER,
                                 namePenWidth, false, false, font );
                }
            }

            if( aDrawPinNum )
            {
                GRPrintText( DC, VECTOR2I(( x1 + aPinPos.x) / 2, y1 - num_offset ), NumColor,
                             number, ANGLE_HORIZONTAL, pinNumSize, GR_TEXT_H_ALIGN_CENTER,
                             GR_TEXT_V_ALIGN_BOTTOM, numPenWidth, false, false, font );
            }
        }
        else            /* Its a vertical line. */
        {
            // Text is drawn from bottom to top (i.e. to negative value for Y axis)
            if( aPinOrient == PIN_DOWN )
            {
                y = y1 + aTextInside;

                if( aDrawPinName )
                {
                    GRPrintText( DC, VECTOR2I( x1, y ), NameColor, name, ANGLE_VERTICAL,
                                 pinNameSize, GR_TEXT_H_ALIGN_RIGHT, GR_TEXT_V_ALIGN_CENTER,
                                 namePenWidth, false, false, font );
                }

                if( aDrawPinNum )
                {
                    GRPrintText( DC, VECTOR2I( x1 - num_offset, ( y1 + aPinPos.y) / 2 ), NumColor,
                                 number, ANGLE_VERTICAL, pinNumSize, GR_TEXT_H_ALIGN_CENTER,
                                 GR_TEXT_V_ALIGN_BOTTOM, numPenWidth, false, false, font );
                }
            }
            else        /* PIN_UP */
            {
                y = y1 - aTextInside;

                if( aDrawPinName )
                {
                    GRPrintText( DC, VECTOR2I( x1, y ), NameColor, name, ANGLE_VERTICAL,
                                 pinNameSize, GR_TEXT_H_ALIGN_LEFT, GR_TEXT_V_ALIGN_CENTER,
                                 namePenWidth, false, false, font );
                }

                if( aDrawPinNum )
                {
                    GRPrintText( DC, VECTOR2I( x1 - num_offset, ( y1 + aPinPos.y) / 2 ), NumColor,
                                 number, ANGLE_VERTICAL, pinNumSize, GR_TEXT_H_ALIGN_CENTER,
                                 GR_TEXT_V_ALIGN_BOTTOM, numPenWidth, false, false, font );
                }
            }
        }
    }
    else     /**** Draw num & text pin outside  ****/
    {
        if( ( aPinOrient == PIN_LEFT) || ( aPinOrient == PIN_RIGHT) )
        {
            /* Its an horizontal line. */
            if( aDrawPinName )
            {
                x = ( x1 + aPinPos.x) / 2;
                GRPrintText( DC, VECTOR2I( x, y1 - name_offset ), NameColor, name, ANGLE_HORIZONTAL,
                             pinNameSize, GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM,
                             namePenWidth, false, false, font );
            }
            if( aDrawPinNum )
            {
                x = ( x1 + aPinPos.x) / 2;
                GRPrintText( DC, VECTOR2I( x, y1 + num_offset ), NumColor, number, ANGLE_HORIZONTAL,
                             pinNumSize, GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_TOP,
                             numPenWidth, false, false, font );
            }
        }
        else     /* Its a vertical line. */
        {
            if( aDrawPinName )
            {
                y = ( y1 + aPinPos.y) / 2;
                GRPrintText( DC, VECTOR2I( x1 - name_offset, y ), NameColor, name, ANGLE_VERTICAL,
                             pinNameSize, GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM,
                             namePenWidth, false, false, font );
            }

            if( aDrawPinNum )
            {
                GRPrintText( DC, VECTOR2I( x1 + num_offset, ( y1 + aPinPos.y) / 2 ), NumColor,
                             number, ANGLE_VERTICAL, pinNumSize, GR_TEXT_H_ALIGN_CENTER,
                             GR_TEXT_V_ALIGN_TOP, numPenWidth, false, false, font );
            }
        }
    }
}



void LIB_PIN::printPinElectricalTypeName( const RENDER_SETTINGS* aSettings, VECTOR2I& aPosition,
                                          int aOrientation )
{
    wxDC*       DC = aSettings->GetPrintDC();
    wxString    typeName = GetElectricalTypeName();

    // Use a reasonable (small) size to draw the text
    int         textSize = ( m_nameTextSize * 3 ) / 4;

    #define ETXT_MAX_SIZE Millimeter2iu( 0.7 )

    if( textSize > ETXT_MAX_SIZE )
        textSize = ETXT_MAX_SIZE;

    // Use a reasonable pen size to draw the text
    int pensize = textSize/6;

    // Get a suitable color
    COLOR4D color = aSettings->GetLayerColor( IsVisible() ? LAYER_NOTES : LAYER_HIDDEN );

    VECTOR2I txtpos = aPosition;
    int offset = Millimeter2iu( 0.4 );
    GR_TEXT_H_ALIGN_T hjustify = GR_TEXT_H_ALIGN_LEFT;
    EDA_ANGLE orient = ANGLE_HORIZONTAL;

    switch( aOrientation )
    {
    case PIN_UP:
        txtpos.y += offset;
        orient = ANGLE_VERTICAL;
        hjustify = GR_TEXT_H_ALIGN_RIGHT;
        break;

    case PIN_DOWN:
        txtpos.y -= offset;
        orient = ANGLE_VERTICAL;
        break;

    case PIN_LEFT:
        txtpos.x += offset;
        break;

    case PIN_RIGHT:
        txtpos.x -= offset;
        hjustify = GR_TEXT_H_ALIGN_RIGHT;
        break;
    }

    GRPrintText( DC, txtpos, color, typeName, orient, wxSize( textSize, textSize ), hjustify,
                 GR_TEXT_V_ALIGN_CENTER, pensize, false, false, GetDrawFont() );
}


void LIB_PIN::PlotSymbol( PLOTTER* aPlotter, const VECTOR2I& aPosition, int aOrientation ) const
{
    int     MapX1, MapY1, x1, y1;
    COLOR4D color = aPlotter->RenderSettings()->GetLayerColor( LAYER_PIN );
    int     penWidth = GetEffectivePenWidth( aPlotter->RenderSettings() );

    aPlotter->SetColor( color );
    aPlotter->SetCurrentLineWidth( penWidth );

    MapX1 = MapY1 = 0;
    x1 = aPosition.x; y1 = aPosition.y;

    switch( aOrientation )
    {
    case PIN_UP:     y1 = aPosition.y - m_length;  MapY1 = 1;   break;
    case PIN_DOWN:   y1 = aPosition.y + m_length;  MapY1 = -1;  break;
    case PIN_LEFT:   x1 = aPosition.x - m_length;  MapX1 = 1;   break;
    case PIN_RIGHT:  x1 = aPosition.x + m_length;  MapX1 = -1;  break;
    }

    if( m_shape == GRAPHIC_PINSHAPE::INVERTED || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK )
    {
        const int radius = externalPinDecoSize( aPlotter->RenderSettings(), *this );
        aPlotter->Circle( VECTOR2I( MapX1 * radius + x1, MapY1 * radius + y1 ), radius * 2,
                          FILL_T::NO_FILL, penWidth );

        aPlotter->MoveTo( VECTOR2I( MapX1 * radius * 2 + x1, MapY1 * radius * 2 + y1 ) );
        aPlotter->FinishTo( aPosition );
    }
    else if( m_shape == GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK )
    {
        const int deco_size = internalPinDecoSize( aPlotter->RenderSettings(), *this );
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( VECTOR2I( x1, y1 + deco_size ) );
            aPlotter->LineTo( VECTOR2I( x1 + MapX1 * deco_size * 2, y1 ) );
            aPlotter->FinishTo( VECTOR2I( x1, y1 - deco_size ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( VECTOR2I( x1 + deco_size, y1 ) );
            aPlotter->LineTo( VECTOR2I( x1, y1 + MapY1 * deco_size * 2 ) );
            aPlotter->FinishTo( VECTOR2I( x1 - deco_size, y1 ) );
        }

        aPlotter->MoveTo( VECTOR2I( MapX1 * deco_size * 2 + x1, MapY1 * deco_size * 2 + y1 ) );
        aPlotter->FinishTo( aPosition );
    }
    else
    {
        aPlotter->MoveTo( VECTOR2I( x1, y1 ) );
        aPlotter->FinishTo( aPosition );
    }

    if( m_shape == GRAPHIC_PINSHAPE::CLOCK
            || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK
            || m_shape == GRAPHIC_PINSHAPE::CLOCK_LOW )
    {
        const int deco_size = internalPinDecoSize( aPlotter->RenderSettings(), *this );
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( VECTOR2I( x1, y1 + deco_size ) );
            aPlotter->LineTo( VECTOR2I( x1 - MapX1 * deco_size * 2, y1 ) );
            aPlotter->FinishTo( VECTOR2I( x1, y1 - deco_size ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( VECTOR2I( x1 + deco_size, y1 ) );
            aPlotter->LineTo( VECTOR2I( x1, y1 - MapY1 * deco_size * 2 ) );
            aPlotter->FinishTo( VECTOR2I( x1 - deco_size, y1 ) );
        }
    }

    if( m_shape == GRAPHIC_PINSHAPE::INPUT_LOW
            || m_shape == GRAPHIC_PINSHAPE::CLOCK_LOW ) /* IEEE symbol "Active Low Input" */
    {
        const int deco_size = externalPinDecoSize( aPlotter->RenderSettings(), *this );

        if( MapY1 == 0 )        /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( VECTOR2I( x1 + MapX1 * deco_size * 2, y1 ) );
            aPlotter->LineTo( VECTOR2I( x1 + MapX1 * deco_size * 2, y1 - deco_size * 2 ) );
            aPlotter->FinishTo( VECTOR2I( x1, y1 ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( VECTOR2I( x1, y1 + MapY1 * deco_size * 2 ) );
            aPlotter->LineTo( VECTOR2I( x1 - deco_size * 2, y1 + MapY1 * deco_size * 2 ) );
            aPlotter->FinishTo( VECTOR2I( x1, y1 ) );
        }
    }

    if( m_shape == GRAPHIC_PINSHAPE::OUTPUT_LOW ) /* IEEE symbol "Active Low Output" */
    {
        const int symbol_size = externalPinDecoSize( aPlotter->RenderSettings(), *this );

        if( MapY1 == 0 )        /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( VECTOR2I( x1, y1 - symbol_size * 2 ) );
            aPlotter->FinishTo( VECTOR2I( x1 + MapX1 * symbol_size * 2, y1 ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( VECTOR2I( x1 - symbol_size * 2, y1 ) );
            aPlotter->FinishTo( VECTOR2I( x1, y1 + MapY1 * symbol_size * 2 ) );
        }
    }
    else if( m_shape == GRAPHIC_PINSHAPE::NONLOGIC ) /* NonLogic pin symbol */
    {
        const int deco_size = externalPinDecoSize( aPlotter->RenderSettings(), *this );
        aPlotter->MoveTo( VECTOR2I( x1 - ( MapX1 + MapY1 ) * deco_size,
                                    y1 - ( MapY1 - MapX1 ) * deco_size ) );
        aPlotter->FinishTo( VECTOR2I( x1 + ( MapX1 + MapY1 ) * deco_size,
                                      y1 + ( MapY1 - MapX1 ) * deco_size ) );
        aPlotter->MoveTo( VECTOR2I( x1 - ( MapX1 - MapY1 ) * deco_size,
                                    y1 - ( MapY1 + MapX1 ) * deco_size ) );
        aPlotter->FinishTo( VECTOR2I( x1 + ( MapX1 - MapY1 ) * deco_size,
                                      y1 + ( MapY1 + MapX1 ) * deco_size ) );
    }

    if( m_type == ELECTRICAL_PINTYPE::PT_NC ) // Draw a N.C. symbol
    {
        const int deco_size = TARGET_PIN_RADIUS;
        const int ex1 = aPosition.x;
        const int ey1 = aPosition.y;
        aPlotter->MoveTo( VECTOR2I( ex1 - deco_size, ey1 - deco_size ) );
        aPlotter->FinishTo( VECTOR2I( ex1 + deco_size, ey1 + deco_size ) );
        aPlotter->MoveTo( VECTOR2I( ex1 + deco_size, ey1 - deco_size ) );
        aPlotter->FinishTo( VECTOR2I( ex1 - deco_size, ey1 + deco_size ) );
    }
}


void LIB_PIN::PlotPinTexts( PLOTTER* aPlotter, const VECTOR2I& aPinPos, int aPinOrient,
                            int aTextInside, bool aDrawPinNum, bool aDrawPinName ) const
{
    wxString name = GetShownName();
    wxString number = GetShownNumber();

    if( name.IsEmpty() )
        aDrawPinName = false;

    if( number.IsEmpty() )
        aDrawPinNum = false;

    if( !aDrawPinNum && !aDrawPinName )
        return;

    int     x, y;
    wxSize  pinNameSize = wxSize( m_nameTextSize, m_nameTextSize );
    wxSize  pinNumSize  = wxSize( m_numTextSize, m_numTextSize );

    int     namePenWidth = std::max( Clamp_Text_PenSize( GetPenWidth(), m_nameTextSize, false ),
                                     aPlotter->RenderSettings()->GetDefaultPenWidth() );
    int     numPenWidth  = std::max( Clamp_Text_PenSize( GetPenWidth(), m_numTextSize, false ),
                                     aPlotter->RenderSettings()->GetDefaultPenWidth() );

    int     name_offset = Mils2iu( PIN_TEXT_MARGIN ) + namePenWidth;
    int     num_offset  = Mils2iu( PIN_TEXT_MARGIN ) + numPenWidth;

    /* Get the num and name colors */
    COLOR4D nameColor = aPlotter->RenderSettings()->GetLayerColor( LAYER_PINNAM );
    COLOR4D numColor  = aPlotter->RenderSettings()->GetLayerColor( LAYER_PINNUM );

    int x1 = aPinPos.x;
    int y1 = aPinPos.y;

    switch( aPinOrient )
    {
    case PIN_UP:     y1 -= m_length;  break;
    case PIN_DOWN:   y1 += m_length;  break;
    case PIN_LEFT:   x1 -= m_length;  break;
    case PIN_RIGHT:  x1 += m_length;  break;
    }

    /* Draw the text inside, but the pin numbers outside. */
    if( aTextInside )
    {
        if( ( aPinOrient == PIN_LEFT) || ( aPinOrient == PIN_RIGHT) ) /* Its an horizontal line. */
        {
            if( aDrawPinName )
            {
                GR_TEXT_H_ALIGN_T hjustify;
                if( aPinOrient == PIN_RIGHT )
                {
                    x = x1 + aTextInside;
                    hjustify = GR_TEXT_H_ALIGN_LEFT;
                }
                else    // orient == PIN_LEFT
                {
                    x = x1 - aTextInside;
                    hjustify = GR_TEXT_H_ALIGN_RIGHT;
                }

                aPlotter->Text( VECTOR2I( x, y1 ), nameColor, name, ANGLE_HORIZONTAL, pinNameSize,
                                hjustify, GR_TEXT_V_ALIGN_CENTER, namePenWidth, false, false );
            }
            if( aDrawPinNum )
            {
                aPlotter->Text( VECTOR2I( ( x1 + aPinPos.x) / 2, y1 - num_offset ), numColor,
                                number, ANGLE_HORIZONTAL, pinNumSize, GR_TEXT_H_ALIGN_CENTER,
                                GR_TEXT_V_ALIGN_BOTTOM, numPenWidth, false, false );
            }
        }
        else         /* Its a vertical line. */
        {
            if( aPinOrient == PIN_DOWN )
            {
                y = y1 + aTextInside;

                if( aDrawPinName )
                    aPlotter->Text( VECTOR2I( x1, y ), nameColor, name, ANGLE_VERTICAL,
                                    pinNameSize, GR_TEXT_H_ALIGN_RIGHT, GR_TEXT_V_ALIGN_CENTER,
                                    namePenWidth, false, false );

                if( aDrawPinNum )
                {
                    aPlotter->Text( VECTOR2I( x1 - num_offset, ( y1 + aPinPos.y) / 2 ), numColor,
                                    number, ANGLE_VERTICAL, pinNumSize, GR_TEXT_H_ALIGN_CENTER,
                                    GR_TEXT_V_ALIGN_BOTTOM, numPenWidth, false, false );
                }
            }
            else        /* PIN_UP */
            {
                y = y1 - aTextInside;

                if( aDrawPinName )
                {
                    aPlotter->Text( VECTOR2I( x1, y ), nameColor, name, ANGLE_VERTICAL,
                                    pinNameSize, GR_TEXT_H_ALIGN_LEFT, GR_TEXT_V_ALIGN_CENTER,
                                    namePenWidth, false, false );
                }

                if( aDrawPinNum )
                {
                    aPlotter->Text( VECTOR2I( x1 - num_offset,  ( y1 + aPinPos.y) / 2 ), numColor,
                                    number, ANGLE_VERTICAL, pinNumSize, GR_TEXT_H_ALIGN_CENTER,
                                    GR_TEXT_V_ALIGN_BOTTOM, numPenWidth, false, false );
                }
            }
        }
    }
    else     /* Draw num & text pin outside */
    {
        if(( aPinOrient == PIN_LEFT) || ( aPinOrient == PIN_RIGHT) )
        {
            /* Its an horizontal line. */
            if( aDrawPinName )
            {
                x = ( x1 + aPinPos.x) / 2;
                aPlotter->Text( VECTOR2I( x, y1 - name_offset ), nameColor, name, ANGLE_HORIZONTAL,
                                pinNameSize, GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM,
                                namePenWidth, false, false );
            }

            if( aDrawPinNum )
            {
                x = ( x1 + aPinPos.x ) / 2;
                aPlotter->Text( VECTOR2I( x, y1 + num_offset ), numColor, number, ANGLE_HORIZONTAL,
                                pinNumSize, GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_TOP,
                                numPenWidth, false, false );
            }
        }
        else     /* Its a vertical line. */
        {
            if( aDrawPinName )
            {
                y = ( y1 + aPinPos.y ) / 2;
                aPlotter->Text( VECTOR2I( x1 - name_offset, y ), nameColor, name, ANGLE_VERTICAL,
                                pinNameSize, GR_TEXT_H_ALIGN_CENTER, GR_TEXT_V_ALIGN_BOTTOM,
                                namePenWidth, false, false );
            }

            if( aDrawPinNum )
            {
                aPlotter->Text( VECTOR2I( x1 + num_offset, ( y1 + aPinPos.y ) / 2 ), numColor,
                                number, ANGLE_VERTICAL, pinNumSize, GR_TEXT_H_ALIGN_CENTER,
                                GR_TEXT_V_ALIGN_TOP, numPenWidth, false, false );
            }
        }
    }
}


int LIB_PIN::PinDrawOrient( const TRANSFORM& aTransform ) const
{
    int      orient;
    VECTOR2I end; // position of pin end starting at 0,0 according to its orientation, length = 1

    switch( m_orientation )
    {
    case PIN_UP:     end.y = 1;   break;
    case PIN_DOWN:   end.y = -1;  break;
    case PIN_LEFT:   end.x = -1;  break;
    case PIN_RIGHT:  end.x = 1;   break;
    }

    // = pos of end point, according to the symbol orientation.
    end    = aTransform.TransformCoordinate( end );
    orient = PIN_UP;

    if( end.x == 0 )
    {
        if( end.y > 0 )
            orient = PIN_DOWN;
    }
    else
    {
        orient = PIN_RIGHT;

        if( end.x < 0 )
            orient = PIN_LEFT;
    }

    return orient;
}


EDA_ITEM* LIB_PIN::Clone() const
{
    return new LIB_PIN( *this );
}


int LIB_PIN::compare( const LIB_ITEM& aOther, LIB_ITEM::COMPARE_FLAGS aCompareFlags ) const
{
    wxASSERT( aOther.Type() == LIB_PIN_T );

    int retv = LIB_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    const LIB_PIN* tmp = (LIB_PIN*) &aOther;

    // When comparing units, we do not compare the part numbers.  If everything else is
    // identical, then we can just renumber the parts for the inherited symbol.
    if( !( aCompareFlags & COMPARE_FLAGS::UNIT ) && m_number != tmp->m_number )
        return m_number.Cmp( tmp->m_number );

    int result = m_name.CmpNoCase( tmp->m_name );

    if( result )
        return result;

    if( m_position.x != tmp->m_position.x )
        return m_position.x - tmp->m_position.x;

    if( m_position.y != tmp->m_position.y )
        return m_position.y - tmp->m_position.y;

    if( m_length != tmp->m_length )
        return m_length - tmp->m_length;

    if( m_orientation != tmp->m_orientation )
        return m_orientation - tmp->m_orientation;

    if( m_shape != tmp->m_shape )
        return static_cast<int>( m_shape ) - static_cast<int>( tmp->m_shape );

    if( m_type != tmp->m_type )
        return static_cast<int>( m_type ) - static_cast<int>( tmp->m_type );

    if( m_attributes != tmp->m_attributes )
        return m_attributes - tmp->m_attributes;

    if( m_numTextSize != tmp->m_numTextSize )
        return m_numTextSize - tmp->m_numTextSize;

    if( m_nameTextSize != tmp->m_nameTextSize )
        return m_nameTextSize - tmp->m_nameTextSize;

    if( m_alternates.size() != tmp->m_alternates.size() )
        return m_alternates.size() - tmp->m_alternates.size();

    auto lhsItem = m_alternates.begin();
    auto rhsItem = tmp->m_alternates.begin();

    while( lhsItem != m_alternates.end() )
    {
        const ALT& lhsAlt = lhsItem->second;
        const ALT& rhsAlt = rhsItem->second;

        retv = lhsAlt.m_Name.Cmp( rhsAlt.m_Name );

        if( retv )
            return retv;

        if( lhsAlt.m_Type != rhsAlt.m_Type )
            return static_cast<int>( lhsAlt.m_Type ) - static_cast<int>( rhsAlt.m_Type );

        if( lhsAlt.m_Shape != rhsAlt.m_Shape )
            return static_cast<int>( lhsAlt.m_Shape ) - static_cast<int>( rhsAlt.m_Shape );

        ++lhsItem;
        ++rhsItem;
    }

    return 0;
}


void LIB_PIN::Offset( const VECTOR2I& aOffset )
{
    m_position += aOffset;
}


void LIB_PIN::MoveTo( const VECTOR2I& aNewPosition )
{
    if( m_position != aNewPosition )
    {
        m_position = aNewPosition;
        SetModified();
    }
}


void LIB_PIN::MirrorHorizontal( const VECTOR2I& aCenter )
{
    m_position.x -= aCenter.x;
    m_position.x *= -1;
    m_position.x += aCenter.x;

    if( m_orientation == PIN_RIGHT )
        m_orientation = PIN_LEFT;
    else if( m_orientation == PIN_LEFT )
        m_orientation = PIN_RIGHT;
}


void LIB_PIN::MirrorVertical( const VECTOR2I& aCenter )
{
    m_position.y -= aCenter.y;
    m_position.y *= -1;
    m_position.y += aCenter.y;

    if( m_orientation == PIN_UP )
        m_orientation = PIN_DOWN;
    else if( m_orientation == PIN_DOWN )
        m_orientation = PIN_UP;
}


void LIB_PIN::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    EDA_ANGLE rot_angle = aRotateCCW ? -ANGLE_90 : ANGLE_90;

    RotatePoint( m_position, aCenter, rot_angle );

    if( aRotateCCW )
    {
        switch( m_orientation )
        {
        case PIN_RIGHT: m_orientation = PIN_UP;    break;
        case PIN_UP:    m_orientation = PIN_LEFT;  break;
        case PIN_LEFT:  m_orientation = PIN_DOWN;  break;
        case PIN_DOWN:  m_orientation = PIN_RIGHT; break;
        }
    }
    else
    {
        switch( m_orientation )
        {
        case PIN_RIGHT: m_orientation = PIN_DOWN;  break;
        case PIN_UP:    m_orientation = PIN_RIGHT; break;
        case PIN_LEFT:  m_orientation = PIN_UP;    break;
        case PIN_DOWN:  m_orientation = PIN_LEFT;  break;
        }
    }
}


void LIB_PIN::Plot( PLOTTER* aPlotter, const VECTOR2I& aOffset, bool aFill,
                    const TRANSFORM& aTransform ) const
{
    if( !IsVisible() )
        return;

    int     orient = PinDrawOrient( aTransform );
    VECTOR2I pos = aTransform.TransformCoordinate( m_position ) + aOffset;

    PlotSymbol( aPlotter, pos, orient );
    PlotPinTexts( aPlotter, pos, orient, GetParent()->GetPinNameOffset(),
                  GetParent()->ShowPinNumbers(), GetParent()->ShowPinNames() );
}


void LIB_PIN::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS units = aFrame->GetUserUnits();

    LIB_ITEM::GetMsgPanelInfo( aFrame, aList );

    aList.emplace_back( _( "Name" ), UnescapeString( GetShownName() ) );
    aList.emplace_back( _( "Number" ), GetShownNumber() );
    aList.emplace_back( _( "Type" ), ElectricalPinTypeGetText( m_type ) );
    aList.emplace_back( _( "Style" ), PinShapeGetText( m_shape ) );

    aList.emplace_back( _( "Style" ), IsVisible() ? _( "Yes" ) : _( "No" ) );

    // Display pin length
    aList.emplace_back( _( "Length" ), MessageTextFromValue( units, m_length, true ) );

    int i = PinOrientationIndex( m_orientation );
    aList.emplace_back( _( "Orientation" ), PinOrientationName( (unsigned) i ) );

    VECTOR2I pinpos = GetPosition();
    pinpos.y = -pinpos.y;   // Display coords are top to bottom; lib item coords are bottom to top

    aList.emplace_back( _( "Pos X" ), MessageTextFromValue( units, pinpos.x, true ) );
    aList.emplace_back( _( "Pos Y" ), MessageTextFromValue( units, pinpos.y, true ) );
}


void LIB_PIN::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 3;
    aLayers[0] = LAYER_DANGLING;    // We don't really show dangling vs non-dangling (since there
                                    // are no connections in the symbol editor), but it's still
                                    // a good visual indication of which end of the pin is which.
    aLayers[1] = LAYER_DEVICE;
    aLayers[2] = LAYER_SELECTION_SHADOWS;
}


const EDA_RECT LIB_PIN::GetBoundingBox( bool aIncludeInvisibles, bool aPinOnly ) const
{
    KIFONT::FONT* font = KIFONT::FONT::GetFont( Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>()->m_Appearance.default_font );

    EDA_RECT       bbox;
    VECTOR2I       begin;
    VECTOR2I       end;
    int            nameTextOffset = 0;
    int            nameTextLength = 0;
    int            nameTextHeight = 0;
    int            numberTextLength = 0;
    int            numberTextHeight = 0;
    wxString       name = GetShownName();
    wxString       number = GetShownNumber();
    bool           showName = !name.IsEmpty();
    bool           showNum = !number.IsEmpty();
    int            minsizeV = TARGET_PIN_RADIUS;
    int            penWidth = GetPenWidth();

    if( !aIncludeInvisibles && !IsVisible() )
        showName = false;

    if( GetParent() )
    {
        if( GetParent()->ShowPinNames() )
            nameTextOffset = GetParent()->GetPinNameOffset();
        else
            showName = false;

        if( !GetParent()->ShowPinNumbers() )
            showNum = false;
    }

    if( aPinOnly )
    {
        showName = false;
        showNum = false;
    }

    // First, calculate boundary box corners position
    if( showNum )
    {
        VECTOR2D fontSize( m_numTextSize, m_numTextSize );
        VECTOR2I numSize = font->StringBoundaryLimits( number, fontSize, penWidth, false, false );

        numberTextLength = numSize.x;
        numberTextHeight = numSize.y;
    }

    if( m_shape == GRAPHIC_PINSHAPE::INVERTED || m_shape == GRAPHIC_PINSHAPE::INVERTED_CLOCK )
        minsizeV = std::max( TARGET_PIN_RADIUS, externalPinDecoSize( nullptr, *this ) );

    // calculate top left corner position
    // for the default pin orientation (PIN_RIGHT)
    begin.y = std::max( minsizeV, numberTextHeight + Mils2iu( PIN_TEXT_MARGIN ) );
    begin.x = std::min( 0, m_length - ( numberTextLength / 2) );

    // calculate bottom right corner position and adjust top left corner position
    if( showName )
    {
        VECTOR2D fontSize( m_nameTextSize, m_nameTextSize );
        VECTOR2I nameSize = font->StringBoundaryLimits( name, fontSize, penWidth, false, false );

        nameTextLength = nameSize.x + nameTextOffset;
        nameTextHeight = nameSize.y + Mils2iu( PIN_TEXT_MARGIN );
    }

    if( nameTextOffset )        // for values > 0, pin name is inside the body
    {
        end.x = m_length + nameTextLength;
        end.y = std::min( -minsizeV, -nameTextHeight / 2 );
    }
    else        // if value == 0:
                // pin name is outside the body, and above the pin line
                // pin num is below the pin line
    {
        end.x   = std::max( m_length, nameTextLength );
        end.y   = -begin.y;
        begin.y = std::max( minsizeV, nameTextHeight );
    }

    // Now, calculate boundary box corners position for the actual pin orientation
    int orient = PinDrawOrient( DefaultTransform );

    /* Calculate the pin position */
    switch( orient )
    {
    case PIN_UP:
        // Pin is rotated and texts positions are mirrored
        RotatePoint( begin, VECTOR2I( 0, 0 ), -ANGLE_90 );
        RotatePoint( end, VECTOR2I( 0, 0 ), -ANGLE_90 );
        break;

    case PIN_DOWN:
        RotatePoint( begin, VECTOR2I( 0, 0 ), ANGLE_90 );
        RotatePoint( end, VECTOR2I( 0, 0 ), ANGLE_90 );
        begin.x = -begin.x;
        end.x = -end.x;
        break;

    case PIN_LEFT:
        begin.x = -begin.x;
        end.x = -end.x;
        break;

    case PIN_RIGHT:
        break;
    }

    begin += m_position;
    end += m_position;

    bbox.SetOrigin( begin );
    bbox.SetEnd( end );
    bbox.Normalize();
    bbox.Inflate( ( GetPenWidth() / 2 ) + 1 );

    // Draw Y axis is reversed in schematic:
    bbox.RevertYAxis();

    return bbox;
}


BITMAPS LIB_PIN::GetMenuImage() const
{
    return ElectricalPinTypeGetBitmap( m_type );
}


wxString LIB_PIN::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    if( !m_name.IsEmpty() )
    {
        return wxString::Format( _( "Pin %s [%s, %s, %s]" ),
                                 GetShownNumber(),
                                 UnescapeString( GetShownName() ),
                                 GetElectricalTypeName(),
                                 PinShapeGetText( m_shape ) );
    }
    else
    {
        return wxString::Format( _( "Pin %s [%s, %s]" ),
                                 GetShownNumber(),
                                 GetElectricalTypeName(),
                                 PinShapeGetText( m_shape ) );
    }
}


#if defined(DEBUG)

void LIB_PIN::Show( int nestLevel, std::ostream& os ) const
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " num=\"" << m_number.mb_str()
                                 << '"' << "/>\n";

//    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif

void LIB_PIN::CalcEdit( const VECTOR2I& aPosition )
{
    if( IsMoving() )
        MoveTo( aPosition );
}
