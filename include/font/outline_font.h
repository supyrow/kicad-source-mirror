/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021-2022 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * Outline font class
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

#ifndef OUTLINE_FONT_H_
#define OUTLINE_FONT_H_

#include <gal/graphics_abstraction_layer.h>
#include <geometry/shape_poly_set.h>
#ifdef _MSC_VER
#include <ft2build.h>
#else
#include <freetype2/ft2build.h>
#endif
#include FT_FREETYPE_H
#include FT_OUTLINE_H
//#include <gal/opengl/opengl_freetype.h>
#include <harfbuzz/hb.h>
#include <font/font.h>
#include <font/glyph.h>
#include <font/outline_decomposer.h>

namespace KIFONT
{
/**
 * Class OUTLINE_FONT implements outline font drawing.
 */
class OUTLINE_FONT : public FONT
{
public:
    OUTLINE_FONT();

    static wxString FreeTypeVersion();

    bool IsOutline() const override { return true; }

    bool IsBold() const override
    {
        return m_face && ( m_face->style_flags & FT_STYLE_FLAG_BOLD );
    }

    bool IsItalic() const override
    {
        return m_face && ( m_face->style_flags & FT_STYLE_FLAG_ITALIC );
    }

    /**
     * Load an outline font. TrueType (.ttf) and OpenType (.otf) are supported.
     * @param aFontFileName is the (platform-specific) fully qualified name of the font file
     */
    static OUTLINE_FONT* LoadFont( const wxString& aFontFileName, bool aBold, bool aItalic );

    /**
     * Compute the vertical position of an overbar.  This is the distance between the text
     * baseline and the overbar.
     */
    double ComputeOverbarVerticalPosition( double aGlyphHeight ) const override;

    /**
     * Compute the distance (interline) between 2 lines of text (for multiline texts).  This is
     * the distance between baselines, not the space between line bounding boxes.
     */
    double GetInterline( double aGlyphHeight = 0.0, double aLineSpacing = 1.0 ) const override;

    VECTOR2I GetTextAsGlyphs( BOX2I* aBoundingBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                              const wxString& aText, const VECTOR2I& aSize,
                              const VECTOR2I& aPosition, const EDA_ANGLE& aAngle, bool aMirror,
                              const VECTOR2I& aOrigin, TEXT_STYLE_FLAGS aTextStyle ) const override;

    void GetLinesAsGlyphs( std::vector<std::unique_ptr<GLYPH>>* aGlyphs, const wxString& aText,
                           const VECTOR2I& aPosition, const TEXT_ATTRIBUTES& aAttrs ) const;

    const FT_Face& GetFace() const { return m_face; }

#if 0
    void RenderToOpenGLCanvas( KIGFX::OPENGL_FREETYPE& aTarget, const wxString& aString,
                               const VECTOR2D& aSize, const wxPoint& aPosition,
                               const EDA_ANGLE& aAngle, bool aMirror ) const;
#endif

protected:
    FT_Error loadFace( const wxString& aFontFileName );

    BOX2I getBoundingBox( const std::vector<std::unique_ptr<GLYPH>>& aGlyphs ) const;

private:
    // FreeType variables
    static FT_Library m_freeType;
    FT_Face           m_face;
    const int         m_faceSize;

    // cache for glyphs converted to straight segments
    // key is glyph index (FT_GlyphSlot field glyph_index)
    std::map<unsigned int, GLYPH_POINTS_LIST> m_contourCache;

    // The height of the KiCad stroke font is the distance between stroke endpoints for a vertical
    // line of cap-height.  So the cap-height of the font is actually stroke-width taller than its
    // height.
    // Outline fonts are normally scaled on full-height (including ascenders and descenders), so we
    // need to compensate to keep them from being much smaller than their stroked counterparts.
    static constexpr double m_outlineFontSizeCompensation = 1.4;

    // FT_Set_Char_Size() gets character width and height specified in 1/64ths of a point
    static constexpr int m_charSizeScaler = 64;

    // The KiCad stroke font uses a subscript/superscript size ratio of 0.7.  This ratio is also
    // commonly used in LaTeX, but fonts with designed-in subscript and superscript glyphs are more
    // likely to use 0.58.
    // For auto-generated subscript and superscript glyphs in outline fonts we split the difference
    // with 0.64.
    static constexpr double m_subscriptSuperscriptSize = 0.64;

    int faceSize( int aSize ) const
    {
        return aSize * m_charSizeScaler * m_outlineFontSizeCompensation;
    };
    int faceSize() const { return faceSize( m_faceSize ); }

    // also for superscripts
    int subscriptSize( int aSize ) const
    {
        return KiROUND( faceSize( aSize ) * m_subscriptSuperscriptSize );
    }
    int subscriptSize() const { return subscriptSize( m_faceSize ); }

    static constexpr double m_subscriptVerticalOffset   = -0.25;
    static constexpr double m_superscriptVerticalOffset = 0.45;
    static constexpr double m_overbarOffsetRatio        = 0.02;
    static constexpr double m_overbarThicknessRatio     = 0.08;
};

} //namespace KIFONT

#endif // OUTLINE_FONT_H_
