/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <utility>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/stdpaths.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include <decompress.hpp>

#include "oce_utils.h"
#include "kicadpad.h"
#include "streamwrapper.h"

#include <IGESCAFControl_Reader.hxx>
#include <IGESCAFControl_Writer.hxx>
#include <IGESControl_Controller.hxx>
#include <IGESData_GlobalSection.hxx>
#include <IGESData_IGESModel.hxx>
#include <Interface_Static.hxx>
#include <Quantity_Color.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <APIHeaderSection_MakeHeader.hxx>
#include <Standard_Version.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx>
#include <TDataStd_TreeNode.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDF_ChildIterator.hxx>
#include <TopExp_Explorer.hxx>
#include <XCAFDoc.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ColorTool.hxx>

#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBuilderAPI.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepAlgoAPI_Cut.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Builder.hxx>

#include <Standard_Failure.hxx>

#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <Geom_BezierCurve.hxx>

#include <macros.h>

static constexpr double USER_PREC = 1e-4;
static constexpr double USER_ANGLE_PREC = 1e-6;

// minimum PCB thickness in mm (2 microns assumes a very thin polyimide film)
static constexpr double THICKNESS_MIN = 0.002;

// default PCB thickness in mm
static constexpr double THICKNESS_DEFAULT = 1.6;

// nominal offset from the board
static constexpr double BOARD_OFFSET = 0.05;

// min. length**2 below which 2 points are considered coincident
static constexpr double MIN_LENGTH2 = MIN_DISTANCE * MIN_DISTANCE;

static void getEndPoints( const KICADCURVE& aCurve, double& spx0, double& spy0,
                          double& epx0, double& epy0 )
{
    if( CURVE_ARC == aCurve.m_form )
    {
        spx0 = aCurve.m_end.x;
        spy0 = aCurve.m_end.y;
        epx0 = aCurve.m_ep.x;
        epy0 = aCurve.m_ep.y;
        return;
    }

    // assume a line
    spx0 = aCurve.m_start.x;
    spy0 = aCurve.m_start.y;
    epx0 = aCurve.m_end.x;
    epy0 = aCurve.m_end.y;
}


static void getCurveEndPoint( const KICADCURVE& aCurve, DOUBLET& aEndPoint )
{
    if( CURVE_CIRCLE == aCurve.m_form )
        return; // circles are closed loops and have no end point

    if( CURVE_ARC == aCurve.m_form )
    {
        aEndPoint.x = aCurve.m_ep.x;
        aEndPoint.y = aCurve.m_ep.y;
        return;
    }

    // assume a line
    aEndPoint.x = aCurve.m_end.x;
    aEndPoint.y = aCurve.m_end.y;
}


static void reverseCurve( KICADCURVE& aCurve )
{
    if( CURVE_NONE ==  aCurve.m_form || CURVE_CIRCLE == aCurve.m_form )
        return;

    if( CURVE_LINE == aCurve.m_form  )
    {
        std::swap( aCurve.m_start, aCurve.m_end );
        return;
    }

    if( CURVE_BEZIER == aCurve.m_form )
    {
        std::swap( aCurve.m_start, aCurve.m_end );
        std::swap( aCurve.m_bezierctrl1, aCurve.m_bezierctrl2 );
        return;
    }

    std::swap( aCurve.m_end, aCurve.m_ep );
    std::swap( aCurve.m_endangle, aCurve.m_startangle );
    aCurve.m_angle = -aCurve.m_angle;
}


// supported file types
enum FormatType
{
    FMT_NONE,
    FMT_STEP,
    FMT_STEPZ,
    FMT_IGES,
    FMT_EMN,
    FMT_IDF,
    FMT_WRL,
    FMT_WRZ
};


FormatType fileType( const char* aFileName )
{
    wxFileName lfile( wxString::FromUTF8Unchecked( aFileName ) );

    if( !lfile.FileExists() )
    {
        wxString msg;
        msg.Printf( " * fileType(): no such file: %s\n",
                    wxString::FromUTF8Unchecked( aFileName ) );

        ReportMessage( msg );
        return FMT_NONE;
    }

    wxString ext = lfile.GetExt().Lower();

    if( ext == "wrl" )
        return FMT_WRL;

    if( ext == "wrz" )
        return FMT_WRZ;

    if( ext == "idf" )
        return FMT_IDF;     // component outline

    if( ext == "emn" )
        return FMT_EMN;     // PCB assembly

    if( ext == "stpz" || ext == "gz" )
        return FMT_STEPZ;

    OPEN_ISTREAM( ifile, aFileName );

    if( ifile.fail() )
        return FMT_NONE;

    char iline[82];
    memset( iline, 0, 82 );
    ifile.getline( iline, 82 );
    CLOSE_STREAM( ifile );
    iline[81] = 0;  // ensure NULL termination when string is too long

    // check for STEP in Part 21 format
    // (this can give false positives since Part 21 is not exclusively STEP)
    if( !strncmp( iline, "ISO-10303-21;", 13 ) )
        return FMT_STEP;

    std::string fstr = iline;

    // check for STEP in XML format
    // (this can give both false positive and false negatives)
    if( fstr.find( "urn:oid:1.0.10303." ) != std::string::npos )
        return FMT_STEP;

    // Note: this is a very simple test which can yield false positives; the only
    // sure method for determining if a file *not* an IGES model is to attempt
    // to load it.
    if( iline[72] == 'S' && ( iline[80] == 0 || iline[80] == 13 || iline[80] == 10 ) )
        return FMT_IGES;

    return FMT_NONE;
}


PCBMODEL::PCBMODEL( const wxString& aPcbName )
{
    m_app = XCAFApp_Application::GetApplication();
    m_app->NewDocument( "MDTV-XCAF", m_doc );
    m_assy = XCAFDoc_DocumentTool::ShapeTool ( m_doc->Main() );
    m_assy_label = m_assy->NewShape();
    m_hasPCB = false;
    m_components = 0;
    m_precision = USER_PREC;
    m_angleprec = USER_ANGLE_PREC;
    m_thickness = THICKNESS_DEFAULT;
    m_minDistance2 = MIN_LENGTH2;
    m_minx = 1.0e10;    // absurdly large number; any valid PCB X value will be smaller
    m_mincurve = m_curves.end();
    m_pcbName = aPcbName;
    BRepBuilderAPI::Precision( MIN_DISTANCE );
}


PCBMODEL::~PCBMODEL()
{
    m_doc->Close();
}


bool PCBMODEL::AddOutlineSegment( KICADCURVE* aCurve )
{
    if( NULL == aCurve || LAYER_EDGE != aCurve->m_layer || CURVE_NONE == aCurve->m_form )
        return false;

    if( CURVE_LINE == aCurve->m_form  || CURVE_BEZIER == aCurve->m_form )
    {
        // reject zero - length lines
        double dx = aCurve->m_end.x - aCurve->m_start.x;
        double dy = aCurve->m_end.y - aCurve->m_start.y;
        double distance = dx * dx + dy * dy;

        if( distance < m_minDistance2 )
        {
            wxString msg;
            msg.Printf( "  * AddOutlineSegment() rejected a zero-length %s\n", aCurve->Describe() );
            ReportMessage( msg );
            return false;
        }
    }
    else
    {
        // ensure that the start (center) and end (start of arc) are not the same point
        double dx = aCurve->m_end.x - aCurve->m_start.x;
        double dy = aCurve->m_end.y - aCurve->m_start.y;
        double rad = dx * dx + dy * dy;

        if( rad < m_minDistance2 )
        {
            wxString msg;
            msg.Printf( "  * AddOutlineSegment() rejected a zero-radius %s\n", aCurve->Describe() );
            ReportMessage( msg );
            return false;
        }

        // calculate the radius and, if applicable, end point
        rad = sqrt( rad );
        aCurve->m_radius = rad;

        if( CURVE_ARC == aCurve->m_form )
        {
            aCurve->m_startangle = atan2( dy, dx );

            if( aCurve->m_startangle < 0.0 )
                aCurve->m_startangle += 2.0 * M_PI;

            double eang = aCurve->m_startangle + aCurve->m_angle;

            if( eang < 0.0 )
                eang += 2.0 * M_PI;

            if( aCurve->m_angle < 0.0 && eang > aCurve->m_startangle )
                aCurve->m_startangle += 2.0 * M_PI;
            else if( aCurve->m_angle >= 0.0 && eang < aCurve->m_startangle )
                eang += 2.0 * M_PI;

            aCurve->m_endangle = eang;
            aCurve->m_ep.x = aCurve->m_start.x + rad * cos( eang );
            aCurve->m_ep.y = aCurve->m_start.y + rad * sin( eang );

            dx = aCurve->m_ep.x - aCurve->m_end.x;
            dy = aCurve->m_ep.y - aCurve->m_end.y;
            rad = dx * dx + dy * dy;

            if( rad < m_minDistance2 )
            {
                ReportMessage( wxString::Format( "  * AddOutlineSegment() rejected an arc with "
                                                 "equivalent end points, %s\n",
                                                 aCurve->Describe() ) );
                return false;
            }
        }
    }

    m_curves.push_back( *aCurve );

    // check if this curve has the current leftmost feature
    switch( aCurve->m_form )
    {
    case CURVE_LINE:
        if( aCurve->m_start.x < m_minx )
        {
            m_minx = aCurve->m_start.x;
            m_mincurve = --( m_curves.end() );
        }

        if( aCurve->m_end.x < m_minx )
        {
            m_minx = aCurve->m_end.x;
            m_mincurve = --( m_curves.end() );
        }

        break;

    case CURVE_CIRCLE:
        do
        {
            double dx = aCurve->m_start.x - aCurve->m_radius;

            if( dx < m_minx )
            {
                m_minx = dx;
                m_mincurve = --( m_curves.end() );
            }
        } while( 0 );

        break;

    case CURVE_ARC:
        do
        {
            double dx0 = aCurve->m_end.x - aCurve->m_start.x;
            double dy0 = aCurve->m_end.y - aCurve->m_start.y;
            int    q0; // quadrant of start point

            if( dx0 > 0.0 && dy0 >= 0.0 )
                q0 = 1;
            else if( dx0 <= 0.0 && dy0 > 0.0 )
                q0 = 2;
            else if( dx0 < 0.0 && dy0 <= 0.0 )
                q0 = 3;
            else
                q0 = 4;

            double dx1 = aCurve->m_ep.x - aCurve->m_start.x;
            double dy1 = aCurve->m_ep.y - aCurve->m_start.y;
            int    q1; // quadrant of end point

            if( dx1 > 0.0 && dy1 >= 0.0 )
                q1 = 1;
            else if( dx1 <= 0.0 && dy1 > 0.0 )
                q1 = 2;
            else if( dx1 < 0.0 && dy1 <= 0.0 )
                q1 = 3;
            else
                q1 = 4;

            // calculate x0, y0 for the start point on a CCW arc
            double x0 = aCurve->m_end.x;
            double x1 = aCurve->m_ep.x;

            if( aCurve->m_angle < 0.0 )
            {
                std::swap( q0, q1 );
                std::swap( x0, x1 );
            }

            double minx;

            if( ( q0 <= 2 && q1 >= 3 ) || ( q0 >= 3 && x0 > x1 ) )
                minx = aCurve->m_start.x - aCurve->m_radius;
            else
                minx = std::min( x0, x1 );

            if( minx < m_minx )
            {
                m_minx = minx;
                m_mincurve = --( m_curves.end() );
            }

        } while( 0 );

        break;

    case CURVE_BEZIER:
        if( aCurve->m_start.x < m_minx )
        {
            m_minx = aCurve->m_start.x;
            m_mincurve = --( m_curves.end() );
        }

        if( aCurve->m_end.x < m_minx )
        {
            m_minx = aCurve->m_end.x;
            m_mincurve = --( m_curves.end() );
        }

        break;

    default:
        // unexpected curve type
        do
        {
            wxString msg;
            msg.Printf( "  * AddOutlineSegment() unsupported curve type: %d\n", aCurve->m_form );
            ReportMessage( msg );
        } while( 0 );

        return false;
    }

    return true;
}


bool PCBMODEL::AddPadHole( const KICADPAD* aPad )
{
    if( NULL == aPad || !aPad->IsThruHole() )
        return false;

    if( !aPad->m_drill.oval )
    {
        TopoDS_Shape s = BRepPrimAPI_MakeCylinder( aPad->m_drill.size.x * 0.5,
                                                   m_thickness * 2.0 ).Shape();
        gp_Trsf shift;
        shift.SetTranslation( gp_Vec( aPad->m_position.x, aPad->m_position.y,
                                      -m_thickness * 0.5 ) );
        BRepBuilderAPI_Transform hole( s, shift );
        m_cutouts.push_back( hole.Shape() );
        return true;
    }

    // slotted hole
    double angle_offset = 0.0;
    double rad;     // radius of the slot
    double hlen;    // half length of the slot

    if( aPad->m_drill.size.x < aPad->m_drill.size.y )
    {
        angle_offset = M_PI_2;
        rad = aPad->m_drill.size.x * 0.5;
        hlen = aPad->m_drill.size.y * 0.5 - rad;
    }
    else
    {
        rad = aPad->m_drill.size.y * 0.5;
        hlen = aPad->m_drill.size.x * 0.5 - rad;
    }

    DOUBLET c0( -hlen, 0.0 );
    DOUBLET c1( hlen, 0.0 );
    DOUBLET p0( -hlen, rad );
    DOUBLET p1( -hlen, -rad );
    DOUBLET p2(  hlen, -rad );
    DOUBLET p3( hlen, rad );

    angle_offset += aPad->m_rotation;
    double dlim = (double)std::numeric_limits< float >::epsilon();

    if( angle_offset < -dlim || angle_offset > dlim )
    {
        double vsin = sin( angle_offset );
        double vcos = cos( angle_offset );

        double x = c0.x * vcos - c0.y * vsin;
        double y = c0.x * vsin + c0.y * vcos;
        c0.x = x;
        c0.y = y;

        x = c1.x * vcos - c1.y * vsin;
        y = c1.x * vsin + c1.y * vcos;
        c1.x = x;
        c1.y = y;

        x = p0.x * vcos - p0.y * vsin;
        y = p0.x * vsin + p0.y * vcos;
        p0.x = x;
        p0.y = y;

        x = p1.x * vcos - p1.y * vsin;
        y = p1.x * vsin + p1.y * vcos;
        p1.x = x;
        p1.y = y;

        x = p2.x * vcos - p2.y * vsin;
        y = p2.x * vsin + p2.y * vcos;
        p2.x = x;
        p2.y = y;

        x = p3.x * vcos - p3.y * vsin;
        y = p3.x * vsin + p3.y * vcos;
        p3.x = x;
        p3.y = y;
    }

    c0.x += aPad->m_position.x;
    c0.y += aPad->m_position.y;
    c1.x += aPad->m_position.x;
    c1.y += aPad->m_position.y;
    p0.x += aPad->m_position.x;
    p0.y += aPad->m_position.y;
    p1.x += aPad->m_position.x;
    p1.y += aPad->m_position.y;
    p2.x += aPad->m_position.x;
    p2.y += aPad->m_position.y;
    p3.x += aPad->m_position.x;
    p3.y += aPad->m_position.y;

    OUTLINE oln;
    oln.SetMinSqDistance( m_minDistance2 );
    KICADCURVE crv0, crv1, crv2, crv3;

    // crv0 = arc
    crv0.m_start = c0;
    crv0.m_end = p0;
    crv0.m_ep = p1;
    crv0.m_angle = M_PI;
    crv0.m_radius = rad;
    crv0.m_form = CURVE_ARC;

    // crv1 = line
    crv1.m_start = p1;
    crv1.m_end = p2;
    crv1.m_form = CURVE_LINE;

    // crv2 = arc
    crv2.m_start = c1;
    crv2.m_end = p2;
    crv2.m_ep = p3;
    crv2.m_angle = M_PI;
    crv2.m_radius = rad;
    crv2.m_form = CURVE_ARC;

    // crv3 = line
    crv3.m_start = p3;
    crv3.m_end = p0;
    crv3.m_form = CURVE_LINE;

    oln.AddSegment( crv0 );
    oln.AddSegment( crv1 );
    oln.AddSegment( crv2 );
    oln.AddSegment( crv3 );
    TopoDS_Shape slot;

    if( oln.MakeShape( slot, m_thickness ) )
    {
        if( !slot.IsNull() )
            m_cutouts.push_back( slot );

        return true;
    }

    return false;
}


bool PCBMODEL::AddComponent( const std::string& aFileNameUTF8, const std::string& aRefDes,
                             bool aBottom, DOUBLET aPosition, double aRotation,
                             TRIPLET aOffset, TRIPLET aOrientation, TRIPLET aScale,
                             bool aSubstituteModels )
{
    if( aFileNameUTF8.empty() )
    {
        ReportMessage( wxString::Format( "No model defined for component %s.\n", aRefDes ) );
        return false;
    }

    wxString fileName( wxString::FromUTF8( aFileNameUTF8.c_str() ) );
    ReportMessage( wxString::Format( "Add component %s.\n", aRefDes ) );

    // first retrieve a label
    TDF_Label lmodel;
    wxString errorMessage;

    if( !getModelLabel( aFileNameUTF8, aScale, lmodel, aSubstituteModels, &errorMessage ) )
    {
        if( errorMessage.IsEmpty() )
            ReportMessage( wxString::Format( "No model for filename '%s'.\n", fileName ) );
        else
            ReportMessage( errorMessage );

        return false;
    }

    // calculate the Location transform
    TopLoc_Location toploc;

    if( !getModelLocation( aBottom, aPosition, aRotation, aOffset, aOrientation, toploc ) )
    {
        ReportMessage( wxString::Format( "No location data for filename '%s'.\n", fileName ) );
        return false;
    }

    // add the located sub-assembly
    TDF_Label llabel = m_assy->AddComponent( m_assy_label, lmodel, toploc );

    if( llabel.IsNull() )
    {
        ReportMessage( wxString::Format( "Could not add component with filename '%s'.\n",
                                         fileName ) );
        return false;
    }

    // attach the RefDes name
    TCollection_ExtendedString refdes( aRefDes.c_str() );
    TDataStd_Name::Set( llabel, refdes );

    return true;
}


void PCBMODEL::SetPCBThickness( double aThickness )
{
    if( aThickness < 0.0 )
        m_thickness = THICKNESS_DEFAULT;
    else if( aThickness < THICKNESS_MIN )
        m_thickness = THICKNESS_MIN;
    else
        m_thickness = aThickness;
}


void PCBMODEL::SetMinDistance( double aDistance )
{
    // m_minDistance2 keeps a squared distance value
    m_minDistance2 = aDistance * aDistance;
    BRepBuilderAPI::Precision( aDistance );
}


bool PCBMODEL::CreatePCB()
{
    if( m_hasPCB )
    {
        if( m_pcb_label.IsNull() )
            return false;

        return true;
    }

    if( m_curves.empty() || m_mincurve == m_curves.end() )
    {
        m_hasPCB = true;
        ReportMessage( "No valid board outline.\n" );
        return false;
    }

    m_hasPCB = true;    // whether or not operations fail we note that CreatePCB has been invoked
    TopoDS_Shape board;
    OUTLINE oln;        // loop to assemble (represents PCB outline and cutouts)
    oln.SetMinSqDistance( m_minDistance2 );
    oln.AddSegment( *m_mincurve );
    m_curves.erase( m_mincurve );

    ReportMessage( wxString::Format( "Build board outline (%d items).\n",
                   (int)m_curves.size() ) );

    while( !m_curves.empty() )
    {
        if( oln.IsClosed() )
        {
            if( board.IsNull() )
            {
                if( !oln.MakeShape( board, m_thickness ) )
                {
                    ReportMessage( "Could not create board extrusion.\n" );
                    return false;
                }
            }
            else
            {
                TopoDS_Shape hole;

                if( oln.MakeShape( hole, m_thickness ) )
                {
                    m_cutouts.push_back( hole );
                }
                else
                {
                    ReportMessage( "Could not create board cutout.\n" );
                }
            }

            oln.Clear();

            if( !m_curves.empty() )
            {
                oln.AddSegment( m_curves.front() );
                m_curves.pop_front();
            }

            continue;
        }

        std::list< KICADCURVE >::iterator sC = m_curves.begin();
        bool added = false;

        while( sC != m_curves.end() )
        {
            if( oln.AddSegment( *sC ) )
            {
                added = true;
                m_curves.erase( sC );
                break;
            }

            ++sC;
        }

        if( !added && !oln.m_curves.empty() )
        {
            wxString msg;
            msg.Printf( "Could not close outline (dropping outline data with %d segments).\n",
                        static_cast<int>( oln.m_curves.size() ) );

            for( const auto& c : oln.m_curves )
                msg << " + " << c.Describe() << "\n";

            ReportMessage( msg );
            oln.Clear();

            if( !m_curves.empty() )
            {
                oln.AddSegment( m_curves.front() );
                m_curves.pop_front();
            }
        }
    }

    if( oln.IsClosed() )
    {
        if( board.IsNull() )
        {
            if( !oln.MakeShape( board, m_thickness ) )
            {
                ReportMessage( "Could not create board extrusion.\n" );
                return false;
            }
        }
        else
        {
            TopoDS_Shape hole;

            if( oln.MakeShape( hole, m_thickness ) )
            {
                m_cutouts.push_back( hole );
            }
            else
            {
                ReportMessage( "Could not create board cutout.\n" );
            }
        }
    }
    else
    {
        ReportMessage( "Could not create closed board outlines.\n" );
        return false;
    }

    // subtract cutouts (if any)
    if( m_cutouts.size() )
    {
        ReportMessage( wxString::Format( "Build board cutouts and holes (%d holes).\n",
                                         (int)m_cutouts.size() ) );
    }

#if 0
    // First version for holes removing: very slow when having many (> 300) holes
    // Substract holes (cutouts) can be time consuming, so display activity
    // state to be sure there is no hang:
    int char_count = 0;
    int cur_count = 0;
    int cntmax =  m_cutouts.size();

    for( auto hole : m_cutouts )
    {
        board = BRepAlgoAPI_Cut( board, hole );

        cur_count++;
        char_count++;

        if( char_count < 80 )
            ReportMessage( "." );
        else
        {
            char_count = 0;
            ReportMessage( wxString::Format( ". %d/%d\n", cur_count, cntmax ) );
        }
    }
#else   // Much faster than first version: group all holes and cut only once
    if( m_cutouts.size() )
    {
        BRepAlgoAPI_Cut Cut;
        TopTools_ListOfShape mainbrd;
        mainbrd.Append( board );

        Cut.SetArguments( mainbrd );
        TopTools_ListOfShape holelist;

        for( auto hole : m_cutouts )
             holelist.Append( hole );

        Cut.SetTools( holelist );
        Cut.Build();
        board = Cut.Shape();
    }
#endif

    // push the board to the data structure
    ReportMessage( "\nGenerate board full shape.\n" );

    // Dont expand the component or else coloring it gets hard
    m_pcb_label = m_assy->AddComponent( m_assy_label, board, false );

    if( m_pcb_label.IsNull() )
        return false;

    // AddComponent adds a label that has a reference (not a parent/child relation) to the real label
    // We need to extract that real label to name it for the STEP output cleanly
    // Why are we trying to name the bare board? Because CAD tools like SolidWorks do fun things
    // like "deduplicate" imported STEPs by swapping STEP assembly components with already identically named assemblies
    // So we want to avoid having the PCB be generally defaulted to "Component" or "Assembly".
    Handle( TDataStd_TreeNode ) node;

    if( m_pcb_label.FindAttribute( XCAFDoc::ShapeRefGUID(), node ) )
    {
        TDF_Label label = node->Father()->Label();
        if( !label.IsNull() )
        {
            wxString                   pcbName = wxString::Format( "%s PCB", m_pcbName );
            std::string                pcbNameStdString( pcbName.ToUTF8() );
            TCollection_ExtendedString partname( pcbNameStdString.c_str() );
            TDataStd_Name::Set( label, partname );
        }
    }


    // color the PCB
    Handle( XCAFDoc_ColorTool ) color = XCAFDoc_DocumentTool::ColorTool( m_doc->Main () );
    Quantity_Color pcb_green( 0.06, 0.4, 0.06, Quantity_TOC_RGB );
    color->SetColor( m_pcb_label, pcb_green, XCAFDoc_ColorSurf );

    TopExp_Explorer topex;
    topex.Init( m_assy->GetShape( m_pcb_label ), TopAbs_SOLID );

    while( topex.More() )
    {
        color->SetColor( topex.Current(), pcb_green, XCAFDoc_ColorSurf );
        topex.Next();
    }

#if ( defined OCC_VERSION_HEX ) && ( OCC_VERSION_HEX > 0x070101 )
    m_assy->UpdateAssemblies();
#endif
    return true;
}


#ifdef SUPPORTS_IGES
// write the assembly model in IGES format
bool PCBMODEL::WriteIGES( const wxString& aFileName )
{
    if( m_pcb_label.IsNull() )
    {
        ReportMessage( wxString::Format( "No valid PCB assembly; cannot create output file '%s'.\n",
                                         aFileName ) );
        return false;
    }

    wxFileName fn( aFileName );
    IGESControl_Controller::Init();
    IGESCAFControl_Writer writer;
    writer.SetColorMode( Standard_True );
    writer.SetNameMode( Standard_True );
    IGESData_GlobalSection header = writer.Model()->GlobalSection();
    header.SetFileName( new TCollection_HAsciiString( fn.GetFullName().ToAscii() ) );
    header.SetSendName( new TCollection_HAsciiString( "KiCad electronic assembly" ) );
    header.SetAuthorName( new TCollection_HAsciiString( Interface_Static::CVal( "write.iges.header.author" ) ) );
    header.SetCompanyName( new TCollection_HAsciiString( Interface_Static::CVal( "write.iges.header.company" ) ) );
    writer.Model()->SetGlobalSection( header );

    if( Standard_False == writer.Perform( m_doc, aFileName.c_str() ) )
        return false;

    return true;
}
#endif


bool PCBMODEL::WriteSTEP( const wxString& aFileName )
{
    if( m_pcb_label.IsNull() )
    {
        ReportMessage( wxString::Format( "No valid PCB assembly; cannot create output file '%s'.\n",
                                         aFileName ) );
        return false;
    }
    wxFileName fn( aFileName );

    STEPCAFControl_Writer writer;
    writer.SetColorMode( Standard_True );
    writer.SetNameMode( Standard_True );

    // This must be set before we "transfer" the document.
    // Should default to kicad_pcb.general.title_block.title,
    // but in the meantime, defaulting to the basename of the output
    // target is still better than "open cascade step translter v..."
    // UTF8 should be ok from ISO 10303-21:2016, but... older stuff? use boring ascii
    if( !Interface_Static::SetCVal( "write.step.product.name", fn.GetName().ToAscii() ) )
        ReportMessage( "Failed to set step product name, but will attempt to continue." );

    if( Standard_False == writer.Transfer( m_doc, STEPControl_AsIs ) )
        return false;

    APIHeaderSection_MakeHeader hdr( writer.ChangeWriter().Model() );

    // Note: use only Ascii7 chars, non Ascii7 chars (therefore UFT8 chars)
    // are creating issues in the step file
    hdr.SetName( new TCollection_HAsciiString( fn.GetFullName().ToAscii() ) );

    // TODO: how to control and ensure consistency with IGES?
    hdr.SetAuthorValue( 1, new TCollection_HAsciiString( "Pcbnew" ) );
    hdr.SetOrganizationValue( 1, new TCollection_HAsciiString( "Kicad" ) );
    hdr.SetOriginatingSystem( new TCollection_HAsciiString( "KiCad to STEP converter" ) );
    hdr.SetDescriptionValue( 1, new TCollection_HAsciiString( "KiCad electronic assembly" ) );

    bool success = true;

    // Creates a temporary file with a ascii7 name, because writer does not know unicode filenames.
    wxString currCWD = wxGetCwd();
    wxString workCWD = fn.GetPath();

    if( !workCWD.IsEmpty() )
        wxSetWorkingDirectory( workCWD );

    char tmpfname[] = "$tempfile$.step";

    if( Standard_False == writer.Write( tmpfname ) )
        success = false;

    if( success )
    {
        if( !wxRenameFile( tmpfname, fn.GetFullName(), true ) )
        {
            ReportMessage( wxString::Format( "Cannot rename temporary file '%s' to '%s'.\n",
                                             tmpfname,
                                             fn.GetFullName() ) );
            success = false;
        }
    }

    wxSetWorkingDirectory( currCWD );

    return success;
}


bool PCBMODEL::getModelLabel( const std::string& aFileNameUTF8, TRIPLET aScale, TDF_Label& aLabel,
                              bool aSubstituteModels, wxString* aErrorMessage )
{
    std::string model_key = aFileNameUTF8 + "_" + std::to_string( aScale.x )
                            + "_" + std::to_string( aScale.y ) + "_" + std::to_string( aScale.z );

    MODEL_MAP::const_iterator mm = m_models.find( model_key );

    if( mm != m_models.end() )
    {
        aLabel = mm->second;
        return true;
    }

    aLabel.Nullify();

    Handle( TDocStd_Document )  doc;
    m_app->NewDocument( "MDTV-XCAF", doc );

    wxString fileName( wxString::FromUTF8( aFileNameUTF8.c_str() ) );
    FormatType modelFmt = fileType( aFileNameUTF8.c_str() );

    switch( modelFmt )
    {
    case FMT_IGES:
        if( !readIGES( doc, aFileNameUTF8.c_str() ) )
        {
            ReportMessage( wxString::Format( "readIGES() failed on filename '%s'.\n", fileName ) );
            return false;
        }
        break;

    case FMT_STEP:
        if( !readSTEP( doc, aFileNameUTF8.c_str() ) )
        {
            ReportMessage( wxString::Format( "readSTEP() failed on filename '%s'.\n", fileName ) );
            return false;
        }
        break;

    case FMT_STEPZ:
    {
        // To export a compressed step file (.stpz or .stp.gz file), the best way is to
        // decaompress it in a temporaty file and load this temporary file
        wxFFileInputStream ifile( fileName );
        wxFileName         outFile( fileName );

        outFile.SetPath( wxStandardPaths::Get().GetTempDir() );
        outFile.SetExt( "step" );
        wxFileOffset size = ifile.GetLength();

        if( size == wxInvalidOffset )
        {
            ReportMessage( wxString::Format( "getModelLabel() failed on filename '%s'.\n",
                                             fileName ) );
            return false;
        }

        {
            bool                success = false;
            wxFFileOutputStream ofile( outFile.GetFullPath() );

            if( !ofile.IsOk() )
                return false;

            char* buffer = new char[size];

            ifile.Read( buffer, size );
            std::string expanded;

            try
            {
                expanded = gzip::decompress( buffer, size );
                success = true;
            }
            catch( ... )
            {
                ReportMessage( wxString::Format( "failed to decompress '%s'.\n", fileName ) );
            }

            if( expanded.empty() )
            {
                ifile.Reset();
                ifile.SeekI( 0 );
                wxZipInputStream            izipfile( ifile );
                std::unique_ptr<wxZipEntry> zip_file( izipfile.GetNextEntry() );

                if( zip_file && !zip_file->IsDir() && izipfile.CanRead() )
                {
                    izipfile.Read( ofile );
                    success = true;
                }
            }
            else
            {
                ofile.Write( expanded.data(), expanded.size() );
            }

            delete[] buffer;
            ofile.Close();

            if( success )
            {
                std::string altFileNameUTF8 = TO_UTF8( outFile.GetFullPath() );
                success = getModelLabel( altFileNameUTF8, TRIPLET( 1.0, 1.0, 1.0 ), aLabel, false );
            }

            return success;
        }

        break;
    }

    case FMT_WRL:
    case FMT_WRZ:
        /* WRL files are preferred for internal rendering, due to superior material properties, etc.
         * However they are not suitable for MCAD export.
         *
         * If a .wrl file is specified, attempt to locate a replacement file for it.
         *
         * If a valid replacement file is found, the label for THAT file will be associated with
         * the .wrl file
         */
        if( aSubstituteModels )
        {
            wxFileName wrlName( fileName );

            wxString basePath = wrlName.GetPath();
            wxString baseName = wrlName.GetName();

            // List of alternate files to look for
            // Given in order of preference
            // (Break if match is found)
            wxArrayString alts;

            // Step files
            alts.Add( "stp" );
            alts.Add( "step" );
            alts.Add( "STP" );
            alts.Add( "STEP" );
            alts.Add( "Stp" );
            alts.Add( "Step" );
            alts.Add( "stpz" );
            alts.Add( "stpZ" );
            alts.Add( "STPZ" );
            alts.Add( "step.gz" );
            alts.Add( "stp.gz" );

            // IGES files
            alts.Add( "iges" );
            alts.Add( "IGES" );
            alts.Add( "igs" );
            alts.Add( "IGS" );

            //TODO - Other alternative formats?

            for( const auto& alt : alts )
            {
                wxFileName altFile( basePath, baseName + "." + alt );

                if( altFile.IsOk() && altFile.FileExists() )
                {
                    std::string altFileNameUTF8 = TO_UTF8( altFile.GetFullPath() );

                    // When substituting a STEP/IGS file for VRML, do not apply the VRML scaling
                    // to the new STEP model.  This process of auto-substitution is janky as all
                    // heck so let's not mix up un-displayed scale factors with potentially
                    // mis-matched files.  And hope that the user doesn't have multiples files
                    // named "model.wrl" and "model.stp" referring to different parts.
                    // TODO: Fix model handling in v7.  Default models should only be STP.
                    //       Have option to override this in DISPLAY.
                    if( getModelLabel( altFileNameUTF8, TRIPLET( 1.0, 1.0, 1.0 ), aLabel, false ) )
                    {
                        return true;
                    }
                }
            }

            return false; // No replacement model found
        }
        else // Substitution is not allowed
        {
            if( aErrorMessage )
                aErrorMessage->Printf( "Cannot add a VRML model to a STEP file.\n" );

            return false;
        }

        break;

        // TODO: implement IDF and EMN converters

    default:
        return false;
    }

    aLabel = transferModel( doc, m_doc, aScale );

    if( aLabel.IsNull() )
    {
        ReportMessage( wxString::Format( "Could not transfer model data from file '%s'.\n",
                                         fileName  ) );
        return false;
    }

    // attach the PART NAME ( base filename: note that in principle
    // different models may have the same base filename )
    wxFileName afile( fileName );
    std::string pname( afile.GetName().ToUTF8() );
    TCollection_ExtendedString partname( pname.c_str() );
    TDataStd_Name::Set( aLabel, partname );

    m_models.insert( MODEL_DATUM( model_key, aLabel ) );
    ++m_components;
    return true;
}


bool PCBMODEL::getModelLocation( bool aBottom, DOUBLET aPosition, double aRotation, TRIPLET aOffset,
                                 TRIPLET aOrientation, TopLoc_Location& aLocation )
{
    // Order of operations:
    // a. aOrientation is applied -Z*-Y*-X
    // b. aOffset is applied
    //      Top ? add thickness to the Z offset
    // c. Bottom ? Rotate on X axis (in contrast to most ECAD which mirror on Y),
    //             then rotate on +Z
    //    Top ? rotate on -Z
    // d. aPosition is applied
    //
    // Note: Y axis is inverted in KiCad

    gp_Trsf lPos;
    lPos.SetTranslation( gp_Vec( aPosition.x, -aPosition.y, 0.0 ) );

    // Offset board thickness
    aOffset.z += BOARD_OFFSET;

    gp_Trsf lRot;

    if( aBottom )
    {
        lRot.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 0.0, 0.0, 1.0 ) ), aRotation );
        lPos.Multiply( lRot );
        lRot.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 1.0, 0.0, 0.0 ) ), M_PI );
        lPos.Multiply( lRot );
    }
    else
    {
        aOffset.z += m_thickness;
        lRot.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 0.0, 0.0, 1.0 ) ), aRotation );
        lPos.Multiply( lRot );
    }

    gp_Trsf lOff;
    lOff.SetTranslation( gp_Vec( aOffset.x, aOffset.y, aOffset.z ) );
    lPos.Multiply( lOff );

    gp_Trsf lOrient;
    lOrient.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 0.0, 0.0, 1.0 ) ),
                         -aOrientation.z );
    lPos.Multiply( lOrient );
    lOrient.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 0.0, 1.0, 0.0 ) ),
                         -aOrientation.y );
    lPos.Multiply( lOrient );
    lOrient.SetRotation( gp_Ax1( gp_Pnt( 0.0, 0.0, 0.0 ), gp_Dir( 1.0, 0.0, 0.0 ) ),
                         -aOrientation.x );
    lPos.Multiply( lOrient );

    aLocation = TopLoc_Location( lPos );
    return true;
}


bool PCBMODEL::readIGES( Handle( TDocStd_Document )& doc, const char* fname )
{
    IGESControl_Controller::Init();
    IGESCAFControl_Reader reader;
    IFSelect_ReturnStatus stat  = reader.ReadFile( fname );

    if( stat != IFSelect_RetDone )
        return false;

    // Enable user-defined shape precision
    if( !Interface_Static::SetIVal( "read.precision.mode", 1 ) )
        return false;

    // Set the shape conversion precision to USER_PREC (default 0.0001 has too many triangles)
    if( !Interface_Static::SetRVal( "read.precision.val", USER_PREC ) )
        return false;

    // set other translation options
    reader.SetColorMode(true);  // use model colors
    reader.SetNameMode(false);  // don't use IGES label names
    reader.SetLayerMode(false); // ignore LAYER data

    if ( !reader.Transfer( doc ) )
    {
        doc->Close();
        return false;
    }

    // are there any shapes to translate?
    if( reader.NbShapes() < 1 )
    {
        doc->Close();
        return false;
    }

    return true;
}


bool PCBMODEL::readSTEP( Handle(TDocStd_Document)& doc, const char* fname )
{
    STEPCAFControl_Reader reader;
    IFSelect_ReturnStatus stat  = reader.ReadFile( fname );

    if( stat != IFSelect_RetDone )
        return false;

    // Enable user-defined shape precision
    if( !Interface_Static::SetIVal( "read.precision.mode", 1 ) )
        return false;

    // Set the shape conversion precision to USER_PREC (default 0.0001 has too many triangles)
    if( !Interface_Static::SetRVal( "read.precision.val", USER_PREC ) )
        return false;

    // set other translation options
    reader.SetColorMode(true);  // use model colors
    reader.SetNameMode(false);  // don't use label names
    reader.SetLayerMode(false); // ignore LAYER data

    if ( !reader.Transfer( doc ) )
    {
        doc->Close();
        return false;
    }

    // are there any shapes to translate?
    if( reader.NbRootsForTransfer() < 1 )
    {
        doc->Close();
        return false;
    }

    return true;
}


TDF_Label PCBMODEL::transferModel( Handle( TDocStd_Document )& source,
                                   Handle( TDocStd_Document )& dest, TRIPLET aScale )
{
    // transfer data from Source into a top level component of Dest

    gp_GTrsf scale_transform;
    scale_transform.SetVectorialPart( gp_Mat( aScale.x, 0, 0,
                                              0, aScale.y, 0,
                                              0, 0, aScale.z ) );
    BRepBuilderAPI_GTransform brep( scale_transform );

    // s_assy = shape tool for the source
    Handle(XCAFDoc_ShapeTool) s_assy = XCAFDoc_DocumentTool::ShapeTool ( source->Main() );

    // retrieve all free shapes within the assembly
    TDF_LabelSequence frshapes;
    s_assy->GetFreeShapes( frshapes );

    // d_assy = shape tool for the destination
    Handle(XCAFDoc_ShapeTool) d_assy = XCAFDoc_DocumentTool::ShapeTool ( dest->Main() );

    // create a new shape within the destination and set the assembly tool to point to it
    TDF_Label component = d_assy->NewShape();

    int nshapes = frshapes.Length();
    int id = 1;
    Handle( XCAFDoc_ColorTool ) scolor = XCAFDoc_DocumentTool::ColorTool( source->Main() );
    Handle( XCAFDoc_ColorTool ) dcolor = XCAFDoc_DocumentTool::ColorTool( dest->Main() );
    TopExp_Explorer dtop;
    TopExp_Explorer stop;

    while( id <= nshapes )
    {
        TopoDS_Shape shape = s_assy->GetShape( frshapes.Value(id) );

        if ( !shape.IsNull() )
        {
            brep.Perform( shape, Standard_False );
            TopoDS_Shape scaled_shape;

            if ( brep.IsDone() )
            {
                scaled_shape = brep.Shape();
            }
            else
            {
                ReportMessage( "  * transfertModel(): failed to scale model\n" );

                scaled_shape = shape;
            }

            TDF_Label niulab = d_assy->AddComponent( component, scaled_shape, Standard_False );

            // check for per-surface colors
            stop.Init( shape, TopAbs_FACE );
            dtop.Init( d_assy->GetShape( niulab ), TopAbs_FACE );

            while( stop.More() && dtop.More() )
            {
                Quantity_Color face_color;

                TDF_Label tl;

                // give priority to the base shape's color
                if( s_assy->FindShape( stop.Current(), tl ) )
                {
                    if( scolor->GetColor( tl, XCAFDoc_ColorSurf, face_color )
                        || scolor->GetColor( tl, XCAFDoc_ColorGen, face_color )
                        || scolor->GetColor( tl, XCAFDoc_ColorCurv, face_color ) )
                    {
                        dcolor->SetColor( dtop.Current(), face_color, XCAFDoc_ColorSurf );
                    }
                }
                else  if( scolor->GetColor( stop.Current(), XCAFDoc_ColorSurf, face_color )
                          || scolor->GetColor( stop.Current(), XCAFDoc_ColorGen, face_color )
                          || scolor->GetColor( stop.Current(), XCAFDoc_ColorCurv, face_color ) )
                {
                    dcolor->SetColor( dtop.Current(), face_color, XCAFDoc_ColorSurf );
                }

                stop.Next();
                dtop.Next();
            }

            // check for per-solid colors
            stop.Init( shape, TopAbs_SOLID );
            dtop.Init( d_assy->GetShape( niulab ), TopAbs_SOLID, TopAbs_FACE );

            while( stop.More() && dtop.More() )
            {
                Quantity_Color face_color;

                TDF_Label tl;

                // give priority to the base shape's color
                if( s_assy->FindShape( stop.Current(), tl ) )
                {
                    if( scolor->GetColor( tl, XCAFDoc_ColorSurf, face_color )
                        || scolor->GetColor( tl, XCAFDoc_ColorGen, face_color )
                        || scolor->GetColor( tl, XCAFDoc_ColorCurv, face_color ) )
                    {
                        dcolor->SetColor( dtop.Current(), face_color, XCAFDoc_ColorGen );
                    }
                }
                else  if( scolor->GetColor( stop.Current(), XCAFDoc_ColorSurf, face_color )
                          || scolor->GetColor( stop.Current(), XCAFDoc_ColorGen, face_color )
                          || scolor->GetColor( stop.Current(), XCAFDoc_ColorCurv, face_color ) )
                {
                    dcolor->SetColor( dtop.Current(), face_color, XCAFDoc_ColorSurf );
                }

                stop.Next();
                dtop.Next();
            }
        }

        ++id;
    };

    return component;
}


OUTLINE::OUTLINE()
{
    m_closed = false;
    m_minDistance2 = MIN_LENGTH2;
}


OUTLINE::~OUTLINE()
{
}


void OUTLINE::Clear()
{
    m_closed = false;
    m_curves.clear();
}


bool OUTLINE::AddSegment( const KICADCURVE& aCurve )
{
    if( m_closed )
        return false;

    if( m_curves.empty() )
    {
        m_curves.push_back( aCurve );

        if( CURVE_CIRCLE == aCurve.m_form )
            m_closed = true;

        return true;
    }

    if( CURVE_CIRCLE == aCurve.m_form )
        return false;

    // get the end points of the first curve
    double spx0, spy0;
    double epx0, epy0;
    getEndPoints( m_curves.front(), spx0, spy0, epx0, epy0 );

    // get the end points of the free curve
    double spx1, spy1;
    double epx1, epy1;
    getEndPoints( aCurve, spx1, spy1, epx1, epy1 );

    // check if the curve attaches to the front
    double dx, dy;
    dx = epx1 - spx0;
    dy = epy1 - spy0;

    if( dx * dx + dy * dy < m_minDistance2 )
    {
        m_curves.push_front( aCurve );
        m_closed = testClosed( m_curves.front(), m_curves.back() );
        return true;
    }
    else
    {
        dx = spx1 - spx0;
        dy = spy1 - spy0;

        if( dx * dx + dy * dy < m_minDistance2 )
        {
            KICADCURVE curve = aCurve;
            reverseCurve( curve );
            m_curves.push_front( curve );
            m_closed = testClosed( m_curves.front(), m_curves.back() );
            return true;
        }
    }

    // check if the curve attaches to the back
    getEndPoints( m_curves.back(), spx0, spy0, epx0, epy0 );
    dx = spx1 - epx0;
    dy = spy1 - epy0;

    if( dx * dx + dy * dy < m_minDistance2 )
    {
        m_curves.push_back( aCurve );
        m_closed = testClosed( m_curves.front(), m_curves.back() );
        return true;
    }
    else
    {
        dx = epx1 - epx0;
        dy = epy1 - epy0;

        if( dx * dx + dy * dy < m_minDistance2 )
        {
            KICADCURVE curve = aCurve;
            reverseCurve( curve );
            m_curves.push_back( curve );
            m_closed = testClosed( m_curves.front(), m_curves.back() );
            return true;
        }
    }

    // this curve is not an end segment of the current loop
    return false;
}


bool OUTLINE::MakeShape( TopoDS_Shape& aShape, double aThickness )
{
    if( !aShape.IsNull() )
        return false;   // there is already data in the shape object

    if( m_curves.empty() )
        return true;    // succeeded in doing nothing

    if( !m_closed )
        return false;   // the loop is not closed

    BRepBuilderAPI_MakeWire wire;
    DOUBLET lastPoint;
    getCurveEndPoint( m_curves.back(), lastPoint );

    for( KICADCURVE& i : m_curves )
    {
        bool success = false;

        try
        {
            success = addEdge( &wire, i, lastPoint );
        }
        catch( const Standard_Failure& e )
        {
            ReportMessage( wxString::Format( "Exception caught: %s\n", e.GetMessageString() ) );
            success = false;
        }

        if( !success )
        {
            ReportMessage( wxString::Format( "failed to add edge: %s\n"
                                             "last valid outline point: %f %f\n",
                                             i.Describe().c_str(),
                                             lastPoint.x,
                                             lastPoint.y ) );
            return false;
        }
    }

    TopoDS_Face face = BRepBuilderAPI_MakeFace( wire );
    aShape = BRepPrimAPI_MakePrism( face, gp_Vec( 0, 0, aThickness ) );

    if( aShape.IsNull() )
    {
        ReportMessage( "failed to create a prismatic shape\n" );
        return false;
    }

    return true;
}


bool OUTLINE::addEdge( BRepBuilderAPI_MakeWire* aWire, KICADCURVE& aCurve, DOUBLET& aLastPoint )
{
    TopoDS_Edge edge;
    DOUBLET endPoint;
    getCurveEndPoint( aCurve, endPoint );

    switch( aCurve.m_form )
    {
    case CURVE_LINE:
        edge = BRepBuilderAPI_MakeEdge( gp_Pnt( aLastPoint.x, aLastPoint.y, 0.0 ),
                                        gp_Pnt( endPoint.x, endPoint.y, 0.0 ) );
        break;

    case CURVE_ARC:
    {
        gp_Circ arc( gp_Ax2( gp_Pnt( aCurve.m_start.x, aCurve.m_start.y, 0.0 ),
                             gp_Dir( 0.0, 0.0, 1.0 ) ), aCurve.m_radius );

        gp_Pnt sa( aLastPoint.x, aLastPoint.y, 0.0 );
        gp_Pnt ea( endPoint.x, endPoint.y, 0.0 );

        if( aCurve.m_angle < 0.0 )
            edge = BRepBuilderAPI_MakeEdge( arc, ea, sa );
        else
            edge = BRepBuilderAPI_MakeEdge( arc, sa, ea );
    }
        break;

    case CURVE_CIRCLE:
        edge = BRepBuilderAPI_MakeEdge(
                gp_Circ( gp_Ax2( gp_Pnt( aCurve.m_start.x, aCurve.m_start.y, 0.0 ),
                                 gp_Dir( 0.0, 0.0, 1.0 ) ), aCurve.m_radius ) );
        break;

    case CURVE_BEZIER:
    {
        TColgp_Array1OfPnt poles( 0, 3 );
        gp_Pnt             pt = gp_Pnt( aCurve.m_start.x, aCurve.m_start.y, 0.0 );

        poles( 0 ) = pt;
        pt = gp_Pnt( aCurve.m_bezierctrl1.x, aCurve.m_bezierctrl1.y, 0.0 );
        poles( 1 ) = pt;
        pt = gp_Pnt( aCurve.m_bezierctrl2.x, aCurve.m_bezierctrl2.y, 0.0 );
        poles( 2 ) = pt;
        pt = gp_Pnt( endPoint.x, endPoint.y, 0.0 );
        poles( 3 ) = pt;

        Geom_BezierCurve* bezier_curve = new Geom_BezierCurve( poles );
        edge = BRepBuilderAPI_MakeEdge( bezier_curve );
    }
        break;

    default:
        ReportMessage( wxString::Format( "unsupported curve type: %d\n", aCurve.m_form ) );
        return false;
    }

    if( edge.IsNull() )
        return false;

    aLastPoint = endPoint;
    aWire->Add( edge );

    if( BRepBuilderAPI_DisconnectedWire == aWire->Error() )
    {
        ReportMessage( "failed to add curve\n" );
        return false;
    }

    return true;
}


bool OUTLINE::testClosed( const KICADCURVE& aFrontCurve, const KICADCURVE& aBackCurve )
{
    double spx0, spy0, epx0, epy0;
    getEndPoints( aFrontCurve, spx0, spy0, epx0, epy0 );

    double spx1, spy1, epx1, epy1;
    getEndPoints( aBackCurve, spx1, spy1, epx1, epy1 );

    double dx = epx1 - spx0;
    double dy = epy1 - spy0;
    double r = dx * dx + dy * dy;

    if( r < m_minDistance2 )
        return true;

    return false;
}
