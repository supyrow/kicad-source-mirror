/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Janito Vaqueiro Ferreira Filho <janito.vff@gmail.com>
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

#include <eda_item.h>
#include "graphics_importer_buffer.h"

using namespace std;

template <typename T, typename... Args>
static std::unique_ptr<T> make_shape( const Args&... aArguments )
{
    return std::make_unique<T>( aArguments... );
}

void GRAPHICS_IMPORTER_BUFFER::AddLine( const VECTOR2D& aStart, const VECTOR2D& aEnd, double aWidth )
{
    m_shapes.push_back( make_shape< IMPORTED_LINE >( aStart, aEnd, aWidth ) );
}


void GRAPHICS_IMPORTER_BUFFER::AddCircle( const VECTOR2D& aCenter, double aRadius, double aWidth, bool aFilled )
{
    m_shapes.push_back( make_shape<IMPORTED_CIRCLE>( aCenter, aRadius, aWidth, aFilled ) );
}


void GRAPHICS_IMPORTER_BUFFER::AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart,
                                       double aAngle, double aWidth )
{
    m_shapes.push_back( make_shape<IMPORTED_ARC>( aCenter, aStart, aAngle, aWidth ) );
}


void GRAPHICS_IMPORTER_BUFFER::AddPolygon( const std::vector< VECTOR2D >& aVertices, double aWidth )
{
    m_shapes.push_back( make_shape<IMPORTED_POLYGON>( aVertices, aWidth ) );
}


void GRAPHICS_IMPORTER_BUFFER::AddText( const VECTOR2D& aOrigin, const wxString& aText,
                                        double aHeight, double aWidth, double aThickness,
                                        double aOrientation, GR_TEXT_H_ALIGN_T aHJustify,
                                        GR_TEXT_V_ALIGN_T aVJustify )
{
    m_shapes.push_back( make_shape< IMPORTED_TEXT >( aOrigin, aText, aHeight, aWidth,
                        aThickness, aOrientation, aHJustify, aVJustify ) );
}


void GRAPHICS_IMPORTER_BUFFER::AddSpline( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd , double aWidth )
{
    m_shapes.push_back( make_shape<IMPORTED_SPLINE>( aStart, aBezierControl1, aBezierControl2, aEnd, aWidth ) );
}


void GRAPHICS_IMPORTER_BUFFER::AddShape( std::unique_ptr<IMPORTED_SHAPE>& aShape )
{
    m_shapes.push_back( std::move( aShape ) );
}


void GRAPHICS_IMPORTER_BUFFER::ImportTo( GRAPHICS_IMPORTER& aImporter )
{
    for( std::unique_ptr<IMPORTED_SHAPE>& shape : m_shapes )
        shape->ImportTo( aImporter );
}
