/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


/*

Pcbnew PLUGIN for Eagle 6.x XML *.brd and footprint format.

XML parsing and converting:
Getting line numbers and byte offsets from the source XML file is not
possible using currently available XML libraries within KiCad project:
wxXmlDocument and boost::property_tree.

property_tree will give line numbers but no byte offsets, and only during
document loading. This means that if we have a problem after the document is
successfully loaded, there is no way to correlate back to line number and byte
offset of the problem. So a different approach is taken, one which relies on the
XML elements themselves using an XPATH type of reporting mechanism. The path to
the problem is reported in the error messages. This means keeping track of that
path as we traverse the XML document for the sole purpose of accurate error
reporting.

User can load the source XML file into firefox or other xml browser and follow
our error message.

Load() TODO's

*) verify zone fill clearances are correct

*/

#include <cerrno>

#include <wx/string.h>
#include <wx/xml/xml.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/wfstream.h>

#include <convert_basic_shapes_to_polygon.h>
#include <core/arraydim.h>
#include <geometry/geometry_utils.h>
#include <string_utils.h>
#include <locale_io.h>
#include <macros.h>
#include <properties.h>
#include <trigo.h>
#include <math/util.h>      // for KiROUND
#include <progress_reporter.h>
#include <project.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <fp_shape.h>
#include <zone.h>
#include <pad_shapes.h>
#include <pcb_text.h>
#include <pcb_dimension.h>

#include <plugins/eagle/eagle_plugin.h>

using namespace std;


/// Parse an eagle distance which is either mm, or mils if there is "mil" suffix.
/// Return is in BIU.
static int parseEagle( const wxString& aDistance )
{
    ECOORD::EAGLE_UNIT unit = ( aDistance.npos != aDistance.find( "mil" ) )
        ? ECOORD::EAGLE_UNIT::EU_MIL : ECOORD::EAGLE_UNIT::EU_MM;

    ECOORD coord( aDistance, unit );

    return coord.ToPcbUnits();
}


// In Eagle one can specify DRC rules where min value > max value,
// in such case the max value has the priority
template<typename T>
static T eagleClamp( T aMin, T aValue, T aMax )
{
    T ret = std::max( aMin, aValue );
    return std::min( aMax, ret );
}


/// Assemble a two part key as a simple concatenation of aFirst and aSecond parts,
/// using a separator.
static wxString makeKey( const wxString& aFirst, const wxString& aSecond )
{
    wxString key = aFirst + '\x02' +  aSecond;
    return key;
}


/// interpret special characters in Eagle text and converts them to KiCAD notation
static wxString interpret_text( const wxString& aText )
{
    wxString text;
    bool sectionOpen = false;

    for ( wxString::size_type i = 0; i < aText.size(); i++ )
    {
        // Interpret escaped characters
        if ( aText[ i ] == '\\' )
        {
            if ( i + 1 != aText.size() )
                text.Append( aText[ i + 1 ] );

            i++;
            continue;
        }

        // Escape ~ for KiCAD
        if( aText[i] == '~' )
        {
            text.Append( '~' );
            text.Append( '~' );
            continue;
        }

        if ( aText[ i ] == '!' )
        {
            if ( sectionOpen )
            {
                text.Append( '~' );
                sectionOpen = false;
                continue;
            }

            static wxString escapeChars( " )]}'\"" );

            if( i + 1 != aText.size() && escapeChars.Find( aText[i + 1] ) == wxNOT_FOUND )
            {
                sectionOpen = true;
                text.Append( '~' );
            }
            else
            {
                text.Append( aText[ i ] );
            }
            continue;
        }

        if( aText[i] == ',' && sectionOpen )
        {
            text.Append( '~' );
            sectionOpen = false;
        }

        text.Append( aText[ i ] );
    }

    return text;
}


static void setKeepoutSettingsToZone( ZONE* aZone, LAYER_NUM aLayer )
{
    if( aLayer == EAGLE_LAYER::TRESTRICT || aLayer == EAGLE_LAYER::BRESTRICT )
    {
        aZone->SetIsRuleArea( true );
        aZone->SetDoNotAllowVias( true );
        aZone->SetDoNotAllowTracks( true );
        aZone->SetDoNotAllowCopperPour( true );
        aZone->SetDoNotAllowPads( true );
        aZone->SetDoNotAllowFootprints( false );

        if( aLayer == EAGLE_LAYER::TRESTRICT ) // front layer keepout
            aZone->SetLayer( F_Cu );
        else // bottom layer keepout
            aZone->SetLayer( B_Cu );
    }
    else if( aLayer == EAGLE_LAYER::VRESTRICT )
    {
        aZone->SetIsRuleArea( true );
        aZone->SetDoNotAllowVias( true );
        aZone->SetDoNotAllowTracks( false );
        aZone->SetDoNotAllowCopperPour( false );
        aZone->SetDoNotAllowPads( false );
        aZone->SetDoNotAllowFootprints( false );

        aZone->SetLayerSet( LSET::AllCuMask() );
    }
}


void ERULES::parse( wxXmlNode* aRules, std::function<void()> aCheckpoint )
{
    wxXmlNode* child = aRules->GetChildren();

    while( child )
    {
        aCheckpoint();

        if( child->GetName() == "param" )
        {
            const wxString& name = child->GetAttribute( "name" );
            const wxString& value = child->GetAttribute( "value" );

            if( name == "psElongationLong" )
                psElongationLong = wxAtoi( value );
            else if( name == "psElongationOffset" )
                psElongationOffset = wxAtoi( value );
            else if( name == "mvStopFrame" )
                value.ToCDouble( &mvStopFrame );
            else if( name == "mvCreamFrame" )
                value.ToCDouble( &mvCreamFrame );
            else if( name == "mlMinStopFrame" )
                mlMinStopFrame = parseEagle( value );
            else if( name == "mlMaxStopFrame" )
                mlMaxStopFrame = parseEagle( value );
            else if( name == "mlMinCreamFrame" )
                mlMinCreamFrame = parseEagle( value );
            else if( name == "mlMaxCreamFrame" )
                mlMaxCreamFrame = parseEagle( value );
            else if( name == "srRoundness" )
                value.ToCDouble( &srRoundness );
            else if( name == "srMinRoundness" )
                srMinRoundness = parseEagle( value );
            else if( name == "srMaxRoundness" )
                srMaxRoundness = parseEagle( value );
            else if( name == "psTop" )
                psTop = wxAtoi( value );
            else if( name == "psBottom" )
                psBottom = wxAtoi( value );
            else if( name == "psFirst" )
                psFirst = wxAtoi( value );
            else if( name == "rvPadTop" )
                value.ToCDouble( &rvPadTop );
            else if( name == "rlMinPadTop" )
                rlMinPadTop = parseEagle( value );
            else if( name == "rlMaxPadTop" )
                rlMaxPadTop = parseEagle( value );
            else if( name == "rvViaOuter" )
                value.ToCDouble( &rvViaOuter );
            else if( name == "rlMinViaOuter" )
                rlMinViaOuter = parseEagle( value );
            else if( name == "rlMaxViaOuter" )
                rlMaxViaOuter = parseEagle( value );
            else if( name == "mdWireWire" )
                mdWireWire = parseEagle( value );
        }

        child = child->GetNext();
    }
}


EAGLE_PLUGIN::EAGLE_PLUGIN() :
        m_rules( new ERULES() ),
        m_xpath( new XPATH() ),
        m_progressReporter( nullptr ),
        m_doneCount( 0 ),
        m_lastProgressCount( 0 ),
        m_totalCount( 0 ),
        m_mod_time( wxDateTime::Now() )
{
    using namespace std::placeholders;

    init( nullptr );
    clear_cu_map();
    RegisterLayerMappingCallback( std::bind( &EAGLE_PLUGIN::DefaultLayerMappingCallback,
                                             this, _1 ) );
}


EAGLE_PLUGIN::~EAGLE_PLUGIN()
{
    deleteTemplates();
    delete m_rules;
    delete m_xpath;
}


const wxString EAGLE_PLUGIN::PluginName() const
{
    return wxT( "Eagle" );
}


const wxString EAGLE_PLUGIN::GetFileExtension() const
{
    return wxT( "brd" );
}


void EAGLE_PLUGIN::checkpoint()
{
    const unsigned PROGRESS_DELTA = 50;

    if( m_progressReporter )
    {
        if( ++m_doneCount > m_lastProgressCount + PROGRESS_DELTA )
        {
            m_progressReporter->SetCurrentProgress( ( (double) m_doneCount )
                                                    / std::max( 1U, m_totalCount ) );

            if( !m_progressReporter->KeepRefreshing() )
                THROW_IO_ERROR( ( "Open cancelled by user." ) );

            m_lastProgressCount = m_doneCount;
        }
    }
}


wxSize inline EAGLE_PLUGIN::kicad_fontz( const ECOORD& d, int aTextThickness ) const
{
    // Eagle includes stroke thickness in the text size, KiCAD does not
    int kz = d.ToPcbUnits();
    return wxSize( kz - aTextThickness, kz - aTextThickness );
}


BOARD* EAGLE_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe,
                           const PROPERTIES* aProperties, PROJECT* aProject,
                           PROGRESS_REPORTER* aProgressReporter )
{
    LOCALE_IO       toggle;     // toggles on, then off, the C locale.
    wxXmlNode*      doc;

    init( aProperties );

    m_board = aAppendToMe ? aAppendToMe : new BOARD();
    m_progressReporter = aProgressReporter;

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    // delete on exception, if I own m_board, according to aAppendToMe
    unique_ptr<BOARD> deleter( aAppendToMe ? nullptr : m_board );

    try
    {
        if( m_progressReporter )
        {
            m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

            if( !m_progressReporter->KeepRefreshing() )
                THROW_IO_ERROR( ( "Open cancelled by user." ) );
        }

        wxFileName fn = aFileName;

        // Load the document
        wxFFileInputStream stream( fn.GetFullPath() );
        wxXmlDocument xmlDocument;

        if( !stream.IsOk() || !xmlDocument.Load( stream ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unable to read file '%s'" ),
                                              fn.GetFullPath() ) );
        }

        doc = xmlDocument.GetRoot();

        m_min_trace    = INT_MAX;
        m_min_hole     = INT_MAX;
        m_min_via      = INT_MAX;
        m_min_annulus  = INT_MAX;

        loadAllSections( doc );

        BOARD_DESIGN_SETTINGS& designSettings = m_board->GetDesignSettings();

        if( m_min_trace < designSettings.m_TrackMinWidth )
            designSettings.m_TrackMinWidth = m_min_trace;

        if( m_min_via < designSettings.m_ViasMinSize )
            designSettings.m_ViasMinSize = m_min_via;

        if( m_min_hole < designSettings.m_MinThroughDrill )
            designSettings.m_MinThroughDrill = m_min_hole;

        if( m_min_annulus < designSettings.m_ViasMinAnnularWidth )
            designSettings.m_ViasMinAnnularWidth = m_min_annulus;

        if( m_rules->mdWireWire )
            designSettings.m_MinClearance = KiROUND( m_rules->mdWireWire );

        NETCLASS defaults( "dummy" );

        auto finishNetclass =
                [&]( NETCLASSPTR netclass )
                {
                    // If Eagle has a clearance matrix then we'll build custom rules from that.
                    // Netclasses should just be the board minimum clearance.
                    netclass->SetClearance( KiROUND( designSettings.m_MinClearance ) );

                    if( netclass->GetTrackWidth() == INT_MAX )
                        netclass->SetTrackWidth( defaults.GetTrackWidth() );

                    if( netclass->GetViaDiameter() == INT_MAX )
                        netclass->SetViaDiameter( defaults.GetViaDiameter() );

                    if( netclass->GetViaDrill() == INT_MAX )
                        netclass->SetViaDrill( defaults.GetViaDrill() );
                };

        finishNetclass( designSettings.GetNetClasses().GetDefault() );

        for( const std::pair<const wxString, NETCLASSPTR>& entry : designSettings.GetNetClasses() )
            finishNetclass( entry.second );

        m_board->m_LegacyNetclassesLoaded = true;
        m_board->m_LegacyDesignSettingsLoaded = true;

        fn.SetExt( "kicad_dru" );
        wxFile rulesFile( fn.GetFullPath(), wxFile::write );
        rulesFile.Write( m_customRules );

        // should be empty, else missing m_xpath->pop()
        wxASSERT( m_xpath->Contents().size() == 0 );
    }
    catch( const XML_PARSER_ERROR &exc )
    {
        wxString errmsg = exc.what();

        errmsg += "\n@ ";
        errmsg += m_xpath->Contents();

        THROW_IO_ERROR( errmsg );
    }

    // IO_ERROR exceptions are left uncaught, they pass upwards from here.

    // Ensure the copper layers count is a multiple of 2
    // Pcbnew does not like boards with odd layers count
    // (these boards cannot exist. they actually have a even layers count)
    int lyrcnt = m_board->GetCopperLayerCount();

    if( (lyrcnt % 2) != 0 )
    {
        lyrcnt++;
        m_board->SetCopperLayerCount( lyrcnt );
    }

    centerBoard();

    deleter.release();
    return m_board;
}


std::vector<FOOTPRINT*> EAGLE_PLUGIN::GetImportedCachedLibraryFootprints()
{
    std::vector<FOOTPRINT*> retval;

    for( std::pair<wxString, FOOTPRINT*> fp : m_templates )
        retval.push_back( static_cast<FOOTPRINT*>( fp.second->Clone() ) );

    return retval;
}


void EAGLE_PLUGIN::init( const PROPERTIES* aProperties )
{
    m_hole_count  = 0;
    m_min_trace   = 0;
    m_min_hole    = 0;
    m_min_via     = 0;
    m_min_annulus = 0;
    m_xpath->clear();
    m_pads_to_nets.clear();

    m_board = nullptr;
    m_props = aProperties;


    delete m_rules;
    m_rules = new ERULES();
}


void EAGLE_PLUGIN::clear_cu_map()
{
    // All cu layers are invalid until we see them in the <layers> section while
    // loading either a board or library.  See loadLayerDefs().
    for( unsigned i = 0;  i < arrayDim(m_cu_map);  ++i )
        m_cu_map[i] = -1;
}


void EAGLE_PLUGIN::loadAllSections( wxXmlNode* aDoc )
{
    wxXmlNode* drawing       = MapChildren( aDoc )["drawing"];
    NODE_MAP drawingChildren = MapChildren( drawing );

    wxXmlNode* board         = drawingChildren["board"];
    NODE_MAP boardChildren   = MapChildren( board );

    auto count_children = [this]( wxXmlNode* aNode )
            {
                if( aNode )
                {
                    wxXmlNode* child = aNode->GetChildren();

                    while( child )
                    {
                        m_totalCount++;
                        child = child->GetNext();
                    }
                }
            };

    wxXmlNode* designrules = boardChildren["designrules"];
    wxXmlNode* layers = drawingChildren["layers"];
    wxXmlNode* plain = boardChildren["plain"];
    wxXmlNode* classes = boardChildren["classes"];
    wxXmlNode* signals = boardChildren["signals"];
    wxXmlNode* libs = boardChildren["libraries"];
    wxXmlNode* elems = boardChildren["elements"];

    if( m_progressReporter )
    {
        m_totalCount = 0;
        m_doneCount = 0;

        count_children( designrules );
        count_children( layers );
        count_children( plain );
        count_children( signals );
        count_children( elems );

        while( libs )
        {
            count_children( MapChildren( libs )["packages"] );
            libs = libs->GetNext();
        }

        // Rewind
        libs = boardChildren["libraries"];
    }

    m_xpath->push( "eagle.drawing" );

    {
        m_xpath->push( "board" );

        loadDesignRules( designrules );

        m_xpath->pop();
    }

    {
        m_xpath->push( "layers" );

        loadLayerDefs( layers );
        mapEagleLayersToKicad();

        m_xpath->pop();
    }

    {
        m_xpath->push( "board" );

        loadPlain( plain );
        loadClasses( classes );
        loadSignals( signals );
        loadLibraries( libs );
        loadElements( elems );

        m_xpath->pop();
    }

    m_xpath->pop();     // "eagle.drawing"
}


void EAGLE_PLUGIN::loadDesignRules( wxXmlNode* aDesignRules )
{
    if( aDesignRules )
    {
        m_xpath->push( "designrules" );
        m_rules->parse( aDesignRules, [this](){ checkpoint(); } );
        m_xpath->pop();     // "designrules"
    }
}


void EAGLE_PLUGIN::loadLayerDefs( wxXmlNode* aLayers )
{
    if( !aLayers )
        return;

    ELAYERS cu;  // copper layers

    // Get the first layer and iterate
    wxXmlNode* layerNode = aLayers->GetChildren();

    m_eagleLayers.clear();
    m_eagleLayersIds.clear();

    while( layerNode )
    {
        ELAYER elayer( layerNode );
        m_eagleLayers.insert( std::make_pair( elayer.number, elayer ) );
        m_eagleLayersIds.insert( std::make_pair( elayer.name, elayer.number ) );

        // find the subset of layers that are copper and active
        if( elayer.number >= 1 && elayer.number <= 16 && ( !elayer.active || *elayer.active ) )
            cu.push_back( elayer );

        layerNode = layerNode->GetNext();
    }

    // establish cu layer map:
    int ki_layer_count = 0;

    for( EITER it = cu.begin();  it != cu.end();  ++it,  ++ki_layer_count )
    {
        if( ki_layer_count == 0 )
        {
            m_cu_map[it->number] = F_Cu;
        }
        else if( ki_layer_count == int( cu.size()-1 ) )
        {
            m_cu_map[it->number] = B_Cu;
        }
        else
        {
            // some eagle boards do not have contiguous layer number sequences.
            m_cu_map[it->number] = ki_layer_count;
        }
    }

    // Set the layer names and cu count if we're loading a board.
    if( m_board )
    {
        m_board->SetCopperLayerCount( cu.size() );

        for( EITER it = cu.begin();  it != cu.end();  ++it )
        {
            PCB_LAYER_ID layer =  kicad_layer( it->number );

            // these function provide their own protection against non enabled layers:
            if( layer >= 0 && layer < PCB_LAYER_ID_COUNT )    // layer should be valid
            {
                m_board->SetLayerName( layer, FROM_UTF8( it->name.c_str() ) );
                m_board->SetLayerType( layer, LT_SIGNAL );
            }

            // could map the colors here
        }
    }
}


#define DIMENSION_PRECISION 1 // 0.001 mm


void EAGLE_PLUGIN::loadPlain( wxXmlNode* aGraphics )
{
    if( !aGraphics )
        return;

    m_xpath->push( "plain" );

    // Get the first graphic and iterate
    wxXmlNode* gr = aGraphics->GetChildren();

    // (polygon | wire | text | circle | rectangle | frame | hole)*
    while( gr )
    {
        checkpoint();

        wxString grName = gr->GetName();

        if( grName == "wire" )
        {
            m_xpath->push( "wire" );

            EWIRE        w( gr );
            PCB_LAYER_ID layer = kicad_layer( w.layer );

            wxPoint start( kicad_x( w.x1 ), kicad_y( w.y1 ) );
            wxPoint end(   kicad_x( w.x2 ), kicad_y( w.y2 ) );

            if( layer != UNDEFINED_LAYER )
            {
                PCB_SHAPE* shape = new PCB_SHAPE( m_board );
                int        width = w.width.ToPcbUnits();

                // KiCad cannot handle zero or negative line widths
                if( width <= 0 )
                    width = m_board->GetDesignSettings().GetLineThickness( layer );

                m_board->Add( shape, ADD_MODE::APPEND );

                if( !w.curve )
                {
                    shape->SetShape( SHAPE_T::SEGMENT );
                    shape->SetStart( start );
                    shape->SetEnd( end );
                }
                else
                {
                    wxPoint center = ConvertArcCenter( start, end, *w.curve );

                    shape->SetShape( SHAPE_T::ARC );
                    shape->SetCenter( center );
                    shape->SetStart( start );
                    shape->SetArcAngleAndEnd( *w.curve * -10.0, true ); // KiCad rotates the other way
                }

                shape->SetLayer( layer );
                shape->SetWidth( width );
            }

            m_xpath->pop();
        }
        else if( grName == "text" )
        {
            m_xpath->push( "text" );

            ETEXT        t( gr );
            PCB_LAYER_ID layer = kicad_layer( t.layer );

            if( layer != UNDEFINED_LAYER )
            {
                PCB_TEXT* pcbtxt = new PCB_TEXT( m_board );
                m_board->Add( pcbtxt, ADD_MODE::APPEND );

                pcbtxt->SetLayer( layer );
                wxString kicadText = interpret_text( t.text );
                pcbtxt->SetText( FROM_UTF8( kicadText.c_str() ) );
                pcbtxt->SetTextPos( wxPoint( kicad_x( t.x ), kicad_y( t.y ) ) );

                double ratio = t.ratio ? *t.ratio : 8;     // DTD says 8 is default
                int textThickness = KiROUND( t.size.ToPcbUnits() * ratio / 100 );
                pcbtxt->SetTextThickness( textThickness );
                pcbtxt->SetTextSize( kicad_fontz( t.size, textThickness ) );

                int align = t.align ? *t.align : ETEXT::BOTTOM_LEFT;

                if( t.rot )
                {
                    int sign = t.rot->mirror ? -1 : 1;
                    pcbtxt->SetMirrored( t.rot->mirror );

                    double degrees = t.rot->degrees;

                    if( degrees == 90 || t.rot->spin )
                    {
                        pcbtxt->SetTextAngle( sign * t.rot->degrees * 10 );
                    }
                    else if( degrees == 180 )
                    {
                        align = -align;
                    }
                    else if( degrees == 270 )
                    {
                        pcbtxt->SetTextAngle( sign * 90 * 10 );
                        align = -align;
                    }
                    else
                    {
                        // Ok so text is not at 90,180 or 270 so do some funny stuff to get
                        // placement right.
                        if( ( degrees > 0 ) &&  ( degrees < 90 ) )
                        {
                            pcbtxt->SetTextAngle( sign * t.rot->degrees * 10 );
                        }
                        else if( ( degrees > 90 ) && ( degrees < 180 ) )
                        {
                            pcbtxt->SetTextAngle( sign * ( t.rot->degrees + 180 ) * 10 );
                            align = ETEXT::TOP_RIGHT;
                        }
                        else if( ( degrees > 180 ) && ( degrees < 270 ) )
                        {
                            pcbtxt->SetTextAngle( sign * ( t.rot->degrees - 180 ) * 10 );
                            align = ETEXT::TOP_RIGHT;
                        }
                        else if( ( degrees > 270 ) && ( degrees < 360 ) )
                        {
                            pcbtxt->SetTextAngle( sign * t.rot->degrees * 10 );
                            align = ETEXT::BOTTOM_LEFT;
                        }
                    }
                }

                switch( align )
                {
                case ETEXT::CENTER:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
                    break;

                case ETEXT::CENTER_LEFT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
                    break;

                case ETEXT::CENTER_RIGHT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
                    break;

                case ETEXT::TOP_CENTER:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                    break;

                case ETEXT::TOP_LEFT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                    break;

                case ETEXT::TOP_RIGHT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                    break;

                case ETEXT::BOTTOM_CENTER:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                    break;

                case ETEXT::BOTTOM_LEFT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                    break;

                case ETEXT::BOTTOM_RIGHT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                    break;
                }
            }

            m_xpath->pop();
        }
        else if( grName == "circle" )
        {
            m_xpath->push( "circle" );

            ECIRCLE      c( gr );

            int width  = c.width.ToPcbUnits();
            int radius = c.radius.ToPcbUnits();

            if( c.layer == EAGLE_LAYER::TRESTRICT || c.layer == EAGLE_LAYER::BRESTRICT
                    || c.layer == EAGLE_LAYER::VRESTRICT )
            {
                ZONE* zone = new ZONE( m_board );
                m_board->Add( zone, ADD_MODE::APPEND );

                setKeepoutSettingsToZone( zone, c.layer );

                // approximate circle as polygon with a edge every 10 degree
                wxPoint center( kicad_x( c.x ), kicad_y( c.y ) );
                int     outlineRadius = radius + ( width / 2 );

                for( int angle = 0; angle < 360; angle += 10 )
                {
                    wxPoint rotatedPoint( outlineRadius, 0 );
                    RotatePoint( &rotatedPoint, angle * 10. );
                    zone->AppendCorner( center + rotatedPoint, -1 );
                }

                if( width > 0 )
                {
                    zone->NewHole();
                    int innerRadius = radius - ( width / 2 );

                    for( int angle = 0; angle < 360; angle += 10 )
                    {
                        wxPoint rotatedPoint( innerRadius, 0 );
                        RotatePoint( &rotatedPoint, angle * 10. );
                        zone->AppendCorner( center + rotatedPoint, 0 );
                    }
                }

                zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                             ZONE::GetDefaultHatchPitch(), true );
            }
            else
            {
                PCB_LAYER_ID layer = kicad_layer( c.layer );

                if( layer != UNDEFINED_LAYER ) // unsupported layer
                {
                    PCB_SHAPE* shape = new PCB_SHAPE( m_board, SHAPE_T::CIRCLE );
                    m_board->Add( shape, ADD_MODE::APPEND );
                    shape->SetFilled( false );
                    shape->SetLayer( layer );
                    shape->SetStart( wxPoint( kicad_x( c.x ), kicad_y( c.y ) ) );
                    shape->SetEnd( wxPoint( kicad_x( c.x ) + radius, kicad_y( c.y ) ) );
                    shape->SetWidth( width );
                }
            }

            m_xpath->pop();
        }
        else if( grName == "rectangle" )
        {
            // This seems to be a simplified rectangular [copper] zone, cannot find any
            // net related info on it from the DTD.
            m_xpath->push( "rectangle" );

            ERECT        r( gr );
            PCB_LAYER_ID layer = kicad_layer( r.layer );

            if( IsCopperLayer( layer ) )
            {
                // use a "netcode = 0" type ZONE:
                ZONE* zone = new ZONE( m_board );
                m_board->Add( zone, ADD_MODE::APPEND );

                zone->SetLayer( layer );
                zone->SetNetCode( NETINFO_LIST::UNCONNECTED );

                ZONE_BORDER_DISPLAY_STYLE outline_hatch = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE;

                const int outlineIdx = -1;      // this is the id of the copper zone main outline
                zone->AppendCorner( wxPoint( kicad_x( r.x1 ), kicad_y( r.y1 ) ), outlineIdx );
                zone->AppendCorner( wxPoint( kicad_x( r.x2 ), kicad_y( r.y1 ) ), outlineIdx );
                zone->AppendCorner( wxPoint( kicad_x( r.x2 ), kicad_y( r.y2 ) ), outlineIdx );
                zone->AppendCorner( wxPoint( kicad_x( r.x1 ), kicad_y( r.y2 ) ), outlineIdx );

                if( r.rot )
                    zone->Rotate( zone->GetPosition(), r.rot->degrees * 10 );

                // this is not my fault:
                zone->SetBorderDisplayStyle( outline_hatch, ZONE::GetDefaultHatchPitch(),
                                             true );
            }

            m_xpath->pop();
        }
        else if( grName == "hole" )
        {
            m_xpath->push( "hole" );

            // Fabricate a FOOTPRINT with a single PAD_ATTRIB::NPTH pad.
            // Use m_hole_count to gen up a unique name.

            FOOTPRINT* footprint = new FOOTPRINT( m_board );
            m_board->Add( footprint, ADD_MODE::APPEND );
            footprint->SetReference( wxString::Format( "@HOLE%d", m_hole_count++ ) );
            footprint->Reference().SetVisible( false );

            packageHole( footprint, gr, true );

            m_xpath->pop();
        }
        else if( grName == "frame" )
        {
            // picture this
        }
        else if( grName == "polygon" )
        {
            m_xpath->push( "polygon" );
            loadPolygon( gr );
            m_xpath->pop();     // "polygon"
        }
        else if( grName == "dimension" )
        {
            EDIMENSION d( gr );
            PCB_LAYER_ID layer = kicad_layer( d.layer );

            if( layer != UNDEFINED_LAYER )
            {
                const BOARD_DESIGN_SETTINGS& designSettings = m_board->GetDesignSettings();
                PCB_DIM_ALIGNED* dimension = new PCB_DIM_ALIGNED( m_board );
                m_board->Add( dimension, ADD_MODE::APPEND );

                if( d.dimensionType )
                {
                    // Eagle dimension graphic arms may have different lengths, but they look
                    // incorrect in KiCad (the graphic is tilted). Make them even length in
                    // such case.
                    if( *d.dimensionType == "horizontal" )
                    {
                        int newY = ( d.y1.ToPcbUnits() + d.y2.ToPcbUnits() ) / 2;
                        d.y1 = ECOORD( newY, ECOORD::EAGLE_UNIT::EU_NM );
                        d.y2 = ECOORD( newY, ECOORD::EAGLE_UNIT::EU_NM );
                    }
                    else if( *d.dimensionType == "vertical" )
                    {
                        int newX = ( d.x1.ToPcbUnits() + d.x2.ToPcbUnits() ) / 2;
                        d.x1 = ECOORD( newX, ECOORD::EAGLE_UNIT::EU_NM );
                        d.x2 = ECOORD( newX, ECOORD::EAGLE_UNIT::EU_NM );
                    }
                }

                dimension->SetLayer( layer );
                dimension->SetPrecision( DIMENSION_PRECISION );

                // The origin and end are assumed to always be in this order from eagle
                dimension->SetStart( wxPoint( kicad_x( d.x1 ), kicad_y( d.y1 ) ) );
                dimension->SetEnd( wxPoint( kicad_x( d.x2 ), kicad_y( d.y2 ) ) );
                dimension->Text().SetTextSize( designSettings.GetTextSize( layer ) );
                dimension->Text().SetTextThickness( designSettings.GetTextThickness( layer ) );
                dimension->SetLineThickness( designSettings.GetLineThickness( layer ) );
                dimension->SetUnits( EDA_UNITS::MILLIMETRES );

                // check which axis the dimension runs in
                // because the "height" of the dimension is perpendicular to that axis
                // Note the check is just if two axes are close enough to each other
                // Eagle appears to have some rounding errors
                if( abs( ( d.x1 - d.x2 ).ToPcbUnits() ) < 50000 )   // 50000 nm = 0.05 mm
                    dimension->SetHeight( kicad_x( d.x3 - d.x1 ) );
                else
                    dimension->SetHeight( kicad_y( d.y3 - d.y1 ) );
            }
        }

        // Get next graphic
        gr = gr->GetNext();
    }

    m_xpath->pop();
}


void EAGLE_PLUGIN::loadLibrary( wxXmlNode* aLib, const wxString* aLibName )
{
    if( !aLib )
        return;

    // library will have <xmlattr> node, skip that and get the single packages node
    wxXmlNode* packages = MapChildren( aLib )["packages"];

    if( !packages )
        return;

    m_xpath->push( "packages" );

    // Create a FOOTPRINT for all the eagle packages, for use later via a copy constructor
    // to instantiate needed footprints in our BOARD.  Save the FOOTPRINT templates in
    // a FOOTPRINT_MAP using a single lookup key consisting of libname+pkgname.

    // Get the first package and iterate
    wxXmlNode* package = packages->GetChildren();

    while( package )
    {
        checkpoint();

        m_xpath->push( "package", "name" );

        wxString pack_ref = package->GetAttribute( "name" );
        ReplaceIllegalFileNameChars( pack_ref, '_' );

        m_xpath->Value( pack_ref.ToUTF8() );

        wxString key = aLibName ? makeKey( *aLibName, pack_ref ) : pack_ref;

        FOOTPRINT* m = makeFootprint( package, pack_ref );

        // add the templating FOOTPRINT to the FOOTPRINT template factory "m_templates"
        std::pair<FOOTPRINT_MAP::iterator, bool> r = m_templates.insert( {key, m} );

        if( !r.second /* && !( m_props && m_props->Value( "ignore_duplicates" ) ) */ )
        {
            wxString lib = aLibName ? *aLibName : m_lib_path;
            const wxString& pkg = pack_ref;

            wxString emsg = wxString::Format( _( "<package> '%s' duplicated in <library> '%s'" ),
                                              pkg,
                                              lib );
            THROW_IO_ERROR( emsg );
        }

        m_xpath->pop();

        package = package->GetNext();
    }

    m_xpath->pop();     // "packages"
}


void EAGLE_PLUGIN::loadLibraries( wxXmlNode* aLibs )
{
    if( !aLibs )
        return;

    m_xpath->push( "libraries.library", "name" );

    // Get the first library and iterate
    wxXmlNode* library = aLibs->GetChildren();

    while( library )
    {
        const wxString& lib_name = library->GetAttribute( "name" );

        m_xpath->Value( lib_name.c_str() );
        loadLibrary( library, &lib_name );
        library = library->GetNext();
    }

    m_xpath->pop();
}


void EAGLE_PLUGIN::loadElements( wxXmlNode* aElements )
{
    if( !aElements )
        return;

    m_xpath->push( "elements.element", "name" );

    EATTR   name;
    EATTR   value;
    bool refanceNamePresetInPackageLayout;
    bool valueNamePresetInPackageLayout;

    // Get the first element and iterate
    wxXmlNode* element = aElements->GetChildren();

    while( element )
    {
        checkpoint();

        if( element->GetName() != "element" )
        {
            // Get next item
            element = element->GetNext();
            continue;
        }

        EELEMENT    e( element );

        // use "NULL-ness" as an indication of presence of the attribute:
        EATTR*      nameAttr  = nullptr;
        EATTR*      valueAttr = nullptr;

        m_xpath->Value( e.name.c_str() );

        wxString pkg_key = makeKey( e.library, e.package );

        FOOTPRINT_MAP::const_iterator it = m_templates.find( pkg_key );

        if( it == m_templates.end() )
        {
            wxString emsg = wxString::Format( _( "No '%s' package in library '%s'." ),
                                              FROM_UTF8( e.package.c_str() ),
                                              FROM_UTF8( e.library.c_str() ) );
            THROW_IO_ERROR( emsg );
        }

        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( it->second->Duplicate() );

        m_board->Add( footprint, ADD_MODE::APPEND );

        // update the nets within the pads of the clone
        for( PAD* pad : footprint->Pads() )
        {
            wxString pn_key = makeKey( e.name, pad->GetNumber() );

            NET_MAP_CITER ni = m_pads_to_nets.find( pn_key );
            if( ni != m_pads_to_nets.end() )
            {
                const ENET* enet = &ni->second;
                pad->SetNetCode( enet->netcode );
            }
        }

        refanceNamePresetInPackageLayout = true;
        valueNamePresetInPackageLayout = true;
        footprint->SetPosition( wxPoint( kicad_x( e.x ), kicad_y( e.y ) ) );

        // Is >NAME field set in package layout ?
        if( footprint->GetReference().size() == 0 )
        {
            footprint->Reference().SetVisible( false ); // No so no show
            refanceNamePresetInPackageLayout = false;
        }

        // Is >VALUE field set in package layout
        if( footprint->GetValue().size() == 0 )
        {
            footprint->Value().SetVisible( false );     // No so no show
            valueNamePresetInPackageLayout = false;
        }

        footprint->SetReference( FROM_UTF8( e.name.c_str() ) );
        footprint->SetValue( FROM_UTF8( e.value.c_str() ) );

        if( !e.smashed )
        {
            // Not smashed so show NAME & VALUE
            if( valueNamePresetInPackageLayout )
                footprint->Value().SetVisible( true );  // Only if place holder in package layout

            if( refanceNamePresetInPackageLayout )
                footprint->Reference().SetVisible( true ); // Only if place holder in package layout
        }
        else if( *e.smashed == true )
        {
            // Smashed so set default to no show for NAME and VALUE
            footprint->Value().SetVisible( false );
            footprint->Reference().SetVisible( false );

            // initialize these to default values in case the <attribute> elements are not present.
            m_xpath->push( "attribute", "name" );

            // VALUE and NAME can have something like our text "effects" overrides
            // in SWEET and new schematic.  Eagle calls these XML elements "attribute".
            // There can be one for NAME and/or VALUE both.  Features present in the
            // EATTR override the ones established in the package only if they are
            // present here (except for rot, which if not present means angle zero).
            // So the logic is a bit different than in packageText() and in plain text.

            // Get the first attribute and iterate
            wxXmlNode* attribute = element->GetChildren();

            while( attribute )
            {
                if( attribute->GetName() != "attribute" )
                {
                    attribute = attribute->GetNext();
                    continue;
                }

                EATTR   a( attribute );

                if( a.name == "NAME" )
                {
                    name = a;
                    nameAttr = &name;

                    // do we have a display attribute ?
                    if( a.display  )
                    {
                        // Yes!
                        switch( *a.display )
                        {
                        case EATTR::VALUE :
                        {
                            wxString reference = e.name;

                            // EAGLE allows references to be single digits.  This breaks KiCad
                            // netlisting, which requires parts to have non-digit + digit
                            // annotation.  If the reference begins with a number, we prepend
                            // 'UNK' (unknown) for the symbol designator.
                            if( reference.find_first_not_of( "0123456789" ) == wxString::npos )
                                reference.Prepend( "UNK" );

                            nameAttr->name = reference;
                            footprint->SetReference( reference );

                            if( refanceNamePresetInPackageLayout )
                                footprint->Reference().SetVisible( true );

                            break;
                        }

                        case EATTR::NAME :
                            if( refanceNamePresetInPackageLayout )
                            {
                                footprint->SetReference( "NAME" );
                                footprint->Reference().SetVisible( true );
                            }

                            break;

                        case EATTR::BOTH :
                            if( refanceNamePresetInPackageLayout )
                                footprint->Reference().SetVisible( true );

                            nameAttr->name =  nameAttr->name + " = " + e.name;
                            footprint->SetReference( "NAME = " + e.name );
                            break;

                        case EATTR::Off :
                            footprint->Reference().SetVisible( false );
                            break;

                        default:
                            nameAttr->name =  e.name;

                            if( refanceNamePresetInPackageLayout )
                                footprint->Reference().SetVisible( true );
                        }
                    }
                    else
                    {
                        // No display, so default is visible, and show value of NAME
                        footprint->Reference().SetVisible( true );
                    }
                }
                else if( a.name == "VALUE" )
                {
                    value = a;
                    valueAttr = &value;

                    if( a.display  )
                    {
                        // Yes!
                        switch( *a.display )
                        {
                        case EATTR::VALUE :
                            valueAttr->value = opt_wxString( e.value );
                            footprint->SetValue( e.value );

                            if( valueNamePresetInPackageLayout )
                                footprint->Value().SetVisible( true );

                            break;

                        case EATTR::NAME :
                            if( valueNamePresetInPackageLayout )
                                footprint->Value().SetVisible( true );

                            footprint->SetValue( "VALUE" );
                            break;

                        case EATTR::BOTH :
                            if( valueNamePresetInPackageLayout )
                                footprint->Value().SetVisible( true );

                            valueAttr->value = opt_wxString( "VALUE = " + e.value );
                            footprint->SetValue( "VALUE = " + e.value );
                            break;

                        case EATTR::Off :
                            footprint->Value().SetVisible( false );
                            break;

                        default:
                            valueAttr->value = opt_wxString( e.value );

                            if( valueNamePresetInPackageLayout )
                                footprint->Value().SetVisible( true );
                        }
                    }
                    else
                    {
                        // No display, so default is visible, and show value of NAME
                        footprint->Value().SetVisible( true );
                    }

                }

                attribute = attribute->GetNext();
            }

            m_xpath->pop();     // "attribute"
        }

        orientFootprintAndText( footprint, e, nameAttr, valueAttr );

        // Set the local coordinates for the footprint text items
        footprint->Reference().SetLocalCoord();
        footprint->Value().SetLocalCoord();

        // Get next element
        element = element->GetNext();
    }

    m_xpath->pop();     // "elements.element"
}


ZONE* EAGLE_PLUGIN::loadPolygon( wxXmlNode* aPolyNode )
{
    EPOLYGON     p( aPolyNode );
    PCB_LAYER_ID layer = kicad_layer( p.layer );
    ZONE*        zone = nullptr;
    bool         keepout = ( p.layer == EAGLE_LAYER::TRESTRICT
                          || p.layer == EAGLE_LAYER::BRESTRICT
                          || p.layer == EAGLE_LAYER::VRESTRICT );

    if( layer == UNDEFINED_LAYER )
    {
        wxLogMessage( wxString::Format( _( "Ignoring a polygon since Eagle layer '%s' (%d) "
                                           "was not mapped" ),
                                        eagle_layer_name( p.layer ), p.layer ) );
        return nullptr;
    }

    if( !IsCopperLayer( layer ) && !keepout )
        return nullptr;

    // use a "netcode = 0" type ZONE:
    zone = new ZONE( m_board );
    m_board->Add( zone, ADD_MODE::APPEND );

    if( !keepout )
        zone->SetLayer( layer );
    else
        setKeepoutSettingsToZone( zone, p.layer );

    // Get the first vertex and iterate
    wxXmlNode* vertex = aPolyNode->GetChildren();
    std::vector<EVERTEX> vertices;

    // Create a circular vector of vertices
    // The "curve" parameter indicates a curve from the current
    // to the next vertex, so we keep the first at the end as well
    // to allow the curve to link back
    while( vertex )
    {
        if( vertex->GetName() == "vertex" )
            vertices.emplace_back( vertex );

        vertex = vertex->GetNext();
    }

    vertices.push_back( vertices[0] );

    SHAPE_POLY_SET polygon;
    polygon.NewOutline();

    for( size_t i = 0; i < vertices.size() - 1; i++ )
    {
        EVERTEX v1 = vertices[i];

        // Append the corner
        polygon.Append( kicad_x( v1.x ), kicad_y( v1.y ) );

        if( v1.curve )
        {
            EVERTEX v2 = vertices[i + 1];
            wxPoint center = ConvertArcCenter( wxPoint( kicad_x( v1.x ), kicad_y( v1.y ) ),
                                               wxPoint( kicad_x( v2.x ), kicad_y( v2.y ) ),
                                               *v1.curve );
            double angle = DEG2RAD( *v1.curve );
            double  end_angle = atan2( kicad_y( v2.y ) - center.y, kicad_x( v2.x ) - center.x );
            double  radius = sqrt( pow( center.x - kicad_x( v1.x ), 2 )
                                  + pow( center.y - kicad_y( v1.y ), 2 ) );

            int segCount = GetArcToSegmentCount( KiROUND( radius ), ARC_HIGH_DEF, *v1.curve );
            double delta_angle = angle / segCount;

            for( double a = end_angle + angle; fabs( a - end_angle ) > fabs( delta_angle );
                 a -= delta_angle )
            {
                polygon.Append( KiROUND( radius * cos( a ) ) + center.x,
                                KiROUND( radius * sin( a ) ) + center.y );
            }
        }
    }

    // Eagle traces the zone such that half of the pen width is outside the polygon.
    // We trace the zone such that the copper is completely inside.
    if( p.width.ToPcbUnits() > 0 )
    {
        polygon.Inflate( p.width.ToPcbUnits() / 2, 32, SHAPE_POLY_SET::ALLOW_ACUTE_CORNERS );
        polygon.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    }

    zone->AddPolygon( polygon.COutline( 0 ) );

    // If the pour is a cutout it needs to be set to a keepout
    if( p.pour == EPOLYGON::CUTOUT )
    {
        zone->SetIsRuleArea( true );
        zone->SetDoNotAllowVias( false );
        zone->SetDoNotAllowTracks( false );
        zone->SetDoNotAllowPads( false );
        zone->SetDoNotAllowFootprints( false );
        zone->SetDoNotAllowCopperPour( true );
        zone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::NO_HATCH );
    }
    else if( p.pour == EPOLYGON::HATCH )
    {
        int spacing = p.spacing ? p.spacing->ToPcbUnits() : 50 * IU_PER_MILS;

        zone->SetFillMode( ZONE_FILL_MODE::HATCH_PATTERN );
        zone->SetHatchThickness( p.width.ToPcbUnits() );
        zone->SetHatchGap( spacing - p.width.ToPcbUnits() );
        zone->SetHatchOrientation( 0 );
    }

    // We divide the thickness by half because we are tracing _inside_ the zone outline
    // This means the radius of curvature will be twice the size for an equivalent EAGLE zone
    zone->SetMinThickness( std::max<int>( ZONE_THICKNESS_MIN_VALUE_MIL * IU_PER_MILS,
                                          p.width.ToPcbUnits() / 2 ) );

    if( p.isolate )
        zone->SetLocalClearance( p.isolate->ToPcbUnits() );
    else
        zone->SetLocalClearance( 1 ); // @todo: set minimum clearance value based on board settings

    // missing == yes per DTD.
    bool thermals = !p.thermals || *p.thermals;
    zone->SetPadConnection( thermals ? ZONE_CONNECTION::THERMAL : ZONE_CONNECTION::FULL );

    if( thermals )
    {
        // FIXME: eagle calculates dimensions for thermal spokes
        //        based on what the zone is connecting to.
        //        (i.e. width of spoke is half of the smaller side of an smd pad)
        //        This is a basic workaround
        zone->SetThermalReliefGap( p.width.ToPcbUnits() + 50000 ); // 50000nm == 0.05mm
        zone->SetThermalReliefSpokeWidth( p.width.ToPcbUnits() + 50000 );
    }

    int rank = p.rank ? (p.max_priority - *p.rank) : p.max_priority;
    zone->SetPriority( rank );

    return zone;
}


void EAGLE_PLUGIN::orientFootprintAndText( FOOTPRINT* aFootprint, const EELEMENT& e,
                                           const EATTR* aNameAttr, const EATTR* aValueAttr )
{
    if( e.rot )
    {
        if( e.rot->mirror )
        {
            double orientation = e.rot->degrees + 180.0;
            aFootprint->SetOrientation( orientation * 10 );
            aFootprint->Flip( aFootprint->GetPosition(), false );
        }
        else
        {
            aFootprint->SetOrientation( e.rot->degrees * 10 );
        }
    }

    orientFPText( aFootprint, e, &aFootprint->Reference(), aNameAttr );
    orientFPText( aFootprint, e, &aFootprint->Value(), aValueAttr );
}


void EAGLE_PLUGIN::orientFPText( FOOTPRINT* aFootprint, const EELEMENT& e, FP_TEXT* aFPText,
                                 const EATTR* aAttr )
{
    // Smashed part ?
    if( aAttr )
    {
        // Yes
        const EATTR& a = *aAttr;

        if( a.value )
        {
            aFPText->SetText( FROM_UTF8( a.value->c_str() ) );
        }

        if( a.x && a.y )    // OPT
        {
            wxPoint pos( kicad_x( *a.x ), kicad_y( *a.y ) );
            aFPText->SetTextPos( pos );
        }

        // Even though size and ratio are both optional, I am not seeing
        // a case where ratio is present but size is not.
        double  ratio = 8;
        wxSize  fontz = aFPText->GetTextSize();
        int     textThickness = KiROUND( fontz.y * ratio / 100 );

        aFPText->SetTextThickness( textThickness );
        if( a.size )
        {
            fontz = kicad_fontz( *a.size, textThickness );
            aFPText->SetTextSize( fontz );

            if( a.ratio )
                ratio = *a.ratio;
        }



        int align = ETEXT::BOTTOM_LEFT;     // bottom-left is eagle default

        if( a.align )
            align = a.align;

        // The "rot" in a EATTR seems to be assumed to be zero if it is not
        // present, and this zero rotation becomes an override to the
        // package's text field.  If they did not want zero, they specify
        // what they want explicitly.
        double  degrees  = a.rot ? a.rot->degrees : 0;
        double  orient;      // relative to parent

        int     sign = 1;
        bool    spin = false;

        if( a.rot )
        {
            spin = a.rot->spin;
            sign = a.rot->mirror ? -1 : 1;
            aFPText->SetMirrored( a.rot->mirror );
        }

        if( degrees == 90 || degrees == 0 || spin )
        {
            orient = degrees - aFootprint->GetOrientation() / 10;
            aFPText->SetTextAngle( sign * orient * 10 );
        }
        else if( degrees == 180 )
        {
            orient = 0 - aFootprint->GetOrientation() / 10;
            aFPText->SetTextAngle( sign * orient * 10 );
            align = -align;
        }
        else if( degrees == 270 )
        {
            orient = 90 - aFootprint->GetOrientation() / 10;
            align = -align;
            aFPText->SetTextAngle( sign * orient * 10 );
        }
        else
        {
            orient = 90 - degrees - aFootprint->GetOrientation() / 10;
            aFPText->SetTextAngle( sign * orient * 10 );
        }

        switch( align )
        {
        case ETEXT::TOP_RIGHT:
            aFPText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
            aFPText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
            break;

        case ETEXT::BOTTOM_LEFT:
            aFPText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            aFPText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            break;

        case ETEXT::TOP_LEFT:
            aFPText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            aFPText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
            break;

        case ETEXT::BOTTOM_RIGHT:
            aFPText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
            aFPText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            break;

        case ETEXT::TOP_CENTER:
            aFPText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
            aFPText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
            break;

        case ETEXT::BOTTOM_CENTER:
            aFPText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
            aFPText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            break;

        case ETEXT::CENTER:
            aFPText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
            aFPText->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
            break;

        case ETEXT::CENTER_LEFT:
            aFPText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            aFPText->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
            break;

        case ETEXT::CENTER_RIGHT:
            aFPText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
            aFPText->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
            break;

        default:
            ;
        }
    }
    else
    {
        // Part is not smash so use Lib default for NAME/VALUE // the text is per the original
        // package, sans <attribute>.
        double degrees = ( aFPText->GetTextAngle() + aFootprint->GetOrientation() ) / 10;

        // @todo there are a few more cases than these to contend with:
        if( ( !aFPText->IsMirrored() && ( abs( degrees ) == 180 || abs( degrees ) == 270 ) )
         || ( aFPText->IsMirrored() && ( degrees == 360 ) ) )
        {
            // ETEXT::TOP_RIGHT:
            aFPText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
            aFPText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        }
    }
}


FOOTPRINT* EAGLE_PLUGIN::makeFootprint( wxXmlNode* aPackage, const wxString& aPkgName )
{
    std::unique_ptr<FOOTPRINT> m = std::make_unique<FOOTPRINT>( m_board );

    LIB_ID fpID;
    fpID.Parse( aPkgName, true );
    m->SetFPID( fpID );

    // Get the first package item and iterate
    wxXmlNode* packageItem = aPackage->GetChildren();

    while( packageItem )
    {
        const wxString& itemName = packageItem->GetName();

        if( itemName == "description" )
            m->SetDescription( FROM_UTF8( packageItem->GetNodeContent().c_str() ) );
        else if( itemName == "wire" )
            packageWire( m.get(), packageItem );
        else if( itemName == "pad" )
            packagePad( m.get(), packageItem );
        else if( itemName == "text" )
            packageText( m.get(), packageItem );
        else if( itemName == "rectangle" )
            packageRectangle( m.get(), packageItem );
        else if( itemName == "polygon" )
            packagePolygon( m.get(), packageItem );
        else if( itemName == "circle" )
            packageCircle( m.get(), packageItem );
        else if( itemName == "hole" )
            packageHole( m.get(), packageItem, false );
        else if( itemName == "smd" )
            packageSMD( m.get(), packageItem );

        packageItem = packageItem->GetNext();
    }

    return m.release();
}


void EAGLE_PLUGIN::packageWire( FOOTPRINT* aFootprint, wxXmlNode* aTree ) const
{
    EWIRE        w( aTree );
    PCB_LAYER_ID layer = kicad_layer( w.layer );
    wxPoint      start( kicad_x( w.x1 ), kicad_y( w.y1 ) );
    wxPoint      end(   kicad_x( w.x2 ), kicad_y( w.y2 ) );
    int          width = w.width.ToPcbUnits();

    if( layer == UNDEFINED_LAYER )
    {
        wxLogMessage( wxString::Format( _( "Ignoring a wire since Eagle layer '%s' (%d) "
                                           "was not mapped" ),
                                        eagle_layer_name( w.layer ), w.layer ) );
        return;
    }

    // KiCad cannot handle zero or negative line widths which apparently have meaning in Eagle.
    if( width <= 0 )
    {
        BOARD* board = aFootprint->GetBoard();

        if( board )
        {
            width = board->GetDesignSettings().GetLineThickness( layer );
        }
        else
        {
            // When loading footprint libraries, there is no board so use the default KiCad
            // line widths.
            switch( layer )
            {
            case Edge_Cuts: width = Millimeter2iu( DEFAULT_EDGE_WIDTH );      break;

            case F_SilkS:
            case B_SilkS:   width = Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ); break;

            case F_CrtYd:
            case B_CrtYd:   width = Millimeter2iu( DEFAULT_COURTYARD_WIDTH ); break;

            default:        width = Millimeter2iu( DEFAULT_LINE_WIDTH );      break;
            }
        }
    }

    // FIXME: the cap attribute is ignored because KiCad can't create lines
    //        with flat ends.
    FP_SHAPE* dwg;

    if( !w.curve )
    {
        dwg = new FP_SHAPE( aFootprint, SHAPE_T::SEGMENT );

        dwg->SetStart0( start );
        dwg->SetEnd0( end );
    }
    else
    {
        dwg = new FP_SHAPE( aFootprint, SHAPE_T::ARC );
        wxPoint center = ConvertArcCenter( start, end, *w.curve );

        dwg->SetCenter0( center );
        dwg->SetStart0( start );
        dwg->SetArcAngleAndEnd0( *w.curve * -10.0, true ); // KiCad rotates the other way
    }

    dwg->SetLayer( layer );
    dwg->SetWidth( width );
    dwg->SetDrawCoord();

    aFootprint->Add( dwg );
}


void EAGLE_PLUGIN::packagePad( FOOTPRINT* aFootprint, wxXmlNode* aTree )
{
    // this is thru hole technology here, no SMDs
    EPAD e( aTree );
    int shape = EPAD::UNDEF;
    int eagleDrillz = e.drill.ToPcbUnits();

    PAD* pad = new PAD( aFootprint );
    aFootprint->Add( pad );
    transferPad( e, pad );

    if( e.first && *e.first && m_rules->psFirst != EPAD::UNDEF )
        shape = m_rules->psFirst;
    else if( aFootprint->GetLayer() == F_Cu && m_rules->psTop != EPAD::UNDEF )
        shape = m_rules->psTop;
    else if( aFootprint->GetLayer() == B_Cu && m_rules->psBottom != EPAD::UNDEF )
        shape = m_rules->psBottom;

    pad->SetDrillSize( wxSize( eagleDrillz, eagleDrillz ) );
    pad->SetLayerSet( LSET::AllCuMask() );

    if( eagleDrillz < m_min_hole )
        m_min_hole = eagleDrillz;

    // Solder mask
    if( !e.stop || *e.stop == true )         // enabled by default
        pad->SetLayerSet( pad->GetLayerSet().set( B_Mask ).set( F_Mask ) );

    if( shape == EPAD::ROUND || shape == EPAD::SQUARE || shape == EPAD::OCTAGON )
        e.shape = shape;

    if( e.shape )
    {
        switch( *e.shape )
        {
        case EPAD::ROUND:
            pad->SetShape( PAD_SHAPE::CIRCLE );
            break;

        case EPAD::OCTAGON:
            // no KiCad octagonal pad shape, use PAD_CIRCLE for now.
            // pad->SetShape( PAD_OCTAGON );
            wxASSERT( pad->GetShape() == PAD_SHAPE::CIRCLE );    // verify set in PAD constructor
            pad->SetShape( PAD_SHAPE::CHAMFERED_RECT );
            pad->SetChamferPositions( RECT_CHAMFER_ALL );
            pad->SetChamferRectRatio( 0.25 );
            break;

        case EPAD::LONG:
            pad->SetShape( PAD_SHAPE::OVAL );
            break;

        case EPAD::SQUARE:
            pad->SetShape( PAD_SHAPE::RECT );
            break;

        case EPAD::OFFSET:
            pad->SetShape( PAD_SHAPE::OVAL );
            break;
        }
    }
    else
    {
        // if shape is not present, our default is circle and that matches their default "round"
    }

    if( e.diameter && e.diameter->value > 0 )
    {
        int diameter = e.diameter->ToPcbUnits();
        pad->SetSize( wxSize( diameter, diameter ) );
    }
    else
    {
        double drillz  = pad->GetDrillSize().x;
        double annulus = drillz * m_rules->rvPadTop;   // copper annulus, eagle "restring"
        annulus = eagleClamp( m_rules->rlMinPadTop, annulus, m_rules->rlMaxPadTop );
        int diameter = KiROUND( drillz + 2 * annulus );
        pad->SetSize( wxSize( KiROUND( diameter ), KiROUND( diameter ) ) );
    }

    if( pad->GetShape() == PAD_SHAPE::OVAL )
    {
        // The Eagle "long" pad is wider than it is tall,
        // m_elongation is percent elongation
        wxSize sz = pad->GetSize();
        sz.x = ( sz.x * ( 100 + m_rules->psElongationLong ) ) / 100;
        pad->SetSize( sz );

        if( e.shape && *e.shape == EPAD::OFFSET )
        {
            int offset = KiROUND( ( sz.x - sz.y ) / 2.0 );
            pad->SetOffset( wxPoint( offset, 0 ) );
        }
    }

    if( e.rot )
    {
        pad->SetOrientation( e.rot->degrees * 10 );
    }
}


void EAGLE_PLUGIN::packageText( FOOTPRINT* aFootprint, wxXmlNode* aTree ) const
{
    ETEXT        t( aTree );
    PCB_LAYER_ID layer = kicad_layer( t.layer );

    if( layer == UNDEFINED_LAYER )
    {
        wxLogMessage( wxString::Format( _( "Ignoring a text since Eagle layer '%s' (%d) "
                                           "was not mapped" ),
                                        eagle_layer_name( t.layer ), t.layer ) );
        return;
    }

    FP_TEXT* txt;

    if( t.text == ">NAME" || t.text == ">name" )
        txt = &aFootprint->Reference();
    else if( t.text == ">VALUE" || t.text == ">value" )
        txt = &aFootprint->Value();
    else
    {
        // FIXME: graphical text items are rotated for some reason.
        txt = new FP_TEXT( aFootprint );
        aFootprint->Add( txt );
    }

    txt->SetText( FROM_UTF8( t.text.c_str() ) );

    wxPoint pos( kicad_x( t.x ), kicad_y( t.y ) );

    txt->SetTextPos( pos );
    txt->SetPos0( pos - aFootprint->GetPosition() );

    txt->SetLayer( layer );

    double ratio = t.ratio ? *t.ratio : 8;  // DTD says 8 is default
    int textThickness = KiROUND( t.size.ToPcbUnits() * ratio / 100 );

    txt->SetTextThickness( textThickness );
    txt->SetTextSize( kicad_fontz( t.size, textThickness ) );

    int align = t.align ? *t.align : ETEXT::BOTTOM_LEFT;  // bottom-left is eagle default

    // An eagle package is never rotated, the DTD does not allow it.
    // angle -= aFootprint->GetOrienation();

    if( t.rot )
    {
        int sign = t.rot->mirror ? -1 : 1;
        txt->SetMirrored( t.rot->mirror );

        double degrees = t.rot->degrees;

        if( degrees == 90 || t.rot->spin )
        {
            txt->SetTextAngle( sign * degrees * 10 );
        }
        else if( degrees == 180 )
        {
            align = ETEXT::TOP_RIGHT;
        }
        else if( degrees == 270 )
        {
            align = ETEXT::TOP_RIGHT;
            txt->SetTextAngle( sign * 90 * 10 );
        }
    }

    switch( align )
    {
    case ETEXT::CENTER:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case ETEXT::CENTER_LEFT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case ETEXT::CENTER_RIGHT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case ETEXT::TOP_CENTER:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;

    case ETEXT::TOP_LEFT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;

    case ETEXT::TOP_RIGHT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;

    case ETEXT::BOTTOM_CENTER:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case ETEXT::BOTTOM_LEFT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case ETEXT::BOTTOM_RIGHT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;
    }
}


void EAGLE_PLUGIN::packageRectangle( FOOTPRINT* aFootprint, wxXmlNode* aTree ) const
{
    ERECT r( aTree );

    if( r.layer == EAGLE_LAYER::TRESTRICT || r.layer == EAGLE_LAYER::BRESTRICT
            || r.layer == EAGLE_LAYER::VRESTRICT )
    {
        FP_ZONE* zone = new FP_ZONE( aFootprint );
        aFootprint->Add( zone, ADD_MODE::APPEND );

        setKeepoutSettingsToZone( zone, r.layer );

        const int outlineIdx = -1; // this is the id of the copper zone main outline
        zone->AppendCorner( wxPoint( kicad_x( r.x1 ), kicad_y( r.y1 ) ), outlineIdx );
        zone->AppendCorner( wxPoint( kicad_x( r.x2 ), kicad_y( r.y1 ) ), outlineIdx );
        zone->AppendCorner( wxPoint( kicad_x( r.x2 ), kicad_y( r.y2 ) ), outlineIdx );
        zone->AppendCorner( wxPoint( kicad_x( r.x1 ), kicad_y( r.y2 ) ), outlineIdx );

        if( r.rot )
        {
            wxPoint center( ( kicad_x( r.x1 ) + kicad_x( r.x2 ) ) / 2,
                    ( kicad_y( r.y1 ) + kicad_y( r.y2 ) ) / 2 );
            zone->Rotate( center, r.rot->degrees * 10 );
        }

        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                     ZONE::GetDefaultHatchPitch(), true );
    }
    else
    {
        PCB_LAYER_ID layer = kicad_layer( r.layer );

        if( layer == UNDEFINED_LAYER )
        {
            wxLogMessage( wxString::Format( _( "Ignoring a rectangle since Eagle layer '%s' (%d) "
                                               "was not mapped" ),
                                            eagle_layer_name( r.layer ), r.layer ) );
            return;
        }

        FP_SHAPE* dwg = new FP_SHAPE( aFootprint, SHAPE_T::POLY );

        aFootprint->Add( dwg );

        dwg->SetLayer( layer );
        dwg->SetWidth( 0 );
        dwg->SetFilled( true );

        std::vector<wxPoint> pts;

        wxPoint start( wxPoint( kicad_x( r.x1 ), kicad_y( r.y1 ) ) );
        wxPoint end( wxPoint( kicad_x( r.x1 ), kicad_y( r.y2 ) ) );

        pts.push_back( start );
        pts.emplace_back( kicad_x( r.x2 ), kicad_y( r.y1 ) );
        pts.emplace_back( kicad_x( r.x2 ), kicad_y( r.y2 ) );
        pts.push_back( end );

        dwg->SetPolyPoints( pts );

        dwg->SetStart0( start );
        dwg->SetEnd0( end );

        if( r.rot )
            dwg->Rotate( dwg->GetCenter(), r.rot->degrees * 10 );
    }
}


void EAGLE_PLUGIN::packagePolygon( FOOTPRINT* aFootprint, wxXmlNode* aTree ) const
{
    EPOLYGON      p( aTree );

    std::vector<wxPoint> pts;

    // Get the first vertex and iterate
    wxXmlNode* vertex = aTree->GetChildren();
    std::vector<EVERTEX> vertices;

    // Create a circular vector of vertices
    // The "curve" parameter indicates a curve from the current
    // to the next vertex, so we keep the first at the end as well
    // to allow the curve to link back
    while( vertex )
    {
        if( vertex->GetName() == "vertex" )
            vertices.emplace_back( vertex );

        vertex = vertex->GetNext();
    }

    vertices.push_back( vertices[0] );

    for( size_t i = 0; i < vertices.size() - 1; i++ )
    {
        EVERTEX v1 = vertices[i];

        // Append the corner
        pts.emplace_back( kicad_x( v1.x ), kicad_y( v1.y ) );

        if( v1.curve )
        {
            EVERTEX v2 = vertices[i + 1];
            wxPoint center =
                    ConvertArcCenter( wxPoint( kicad_x( v1.x ), kicad_y( v1.y ) ),
                                      wxPoint( kicad_x( v2.x ), kicad_y( v2.y ) ), *v1.curve );
            double angle = DEG2RAD( *v1.curve );
            double end_angle = atan2( kicad_y( v2.y ) - center.y, kicad_x( v2.x ) - center.x );
            double radius = sqrt( pow( center.x - kicad_x( v1.x ), 2 )
                                  + pow( center.y - kicad_y( v1.y ), 2 ) );

            // Don't allow a zero-radius curve
            if( KiROUND( radius ) == 0 )
                radius = 1.0;

            int segCount = GetArcToSegmentCount( KiROUND( radius ), ARC_HIGH_DEF, *v1.curve );
            double delta = angle / segCount;

            for( double a = end_angle + angle; fabs( a - end_angle ) > fabs( delta ); a -= delta )
            {
                pts.push_back( wxPoint( KiROUND( radius * cos( a ) ),
                                        KiROUND( radius * sin( a ) ) ) + center );
            }
        }
    }

    if( p.layer == EAGLE_LAYER::TRESTRICT
     || p.layer == EAGLE_LAYER::BRESTRICT
     || p.layer == EAGLE_LAYER::VRESTRICT )
    {
        FP_ZONE* zone = new FP_ZONE( aFootprint );
        aFootprint->Add( zone, ADD_MODE::APPEND );

        setKeepoutSettingsToZone( zone, p.layer );

        SHAPE_LINE_CHAIN outline( pts );
        outline.SetClosed( true );
        zone->Outline()->AddOutline( outline );

        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                     ZONE::GetDefaultHatchPitch(), true );
    }
    else
    {
        PCB_LAYER_ID layer = kicad_layer( p.layer );

        if( layer == UNDEFINED_LAYER )
        {
            wxLogMessage( wxString::Format( _( "Ignoring a polygon since Eagle layer '%s' (%d) "
                                               "was not mapped" ),
                                            eagle_layer_name( p.layer ), p.layer ) );
            return;
        }

        FP_SHAPE* dwg = new FP_SHAPE( aFootprint, SHAPE_T::POLY );

        aFootprint->Add( dwg );

        dwg->SetWidth( 0 ); // it's filled, no need for boundary width
        dwg->SetFilled( true );
        dwg->SetLayer( layer );

        dwg->SetPolyPoints( pts );
        dwg->SetStart0( *pts.begin() );
        dwg->SetEnd0( pts.back() );
        dwg->SetDrawCoord();
        dwg->GetPolyShape().Inflate( p.width.ToPcbUnits() / 2, 32,
                                     SHAPE_POLY_SET::ALLOW_ACUTE_CORNERS );
    }
}


void EAGLE_PLUGIN::packageCircle( FOOTPRINT* aFootprint, wxXmlNode* aTree ) const
{
    ECIRCLE e( aTree );

    int width  = e.width.ToPcbUnits();
    int radius = e.radius.ToPcbUnits();

    if( e.layer == EAGLE_LAYER::TRESTRICT
     || e.layer == EAGLE_LAYER::BRESTRICT
     || e.layer == EAGLE_LAYER::VRESTRICT )
    {
        FP_ZONE* zone = new FP_ZONE( aFootprint );
        aFootprint->Add( zone, ADD_MODE::APPEND );

        setKeepoutSettingsToZone( zone, e.layer );

        // approximate circle as polygon with a edge every 10 degree
        wxPoint center( kicad_x( e.x ), kicad_y( e.y ) );
        int     outlineRadius = radius + ( width / 2 );

        for( int angle = 0; angle < 360; angle += 10 )
        {
            wxPoint rotatedPoint( outlineRadius, 0 );
            RotatePoint( &rotatedPoint, angle * 10. );
            zone->AppendCorner( center + rotatedPoint, -1 );
        }

        if( width > 0 )
        {
            zone->NewHole();
            int innerRadius = radius - ( width / 2 );

            for( int angle = 0; angle < 360; angle += 10 )
            {
                wxPoint rotatedPoint( innerRadius, 0 );
                RotatePoint( &rotatedPoint, angle * 10. );
                zone->AppendCorner( center + rotatedPoint, 0 );
            }
        }

        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                     ZONE::GetDefaultHatchPitch(), true );
    }
    else
    {
        PCB_LAYER_ID layer = kicad_layer( e.layer );

        if( layer == UNDEFINED_LAYER )
        {
            wxLogMessage( wxString::Format( _( "Ignoring a circle since Eagle layer '%s' (%d) "
                                               "was not mapped" ),
                                            eagle_layer_name( e.layer ), e.layer ) );
            return;
        }

        FP_SHAPE* gr = new FP_SHAPE( aFootprint, SHAPE_T::CIRCLE );

        // width == 0 means filled circle
        if( width <= 0 )
        {
            width  = radius;
            radius = radius / 2;
            gr->SetFilled( true );
        }

        aFootprint->Add( gr );
        gr->SetWidth( width );

        switch( (int) layer )
        {
        case UNDEFINED_LAYER:
            layer = Cmts_User;
            break;
        default:
            break;
        }

        gr->SetLayer( layer );
        gr->SetStart0( wxPoint( kicad_x( e.x ), kicad_y( e.y ) ) );
        gr->SetEnd0( wxPoint( kicad_x( e.x ) + radius, kicad_y( e.y ) ) );
        gr->SetDrawCoord();
    }
}


void EAGLE_PLUGIN::packageHole( FOOTPRINT* aFootprint, wxXmlNode* aTree, bool aCenter ) const
{
    EHOLE   e( aTree );

    if( e.drill.value == 0 )
        return;

    // we add a PAD_ATTRIB::NPTH pad to this footprint.
    PAD* pad = new PAD( aFootprint );
    aFootprint->Add( pad );

    pad->SetShape( PAD_SHAPE::CIRCLE );
    pad->SetAttribute( PAD_ATTRIB::NPTH );

    // Mechanical purpose only:
    // no offset, no net name, no pad name allowed
    // pad->SetOffset( wxPoint( 0, 0 ) );
    // pad->SetNumber( wxEmptyString );

    wxPoint padpos( kicad_x( e.x ), kicad_y( e.y ) );

    if( aCenter )
    {
        pad->SetPos0( wxPoint( 0, 0 ) );
        aFootprint->SetPosition( padpos );
        pad->SetPosition( padpos );
    }
    else
    {
        pad->SetPos0( padpos );
        pad->SetPosition( padpos + aFootprint->GetPosition() );
    }

    wxSize  sz( e.drill.ToPcbUnits(), e.drill.ToPcbUnits() );

    pad->SetDrillSize( sz );
    pad->SetSize( sz );

    pad->SetLayerSet( LSET::AllCuMask().set( B_Mask ).set( F_Mask ) );
}


void EAGLE_PLUGIN::packageSMD( FOOTPRINT* aFootprint, wxXmlNode* aTree ) const
{
    ESMD e( aTree );
    PCB_LAYER_ID layer = kicad_layer( e.layer );

    if( !IsCopperLayer( layer ) || e.dx.value == 0 || e.dy.value == 0 )
        return;

    PAD* pad = new PAD( aFootprint );
    aFootprint->Add( pad );
    transferPad( e, pad );

    pad->SetShape( PAD_SHAPE::RECT );
    pad->SetAttribute( PAD_ATTRIB::SMD );

    wxSize padSize( e.dx.ToPcbUnits(), e.dy.ToPcbUnits() );
    pad->SetSize( padSize );
    pad->SetLayer( layer );

    const LSET front( 3, F_Cu, F_Paste, F_Mask );
    const LSET back(  3, B_Cu, B_Paste, B_Mask );

    if( layer == F_Cu )
        pad->SetLayerSet( front );
    else if( layer == B_Cu )
        pad->SetLayerSet( back );

    int minPadSize = std::min( padSize.x, padSize.y );

    // Rounded rectangle pads
    int roundRadius =
            eagleClamp( m_rules->srMinRoundness * 2, (int) ( minPadSize * m_rules->srRoundness ),
                        m_rules->srMaxRoundness * 2 );

    if( e.roundness || roundRadius > 0 )
    {
        double roundRatio = (double) roundRadius / minPadSize / 2.0;

        // Eagle uses a different definition of roundness, hence division by 200
        if( e.roundness )
            roundRatio = std::fmax( *e.roundness / 200.0, roundRatio );

        pad->SetShape( PAD_SHAPE::ROUNDRECT );
        pad->SetRoundRectRadiusRatio( roundRatio );
    }

    if( e.rot )
        pad->SetOrientation( e.rot->degrees * 10 );

    pad->SetLocalSolderPasteMargin( -eagleClamp( m_rules->mlMinCreamFrame,
                                                 (int) ( m_rules->mvCreamFrame * minPadSize ),
                                                 m_rules->mlMaxCreamFrame ) );

    // Solder mask
    if( e.stop && *e.stop == false )         // enabled by default
    {
        if( layer == F_Cu )
            pad->SetLayerSet( pad->GetLayerSet().set( F_Mask, false ) );
        else if( layer == B_Cu )
            pad->SetLayerSet( pad->GetLayerSet().set( B_Mask, false ) );
    }

    // Solder paste (only for SMD pads)
    if( e.cream && *e.cream == false )         // enabled by default
    {
        if( layer == F_Cu )
            pad->SetLayerSet( pad->GetLayerSet().set( F_Paste, false ) );
        else if( layer == B_Cu )
            pad->SetLayerSet( pad->GetLayerSet().set( B_Paste, false ) );
    }
}


void EAGLE_PLUGIN::transferPad( const EPAD_COMMON& aEaglePad, PAD* aPad ) const
{
    aPad->SetNumber( FROM_UTF8( aEaglePad.name.c_str() ) );

    // pad's "Position" is not relative to the footprint's,
    // whereas Pos0 is relative to the footprint's but is the unrotated coordinate.
    wxPoint padPos( kicad_x( aEaglePad.x ), kicad_y( aEaglePad.y ) );
    aPad->SetPos0( padPos );

    // Solder mask
    const wxSize& padSize( aPad->GetSize() );

    aPad->SetLocalSolderMaskMargin(
            eagleClamp( m_rules->mlMinStopFrame,
                        (int) ( m_rules->mvStopFrame * std::min( padSize.x, padSize.y ) ),
                        m_rules->mlMaxStopFrame ) );

    // Solid connection to copper zones
    if( aEaglePad.thermals && !*aEaglePad.thermals )
        aPad->SetZoneConnection( ZONE_CONNECTION::FULL );

    FOOTPRINT* footprint = aPad->GetParent();
    wxCHECK( footprint, /* void */ );
    RotatePoint( &padPos, footprint->GetOrientation() );
    aPad->SetPosition( padPos + footprint->GetPosition() );
}


void EAGLE_PLUGIN::deleteTemplates()
{
    for( auto& t : m_templates )
        delete t.second;

    m_templates.clear();
}


void EAGLE_PLUGIN::loadClasses( wxXmlNode* aClasses )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    m_xpath->push( "classes.class", "number" );

    std::vector<ECLASS> eClasses;
    wxXmlNode*          classNode = aClasses->GetChildren();

    while( classNode )
    {
        checkpoint();

        ECLASS      eClass( classNode );
        NETCLASSPTR netclass;

        if( eClass.name.CmpNoCase( "default" ) == 0 )
        {
            netclass = bds.GetNetClasses().GetDefault();
        }
        else
        {
            netclass.reset( new NETCLASS( eClass.name ) );
            m_board->GetDesignSettings().GetNetClasses().Add( netclass );
        }

        netclass->SetTrackWidth( INT_MAX );
        netclass->SetViaDiameter( INT_MAX );
        netclass->SetViaDrill( INT_MAX );

        eClasses.emplace_back( eClass );
        m_classMap[ eClass.number ] = netclass;

        // Get next class
        classNode = classNode->GetNext();
    }

    m_customRules = "(version 1)";

    for( ECLASS& eClass : eClasses )
    {
        for( std::pair<const wxString&, ECOORD> entry : eClass.clearanceMap )
        {
            wxString rule;
            rule.Printf( "(rule \"class %s:%s\"\n"
                         "  (condition \"A.NetClass == '%s' && B.NetClass == '%s'\")\n"
                         "  (constraint clearance (min %smm)))\n",
                         eClass.number,
                         entry.first,
                         eClass.name,
                         m_classMap[ entry.first ]->GetName(),
                         StringFromValue( EDA_UNITS::MILLIMETRES, entry.second.ToPcbUnits() ) );

            m_customRules += "\n" + rule;
        }
    }

    m_xpath->pop(); // "classes.class"
}


void EAGLE_PLUGIN::loadSignals( wxXmlNode* aSignals )
{
    ZONES zones;      // per net
    int   netCode = 1;

    m_xpath->push( "signals.signal", "name" );

    // Get the first signal and iterate
    wxXmlNode* net = aSignals->GetChildren();

    while( net )
    {
        checkpoint();

        bool    sawPad = false;

        zones.clear();

        const wxString& netName = escapeName( net->GetAttribute( "name" ) );
        NETINFO_ITEM*   netInfo = new NETINFO_ITEM( m_board, netName, netCode );
        NETCLASSPTR     netclass;

        if( net->HasAttribute( "class" ) )
        {
            netclass = m_classMap[ net->GetAttribute( "class" ) ];

            netclass->Add( netName );
            netInfo->SetNetClass( netclass );
        }

        m_board->Add( netInfo );

        m_xpath->Value( netName.c_str() );

        // Get the first net item and iterate
        wxXmlNode* netItem = net->GetChildren();

        // (contactref | polygon | wire | via)*
        while( netItem )
        {
            const wxString& itemName = netItem->GetName();

            if( itemName == "wire" )
            {
                m_xpath->push( "wire" );

                EWIRE        w( netItem );
                PCB_LAYER_ID layer = kicad_layer( w.layer );

                if( IsCopperLayer( layer ) )
                {
                    wxPoint start( kicad_x( w.x1 ), kicad_y( w.y1 ) );
                    double angle = 0.0;
                    double end_angle = 0.0;
                    double radius = 0.0;
                    double delta_angle = 0.0;
                    wxPoint center;

                    int width = w.width.ToPcbUnits();

                    if( width < m_min_trace )
                        m_min_trace = width;

                    if( netclass && width < netclass->GetTrackWidth() )
                        netclass->SetTrackWidth( width );

                    if( w.curve )
                    {
                        center = ConvertArcCenter( wxPoint( kicad_x( w.x1 ), kicad_y( w.y1 ) ),
                                                   wxPoint( kicad_x( w.x2 ), kicad_y( w.y2 ) ),
                                                   *w.curve );

                        angle = DEG2RAD( *w.curve );

                        end_angle = atan2( kicad_y( w.y2 ) - center.y,
                                           kicad_x( w.x2 ) - center.x );

                        radius = sqrt( pow( center.x - kicad_x( w.x1 ), 2 ) +
                                       pow( center.y - kicad_y( w.y1 ), 2 ) );

                        int segs = GetArcToSegmentCount( KiROUND( radius ), ARC_HIGH_DEF,
                                                         *w.curve );
                        delta_angle = angle / segs;
                    }

                    while( fabs( angle ) > fabs( delta_angle ) )
                    {
                        wxASSERT( radius > 0.0 );
                        wxPoint end( KiROUND( radius * cos( end_angle + angle ) + center.x ),
                                     KiROUND( radius * sin( end_angle + angle ) + center.y ) );

                        PCB_TRACK*  t = new PCB_TRACK( m_board );

                        t->SetPosition( start );
                        t->SetEnd( end );
                        t->SetWidth( width );
                        t->SetLayer( layer );
                        t->SetNetCode( netCode );

                        m_board->Add( t );

                        start = end;
                        angle -= delta_angle;
                    }

                    PCB_TRACK*  t = new PCB_TRACK( m_board );

                    t->SetPosition( start );
                    t->SetEnd( wxPoint( kicad_x( w.x2 ), kicad_y( w.y2 ) ) );
                    t->SetWidth( width );
                    t->SetLayer( layer );
                    t->SetNetCode( netCode );

                    m_board->Add( t );
                }
                else
                {
                    // put non copper wires where the sun don't shine.
                }

                m_xpath->pop();
            }
            else if( itemName == "via" )
            {
                m_xpath->push( "via" );
                EVIA    v( netItem );

                if( v.layer_front_most > v.layer_back_most )
                    std::swap( v.layer_front_most, v.layer_back_most );

                PCB_LAYER_ID  layer_front_most = kicad_layer( v.layer_front_most );
                PCB_LAYER_ID  layer_back_most  = kicad_layer( v.layer_back_most );

                if( IsCopperLayer( layer_front_most ) && IsCopperLayer( layer_back_most )
                        && layer_front_most != layer_back_most )
                {
                    int      kidiam;
                    int      drillz = v.drill.ToPcbUnits();
                    PCB_VIA* via = new PCB_VIA( m_board );
                    m_board->Add( via );

                    if( v.diam )
                    {
                        kidiam = v.diam->ToPcbUnits();
                        via->SetWidth( kidiam );
                    }
                    else
                    {
                        double annulus = drillz * m_rules->rvViaOuter;  // eagle "restring"
                        annulus = eagleClamp( m_rules->rlMinViaOuter, annulus,
                                              m_rules->rlMaxViaOuter );
                        kidiam = KiROUND( drillz + 2 * annulus );
                        via->SetWidth( kidiam );
                    }

                    via->SetDrill( drillz );

                    // make sure the via diameter respects the restring rules

                    if( !v.diam || via->GetWidth() <= via->GetDrill() )
                    {
                        double annulus =
                                eagleClamp( m_rules->rlMinViaOuter,
                                            (double) ( via->GetWidth() / 2 - via->GetDrill() ),
                                            m_rules->rlMaxViaOuter );
                        via->SetWidth( drillz + 2 * annulus );
                    }

                    if( kidiam < m_min_via )
                        m_min_via = kidiam;

                    if( netclass && kidiam < netclass->GetViaDiameter() )
                        netclass->SetViaDiameter( kidiam );

                    if( drillz < m_min_hole )
                        m_min_hole = drillz;

                    if( netclass && drillz < netclass->GetViaDrill() )
                        netclass->SetViaDrill( drillz );

                    if( ( kidiam - drillz ) / 2 < m_min_annulus )
                        m_min_annulus = ( kidiam - drillz ) / 2;

                    if( layer_front_most == F_Cu && layer_back_most == B_Cu )
                    {
                        via->SetViaType( VIATYPE::THROUGH );
                    }
                    /// This is, at best, a guess.  Eagle doesn't seem to differentiate
                    /// between blind/buried vias that only go one layer and micro vias
                    /// so the user will need to clean up a bit
                    else if( v.layer_back_most - v.layer_front_most == 1 )
                    {
                        via->SetViaType( VIATYPE::MICROVIA );
                    }
                    else
                    {
                        via->SetViaType( VIATYPE::BLIND_BURIED );
                    }

                    wxPoint pos( kicad_x( v.x ), kicad_y( v.y ) );

                    via->SetLayerPair( layer_front_most, layer_back_most );
                    via->SetPosition( pos  );
                    via->SetEnd( pos );

                    via->SetNetCode( netCode );
                }

                m_xpath->pop();
            }

            else if( itemName == "contactref" )
            {
                m_xpath->push( "contactref" );
                // <contactref element="RN1" pad="7"/>

                const wxString& reference = netItem->GetAttribute( "element" );
                const wxString& pad       = netItem->GetAttribute( "pad" );
                wxString key = makeKey( reference, pad ) ;

                m_pads_to_nets[ key ] = ENET( netCode, netName );

                m_xpath->pop();

                sawPad = true;
            }

            else if( itemName == "polygon" )
            {
                m_xpath->push( "polygon" );
                auto* zone = loadPolygon( netItem );

                if( zone )
                {
                    zones.push_back( zone );

                    if( !zone->GetIsRuleArea() )
                        zone->SetNetCode( netCode );
                }

                m_xpath->pop();     // "polygon"
            }

            netItem = netItem->GetNext();
        }

        if( zones.size() && !sawPad )
        {
            // KiCad does not support an unconnected zone with its own non-zero netcode,
            // but only when assigned netcode = 0 w/o a name...
            for( ZONE* zone : zones )
                zone->SetNetCode( NETINFO_LIST::UNCONNECTED );

            // therefore omit this signal/net.
        }
        else
        {
            netCode++;
        }

        // Get next signal
        net = net->GetNext();
    }

    m_xpath->pop();     // "signals.signal"
}


std::map<wxString, PCB_LAYER_ID> EAGLE_PLUGIN::DefaultLayerMappingCallback(
            const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector )
{
    std::map<wxString, PCB_LAYER_ID> layer_map;

    for ( const INPUT_LAYER_DESC& layer : aInputLayerDescriptionVector )
    {
        PCB_LAYER_ID layerId = std::get<0>( defaultKicadLayer( eagle_layer_id( layer.Name ) ) );
        layer_map.emplace( layer.Name, layerId );
    }

    return layer_map;
}


void EAGLE_PLUGIN::mapEagleLayersToKicad()
{
    std::vector<INPUT_LAYER_DESC> inputDescs;

    for ( const std::pair<const int, ELAYER>& layerPair : m_eagleLayers )
    {
        const ELAYER& eLayer = layerPair.second;

        INPUT_LAYER_DESC layerDesc;
        std::tie( layerDesc.AutoMapLayer, layerDesc.PermittedLayers, layerDesc.Required ) =
                defaultKicadLayer( eLayer.number );

        if( layerDesc.AutoMapLayer == UNDEFINED_LAYER )
            continue; // Ignore unused copper layers

        layerDesc.Name = eLayer.name;

        inputDescs.push_back( layerDesc );
    }

    if( m_progressReporter && dynamic_cast<wxWindow*>( m_progressReporter ) )
        dynamic_cast<wxWindow*>( m_progressReporter )->Hide();

    m_layer_map = m_layer_mapping_handler( inputDescs );

    if( m_progressReporter && dynamic_cast<wxWindow*>( m_progressReporter ))
        dynamic_cast<wxWindow*>( m_progressReporter )->Show();
}


PCB_LAYER_ID EAGLE_PLUGIN::kicad_layer( int aEagleLayer ) const
{
    auto result = m_layer_map.find( eagle_layer_name( aEagleLayer ) );
    return result == m_layer_map.end() ? UNDEFINED_LAYER : result->second;
}


std::tuple<PCB_LAYER_ID, LSET, bool> EAGLE_PLUGIN::defaultKicadLayer( int aEagleLayer ) const
{
    // eagle copper layer:
    if( aEagleLayer >= 1 && aEagleLayer < int( arrayDim( m_cu_map ) ) )
    {
        LSET copperLayers;

        for( int copperLayer : m_cu_map )
        {
            if( copperLayer >= 0 )
                copperLayers[copperLayer] = true;
        }

        return { PCB_LAYER_ID( m_cu_map[aEagleLayer] ), copperLayers, true };
    }

    int  kiLayer  = UNSELECTED_LAYER;
    bool required = false;
    LSET permittedLayers;

    permittedLayers.set();

    // translate non-copper eagle layer to pcbnew layer
    switch( aEagleLayer )
    {
    // Eagle says "Dimension" layer, but it's for board perimeter
    case EAGLE_LAYER::DIMENSION:
        kiLayer         = Edge_Cuts;
        required        = true;
        permittedLayers = LSET( 1, Edge_Cuts );
        break;

    case EAGLE_LAYER::TPLACE:
        kiLayer = F_SilkS;
        break;
    case EAGLE_LAYER::BPLACE:
        kiLayer = B_SilkS;
        break;
    case EAGLE_LAYER::TNAMES:
        kiLayer = F_SilkS;
        break;
    case EAGLE_LAYER::BNAMES:
        kiLayer = B_SilkS;
        break;
    case EAGLE_LAYER::TVALUES:
        kiLayer = F_Fab;
        break;
    case EAGLE_LAYER::BVALUES:
        kiLayer = B_Fab;
        break;
    case EAGLE_LAYER::TSTOP:
        kiLayer = F_Mask;
        break;
    case EAGLE_LAYER::BSTOP:
        kiLayer = B_Mask;
        break;
    case EAGLE_LAYER::TCREAM:
        kiLayer = F_Paste;
        break;
    case EAGLE_LAYER::BCREAM:
        kiLayer = B_Paste;
        break;
    case EAGLE_LAYER::TFINISH:
        kiLayer = F_Mask;
        break;
    case EAGLE_LAYER::BFINISH:
        kiLayer = B_Mask;
        break;
    case EAGLE_LAYER::TGLUE:
        kiLayer = F_Adhes;
        break;
    case EAGLE_LAYER::BGLUE:
        kiLayer = B_Adhes;
        break;
    case EAGLE_LAYER::DOCUMENT:
        kiLayer = Cmts_User;
        break;
    case EAGLE_LAYER::REFERENCELC:
        kiLayer = Cmts_User;
        break;
    case EAGLE_LAYER::REFERENCELS:
        kiLayer = Cmts_User;
        break;

    // Packages show the future chip pins on SMD parts using layer 51.
    // This is an area slightly smaller than the PAD/SMD copper area.
    // Carry those visual aids into the FOOTPRINT on the fabrication layer,
    // not silkscreen. This is perhaps not perfect, but there is not a lot
    // of other suitable paired layers
    case EAGLE_LAYER::TDOCU:
        kiLayer = F_Fab;
        break;
    case EAGLE_LAYER::BDOCU:
        kiLayer = B_Fab;
        break;

    // these layers are defined as user layers. put them on ECO layers
    case EAGLE_LAYER::USERLAYER1:
        kiLayer = Eco1_User;
        break;
    case EAGLE_LAYER::USERLAYER2:
        kiLayer = Eco2_User;
        break;

    // these will also appear in the ratsnest, so there's no need for a warning
    case EAGLE_LAYER::UNROUTED:
        kiLayer = Dwgs_User;
        break;

    case EAGLE_LAYER::TKEEPOUT:
        kiLayer = F_CrtYd;
        break;
    case EAGLE_LAYER::BKEEPOUT:
        kiLayer = B_CrtYd;
        break;

    case EAGLE_LAYER::MILLING:
    case EAGLE_LAYER::TTEST:
    case EAGLE_LAYER::BTEST:
    case EAGLE_LAYER::HOLES:
    default:
        kiLayer = UNSELECTED_LAYER;
        break;
    }

    return { PCB_LAYER_ID( kiLayer ), permittedLayers, required };
}


const wxString& EAGLE_PLUGIN::eagle_layer_name( int aLayer ) const
{
    static const wxString unknown( "unknown" );
    auto it = m_eagleLayers.find( aLayer );
    return it == m_eagleLayers.end() ? unknown : it->second.name;
}


int EAGLE_PLUGIN::eagle_layer_id( const wxString& aLayerName ) const
{
    static const int unknown = -1;
    auto it = m_eagleLayersIds.find( aLayerName );
    return it == m_eagleLayersIds.end() ? unknown : it->second;
}


void EAGLE_PLUGIN::centerBoard()
{
    if( m_props )
    {
        UTF8 page_width;
        UTF8 page_height;

        if( m_props->Value( "page_width",  &page_width ) &&
            m_props->Value( "page_height", &page_height ) )
        {
            EDA_RECT bbbox = m_board->GetBoardEdgesBoundingBox();

            int w = atoi( page_width.c_str() );
            int h = atoi( page_height.c_str() );

            int desired_x = ( w - bbbox.GetWidth() )  / 2;
            int desired_y = ( h - bbbox.GetHeight() ) / 2;

            m_board->Move( wxPoint( desired_x - bbbox.GetX(), desired_y - bbbox.GetY() ) );
        }
    }
}


wxDateTime EAGLE_PLUGIN::getModificationTime( const wxString& aPath )
{
    // File hasn't been loaded yet.
    if( aPath.IsEmpty() )
        return wxDateTime::Now();

    wxFileName  fn( aPath );

    if( fn.IsFileReadable() )
        return fn.GetModificationTime();
    else
        return wxDateTime( 0.0 );
}


void EAGLE_PLUGIN::cacheLib( const wxString& aLibPath )
{
    try
    {
        wxDateTime  modtime = getModificationTime( aLibPath );

        // Fixes assertions in wxWidgets debug builds for the wxDateTime object.  Refresh the
        // cache if either of the wxDateTime objects are invalid or the last file modification
        // time differs from the current file modification time.
        bool load = !m_mod_time.IsValid() || !modtime.IsValid() || m_mod_time != modtime;

        if( aLibPath != m_lib_path || load )
        {
            wxXmlNode*  doc;
            LOCALE_IO   toggle;     // toggles on, then off, the C locale.

            deleteTemplates();

            // Set this before completion of loading, since we rely on it for
            // text of an exception.  Delay setting m_mod_time until after successful load
            // however.
            m_lib_path = aLibPath;

            // 8 bit "filename" should be encoded according to disk filename encoding,
            // (maybe this is current locale, maybe not, its a filesystem issue),
            // and is not necessarily utf8.
            string filename = (const char*) aLibPath.char_str( wxConvFile );

            // Load the document
            wxFileName fn( filename );
            wxFFileInputStream stream( fn.GetFullPath() );
            wxXmlDocument xmlDocument;

            if( !stream.IsOk() || !xmlDocument.Load( stream ) )
            {
                THROW_IO_ERROR( wxString::Format( _( "Unable to read file '%s'." ),
                                                  fn.GetFullPath() ) );
            }

            doc = xmlDocument.GetRoot();

            wxXmlNode* drawing       = MapChildren( doc )["drawing"];
            NODE_MAP drawingChildren = MapChildren( drawing );

            // clear the cu map and then rebuild it.
            clear_cu_map();

            m_xpath->push( "eagle.drawing.layers" );
            wxXmlNode* layers  = drawingChildren["layers"];
            loadLayerDefs( layers );
            mapEagleLayersToKicad();
            m_xpath->pop();

            m_xpath->push( "eagle.drawing.library" );
            wxXmlNode* library = drawingChildren["library"];
            loadLibrary( library, nullptr );
            m_xpath->pop();

            m_mod_time = modtime;
        }
    }
    catch(...){}
    // TODO: Handle exceptions
    // catch( file_parser_error fpe )
    // {
    //     // for xml_parser_error, what() has the line number in it,
    //     // but no byte offset.  That should be an adequate error message.
    //     THROW_IO_ERROR( fpe.what() );
    // }
    //
    // // Class ptree_error is a base class for xml_parser_error & file_parser_error,
    // // so one catch should be OK for all errors.
    // catch( ptree_error pte )
    // {
    //     string errmsg = pte.what();
    //
    //     errmsg += " @\n";
    //     errmsg += m_xpath->Contents();
    //
    //     THROW_IO_ERROR( errmsg );
    // }
}


void EAGLE_PLUGIN::FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                                       bool aBestEfforts, const PROPERTIES* aProperties )
{
    wxString errorMsg;

    init( aProperties );

    try
    {
        cacheLib( aLibraryPath );
    }
    catch( const IO_ERROR& ioe )
    {
        errorMsg = ioe.What();
    }

    // Some of the files may have been parsed correctly so we want to add the valid files to
    // the library.

    for( FOOTPRINT_MAP::const_iterator it = m_templates.begin(); it != m_templates.end(); ++it )
        aFootprintNames.Add( FROM_UTF8( it->first.c_str() ) );

    if( !errorMsg.IsEmpty() && !aBestEfforts )
        THROW_IO_ERROR( errorMsg );
}


FOOTPRINT* EAGLE_PLUGIN::FootprintLoad( const wxString& aLibraryPath,
                                        const wxString& aFootprintName, bool aKeepUUID,
                                        const PROPERTIES* aProperties )
{
    init( aProperties );
    cacheLib( aLibraryPath );
    FOOTPRINT_MAP::const_iterator it = m_templates.find( aFootprintName );

    if( it == m_templates.end() )
        return nullptr;

    // Return a copy of the template
    FOOTPRINT* copy = (FOOTPRINT*) it->second->Duplicate();
    copy->SetParent( nullptr );
    return copy;
}


void EAGLE_PLUGIN::FootprintLibOptions( PROPERTIES* aListToAppendTo ) const
{
    PLUGIN::FootprintLibOptions( aListToAppendTo );
}
