/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <array>
#include <eda_draw_frame.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/geometry_utils.h>
#include <origin_viewitem.h>

using namespace KIGFX;

ORIGIN_VIEWITEM::ORIGIN_VIEWITEM( const COLOR4D& aColor, MARKER_STYLE aStyle, int aSize, const VECTOR2D& aPosition ) :
    EDA_ITEM( nullptr, NOT_USED ),   // this item is never added to a BOARD/SCHEMATIC so it needs no type
    m_position( aPosition ),
    m_size( aSize ),
    m_color( aColor ),
    m_style( aStyle ),
    m_drawAtZero( false )
{
}


ORIGIN_VIEWITEM::ORIGIN_VIEWITEM( const VECTOR2D& aPosition, EDA_ITEM_FLAGS flags ) :
    EDA_ITEM( nullptr, NOT_USED ),   // this item is never added to a BOARD/SCHEMATIC so it needs no type
    m_position( aPosition ),
    m_size( NOT_USED ),
    m_color( UNSPECIFIED_COLOR ),
    m_style( NO_GRAPHIC ),
    m_drawAtZero( false )
{
    SetFlags( flags );
}


ORIGIN_VIEWITEM* ORIGIN_VIEWITEM::Clone() const
{
    return new ORIGIN_VIEWITEM( m_color, m_style, m_size, m_position );
}


const BOX2I ORIGIN_VIEWITEM::ViewBBox() const
{
    BOX2I bbox;
    bbox.SetMaximum();
    return bbox;
}

void ORIGIN_VIEWITEM::ViewDraw( int, VIEW* aView ) const
{
    auto gal = aView->GetGAL();

    // Nothing to do if the target shouldn't be drawn at 0,0 and that's where the target is.
    if( !m_drawAtZero && ( m_position.x == 0 ) && ( m_position.y == 0 ) )
        return;

    gal->SetIsStroke( true );
    gal->SetIsFill( false );
    gal->SetLineWidth( 1 );
    gal->SetStrokeColor( m_color );
    VECTOR2D scaledSize = aView->ToWorld( VECTOR2D( m_size, m_size ), false );

    // Draw a circle around the marker's centre point if the style demands it
    if( ( m_style == CIRCLE_CROSS ) || ( m_style == CIRCLE_DOT ) || ( m_style == CIRCLE_X ) )
        gal->DrawCircle( m_position, fabs( scaledSize.x ) );

    switch( m_style )
    {
        case NO_GRAPHIC:
            break;

        case CROSS:
        case CIRCLE_CROSS:
            gal->DrawLine( m_position - VECTOR2D( scaledSize.x, 0 ),
                            m_position + VECTOR2D( scaledSize.x, 0 ) );
            gal->DrawLine( m_position - VECTOR2D( 0, scaledSize.y ),
                            m_position + VECTOR2D( 0, scaledSize.y ) );
            break;

        case DASH_LINE:
        {
            gal->DrawCircle( m_position, scaledSize.x / 4 );

            VECTOR2D start( m_position );
            VECTOR2D end( m_end );
            EDA_RECT clip( wxPoint( start ), wxSize( end.x - start.x, end.y - start.y ) );
            clip.Normalize();

            double               theta = atan2( end.y - start.y, end.x - start.x );
            std::array<double,2> strokes = { scaledSize.x, scaledSize.x / 2 };

            for( size_t i = 0; i < 10000; ++i )
            {
                VECTOR2D next( start.x + strokes[ i % 2 ] * cos( theta ),
                               start.y + strokes[ i % 2 ] * sin( theta ) );

                // Drawing each segment can be done rounded to ints.
                wxPoint segStart( KiROUND( start.x ), KiROUND( start.y ) );
                wxPoint segEnd( KiROUND( next.x ), KiROUND( next.y ) );

                if( ClipLine( &clip, segStart.x, segStart.y, segEnd.x, segEnd.y ) )
                    break;
                else if( i % 2 == 0 )
                    gal->DrawLine( segStart, segEnd );

                start = next;
            }

            gal->DrawCircle( m_end, scaledSize.x / 4 );
            break;
        }

        case X:
        case CIRCLE_X:
            gal->DrawLine( m_position - scaledSize, m_position + scaledSize );
            scaledSize.y = -scaledSize.y;
            gal->DrawLine( m_position - scaledSize, m_position + scaledSize );
            break;

        case DOT:
        case CIRCLE_DOT:
            gal->DrawCircle( m_position, scaledSize.x / 4 );
            break;
    }
}
