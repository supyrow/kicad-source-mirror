/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EDA_TEXT_H_
#define EDA_TEXT_H_

#include <memory>
#include <vector>

#include <outline_mode.h>
#include <eda_rect.h>
#include <font/text_attributes.h>

class OUTPUTFORMATTER;
class SHAPE_COMPOUND;
class SHAPE_POLY_SET;
class wxFindReplaceData;


/**
 * A helper for the text to polygon callback function.
 *
 * These variables are parameters used in #addTextSegmToPoly but #addTextSegmToPoly is a
 * callback function so the cannot be sent as arguments.
 */
struct TSEGM_2_POLY_PRMS
{
    int m_textWidth;
    int m_error;
    SHAPE_POLY_SET* m_cornerBuffer;
};


/**
 * Callback function used to convert text segments to polygons.
 */
extern void addTextSegmToPoly( int x0, int y0, int xf, int yf, void* aData );


namespace KIGFX
{
    class RENDER_SETTINGS;
    class COLOR4D;
}

using KIGFX::RENDER_SETTINGS;
using KIGFX::COLOR4D;

// part of the kicad_plugin.h family of defines.
// See kicad_plugin.h for the choice of the value
// When set when calling  EDA_TEXT::Format, disable writing the "hide" keyword in save file
#define CTL_OMIT_HIDE               (1 << 6)


/**
 * This is the "default-of-the-default" hardcoded text size; individual
 * application define their own default policy starting with this
 * (usually with a user option or project).
 */
#define DEFAULT_SIZE_TEXT   50     // default text height (in mils, i.e. 1/1000")
#define DIM_ANCRE_TEXTE     2      // Anchor size for text


/**
 * A mix-in class (via multiple inheritance) that handles texts such as labels, parts,
 * components, or footprints.  Because it's a mix-in class, care is used to provide
 * function names (accessors) that to not collide with function names likely to be seen
 * in the combined derived classes.
 */
class EDA_TEXT
{
public:
    EDA_TEXT( const wxString& text = wxEmptyString );

    EDA_TEXT( const EDA_TEXT& aText );

    virtual ~EDA_TEXT();

    /**
     * Return the string associated with the text object.
     *
     * @return a const wxString reference containing the string of the item.
     */
    virtual const wxString& GetText() const { return m_text; }

    /**
     * Return the string actually shown after processing of the base text.
     *
     * @param aDepth is used to prevent infinite recursions and loops when expanding
     * text variables.
     */
    virtual wxString GetShownText( int aDepth = 0 ) const { return m_shown_text; }

    /**
     * Returns a shortened version (max 15 characters) of the shown text
     */
    wxString ShortenedShownText() const;

    /**
     * Indicates the ShownText has text var references which need to be processed.
     */
    bool HasTextVars() const { return m_shown_text_has_text_var_refs; }

    virtual void SetText( const wxString& aText );

    /**
     * The TextThickness is that set by the user.  The EffectiveTextPenWidth also factors
     * in bold text and thickness clamping.
     */
    void SetTextThickness( int aWidth ) { m_attributes.m_StrokeWidth = aWidth; };
    int GetTextThickness() const        { return m_attributes.m_StrokeWidth; };

    /**
     * The EffectiveTextPenWidth uses the text thickness if > 1 or aDefaultWidth.
     */
    int GetEffectiveTextPenWidth( int aDefaultWidth = 0 ) const;

    virtual void SetTextAngle( double aAngleInTenthsOfADegree )
    {
        // Higher level classes may be more restrictive than this by overloading
        // SetTextAngle() or merely calling EDA_TEXT::SetTextAngle() after clamping
        // aAngle before calling this lowest inline accessor.
        m_attributes.m_Angle = EDA_ANGLE( aAngleInTenthsOfADegree, EDA_ANGLE::TENTHS_OF_A_DEGREE );
    }

    void SetTextAngle( const EDA_ANGLE& aAngle )
    {
        m_attributes.m_Angle = aAngle;
    }

    const EDA_ANGLE& GetTextAngle() const       { return m_attributes.m_Angle; }

    void SetItalic( bool aItalic )              { m_attributes.m_Italic = aItalic; }
    bool IsItalic() const                       { return m_attributes.m_Italic; }

    void SetBold( bool aBold )                  { m_attributes.m_Bold = aBold; }
    bool IsBold() const                         { return m_attributes.m_Bold; }

    virtual void SetVisible( bool aVisible )    { m_attributes.m_Visible = aVisible; }
    virtual bool IsVisible() const              { return m_attributes.m_Visible; }

    void SetMirrored( bool isMirrored )         { m_attributes.m_Mirrored = isMirrored; }
    bool IsMirrored() const                     { return m_attributes.m_Mirrored; }

    /**
     * @param aAllow true if ok to use multiline option, false if ok to use only single line
     *               text.  (Single line is faster in calculations than multiline.)
     */
    void SetMultilineAllowed( bool aAllow )     { m_attributes.m_Multiline = aAllow; }
    bool IsMultilineAllowed() const             { return m_attributes.m_Multiline; }

    GR_TEXT_H_ALIGN_T GetHorizJustify() const   { return m_attributes.m_Halign; };
    GR_TEXT_V_ALIGN_T GetVertJustify() const    { return m_attributes.m_Valign; };

    void SetHorizJustify( GR_TEXT_H_ALIGN_T aType )   { m_attributes.m_Halign = aType; };
    void SetVertJustify( GR_TEXT_V_ALIGN_T aType )    { m_attributes.m_Valign = aType; };

    void SetKeepUpright( bool aKeepUpright )    { m_attributes.m_KeepUpright = aKeepUpright; }
    bool IsKeepUpright() const                  { return m_attributes.m_KeepUpright; }

    /**
     * Set the text attributes from another instance.
     */
    void SetAttributes( const EDA_TEXT& aSrc );

    /**
     * Swap the text attributes of the two involved instances.
     */
    void SwapAttributes( EDA_TEXT& aTradingPartner );

    void SwapText( EDA_TEXT& aTradingPartner );

    void CopyText( const EDA_TEXT& aSrc );

    /**
     * Helper function used in search and replace dialog.
     *
     * Perform a text replace using the find and replace criteria in \a aSearchData.
     *
     * @param aSearchData A reference to a wxFindReplaceData object containing the
     *                    search and replace criteria.
     * @return True if the text item was modified, otherwise false.
     */
    bool Replace( const wxFindReplaceData& aSearchData );

    bool IsDefaultFormatting() const;

    void SetFont( KIFONT::FONT* aFont )         { m_attributes.m_Font = aFont; }
    KIFONT::FONT* GetFont() const               { return m_attributes.m_Font; }

    wxString GetFontName() const;

    void SetLineSpacing( double aLineSpacing )  { m_attributes.m_LineSpacing = aLineSpacing; }
    double GetLineSpacing() const               { return m_attributes.m_LineSpacing; }

    void SetTextSize( const wxSize& aNewSize )  { m_attributes.m_Size = aNewSize; }
    wxSize GetTextSize() const                  { return wxSize( m_attributes.m_Size.x,
                                                                 m_attributes.m_Size.y ); }

    void SetTextWidth( int aWidth )             { m_attributes.m_Size.x = aWidth; }
    int GetTextWidth() const                    { return m_attributes.m_Size.x; }

    void SetTextHeight( int aHeight )           { m_attributes.m_Size.y = aHeight; }
    int GetTextHeight() const                   { return m_attributes.m_Size.y; }

    void SetTextPos( const wxPoint& aPoint )    { m_pos = aPoint; }
    const wxPoint& GetTextPos() const           { return m_pos; }

    void SetTextX( int aX )                     { m_pos.x = aX; }
    void SetTextY( int aY )                     { m_pos.y = aY; }

    void Offset( const wxPoint& aOffset )       { m_pos += aOffset; }

    void Empty()                                { m_text.Empty(); }

    static GR_TEXT_H_ALIGN_T MapHorizJustify( int aHorizJustify );

    static GR_TEXT_V_ALIGN_T MapVertJustify( int aVertJustify );

    /**
     * Print this text object to the device context \a aDC.
     *
     * @param aDC the current Device Context.
     * @param aOffset draw offset (usually (0,0)).
     * @param aColor text color.
     * @param aDisplay_mode #FILLED or #SKETCH.
     */
    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                const COLOR4D& aColor, OUTLINE_MODE aDisplay_mode = FILLED );

    /**
     * Convert the text shape to a list of segment.
     *
     * Each segment is stored as 2 wxPoints: the starting point and the ending point
     * there are therefore 2*n points.
     */
    std::vector<wxPoint> TransformToSegmentList() const;

    /**
     * Convert the text bounding box to a rectangular polygon depending on the text
     * orientation, the bounding box is not always horizontal or vertical
     *
     * Used in filling zones calculations
     * Circles and arcs are approximated by segments
     *
     * @param aCornerBuffer a buffer to store the polygon.
     * @param aClearanceValue the clearance around the text bounding box
     * to the real clearance value (usually near from 1.0).
     */
    void TransformBoundingBoxWithClearanceToPolygon( SHAPE_POLY_SET* aCornerBuffer,
                                                     int aClearanceValue ) const;

    std::shared_ptr<SHAPE_COMPOUND> GetEffectiveTextShape() const;

    /**
     * Test if \a aPoint is within the bounds of this object.
     *
     * @param aPoint A wxPoint to test.
     * @param aAccuracy Amount to inflate the bounding box.
     * @return true if a hit, else false.
     */
    virtual bool TextHitTest( const wxPoint& aPoint, int aAccuracy = 0 ) const;

    /**
     * Test if object bounding box is contained within or intersects \a aRect.
     *
     * @param aRect Rect to test against.
     * @param aContains Test for containment instead of intersection if true.
     * @param aAccuracy Amount to inflate the bounding box.
     * @return true if a hit, else false.
     */
    virtual bool TextHitTest( const EDA_RECT& aRect, bool aContains, int aAccuracy = 0 ) const;

    /**
     * @return the text length in internal units.
     * @param aLine the line of text to consider.  For single line text, this parameter
     *              is always m_Text.
     * @param aThickness the stroke width of the text.
     */
    int LenSize( const wxString& aLine, int aThickness ) const;

    /**
     * Useful in multiline texts to calculate the full text or a line area (for zones filling,
     * locate functions....)
     *
     * @param aLine The line of text to consider.  Pass -1 for all lines.
     * @param aInvertY Invert the Y axis when calculating bounding box.
     * @return the rect containing the line of text (i.e. the position and the size of one line)
     *         this rectangle is calculated for 0 orient text.
     *         If orientation is not 0 the rect must be rotated to match the physical area
     */
    EDA_RECT GetTextBox( int aLine = -1, bool aInvertY = false ) const;

    /**
     * Return the distance between two lines of text.
     *
     * Calculates the distance (pitch) between two lines of text.  This distance includes the
     * interline distance plus room for characters like j, {, and [.  It also used for single
     * line text, to calculate the text bounding box.
     */
    int GetInterline() const;

    /**
     * @return a wxString with the style name( Normal, Italic, Bold, Bold+Italic).
     */
    wxString GetTextStyleName() const;

    /**
     * Populate \a aPositions with the position of each line of a multiline text, according
     * to the vertical justification and the rotation of the whole text.
     *
     * @param aPositions is the list to populate by the wxPoint positions.
     * @param aLineCount is the number of lines (not recalculated here for efficiency reasons.
     */
    void GetLinePositions( std::vector<wxPoint>& aPositions, int aLineCount ) const;

    /**
     * Output the object to \a aFormatter in s-expression form.
     *
     * @param aFormatter The #OUTPUTFORMATTER object to write to.
     * @param aNestLevel The indentation next level.
     * @param aControlBits The control bit definition for object specific formatting.
     * @throw IO_ERROR on write error.
     */
    virtual void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

    virtual EDA_ANGLE GetDrawRotation() const               { return GetTextAngle(); }
    virtual wxPoint GetDrawPos() const                      { return GetTextPos(); }
    virtual GR_TEXT_H_ALIGN_T GetDrawHorizJustify() const   { return GetHorizJustify(); };
    virtual GR_TEXT_V_ALIGN_T GetDrawVertJustify() const    { return GetVertJustify(); };

    int Compare( const EDA_TEXT* aOther ) const;

private:
    void cacheShownText();

    /**
     * Print each line of this EDA_TEXT.
     *
     * @param aOffset draw offset (usually (0,0)).
     * @param aColor text color.
     * @param aFillMode FILLED or SKETCH
     * @param aText the single line of text to draw.
     * @param aPos the position of this line ).
     */
    void printOneLineOfText( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                             const COLOR4D& aColor, OUTLINE_MODE aFillMode, const wxString& aText,
                             const wxPoint& aPos );

    wxString        m_text;
    wxString        m_shown_text;           // Cache of unescaped text for efficient access
    bool            m_shown_text_has_text_var_refs;

    TEXT_ATTRIBUTES m_attributes;
    wxPoint         m_pos;
};


#endif   //  EDA_TEXT_H_
