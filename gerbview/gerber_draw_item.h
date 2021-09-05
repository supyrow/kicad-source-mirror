/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 <Jean-Pierre Charras>
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

/**
 * @file gerber_draw_item.h
 */

#ifndef GERBER_DRAW_ITEM_H
#define GERBER_DRAW_ITEM_H

#include <eda_item.h>
#include <layer_ids.h>
#include <gr_basic.h>
#include <gbr_netlist_metadata.h>
#include <dcode.h>
#include <geometry/shape_poly_set.h>

class GERBER_FILE_IMAGE;
class GBR_LAYOUT;
class D_CODE;
class MSG_PANEL_ITEM;
class GBR_DISPLAY_OPTIONS;
class PCB_BASE_FRAME;

namespace KIGFX
{
    class VIEW;
}


/* Shapes id for basic shapes ( .m_Shape member ) */
enum Gbr_Basic_Shapes {
    GBR_SEGMENT = 0,        // usual segment : line with rounded ends
    GBR_ARC,                // Arcs (with rounded ends)
    GBR_CIRCLE,             // ring
    GBR_POLYGON,            // polygonal shape
    GBR_SPOT_CIRCLE,        // flashed shape: round shape (can have hole)
    GBR_SPOT_RECT,          // flashed shape: rectangular shape can have hole)
    GBR_SPOT_OVAL,          // flashed shape: oval shape
    GBR_SPOT_POLY,          // flashed shape: regular polygon, 3 to 12 edges
    GBR_SPOT_MACRO,         // complex shape described by a macro
    GBR_LAST                // last value for this list
};

class GERBER_DRAW_ITEM : public EDA_ITEM
{
public:
    GERBER_DRAW_ITEM( GERBER_FILE_IMAGE* aGerberparams );
    ~GERBER_DRAW_ITEM();

    void SetNetAttributes( const GBR_NETLIST_METADATA& aNetAttributes );
    const GBR_NETLIST_METADATA& GetNetAttributes() const { return m_netAttributes; }

    /**
     * Return the layer this item is on.
     */
    int GetLayer() const;

    bool GetLayerPolarity() const
    {
        return m_LayerNegative;
    }

    /**
     * Return the best size and orientation to display the D_Code on screen.
     *
     * @param aSize is a reference to return the text size
     * @param aPos is a reference to return the text position
     * @param aOrientation is a reference to return the text orientation
     * @return true if the parameters can be calculated, false for unknown D_Code
     */
    bool GetTextD_CodePrms( int& aSize, wxPoint& aPos, double& aOrientation );

    /**
     * Return the best size and orientation to display the D_Code in GAL
     * aOrientation is returned in radians.
     */
    bool GetTextD_CodePrms( double& aSize, VECTOR2D& aPos, double& aOrientation );

    /**
     * Optimize screen refresh (when no items are in background color refresh can be faster).
     *
     * @return true if this item or at least one shape (when using aperture macros
     *         must be drawn in background color.
     */
    bool HasNegativeItems();

    /**
     * Initialize parameters from Image and Layer parameters found in the gerber file:
     *   m_UnitsMetric,
     *   m_MirrorA, m_MirrorB,
     *   m_DrawScale, m_DrawOffset
     */
    void SetLayerParameters();

    void SetLayerPolarity( bool aNegative)
    {
        m_LayerNegative = aNegative;
    }

    /**
     * Move this object.
     *
     * @param aMoveVector the move vector for this object.
     */
    void MoveAB( const wxPoint& aMoveVector );

     /**
      * Move this object.
      *
      * @param aMoveVector the move vector for this object, in XY gerber axis.
      */
    void MoveXY( const wxPoint& aMoveVector );

    /**
     * Return the position of this object.
     *
     * This function exists mainly to satisfy the virtual GetPosition() in parent class
     *
     * @return The position of this object.
     */
    wxPoint GetPosition() const override                { return m_Start; }
    void SetPosition( const wxPoint& aPos ) override    {  m_Start = aPos; }

    /**
     * Return the image position of aPosition for this object.
     *
     * Image position is the value of aPosition, modified by image parameters:
     * offsets, axis selection, scale, rotation
     *
     * @param aXYPosition is position in X,Y gerber axis
     * @return  The given position in plotter A,B axis.
     */
    wxPoint GetABPosition( const wxPoint& aXYPosition ) const;

    VECTOR2I GetABPosition( const VECTOR2I& aXYPosition ) const
    {
        return VECTOR2I( GetABPosition( wxPoint( aXYPosition.x, aXYPosition.y ) ) );
    }

    /**
     * Return the image position of aPosition for this object.
     *
     * Image position is the value of aPosition, modified by image parameters:
     * offsets, axis selection, scale, rotation
     *
     * @param aABPosition = position in A,B plotter axis
     * @return The given position in X,Y axis.
     */
    wxPoint GetXYPosition( const wxPoint& aABPosition ) const;

    /**
     * Return the GetDcodeDescr of this object, or NULL.
     *
     * @return a pointer to the DCode description (for flashed items).
     */
    D_CODE* GetDcodeDescr() const;

    const EDA_RECT GetBoundingBox() const override;

    void Print( wxDC* aDC, const wxPoint& aOffset, GBR_DISPLAY_OPTIONS* aOptions );

    /**
     * Convert a line to an equivalent polygon.
     *
     * Useful when a line is plotted using a rectangular pen.
     * In this case, the usual segment plot function cannot be used
     */
    void ConvertSegmentToPolygon();

    /**
     * Print the polygon stored in m_PolyCorners.
     */
    void PrintGerberPoly( wxDC* aDC, const COLOR4D& aColor, const wxPoint& aOffset,
                          bool aFilledShape );

    int Shape() const { return m_Shape; }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    wxString ShowGBRShape() const;

    /**
     * Test if the given wxPoint is within the bounds of this object.
     *
     * @param aRefPos a wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTest( const wxPoint& aRefPos, int aAccuracy = 0 ) const override;

    /**
     * Test if the given wxRect intersect this object.
     *
     * For now, an ending point must be inside this rect.
     *
     * @param aRefArea a wxPoint to test
     * @return true if a hit, else false
     */
    bool HitTest( const EDA_RECT& aRefArea, bool aContained, int aAccuracy = 0 ) const override;

    /**
     * @return the class name string.
     */
    wxString GetClass() const override
    {
        return wxT( "GERBER_DRAW_ITEM" );
    }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    /// @copydoc VIEW_ITEM::ViewGetLayers()
    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

    /// @copydoc VIEW_ITEM::ViewBBox()
    virtual const BOX2I ViewBBox() const override;

    /// @copydoc VIEW_ITEM::ViewGetLOD()
    double ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const override;

    ///< @copydoc EDA_ITEM::Visit()
    SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] ) override;

    ///< @copydoc EDA_ITEM::GetSelectMenuText()
    virtual wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    ///< @copydoc EDA_ITEM::GetMenuImage()
    BITMAPS GetMenuImage() const override;

    bool               m_UnitsMetric;       // store here the gerber units (inch/mm).  Used
                                            // only to calculate aperture macros shapes sizes
    int                m_Shape;             // Shape and type of this gerber item
    wxPoint            m_Start;             // Line or arc start point or position of the shape
                                            // for flashed items
    wxPoint            m_End;               // Line or arc end point
    wxPoint            m_ArcCentre;         // for arcs only: Center of arc
    SHAPE_POLY_SET     m_Polygon;           // Polygon shape data (G36 to G37 coordinates)
                                            // or for complex shapes which are converted to polygon
    wxSize             m_Size;              // Flashed shapes: size of the shape
                                            // Lines : m_Size.x = m_Size.y = line width
    bool               m_Flashed;           // True for flashed items
    int                m_DCode;             // DCode used to draw this item.
                                            // Allowed values are >= 10. 0 when unknown
                                            // values 0 to 9 can be used for special purposes
                                            // Regions (polygons) do not use DCode,
                                            // so it is set to 0
    wxString           m_AperFunction;      // the aperture function set by a %TA.AperFunction, xxx
                                            // (stores the xxx value). Used for regions that do
                                            // not have a attached DCode, but
                                            // have a TA.AperFunction defined
    GERBER_FILE_IMAGE* m_GerberImageFile;   /* Gerber file image source of this item
                                             * Note: some params stored in this class are common
                                             * to the whole gerber file (i.e) the whole graphic
                                             * layer and some can change when reading the file,
                                             * so they are stored inside this item if there is no
                                             * redundancy for these parameters
                                             */

    // This polygon is to draw this item (mainly GBR_POLYGON), according to layer parameters
    SHAPE_POLY_SET   m_AbsolutePolygon;     // the polygon to draw, in absolute coordinates
private:
    // These values are used to draw this item, according to gerber layers parameters
    // Because they can change inside a gerber image, they are stored here
    // for each item
    bool        m_LayerNegative;            // true = item in negative Layer
    bool        m_swapAxis;                 // false if A = X, B = Y; true if A =Y, B = Y
    bool        m_mirrorA;                  // true: mirror / axis A
    bool        m_mirrorB;                  // true: mirror / ax's B
    wxRealPoint m_drawScale;                // A and B scaling factor
    wxPoint     m_layerOffset;              // Offset for A and B axis, from OF parameter
    double      m_lyrRotation;              // Fine rotation, from OR parameter, in degrees
    GBR_NETLIST_METADATA m_netAttributes;   ///< the string given by a %TO attribute set in aperture
                                            ///< (dcode). Stored in each item, because %TO is
                                            ///< a dynamic object attribute
};


class GERBER_NEGATIVE_IMAGE_BACKDROP : public EDA_ITEM
{

};

#endif /* GERBER_DRAW_ITEM_H */
