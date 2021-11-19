/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gr_basic.h>
#include <trigo.h>
#include <eda_item.h>
#include <math_for_graphics.h>
#include <wx/graphics.h>
#include <geometry/geometry_utils.h>
#include <math/util.h>      // for KiROUND

#include <algorithm>

static const bool FILLED = true;
static const bool NOT_FILLED = false;

/* Important Note:
 * These drawing functions  clip draw item before send these items to wxDC draw
 * functions.  For guy who asks why i did it, see a sample of problems encountered
 * when pixels
 * coordinates overflow 16 bits values:
 * http://trac.wxwidgets.org/ticket/10446
 * Problems can be found under Windows **and** Linux (mainly when drawing arcs)
 * (mainly at low zoom values (2, 1 or 0.5), in Pcbnew)
 * some of these problems could be now fixed in recent distributions.
 *
 * Currently (feb 2009) there are overflow problems when drawing solid (filled)
 * polygons under linux without clipping
 *
 * So before removing clipping functions, be aware these bug (they are not in
 * KiCad or wxWidgets) are fixed by testing how are drawn complex lines arcs
 * and solid polygons under Windows and Linux and remember users can have old
 * versions with bugs
 */


/* Definitions for enabling and disabling debugging features in gr_basic.cpp.
 * Please remember to set these back to 0 before making LAUNCHPAD commits.
 */
#define DEBUG_DUMP_CLIP_ERROR_COORDS 0  // Set to 1 to dump clip algorithm errors.
#define DEBUG_DUMP_CLIP_COORDS       0  // Set to 1 to dump clipped coordinates.


// For draw mode = XOR GR_XOR or GR_NXOR by background color
GR_DRAWMODE g_XorMode = GR_NXOR;


static void ClipAndDrawPoly( EDA_RECT* ClipBox, wxDC* DC, const wxPoint* Points, int n );

/* These functions are used by corresponding functions
 * ( GRSCircle is called by GRCircle for instance) after mapping coordinates
 * from user units to screen units(pixels coordinates)
 */
static void GRSRect( EDA_RECT* aClipBox, wxDC* aDC, int x1, int y1,
                     int x2, int y2, int aWidth, const COLOR4D& aColor );

/**/

static int     GRLastMoveToX, GRLastMoveToY;
static bool    s_ForceBlackPen;   /* if true: draws in black instead of
                                        * color for printing. */
static int     xcliplo = 0,
               ycliplo = 0,
               xcliphi = 2000,
               ycliphi = 2000;

static COLOR4D s_DC_lastbrushcolor( 0, 0, 0, 0 );
static bool    s_DC_lastbrushfill  = false;
static wxDC*   s_DC_lastDC = nullptr;

static void WinClipAndDrawLine( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                                int width )
{
    GRLastMoveToX = x2;
    GRLastMoveToY = y2;

    if( ClipBox )
    {
        EDA_RECT clipbox(*ClipBox);
        clipbox.Inflate(width/2);
        if( ClipLine( &clipbox, x1, y1, x2, y2 ) )
            return;
    }

    DC->DrawLine( x1, y1, x2, y2 );
}


void GRResetPenAndBrush( wxDC* DC )
{
    GRSetBrush( DC, BLACK );  // Force no fill
    s_DC_lastbrushcolor = COLOR4D::UNSPECIFIED;
    s_DC_lastDC    = nullptr;
}


void GRSetColorPen( wxDC* DC, const COLOR4D& Color, int width, wxPenStyle style )
{
    COLOR4D color = Color;

    wxDash dots[2] = { 1, 3 };

    // Under OSX and while printing when wxPen is set to 0, renderer follows the request drawing
    // nothing & in the bitmap world the minimum is enough to light a pixel, in vectorial one not
    if( width <= 1 )
        width = DC->DeviceToLogicalXRel( 1 );

    if( s_ForceBlackPen )
        color = COLOR4D::BLACK;

    const wxPen& curr_pen = DC->GetPen();

    if( !curr_pen.IsOk() || curr_pen.GetColour() != color.ToColour()
       || curr_pen.GetWidth() != width || curr_pen.GetStyle() != style )
    {
        wxPen pen;
        pen.SetColour( color.ToColour() );

        if( style == wxPENSTYLE_DOT )
        {
            style = wxPENSTYLE_USER_DASH;
            pen.SetDashes( 2, dots );
        }

        pen.SetWidth( width );
        pen.SetStyle( style );
        DC->SetPen( pen );
    }
    else
    {
        // Should be not needed, but on Linux, in printing process
        // the curr pen settings needs to be sometimes re-initialized
        // Clearly, this is due to a bug, related to SetBrush(),
        // but we have to live with it, at least on wxWidgets 3.0
        DC->SetPen( curr_pen );
    }
}


void GRSetBrush( wxDC* DC, const COLOR4D& Color, bool fill )
{
    COLOR4D color = Color;

    if( s_ForceBlackPen )
        color = COLOR4D::BLACK;

    if( s_DC_lastbrushcolor != color || s_DC_lastbrushfill  != fill || s_DC_lastDC != DC )
    {
        wxBrush brush;

        brush.SetColour( color.ToColour() );

        if( fill )
            brush.SetStyle( wxBRUSHSTYLE_SOLID );
        else
            brush.SetStyle( wxBRUSHSTYLE_TRANSPARENT );

        DC->SetBrush( brush );

        s_DC_lastbrushcolor = color;
        s_DC_lastbrushfill  = fill;
        s_DC_lastDC = DC;
    }
}


void GRForceBlackPen( bool flagforce )
{
    s_ForceBlackPen = flagforce;
}


bool GetGRForceBlackPenState( void )
{
    return s_ForceBlackPen;
}


void GRLine( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, int width,
             const COLOR4D& Color, wxPenStyle aStyle)
{
    GRSetColorPen( DC, Color, width, aStyle );
    WinClipAndDrawLine( ClipBox, DC, x1, y1, x2, y2, width );
    GRLastMoveToX = x2;
    GRLastMoveToY = y2;
}


void GRLine( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aStart, const wxPoint& aEnd, int aWidth,
             const COLOR4D& aColor, wxPenStyle aStyle )
{
    GRLine( aClipBox, aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aWidth, aColor, aStyle );
}


void GRMoveTo( int x, int y )
{
    GRLastMoveToX = x;
    GRLastMoveToY = y;
}


void GRLineTo( EDA_RECT* ClipBox, wxDC* DC, int x, int y, int width, const COLOR4D& Color )
{
    GRLine( ClipBox, DC, GRLastMoveToX, GRLastMoveToY, x, y, width, Color );
}


void GRCSegm( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, int width,
              int aPenSize, const COLOR4D& Color )
{
    GRLastMoveToX = x2;
    GRLastMoveToY = y2;

    if( ClipBox )
    {
        EDA_RECT clipbox( *ClipBox );
        clipbox.Inflate( width / 2 );

        if( ClipLine( &clipbox, x1, y1, x2, y2 ) )
            return;
    }

    if( width <= 2 )   /*  single line or 2 pixels */
    {
        GRSetColorPen( DC, Color, width );
        DC->DrawLine( x1, y1, x2, y2 );
        return;
    }

    GRSetBrush( DC, Color, NOT_FILLED );
    GRSetColorPen( DC, Color, aPenSize );

    int radius = ( width + 1 ) >> 1;
    int dx = x2 - x1;
    int dy = y2 - y1;
    double angle = -ArcTangente( dy, dx );
    wxPoint start;
    wxPoint end;
    wxPoint org( x1, y1 );
    int len = (int) hypot( dx, dy );

    // We know if the DC is mirrored, to draw arcs
    int slx = DC->DeviceToLogicalX( 1 ) - DC->DeviceToLogicalX( 0 );
    int sly = DC->DeviceToLogicalY( 1 ) - DC->DeviceToLogicalY( 0 );
    bool mirrored = ( slx > 0 && sly < 0 ) || ( slx < 0 && sly > 0 );

    // first edge
    start.x = 0;
    start.y = radius;
    end.x = len;
    end.y = radius;
    RotatePoint( &start, angle );
    RotatePoint( &end, angle );

    start += org;
    end += org;

    DC->DrawLine( start, end );

    // first rounded end
    end.x = 0;
    end.y = -radius;
    RotatePoint( &end, angle );
    end += org;

    if( !mirrored )
        DC->DrawArc( end, start, org );
    else
        DC->DrawArc( start, end, org );

    // second edge
    start.x = len;
    start.y = -radius;
    RotatePoint( &start, angle );
    start += org;

    DC->DrawLine( start, end );

    // second rounded end
    end.x = len;
    end.y = radius;
    RotatePoint( &end, angle);
    end += org;

    if( !mirrored )
        DC->DrawArc( end.x, end.y, start.x, start.y, x2, y2 );
    else
        DC->DrawArc( start.x, start.y, end.x, end.y, x2, y2 );
}


void GRCSegm( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, int width,
              const COLOR4D& Color )
{
    GRCSegm( ClipBox, DC, x1, y1, x2, y2, width, 0, Color );
}


void GRCSegm( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aStart, const wxPoint& aEnd, int aWidth,
              const COLOR4D& aColor )
{
    GRCSegm( aClipBox, aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aWidth, 0, aColor );
}


void GRFillCSegm( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, int width,
                  const COLOR4D& Color )
{
    GRSetColorPen( DC, Color, width );
    WinClipAndDrawLine( ClipBox, DC, x1, y1, x2, y2, width );
}


void GRFilledSegment( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aStart, const wxPoint& aEnd,
                      int aWidth, const COLOR4D& aColor )
{
    GRSetColorPen( aDC, aColor, aWidth );
    WinClipAndDrawLine( aClipBox, aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aWidth );
}

static bool IsGRSPolyDrawable( EDA_RECT* ClipBox, int n, const wxPoint* Points )
{
    if( !ClipBox )
        return true;

    if( n <= 0 )
        return false;

    int Xmin, Xmax, Ymin, Ymax;

    Xmin = Xmax = Points[0].x;
    Ymin = Ymax = Points[0].y;

    for( int ii = 1; ii < n; ii++ )     // calculate rectangle
    {
        Xmin = std::min( Xmin, Points[ii].x );
        Xmax = std::max( Xmax, Points[ii].x );
        Ymin = std::min( Ymin, Points[ii].y );
        Ymax = std::max( Ymax, Points[ii].y );
    }

    xcliplo = ClipBox->GetX();
    ycliplo = ClipBox->GetY();
    xcliphi = ClipBox->GetRight();
    ycliphi = ClipBox->GetBottom();

    if( Xmax < xcliplo )
        return false;
    if( Xmin > xcliphi )
        return false;
    if( Ymax < ycliplo )
        return false;
    if( Ymin > ycliphi )
        return false;

    return true;
}


/**
 * Draw a new polyline and fill it if Fill, in screen space.
 */
static void GRSPoly( EDA_RECT* ClipBox, wxDC* DC, int n, const wxPoint* Points, bool Fill,
                     int width, const COLOR4D& Color, const COLOR4D& BgColor )
{
    if( !IsGRSPolyDrawable( ClipBox, n, Points ) )
        return;

    if( Fill && ( n > 2 ) )
    {
        GRSetBrush( DC, BgColor, FILLED );
        GRSetColorPen( DC, Color, width );

        /* clip before send the filled polygon to wxDC, because under linux
         * (GTK?) polygons having large coordinates are incorrectly drawn
         * (integer overflow in coordinates, I am guessing)
         */
        ClipAndDrawPoly( ClipBox, DC, Points, n );
    }
    else
    {

        GRMoveTo( Points[0].x, Points[0].y );

        for( int i = 1; i < n; ++i )
        {
            GRLineTo( ClipBox, DC, Points[i].x, Points[i].y, width, Color );
        }
    }
}


/**
 * Draw a new closed polyline and fill it if Fill, in screen space.
 */
static void GRSClosedPoly( EDA_RECT* aClipBox, wxDC* aDC, int aPointCount, const wxPoint* aPoints,
                           bool aFill, int aWidth, const COLOR4D& aColor, const COLOR4D& aBgColor )
{
    if( !IsGRSPolyDrawable( aClipBox, aPointCount, aPoints ) )
        return;

    if( aFill && ( aPointCount > 2 ) )
    {
        GRLastMoveToX = aPoints[aPointCount - 1].x;
        GRLastMoveToY = aPoints[aPointCount - 1].y;
        GRSetBrush( aDC, aBgColor, FILLED );
        GRSetColorPen( aDC, aColor, aWidth );
        ClipAndDrawPoly( aClipBox, aDC, aPoints, aPointCount );
    }
    else
    {

        GRMoveTo( aPoints[0].x, aPoints[0].y );

        for( int i = 1; i < aPointCount; ++i )
        {
            GRLineTo( aClipBox, aDC, aPoints[i].x, aPoints[i].y, aWidth, aColor );
        }

        int lastpt = aPointCount - 1;

        // Close the polygon
        if( aPoints[lastpt] != aPoints[0] )
        {
            GRLineTo( aClipBox, aDC, aPoints[0].x, aPoints[0].y, aWidth, aColor );
        }
    }
}


/**
 * Draw a new polyline and fill it if Fill, in drawing space.
 */
void GRPoly( EDA_RECT* ClipBox, wxDC* DC, int n, const wxPoint* Points, bool Fill, int width,
             const COLOR4D& Color, const COLOR4D& BgColor )
{
    GRSPoly( ClipBox, DC, n, Points, Fill, width, Color, BgColor );
}


/**
 * Draw a closed polyline and fill it if Fill, in object space.
 */
void GRClosedPoly( EDA_RECT* ClipBox, wxDC* DC, int n, const wxPoint* Points, bool Fill,
                   const COLOR4D& Color, const COLOR4D& BgColor )
{
    GRClosedPoly( ClipBox, DC, n, Points, Fill, 0, Color, BgColor );
}


void GRClosedPoly( EDA_RECT* ClipBox, wxDC* DC, int n, const wxPoint* Points, bool Fill, int width,
                   const COLOR4D& Color, const COLOR4D& BgColor )
{
    GRSClosedPoly( ClipBox, DC, n, Points, Fill, width, Color, BgColor );
}


static bool clipCircle( EDA_RECT* aClipBox, int xc, int yc, int r, int aWidth )
{
    // Clip circles that are outside the ClipBox.
    if( aClipBox )
    {
        int x0, y0, xm, ym;
        x0 = aClipBox->GetX();
        y0 = aClipBox->GetY();
        xm = aClipBox->GetRight();
        ym = aClipBox->GetBottom();

        r += aWidth;

        if( xc < ( x0 - r ) )
            return true;

        if( yc < ( y0 - r ) )
            return true;

        if( xc > ( r + xm ) )
            return true;

        if( yc > ( r + ym ) )
            return true;
    }

    return false;
}


void GRCircle( EDA_RECT* ClipBox, wxDC* DC, int xc, int yc, int r, int width, const COLOR4D& Color )
{
    if( clipCircle( ClipBox, xc, yc, r, width ) || r <= 0 )
        return;

    GRSetBrush( DC, Color, NOT_FILLED );
    GRSetColorPen( DC, Color, width );
    DC->DrawEllipse( xc - r, yc - r, r + r, r + r );
}


void GRCircle( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aPos, int aRadius, int aWidth,
               const COLOR4D& aColor )
{
    GRCircle( aClipBox, aDC, aPos.x, aPos.y, aRadius, aWidth, aColor );
}


void GRFilledCircle( EDA_RECT* ClipBox, wxDC* DC, int x, int y, int r, int width,
                     const COLOR4D& Color, const COLOR4D& BgColor )
{
    if( clipCircle( ClipBox, x, y, r, width ) || r <= 0 )
        return;

    GRSetBrush( DC, BgColor, FILLED );
    GRSetColorPen( DC, Color, width );
    DC->DrawEllipse( x - r, y - r, r + r, r + r );
}


void GRFilledCircle( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aPos, int aRadius,
                     const COLOR4D& aColor )
{
    GRFilledCircle( aClipBox, aDC, aPos.x, aPos.y, aRadius, 0, aColor, aColor );
}


void GRArc1( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, int xc, int yc,
             int width, const COLOR4D& Color )
{
    /* Clip arcs off screen. */
    if( ClipBox )
    {
        int x0, y0, xm, ym, r;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();
        r  = KiROUND( Distance( x1, y1, xc, yc ) );

        if( xc < ( x0 - r ) )
            return;

        if( yc < ( y0 - r ) )
            return;

        if( xc > ( r + xm ) )
            return;

        if( yc > ( r + ym ) )
            return;
    }

    GRSetBrush( DC, Color );
    GRSetColorPen( DC, Color, width );
    DC->DrawArc( x1, y1, x2, y2, xc, yc );
}


void GRArc1( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint& aStart, const wxPoint& aEnd,
             const wxPoint& aCenter, int aWidth, const COLOR4D& aColor )
{
    GRArc1( aClipBox, aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aCenter.x, aCenter.y,
            aWidth, aColor );
}


void GRFilledArc1( EDA_RECT* ClipBox, wxDC* DC, const wxPoint& aStart, const wxPoint& aEnd,
                   const wxPoint& aCenter, int width, const COLOR4D& Color, const COLOR4D& BgColor )
{
    /* Clip arcs off screen. */
    if( ClipBox )
    {
        int x0, y0, xm, ym, r;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();
        r  = KiROUND( Distance( aStart.x, aStart.y, aCenter.x, aCenter.y ) );

        if( aCenter.x < ( x0 - r ) )
            return;

        if( aCenter.y < ( y0 - r ) )
            return;

        if( aCenter.x > ( r + xm ) )
            return;

        if( aCenter.y > ( r + ym ) )
            return;
    }

    GRSetBrush( DC, BgColor, FILLED );
    GRSetColorPen( DC, Color, width );
    DC->DrawArc( aStart.x, aStart.y, aEnd.x, aEnd.y, aCenter.x, aCenter.y );
}


void GRArc( EDA_RECT* ClipBox, wxDC* DC, int xc, int yc, double StAngle, double EndAngle, int r,
            const COLOR4D& Color )
{
    int x1, y1, x2, y2;

    /* Clip arcs off screen */
    if( ClipBox )
    {
        int radius = r + 1;
        int x0, y0, xm, ym, x, y;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();
        x = xc;
        y = yc;

        if( x < ( x0 - radius ) )
            return;

        if( y < ( y0 - radius ) )
            return;

        if( x > ( xm + radius ) )
            return;

        if( y > ( ym + radius ) )
            return;
    }

    x1 = r;
    y1 = 0;
    RotatePoint( &x1, &y1, EndAngle );

    x2 = r;
    y2 = 0;
    RotatePoint( &x2, &y2, StAngle );

    GRSetBrush( DC, Color, NOT_FILLED );
    GRSetColorPen( DC, Color );
    DC->DrawArc( xc + x1, yc - y1, xc + x2, yc - y2, xc, yc );
}


void GRArc( EDA_RECT* ClipBox, wxDC* DC, int x, int y, double StAngle, double EndAngle, int r,
            int width, const COLOR4D& Color )
{
    int x1, y1, x2, y2;

    /* Clip arcs off screen. */
    if( ClipBox )
    {
        int x0, y0, xm, ym;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();

        if( x < ( x0 - r - width ) )
            return;

        if( y < ( y0 - r - width ) )
            return;

        if( x > ( r + xm + width ) )
            return;

        if( y > ( r + ym + width ) )
            return;
    }

    x1 = r;
    y1 = 0;
    RotatePoint( &x1, &y1, EndAngle );

    x2 = r;
    y2 = 0;
    RotatePoint( &x2, &y2, StAngle );

    GRSetBrush( DC, Color );
    GRSetColorPen( DC, Color, width );
    DC->DrawArc( x + x1, y - y1, x + x2, y - y2, x, y );
}


void GRRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, int width,
             const COLOR4D& Color )
{
    GRSRect( ClipBox, DC, x1, y1, x2, y2, width, Color );
}


void GRFilledRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                   const COLOR4D& Color, const COLOR4D& BgColor )
{
    GRSFilledRect( ClipBox, DC, x1, y1, x2, y2, 0, Color, BgColor );
}


void GRFilledRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, int width,
                   const COLOR4D& Color, const COLOR4D& BgColor )
{
    GRSFilledRect( ClipBox, DC, x1, y1, x2, y2, width, Color, BgColor );
}


void GRSRect( EDA_RECT* aClipBox, wxDC* aDC, int x1, int y1, int x2, int y2, int aWidth,
              const COLOR4D& aColor )
{
    wxPoint points[5];
    points[0] = wxPoint( x1, y1 );
    points[1] = wxPoint( x1, y2 );
    points[2] = wxPoint( x2, y2 );
    points[3] = wxPoint( x2, y1 );
    points[4] = points[0];
    GRSClosedPoly( aClipBox, aDC, 5, points, NOT_FILLED, aWidth, aColor, aColor );
}


void GRSFilledRect( EDA_RECT* aClipBox, wxDC* aDC, int x1, int y1, int x2, int y2, int aWidth,
                    const COLOR4D& aColor, const COLOR4D& aBgColor )
{
    wxPoint points[5];
    points[0] = wxPoint( x1, y1 );
    points[1] = wxPoint( x1, y2 );
    points[2] = wxPoint( x2, y2 );
    points[3] = wxPoint( x2, y1 );
    points[4] = points[0];

    GRSetBrush( aDC, aBgColor, FILLED );
    GRSetColorPen( aDC, aBgColor, aWidth );

    if( aClipBox && ( aWidth > 0 ) )
    {
        EDA_RECT clipbox( *aClipBox );
        clipbox.Inflate( aWidth );
        ClipAndDrawPoly( &clipbox, aDC, points, 5 ); // polygon approach is more accurate
    }
    else
    {
        ClipAndDrawPoly(aClipBox, aDC, points, 5 );
    }
}

/**
 * Used to clip a polygon and draw it as Filled Polygon.
 *
 * Uses the Sutherland and Hodgman algo to clip the given poly against a rectangle.  This
 * rectangle is the drawing area this is useful under Linux (2009) because filled polygons
 * are incorrectly drawn if they have  too large coordinates (seems due to integer overflows
 * in calculations).   Could be removed in some years, if become unnecessary.
 */

#include <SutherlandHodgmanClipPoly.h>

void ClipAndDrawPoly( EDA_RECT* aClipBox, wxDC* aDC, const wxPoint* Points, int n )
{
    if( aClipBox == nullptr )
    {
        aDC->DrawPolygon( n, Points );
        return;
    }

    // A clip box exists: clip and draw the polygon.
    static std::vector<wxPoint> clippedPolygon;
    static pointVector inputPolygon, outputPolygon;

    inputPolygon.clear();
    outputPolygon.clear();
    clippedPolygon.clear();

    for( int ii = 0; ii < n; ii++ )
        inputPolygon.push_back( PointF( (REAL) Points[ii].x, (REAL) Points[ii].y ) );

    RectF window( (REAL) aClipBox->GetX(), (REAL) aClipBox->GetY(),
                  (REAL) aClipBox->GetWidth(), (REAL) aClipBox->GetHeight() );

    SutherlandHodgman sh( window );
    sh.Clip( inputPolygon, outputPolygon );

    for( cpointIterator cit = outputPolygon.begin(); cit != outputPolygon.end(); ++cit )
    {
        clippedPolygon.emplace_back( KiROUND( cit->X ), KiROUND( cit->Y ) );
    }

    if( clippedPolygon.size() )
        aDC->DrawPolygon( clippedPolygon.size(), &clippedPolygon[0] );
}


