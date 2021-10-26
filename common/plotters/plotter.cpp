/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file plotter.cpp
 * @brief KiCad: Base of all the specialized plotters
 * the class PLOTTER handle basic functions to plot schematic and boards
 * with different plot formats.
 *
 * There are currently engines for:
 * HPGL
 * POSTSCRIPT
 * GERBER
 * DXF
 * an SVG 'plot' is also provided along with the 'print' function by wx, but
 * is not handled here.
 */

#include <trigo.h>
#include <eda_item.h>
#include <plotters/plotter.h>
#include <geometry/shape_line_chain.h>
#include <geometry/geometry_utils.h>
#include <bezier_curves.h>
#include <math/util.h>      // for KiROUND


PLOTTER::PLOTTER( )
{
    m_plotScale = 1;
    m_currentPenWidth = -1;       // To-be-set marker
    m_penState = 'Z';             // End-of-path idle
    m_plotMirror = false;       // Plot mirror option flag
    m_mirrorIsHorizontal = true;
    m_yaxisReversed = false;
    m_outputFile = nullptr;
    m_colorMode = false;          // Starts as a BW plot
    m_negativeMode = false;

    // Temporary init to avoid not initialized vars, will be set later
    m_IUsPerDecimil = 1;        // will be set later to the actual value
    m_iuPerDeviceUnit = 1;        // will be set later to the actual value
    m_renderSettings = nullptr;
}


PLOTTER::~PLOTTER()
{
    // Emergency cleanup, but closing the file is usually made in EndPlot().
    if( m_outputFile )
        fclose( m_outputFile );
}


bool PLOTTER::OpenFile( const wxString& aFullFilename )
{
    m_filename = aFullFilename;

    wxASSERT( !m_outputFile );

    // Open the file in text mode (not suitable for all plotters but only for most of them.
    m_outputFile = wxFopen( m_filename, wxT( "wt" ) );

    if( m_outputFile == nullptr )
        return false ;

    return true;
}


DPOINT PLOTTER::userToDeviceCoordinates( const wxPoint& aCoordinate )
{
    wxPoint pos = aCoordinate - m_plotOffset;

    // Don't allow overflows; they can cause rendering failures in some file viewers
    // (such as Acrobat)
    int clampSize = MAX_PAGE_SIZE_MILS * m_IUsPerDecimil * 10 / 2;
    pos.x = std::max( -clampSize, std::min( pos.x, clampSize ) );
    pos.y = std::max( -clampSize, std::min( pos.y, clampSize ) );

    double x = pos.x * m_plotScale;
    double y = ( m_paperSize.y - pos.y * m_plotScale );

    if( m_plotMirror )
    {
        if( m_mirrorIsHorizontal )
            x = ( m_paperSize.x - pos.x * m_plotScale );
        else
            y = pos.y * m_plotScale;
    }

    if( m_yaxisReversed )
        y = m_paperSize.y - y;

    x *= m_iuPerDeviceUnit;
    y *= m_iuPerDeviceUnit;

    return DPOINT( x, y );
}


DPOINT PLOTTER::userToDeviceSize( const wxSize& size )
{
    return DPOINT( size.x * m_plotScale * m_iuPerDeviceUnit,
                   size.y * m_plotScale * m_iuPerDeviceUnit );
}


double PLOTTER::userToDeviceSize( double size ) const
{
    return size * m_plotScale * m_iuPerDeviceUnit;
}


#define IU_PER_MILS ( m_IUsPerDecimil * 10 )


double PLOTTER::GetDotMarkLenIU() const
{
    return userToDeviceSize( dot_mark_len( GetCurrentLineWidth() ) );
}


double PLOTTER::GetDashMarkLenIU() const
{
    return userToDeviceSize( dash_mark_len( GetCurrentLineWidth() ) );
}


double PLOTTER::GetDashGapLenIU() const
{
    return userToDeviceSize( dash_gap_len( GetCurrentLineWidth() ) );
}


void PLOTTER::Arc( const SHAPE_ARC& aArc )
{
    Arc( wxPoint( aArc.GetCenter() ), aArc.GetStartAngle(), aArc.GetEndAngle(), aArc.GetRadius(),
         FILL_T::NO_FILL, aArc.GetWidth() );
}


void PLOTTER::Arc( const wxPoint& centre, double StAngle, double EndAngle, int radius,
                   FILL_T fill, int width )
{
    wxPoint   start, end;
    const int delta = 50;   // increment (in 0.1 degrees) to draw circles

    if( StAngle > EndAngle )
        std::swap( StAngle, EndAngle );

    SetCurrentLineWidth( width );

    /* Please NOTE the different sign due to Y-axis flip */
    start.x = centre.x + KiROUND( cosdecideg( radius, -StAngle ) );
    start.y = centre.y + KiROUND( sindecideg( radius, -StAngle ) );

    if( fill != FILL_T::NO_FILL )
    {
        MoveTo( centre );
        LineTo( start );
    }
    else
    {
        MoveTo( start );
    }

    for( int ii = StAngle + delta; ii < EndAngle; ii += delta )
    {
        end.x = centre.x + KiROUND( cosdecideg( radius, -ii ) );
        end.y = centre.y + KiROUND( sindecideg( radius, -ii ) );
        LineTo( end );
    }

    end.x = centre.x + KiROUND( cosdecideg( radius, -EndAngle ) );
    end.y = centre.y + KiROUND( sindecideg( radius, -EndAngle ) );

    if( fill != FILL_T::NO_FILL )
    {
        LineTo( end );
        FinishTo( centre );
    }
    else
    {
        FinishTo( end );
    }
}


void PLOTTER::BezierCurve( const wxPoint& aStart, const wxPoint& aControl1,
                           const wxPoint& aControl2, const wxPoint& aEnd,
                           int aTolerance, int aLineThickness )
{
    // Generic fallback: Quadratic Bezier curve plotted as a polyline
    int minSegLen = aLineThickness;  // The segment min length to approximate a bezier curve

    std::vector<wxPoint> ctrlPoints;
    ctrlPoints.push_back( aStart );
    ctrlPoints.push_back( aControl1 );
    ctrlPoints.push_back( aControl2 );
    ctrlPoints.push_back( aEnd );

    BEZIER_POLY bezier_converter( ctrlPoints );

    std::vector<wxPoint> approxPoints;
    bezier_converter.GetPoly( approxPoints, minSegLen );

    SetCurrentLineWidth( aLineThickness );
    MoveTo( aStart );

    for( unsigned ii = 1; ii < approxPoints.size()-1; ii++ )
        LineTo( approxPoints[ii] );

    FinishTo( aEnd );
}


void PLOTTER::PlotImage(const wxImage& aImage, const wxPoint& aPos, double aScaleFactor )
{
    wxSize size( aImage.GetWidth() * aScaleFactor, aImage.GetHeight() * aScaleFactor );

    wxPoint start = aPos;
    start.x -= size.x / 2;
    start.y -= size.y / 2;

    wxPoint end = start;
    end.x += size.x;
    end.y += size.y;

    Rect( start, end, FILL_T::NO_FILL );
}


void PLOTTER::markerSquare( const wxPoint& position, int radius )
{
    double r = KiROUND( radius / 1.4142 );
    std::vector< wxPoint > corner_list;
    wxPoint corner;
    corner.x = position.x + r;
    corner.y = position.y + r;
    corner_list.push_back( corner );
    corner.x = position.x + r;
    corner.y = position.y - r;
    corner_list.push_back( corner );
    corner.x = position.x - r;
    corner.y = position.y - r;
    corner_list.push_back( corner );
    corner.x = position.x - r;
    corner.y = position.y + r;
    corner_list.push_back( corner );
    corner.x = position.x + r;
    corner.y = position.y + r;
    corner_list.push_back( corner );

    PlotPoly( corner_list, FILL_T::NO_FILL, GetCurrentLineWidth() );
}


void PLOTTER::markerCircle( const wxPoint& position, int radius )
{
    Circle( position, radius * 2, FILL_T::NO_FILL, GetCurrentLineWidth() );
}


void PLOTTER::markerLozenge( const wxPoint& position, int radius )
{
    std::vector< wxPoint > corner_list;
    wxPoint corner;
    corner.x = position.x;
    corner.y = position.y + radius;
    corner_list.push_back( corner );
    corner.x = position.x + radius;
    corner.y = position.y,
    corner_list.push_back( corner );
    corner.x = position.x;
    corner.y = position.y - radius;
    corner_list.push_back( corner );
    corner.x = position.x - radius;
    corner.y = position.y;
    corner_list.push_back( corner );
    corner.x = position.x;
    corner.y = position.y + radius;
    corner_list.push_back( corner );

    PlotPoly( corner_list, FILL_T::NO_FILL, GetCurrentLineWidth() );
}


void PLOTTER::markerHBar( const wxPoint& pos, int radius )
{
    MoveTo( wxPoint( pos.x - radius, pos.y ) );
    FinishTo( wxPoint( pos.x + radius, pos.y ) );
}


void PLOTTER::markerSlash( const wxPoint& pos, int radius )
{
    MoveTo( wxPoint( pos.x - radius, pos.y - radius ) );
    FinishTo( wxPoint( pos.x + radius, pos.y + radius ) );
}


void PLOTTER::markerBackSlash( const wxPoint& pos, int radius )
{
    MoveTo( wxPoint( pos.x + radius, pos.y - radius ) );
    FinishTo( wxPoint( pos.x - radius, pos.y + radius ) );
}


void PLOTTER::markerVBar( const wxPoint& pos, int radius )
{
    MoveTo( wxPoint( pos.x, pos.y - radius ) );
    FinishTo( wxPoint( pos.x, pos.y + radius ) );
}


void PLOTTER::Marker( const wxPoint& position, int diametre, unsigned aShapeId )
{
    int radius = diametre / 2;

    /* Marker are composed by a series of 'parts' superimposed; not every
       combination make sense, obviously. Since they are used in order I
       tried to keep the uglier/more complex constructions at the end.
       Also I avoided the |/ |\ -/ -\ construction because they're *very*
       ugly... if needed they could be added anyway... I'd like to see
       a board with more than 58 drilling/slotting tools!
       If Visual C++ supported the 0b literals they would be optimally
       and easily encoded as an integer array. We have to do with octal */
    static const unsigned char marker_patterns[MARKER_COUNT] = {

        // Bit order:  O Square Lozenge - | \ /
        // First choice: simple shapes
        0003,  // X
        0100,  // O
        0014,  // +
        0040,  // Sq
        0020,  // Lz

        // Two simple shapes
        0103,  // X O
        0017,  // X +
        0043,  // X Sq
        0023,  // X Lz
        0114,  // O +
        0140,  // O Sq
        0120,  // O Lz
        0054,  // + Sq
        0034,  // + Lz
        0060,  // Sq Lz

        // Three simple shapes
        0117,  // X O +
        0143,  // X O Sq
        0123,  // X O Lz
        0057,  // X + Sq
        0037,  // X + Lz
        0063,  // X Sq Lz
        0154,  // O + Sq
        0134,  // O + Lz
        0074,  // + Sq Lz

        // Four simple shapes
        0174,  // O Sq Lz +
        0163,  // X O Sq Lz
        0157,  // X O Sq +
        0137,  // X O Lz +
        0077,  // X Sq Lz +

        // This draws *everything *
        0177,  // X O Sq Lz +

        // Here we use the single bars... so the cross is forbidden
        0110,  // O -
        0104,  // O |
        0101,  // O /
        0050,  // Sq -
        0044,  // Sq |
        0041,  // Sq /
        0030,  // Lz -
        0024,  // Lz |
        0021,  // Lz /
        0150,  // O Sq -
        0144,  // O Sq |
        0141,  // O Sq /
        0130,  // O Lz -
        0124,  // O Lz |
        0121,  // O Lz /
        0070,  // Sq Lz -
        0064,  // Sq Lz |
        0061,  // Sq Lz /
        0170,  // O Sq Lz -
        0164,  // O Sq Lz |
        0161,  // O Sq Lz /

        // Last resort: the backlash component (easy to confound)
        0102,  // \ O
        0042,  // \ Sq
        0022,  // \ Lz
        0142,  // \ O Sq
        0122,  // \ O Lz
        0062,  // \ Sq Lz
        0162   // \ O Sq Lz
    };

    if( aShapeId >= MARKER_COUNT )
    {
        // Fallback shape
        markerCircle( position, radius );
    }
    else
    {
        // Decode the pattern and draw the corresponding parts
        unsigned char pat = marker_patterns[aShapeId];

        if( pat & 0001 )
            markerSlash( position, radius );

        if( pat & 0002 )
            markerBackSlash( position, radius );

        if( pat & 0004 )
            markerVBar( position, radius );

        if( pat & 0010 )
            markerHBar( position, radius );

        if( pat & 0020 )
            markerLozenge( position, radius );

        if( pat & 0040 )
            markerSquare( position, radius );

        if( pat & 0100 )
            markerCircle( position, radius );
    }
}


void PLOTTER::segmentAsOval( const wxPoint& start, const wxPoint& end, int width,
                             OUTLINE_MODE tracemode )
{
    wxPoint center( (start.x + end.x) / 2, (start.y + end.y) / 2 );
    wxSize  size( end.x - start.x, end.y - start.y );
    double  orient;

    if( size.y == 0 )
        orient = 0;
    else if( size.x == 0 )
        orient = 900;
    else
        orient = -ArcTangente( size.y, size.x );

    size.x = KiROUND( EuclideanNorm( size ) ) + width;
    size.y = width;

    FlashPadOval( center, size, orient, tracemode, nullptr );
}


void PLOTTER::sketchOval( const wxPoint& pos, const wxSize& aSize, double orient, int width )
{
    SetCurrentLineWidth( width );
    width = m_currentPenWidth;
    int radius, deltaxy, cx, cy;
    wxSize size( aSize );

    if( size.x > size.y )
    {
        std::swap( size.x, size.y );
        orient = AddAngles( orient, 900 );
    }

    deltaxy = size.y - size.x;       /* distance between centers of the oval */
    radius   = ( size.x - width ) / 2;
    cx = -radius;
    cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    MoveTo( wxPoint( cx + pos.x, cy + pos.y ) );
    cx = -radius;
    cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    FinishTo( wxPoint( cx + pos.x, cy + pos.y ) );

    cx = radius;
    cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    MoveTo( wxPoint( cx + pos.x, cy + pos.y ) );
    cx = radius;
    cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    FinishTo( wxPoint( cx + pos.x, cy + pos.y ) );

    cx = 0;
    cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    Arc( wxPoint( cx + pos.x, cy + pos.y ), orient + 1800, orient + 3600, radius, FILL_T::NO_FILL );
    cx = 0;
    cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    Arc( wxPoint( cx + pos.x, cy + pos.y ), orient, orient + 1800, radius, FILL_T::NO_FILL );
}


void PLOTTER::ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                            OUTLINE_MODE tracemode, void* aData )
{
    if( tracemode == FILLED )
    {
        if( start == end )
        {
            Circle( start, width, FILL_T::FILLED_SHAPE, 0 );
        }
        else
        {
            SetCurrentLineWidth( width );
            MoveTo( start );
            FinishTo( end );
        }
    }
    else
    {
        SetCurrentLineWidth( -1 );
        segmentAsOval( start, end, width, tracemode );
    }
}


void PLOTTER::ThickArc( const wxPoint& centre, double StAngle, double EndAngle,
                        int radius, int width, OUTLINE_MODE tracemode, void* aData )
{
    if( tracemode == FILLED )
    {
        Arc( centre, StAngle, EndAngle, radius, FILL_T::NO_FILL, width );
    }
    else
    {
        SetCurrentLineWidth( -1 );
        Arc( centre, StAngle, EndAngle, radius - ( width - m_currentPenWidth ) / 2,
             FILL_T::NO_FILL, -1 );
        Arc( centre, StAngle, EndAngle, radius + ( width - m_currentPenWidth ) / 2,
             FILL_T::NO_FILL, -1 );
    }
}


void PLOTTER::ThickRect( const wxPoint& p1, const wxPoint& p2, int width,
                         OUTLINE_MODE tracemode, void* aData )
{
    if( tracemode == FILLED )
    {
        Rect( p1, p2, FILL_T::NO_FILL, width );
    }
    else
    {
        SetCurrentLineWidth( -1 );
        wxPoint offsetp1( p1.x - (width - m_currentPenWidth) / 2,
                          p1.y - (width - m_currentPenWidth) / 2 );
        wxPoint offsetp2( p2.x + (width - m_currentPenWidth) / 2,
                          p2.y + (width - m_currentPenWidth) / 2 );
        Rect( offsetp1, offsetp2, FILL_T::NO_FILL, -1 );
        offsetp1.x += ( width - m_currentPenWidth );
        offsetp1.y += ( width - m_currentPenWidth );
        offsetp2.x -= ( width - m_currentPenWidth );
        offsetp2.y -= ( width - m_currentPenWidth );
        Rect( offsetp1, offsetp2, FILL_T::NO_FILL, -1 );
    }
}


void PLOTTER::ThickCircle( const wxPoint& pos, int diametre, int width, OUTLINE_MODE tracemode,
                           void* aData )
{
    if( tracemode == FILLED )
    {
        Circle( pos, diametre, FILL_T::NO_FILL, width );
    }
    else
    {
        SetCurrentLineWidth( -1 );
        Circle( pos, diametre - width + m_currentPenWidth, FILL_T::NO_FILL, -1 );
        Circle( pos, diametre + width - m_currentPenWidth, FILL_T::NO_FILL, -1 );
    }
}


void PLOTTER::FilledCircle( const wxPoint& pos, int diametre, OUTLINE_MODE tracemode, void* aData )
{
    if( tracemode == FILLED )
    {
        Circle( pos, diametre, FILL_T::FILLED_SHAPE, 0 );
    }
    else
    {
        SetCurrentLineWidth( -1 );
        Circle( pos, diametre, FILL_T::NO_FILL, -1 );
    }
}


void PLOTTER::PlotPoly( const SHAPE_LINE_CHAIN& aCornerList, FILL_T aFill, int aWidth, void* aData )
{
    std::vector<wxPoint> cornerList;
    cornerList.reserve( aCornerList.PointCount() );

    for( int ii = 0; ii < aCornerList.PointCount(); ii++ )
        cornerList.emplace_back( aCornerList.CPoint( ii ) );

    if( aCornerList.IsClosed() && cornerList.front() != cornerList.back() )
        cornerList.emplace_back( aCornerList.CPoint( 0 ) );

    PlotPoly( cornerList, aFill, aWidth, aData );
}
