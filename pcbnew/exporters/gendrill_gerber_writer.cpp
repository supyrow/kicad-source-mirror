/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
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
 * @file gendrill_gerber_writer.cpp
 * @brief Functions to create drill files in gerber X2 format.
 */

#include <plotters/plotter_gerber.h>
#include <string_utils.h>
#include <locale_io.h>
#include <board.h>
#include <footprint.h>
#include <pcb_track.h>
#include <pad.h>
#include <pcbplot.h>
#include <gendrill_gerber_writer.h>
#include <reporter.h>
#include <gbr_metadata.h>

// set to 1 to use flashed oblong holes, 0 to draw them by a line.
#define FLASH_OVAL_HOLE 1


GERBER_WRITER::GERBER_WRITER( BOARD* aPcb )
    : GENDRILL_WRITER_BASE( aPcb )
{
    m_zeroFormat      = SUPPRESS_LEADING;
    m_conversionUnits = 1.0;
    m_unitsMetric    = true;
    m_drillFileExtension = "gbr";
    m_merge_PTH_NPTH = false;
}


void GERBER_WRITER::CreateDrillandMapFilesSet( const wxString& aPlotDirectory, bool aGenDrill,
                                               bool aGenMap, REPORTER* aReporter )
{
    // Note: In Gerber drill files, NPTH and PTH are always separate files
    m_merge_PTH_NPTH = false;

    wxFileName  fn;
    wxString    msg;

    std::vector<DRILL_LAYER_PAIR> hole_sets = getUniqueLayerPairs();

    // append a pair representing the NPTH set of holes, for separate drill files.
    // (Gerber drill files are separate files for PTH and NPTH)
    hole_sets.emplace_back( F_Cu, B_Cu );

    for( std::vector<DRILL_LAYER_PAIR>::const_iterator it = hole_sets.begin();
         it != hole_sets.end();  ++it )
    {
        DRILL_LAYER_PAIR  pair = *it;
        // For separate drill files, the last layer pair is the NPTH drill file.
        bool doing_npth = ( it == hole_sets.end() - 1 );

        buildHolesList( pair, doing_npth );

        // The file is created if it has holes, or if it is the non plated drill file
        // to be sure the NPTH file is up to date in separate files mode.
        // Also a PTH drill/map file is always created, to be sure at least one plated hole drill
        // file is created (do not create any PTH drill file can be seen as not working drill
        // generator).
        if( getHolesCount() > 0 || doing_npth || pair == DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
        {
            fn = getDrillFileName( pair, doing_npth, false );
            fn.SetPath( aPlotDirectory );

            if( aGenDrill )
            {
                wxString fullFilename = fn.GetFullPath();

                int result = createDrillFile( fullFilename, doing_npth, pair );

                if( result < 0 )
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "Failed to create file '%s'." ), fullFilename );
                        aReporter->Report( msg, RPT_SEVERITY_ERROR );
                    }

                    break;
                }
                else
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "Created file '%s'." ), fullFilename );
                        aReporter->Report( msg, RPT_SEVERITY_INFO );
                    }
                }

            }
        }
    }

    if( aGenMap )
        CreateMapFilesSet( aPlotDirectory, aReporter );
}


#if !FLASH_OVAL_HOLE
// A helper class to transform an oblong hole to a segment
static void convertOblong2Segment( wxSize aSize, double aOrient, wxPoint& aStart, wxPoint& aEnd );
#endif


int GERBER_WRITER::createDrillFile( wxString& aFullFilename, bool aIsNpth,
                                    DRILL_LAYER_PAIR aLayerPair )
{
    int    holes_count;

    LOCALE_IO dummy;    // Use the standard notation for double numbers

    GERBER_PLOTTER plotter;

    // Gerber drill file imply X2 format:
    plotter.UseX2format( true );
    plotter.UseX2NetAttributes( true );
    plotter.DisableApertMacros( false );

    // Add the standard X2 header, without FileFunction
    AddGerberX2Header( &plotter, m_pcb );
    plotter.SetViewport( m_offset, IU_PER_MILS/10, /* scale */ 1.0, /* mirror */false );

    // has meaning only for gerber plotter. Must be called only after SetViewport
    plotter.SetGerberCoordinatesFormat( 6 );
    plotter.SetCreator( wxT( "PCBNEW" ) );

    // Add the standard X2 FileFunction for drill files
    // %TF.FileFunction,Plated[NonPlated],layer1num,layer2num,PTH[NPTH][Blind][Buried],Drill[Route][Mixed]*%
    wxString text = BuildFileFunctionAttributeString( aLayerPair,
                                                      aIsNpth
                                                            ? TYPE_FILE::NPTH_FILE
                                                            : TYPE_FILE::PTH_FILE );
    plotter.AddLineToHeader( text );

    // Add file polarity (positive)
    text = "%TF.FilePolarity,Positive*%";
    plotter.AddLineToHeader( text );

    if( !plotter.OpenFile( aFullFilename ) )
        return -1;

    plotter.StartPlot();

    holes_count = 0;

    wxPoint hole_pos;
    bool last_item_is_via = true;   // a flag to clear object attributes when a via hole is created.

    for( unsigned ii = 0; ii < m_holeListBuffer.size(); ii++ )
    {
        HOLE_INFO& hole_descr = m_holeListBuffer[ii];
        hole_pos = hole_descr.m_Hole_Pos;

        // Manage the aperture attributes: in drill files 3 attributes can be used:
        // "ViaDrill", only for vias, not pads
        // "ComponentDrill", only for Through Holes pads
        // "Slot" for oblong holes;
        GBR_METADATA gbr_metadata;

        if( dyn_cast<const PCB_VIA*>( hole_descr.m_ItemParent ) )
        {
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_VIADRILL );

            if( !last_item_is_via )
            {
                // be sure the current object attribute is cleared for vias
                plotter.EndBlock( nullptr );
            }

            last_item_is_via = true;
        }
        else if( dyn_cast<const PAD*>( hole_descr.m_ItemParent ) )
        {
            last_item_is_via = false;
            const PAD* pad = dyn_cast<const PAD*>( hole_descr.m_ItemParent );

            if( pad->GetProperty() == PAD_PROP::CASTELLATED )
            {
                gbr_metadata.SetApertureAttrib(
                        GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CASTELLATEDDRILL );
            }
            else
            {
                // Good practice of oblong pad holes (slots) is to use a specific aperture for
                // routing, not used in drill commands.
                if( hole_descr.m_Hole_Shape )
                    gbr_metadata.SetApertureAttrib(
                            GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CMP_OBLONG_DRILL );
                else
                    gbr_metadata.SetApertureAttrib(
                            GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CMP_DRILL );
            }

            // Add object attribute: component reference to pads (mainly useful for users)
            wxString ref = pad->GetParent()->GetReference();

            gbr_metadata.SetCmpReference( ref );
            gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );
        }

        if( hole_descr.m_Hole_Shape )
        {
#if FLASH_OVAL_HOLE     // set to 1 to use flashed oblong holes,
                                    // 0 to draw them as a line.
            plotter.FlashPadOval( hole_pos, hole_descr.m_Hole_Size,
                                  hole_descr.m_Hole_Orient, FILLED, &gbr_metadata );
#else
            // Use routing for oblong hole (Slots)
            wxPoint start, end;
            convertOblong2Segment( hole_descr.m_Hole_Size, hole_descr.m_Hole_Orient, start, end );
            int width = std::min( hole_descr.m_Hole_Size.x, hole_descr.m_Hole_Size.y );

            if ( width == 0 )
                continue;

            plotter.ThickSegment( start+hole_pos, end+hole_pos, width, FILLED, &gbr_metadata );
#endif
        }
        else
        {
            int diam = std::min( hole_descr.m_Hole_Size.x, hole_descr.m_Hole_Size.y );
            plotter.FlashPadCircle( hole_pos, diam, FILLED, &gbr_metadata );
        }

        holes_count++;
    }

    plotter.EndPlot();

    return holes_count;
}


#if !FLASH_OVAL_HOLE
void convertOblong2Segment( wxSize aSize, double aOrient, wxPoint& aStart, wxPoint& aEnd )
{
    wxSize  size( aSize );
    double orient = aOrient;

    /* The pad will be drawn as an oblong shape with size.y > size.x
     * (Oval vertical orientation 0)
     */
    if( size.x > size.y )
    {
        std::swap( size.x, size.y );
        orient = AddAngles( orient, 900 );
    }

    int deltaxy = size.y - size.x;     // distance between centers of the oval

    int cx = 0;
    int cy = deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    aStart = wxPoint( cx, cy );
    cx = 0; cy = -deltaxy / 2;
    RotatePoint( &cx, &cy, orient );
    aEnd = wxPoint( cx, cy );
}
#endif


void GERBER_WRITER::SetFormat( int aRightDigits )
{
    /* Set conversion scale depending on drill file units */
    m_conversionUnits = 1.0 / IU_PER_MM;        // Gerber units = mm

    // Set precision (unit is mm).
    m_precision.m_Lhs = 4;
    m_precision.m_Rhs = aRightDigits == 6 ? 6 : 5;
}


const wxString GERBER_WRITER::getDrillFileName( DRILL_LAYER_PAIR aPair, bool aNPTH,
                                                bool aMerge_PTH_NPTH ) const
{
    // Gerber files extension is always .gbr.
    // Therefore, to mark drill files, add "-drl" to the filename.
    wxFileName fname( GENDRILL_WRITER_BASE::getDrillFileName( aPair, aNPTH, aMerge_PTH_NPTH ) );
    fname.SetName( fname.GetName() + "-drl" );

    return fname.GetFullPath();
}
