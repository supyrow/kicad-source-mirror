/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2016-2022 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * Stroke font class
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

#include <gal/graphics_abstraction_layer.h>
#include <wx/string.h>
#include <wx/textfile.h>
#include <newstroke_font.h>
#include <font/glyph.h>
#include <font/stroke_font.h>
#include <geometry/shape_line_chain.h>
#include <trigo.h>

// The "official" name of the building Kicad stroke font (always existing)
#include <font/kicad_font_name.h>

#include <mutex>

using namespace KIFONT;


///< Factor that determines relative vertical position of the overbar.
static constexpr double OVERBAR_POSITION_FACTOR = 1.33;

///< Factor that determines relative vertical position of the underline.
static constexpr double UNDERLINE_POSITION_FACTOR = -0.16;

///< Scale factor for a glyph
static constexpr double STROKE_FONT_SCALE = 1.0 / 21.0;

static constexpr int FONT_OFFSET = -10;


bool                                g_defaultFontInitialized = false;
std::vector<std::shared_ptr<GLYPH>> g_defaultFontGlyphs;
std::vector<BOX2D>*                 g_defaultFontGlyphBoundingBoxes;
std::mutex                          g_defaultFontLoadMutex;


STROKE_FONT::STROKE_FONT() :
        m_glyphs( nullptr ),
        m_glyphBoundingBoxes( nullptr ),
        m_maxGlyphWidth( 0.0 )
{
}


STROKE_FONT* STROKE_FONT::LoadFont( const wxString& aFontName )
{
    if( aFontName.empty() )
    {
        STROKE_FONT* font = new STROKE_FONT();
        font->loadNewStrokeFont( newstroke_font, newstroke_font_bufsize );
        return font;
    }
    else
    {
        // FONT TODO: support for other stroke fonts?
        return nullptr;
    }
}


void buildGlyphBoundingBox( std::shared_ptr<STROKE_GLYPH>& aGlyph, double aGlyphWidth )
{
    VECTOR2D min( 0, 0 );
    VECTOR2D max( aGlyphWidth, 0 );

    for( const std::vector<VECTOR2D>& pointList : *aGlyph )
    {
        for( const VECTOR2D& point : pointList )
        {
            min.y = std::min( min.y, point.y );
            max.y = std::max( max.y, point.y );
        }
    }

    aGlyph->SetBoundingBox( BOX2D( min, max - min ) );
}


void STROKE_FONT::loadNewStrokeFont( const char* const aNewStrokeFont[], int aNewStrokeFontSize )
{
    // Protect the initialization sequence against multiple entries
    std::lock_guard<std::mutex> lock( g_defaultFontLoadMutex );

    if( !g_defaultFontInitialized )
    {
        g_defaultFontGlyphs.reserve( aNewStrokeFontSize );

        g_defaultFontGlyphBoundingBoxes = new std::vector<BOX2D>;
        g_defaultFontGlyphBoundingBoxes->reserve( aNewStrokeFontSize );

        for( int j = 0; j < aNewStrokeFontSize; j++ )
        {
            std::shared_ptr<STROKE_GLYPH> glyph = std::make_shared<STROKE_GLYPH>();

            double glyphStartX = 0.0;
            double glyphEndX = 0.0;
            double glyphWidth = 0.0;
            int    strokes = 0;
            int    i = 0;

            while( aNewStrokeFont[j][i] )
            {

                if( aNewStrokeFont[j][i] == ' ' && aNewStrokeFont[j][i+1] == 'R' )
                    strokes++;

                i += 2;
            }

            glyph->reserve( strokes + 1 );

            i = 0;

            while( aNewStrokeFont[j][i] )
            {
                VECTOR2D point( 0.0, 0.0 );
                char     coordinate[2] = { 0, };

                for( int k : { 0, 1 } )
                    coordinate[k] = aNewStrokeFont[j][i + k];

                if( i < 2 )
                {
                    // The first two values contain the width of the char
                    glyphStartX = ( coordinate[0] - 'R' ) * STROKE_FONT_SCALE;
                    glyphEndX   = ( coordinate[1] - 'R' ) * STROKE_FONT_SCALE;
                    glyphWidth  = glyphEndX - glyphStartX;
                }
                else if( ( coordinate[0] == ' ' ) && ( coordinate[1] == 'R' ) )
                {
                    glyph->RaisePen();
                }
                else
                {
                    // In stroke font, coordinates values are coded as <value> + 'R', where
                    // <value> is an ASCII char.
                    // therefore every coordinate description of the Hershey format has an offset,
                    // it has to be subtracted
                    // Note:
                    //  * the stroke coordinates are stored in reduced form (-1.0 to +1.0),
                    //    and the actual size is stroke coordinate * glyph size
                    //  * a few shapes have a height slightly bigger than 1.0 ( like '{' '[' )
                    point.x = (double) ( coordinate[0] - 'R' ) * STROKE_FONT_SCALE - glyphStartX;

                    // FONT_OFFSET is here for historical reasons, due to the way the stroke font
                    // was built. It allows shapes coordinates like W M ... to be >= 0
                    // Only shapes like j y have coordinates < 0
                    point.y = (double) ( coordinate[1] - 'R' + FONT_OFFSET ) * STROKE_FONT_SCALE;

                    glyph->AddPoint( point );
                }

                i += 2;
            }

            glyph->Finalize();

            // Compute the bounding box of the glyph
            buildGlyphBoundingBox( glyph, glyphWidth );
            g_defaultFontGlyphBoundingBoxes->emplace_back( glyph->BoundingBox() );
            g_defaultFontGlyphs.push_back( glyph );
            m_maxGlyphWidth = std::max( m_maxGlyphWidth, glyphWidth );
        }

        g_defaultFontInitialized = true;
    }

    m_glyphs = &g_defaultFontGlyphs;
    m_glyphBoundingBoxes = g_defaultFontGlyphBoundingBoxes;
    m_fontName = KICAD_FONT_NAME;
    m_fontFileName = wxEmptyString;
}


double STROKE_FONT::GetInterline( double aGlyphHeight, double aLineSpacing ) const
{
    // Do not add the glyph thickness to the interline.  This makes bold text line-spacing
    // different from normal text, which is poor typography.
    return ( aGlyphHeight * aLineSpacing * INTERLINE_PITCH_RATIO );
}


double STROKE_FONT::ComputeOverbarVerticalPosition( double aGlyphHeight ) const
{
    return aGlyphHeight * OVERBAR_POSITION_FACTOR;
}


double STROKE_FONT::ComputeUnderlineVerticalPosition( double aGlyphHeight ) const
{
    return aGlyphHeight * UNDERLINE_POSITION_FACTOR;
}


VECTOR2I STROKE_FONT::GetTextAsGlyphs( BOX2I* aBBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                                       const wxString& aText, const VECTOR2I& aSize,
                                       const VECTOR2I& aPosition, const EDA_ANGLE& aAngle,
                                       bool aMirror, const VECTOR2I& aOrigin,
                                       TEXT_STYLE_FLAGS aTextStyle ) const
{
    constexpr double SPACE_WIDTH = 0.6;
    constexpr double INTER_CHAR = 0.2;
    constexpr double TAB_WIDTH = 4 * 0.82;  // Not quite as wide as 5.1/6.0 tab formatting, but
                                            // a better match for Scintilla, and closer to the
                                            // nominal SPACE_WIDTH + INTER_CHAR
    constexpr double SUPER_SUB_SIZE_MULTIPLIER = 0.7;
    constexpr double SUPER_HEIGHT_OFFSET = 0.5;
    constexpr double SUB_HEIGHT_OFFSET = 0.3;

    VECTOR2I cursor( aPosition );
    VECTOR2D glyphSize( aSize );
    double   tilt = ( aTextStyle & TEXT_STYLE::ITALIC ) ? ITALIC_TILT : 0.0;

    if( aTextStyle & TEXT_STYLE::SUBSCRIPT || aTextStyle & TEXT_STYLE::SUPERSCRIPT )
    {
        glyphSize = glyphSize * SUPER_SUB_SIZE_MULTIPLIER;

        if( aTextStyle & TEXT_STYLE::SUBSCRIPT )
            cursor.y += glyphSize.y * SUB_HEIGHT_OFFSET;
        else
            cursor.y -= glyphSize.y * SUPER_HEIGHT_OFFSET;
    }

    for( wxUniChar c : aText )
    {
        // Handle tabs as locked to the nearest 4th column (in space-widths).
        if( c == '\t' )
        {
            int tabWidth = KiROUND( glyphSize.x * TAB_WIDTH );
            int currentIntrusion = ( cursor.x - aOrigin.x ) % tabWidth;

            cursor.x += tabWidth - currentIntrusion;
        }
        else if( c == ' ' )
        {
            // 'space' character - draw nothing, advance cursor position
            cursor.x += KiROUND( glyphSize.x * SPACE_WIDTH );
        }
        else
        {
            // dd is the index into bounding boxes table
            int dd = (signed) c - ' ';

            // Filtering non existing glyphs and non printable chars
            if( dd < 0 || dd >= (int) m_glyphBoundingBoxes->size() )
            {
                c = '?';
                dd = (signed) c - ' ';
            }

            STROKE_GLYPH* source = static_cast<STROKE_GLYPH*>( m_glyphs->at( dd ).get() );

            if( aGlyphs )
            {
                aGlyphs->push_back( source->Transform( glyphSize, cursor, tilt, aAngle, aMirror,
                                                       aOrigin ) );
            }

            VECTOR2D glyphExtents = source->BoundingBox().GetEnd();

            glyphExtents *= glyphSize;

            if( tilt > 0.0 )
                glyphExtents.x -= glyphExtents.y * tilt;

            cursor.x += KiROUND( glyphExtents.x );
        }
    }

    VECTOR2D barOffset( 0.0, 0.0 );

    // Shorten the bar a little so its rounded ends don't make it over-long
    double   barTrim = glyphSize.x * 0.1;

    if( aTextStyle & TEXT_STYLE::OVERBAR )
    {
        barOffset.y = ComputeOverbarVerticalPosition( glyphSize.y );

        if( aTextStyle & TEXT_STYLE::ITALIC )
            barOffset.x = barOffset.y * ITALIC_TILT;

        VECTOR2D barStart( aPosition.x + barOffset.x + barTrim, cursor.y - barOffset.y );
        VECTOR2D barEnd( cursor.x + barOffset.x - barTrim, cursor.y - barOffset.y );

        if( aGlyphs )
        {
            STROKE_GLYPH overbarGlyph;

            overbarGlyph.AddPoint( barStart );
            overbarGlyph.AddPoint( barEnd );
            overbarGlyph.Finalize();

            aGlyphs->push_back( overbarGlyph.Transform( { 1.0, 1.0 }, { 0, 0 }, false,
                                                         aAngle, aMirror, aOrigin ) );
        }
    }

    if( aTextStyle & TEXT_STYLE::UNDERLINE )
    {
        barOffset.y = ComputeUnderlineVerticalPosition( glyphSize.y );

        if( aTextStyle & TEXT_STYLE::ITALIC )
            barOffset.x = barOffset.y * ITALIC_TILT;

        VECTOR2D barStart( aPosition.x + barOffset.x + barTrim, cursor.y - barOffset.y );
        VECTOR2D barEnd( cursor.x + barOffset.x - barTrim, cursor.y - barOffset.y );

        if( aGlyphs )
        {
            STROKE_GLYPH underlineGlyph;

            underlineGlyph.AddPoint( barStart );
            underlineGlyph.AddPoint( barEnd );
            underlineGlyph.Finalize();

            aGlyphs->push_back( underlineGlyph.Transform( { 1.0, 1.0 }, { 0, 0 }, false,
                                                          aAngle, aMirror, aOrigin ) );
        }
    }

    if( aBBox )
    {
        aBBox->SetOrigin( aPosition );
        aBBox->SetEnd( cursor.x + barOffset.x - KiROUND( glyphSize.x * INTER_CHAR ),
                       cursor.y + std::max( glyphSize.y, barOffset.y * OVERBAR_POSITION_FACTOR ) );
        aBBox->Normalize();
    }

    return VECTOR2I( cursor.x, aPosition.y );
}
