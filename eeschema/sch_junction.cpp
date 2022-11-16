/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <sch_draw_panel.h>
#include <trigo.h>
#include <common.h>
#include <plotters/plotter.h>
#include <bitmaps.h>
#include <core/mirror.h>
#include <geometry/shape_rect.h>
#include <sch_painter.h>
#include <sch_junction.h>
#include <sch_connection.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <connection_graph.h>


SCH_JUNCTION::SCH_JUNCTION( const VECTOR2I& aPosition, int aDiameter, SCH_LAYER_ID aLayer ) :
    SCH_ITEM( nullptr, SCH_JUNCTION_T )
{
    m_pos   = aPosition;
    m_color = COLOR4D::UNSPECIFIED;
    m_diameter = aDiameter;
    m_layer = aLayer;

    m_lastResolvedDiameter = KiROUND( schIUScale.MilsToIU( DEFAULT_WIRE_WIDTH_MILS ) * 1.7 );
    m_lastResolvedColor = COLOR4D::UNSPECIFIED;
}


EDA_ITEM* SCH_JUNCTION::Clone() const
{
    return new SCH_JUNCTION( *this );
}


void SCH_JUNCTION::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( ( aItem != nullptr ) && ( aItem->Type() == SCH_JUNCTION_T ),
                 wxT( "Cannot swap junction data with invalid item." ) );

    SCH_JUNCTION* item = (SCH_JUNCTION*) aItem;
    std::swap( m_pos, item->m_pos );
    std::swap( m_diameter, item->m_diameter );
    std::swap( m_color, item->m_color );
}


void SCH_JUNCTION::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 2;
    aLayers[0] = m_layer;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


SHAPE_CIRCLE SCH_JUNCTION::getEffectiveShape() const
{
    if( m_diameter != 0 )
        m_lastResolvedDiameter = m_diameter;
    else if( Schematic() )
        m_lastResolvedDiameter = Schematic()->Settings().m_JunctionSize;
    else
        m_lastResolvedDiameter = schIUScale.MilsToIU( DEFAULT_JUNCTION_DIAM );

    if( m_lastResolvedDiameter != 1 )  // Diameter 1 means user doesn't want to draw junctions
    {
        // If we know what we're connected to, then enforce a minimum size of 170% of the
        // connected wire width:
        if( !IsConnectivityDirty() )
        {
            m_lastResolvedDiameter = std::max<int>( m_lastResolvedDiameter,
                                                    GetEffectiveNetClass()->GetWireWidth() * 1.7 );
        }
    }

    return SHAPE_CIRCLE( m_pos, std::max( m_lastResolvedDiameter / 2, 1 ) );
}


const BOX2I SCH_JUNCTION::GetBoundingBox() const
{
    BOX2I bbox( m_pos );
    bbox.Inflate( getEffectiveShape().GetRadius() );

    return bbox;
}


void SCH_JUNCTION::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    wxDC*   DC    = aSettings->GetPrintDC();
    COLOR4D color = GetJunctionColor();

    if( color == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( GetLayer() );

    SHAPE_CIRCLE circle = getEffectiveShape();

    GRFilledCircle( DC, circle.GetCenter() + aOffset, circle.GetRadius(), 0, color, color );
}


void SCH_JUNCTION::MirrorVertically( int aCenter )
{
    MIRROR( m_pos.y, aCenter );
}


void SCH_JUNCTION::MirrorHorizontally( int aCenter )
{
    MIRROR( m_pos.x, aCenter );
}


void SCH_JUNCTION::Rotate( const VECTOR2I& aCenter )
{
    RotatePoint( m_pos, aCenter, ANGLE_90 );
}


void SCH_JUNCTION::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    DANGLING_END_ITEM item( JUNCTION_END, this, m_pos );
    aItemList.push_back( item );
}


std::vector<VECTOR2I> SCH_JUNCTION::GetConnectionPoints() const
{
    return { m_pos };
}


#if defined(DEBUG)
void SCH_JUNCTION::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << m_pos << ", " << m_diameter
                                 << "/>\n";
}
#endif


void SCH_JUNCTION::SetDiameter( int aDiameter )
{
    m_diameter = aDiameter;
    m_lastResolvedDiameter = aDiameter;
}


COLOR4D SCH_JUNCTION::GetJunctionColor() const
{
    if( m_color != COLOR4D::UNSPECIFIED )
        m_lastResolvedColor = m_color;
    else if( !IsConnectivityDirty() )
        m_lastResolvedColor = GetEffectiveNetClass()->GetSchematicColor();

    return m_lastResolvedColor;
}


void SCH_JUNCTION::SetColor( const COLOR4D& aColor )
{
    m_color = aColor;
    m_lastResolvedColor = aColor;
}


int SCH_JUNCTION::GetEffectiveDiameter() const
{
    return getEffectiveShape().GetRadius() * 2;
}


bool SCH_JUNCTION::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    if( aAccuracy >= 0 )
        return getEffectiveShape().Collide( SEG( aPosition, aPosition ), aAccuracy );
    else
        return aPosition == m_pos;
}


bool SCH_JUNCTION::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & STRUCT_DELETED || m_flags & SKIP_STRUCT )
        return false;

    if( aContained )
    {
        BOX2I selRect( aRect );

        return selRect.Inflate( aAccuracy ).Contains( GetBoundingBox() );
    }
    else
    {
        SHAPE_CIRCLE junction = getEffectiveShape();
        SHAPE_RECT   selRect( aRect.GetPosition(), aRect.GetWidth(), aRect.GetHeight() );

        return selRect.Collide( &junction, aAccuracy );
    }
}


bool SCH_JUNCTION::doIsConnected( const VECTOR2I& aPosition ) const
{
    return m_pos == aPosition;
}


void SCH_JUNCTION::Plot( PLOTTER* aPlotter, bool aBackground ) const
{
    if( aBackground )
        return;

    RENDER_SETTINGS* settings = aPlotter->RenderSettings();
    COLOR4D          color = GetJunctionColor();

    if( color == COLOR4D::UNSPECIFIED )
        color = settings->GetLayerColor( GetLayer() );

    aPlotter->SetColor( color );

    aPlotter->Circle( m_pos, GetEffectiveDiameter(), FILL_T::FILLED_SHAPE );
}


BITMAPS SCH_JUNCTION::GetMenuImage() const
{
    return BITMAPS::add_junction;
}


bool SCH_JUNCTION::operator <( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    if( GetLayer() != aItem.GetLayer() )
        return GetLayer() < aItem.GetLayer();

    const SCH_JUNCTION* junction = static_cast<const SCH_JUNCTION*>( &aItem );

    if( GetPosition().x != junction->GetPosition().x )
        return GetPosition().x < junction->GetPosition().x;

    if( GetPosition().y != junction->GetPosition().y )
        return GetPosition().y < junction->GetPosition().y;

    if( GetDiameter() != junction->GetDiameter() )
        return GetDiameter() < junction->GetDiameter();

    return GetColor() < junction->GetColor();
}

