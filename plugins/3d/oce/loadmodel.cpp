/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <map>
#include <vector>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/string.h>
#include <wx/utils.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include <decompress.hpp>

#include <TDocStd_Document.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <Quantity_Color.hxx>
#include <XCAFApp_Application.hxx>

#include <AIS_Shape.hxx>

#include <IGESControl_Reader.hxx>
#include <IGESCAFControl_Reader.hxx>
#include <Interface_Static.hxx>

#include <STEPControl_Reader.hxx>
#include <STEPCAFControl_Reader.hxx>

#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>

#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp_Explorer.hxx>

#include <Quantity_Color.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Precision.hxx>

#include <TDF_LabelSequence.hxx>
#include <TDF_ChildIterator.hxx>

#include "plugins/3dapi/ifsg_all.h"


// log mask for wxLogTrace
#define MASK_OCE "PLUGIN_OCE"

// precision for mesh creation; 0.07 should be good enough for ECAD viewing
#define USER_PREC (0.14)

// angular deflection for meshing
// 10 deg (36 faces per circle) = 0.17453293
// 20 deg (18 faces per circle) = 0.34906585
// 30 deg (12 faces per circle) = 0.52359878
#define USER_ANGLE (0.52359878)

typedef std::map< Standard_Real, SGNODE* > COLORMAP;
typedef std::map< std::string, SGNODE* >   FACEMAP;
typedef std::map< std::string, std::vector< SGNODE* > > NODEMAP;
typedef std::pair< std::string, std::vector< SGNODE* > > NODEITEM;

struct DATA;

bool processNode( const TopoDS_Shape& shape, DATA& data, SGNODE* parent,
                  std::vector< SGNODE* >* items );


bool processComp( const TopoDS_Shape& shape, DATA& data, SGNODE* parent,
                  std::vector< SGNODE* >* items );


bool processFace( const TopoDS_Face& face, DATA& data, SGNODE* parent,
                  std::vector< SGNODE* >* items, Quantity_Color* color );


struct DATA
{
    Handle( TDocStd_Document ) m_doc;
    Handle( XCAFDoc_ColorTool ) m_color;
    Handle( XCAFDoc_ShapeTool ) m_assy;
    SGNODE* scene;
    SGNODE* defaultColor;
    Quantity_Color refColor;
    NODEMAP  shapes;    // SGNODE lists representing a TopoDS_SOLID / COMPOUND
    COLORMAP colors;    // SGAPPEARANCE nodes
    FACEMAP  faces;     // SGSHAPE items representing a TopoDS_FACE
    bool renderBoth;    // set TRUE if we're processing IGES
    bool hasSolid;      // set TRUE if there is no parent SOLID

    DATA()
    {
        scene = nullptr;
        defaultColor = nullptr;
        refColor.SetValues( Quantity_NOC_BLACK );
        renderBoth = false;
        hasSolid = false;
    }

    ~DATA()
    {
        // destroy any colors with no parent
        if( !colors.empty() )
        {
            COLORMAP::iterator sC = colors.begin();
            COLORMAP::iterator eC = colors.end();

            while( sC != eC )
            {
                if( nullptr == S3D::GetSGNodeParent( sC->second ) )
                    S3D::DestroyNode( sC->second );

                ++sC;
            }

            colors.clear();
        }

        if( defaultColor && nullptr == S3D::GetSGNodeParent( defaultColor ) )
            S3D::DestroyNode(defaultColor);

        // destroy any faces with no parent
        if( !faces.empty() )
        {
            FACEMAP::iterator sF = faces.begin();
            FACEMAP::iterator eF = faces.end();

            while( sF != eF )
            {
                if( nullptr == S3D::GetSGNodeParent( sF->second ) )
                    S3D::DestroyNode( sF->second );

                ++sF;
            }

            faces.clear();
        }

        // destroy any shapes with no parent
        if( !shapes.empty() )
        {
            NODEMAP::iterator sS = shapes.begin();
            NODEMAP::iterator eS = shapes.end();

            while( sS != eS )
            {
                std::vector< SGNODE* >::iterator sV = sS->second.begin();
                std::vector< SGNODE* >::iterator eV = sS->second.end();

                while( sV != eV )
                {
                    if( nullptr == S3D::GetSGNodeParent( *sV ) )
                        S3D::DestroyNode( *sV );

                    ++sV;
                }

                sS->second.clear();
                ++sS;
            }

            shapes.clear();
        }

        if( scene )
            S3D::DestroyNode(scene);
    }

    // find collection of tagged nodes
    bool GetShape( const std::string& id, std::vector< SGNODE* >*& listPtr )
    {
        listPtr = nullptr;
        NODEMAP::iterator item;
        item = shapes.find( id );

        if( item == shapes.end() )
            return false;

        listPtr = &item->second;
        return true;
    }

    // find collection of tagged nodes
    SGNODE* GetFace( const std::string& id )
    {
        FACEMAP::iterator item;
        item = faces.find( id );

        if( item == faces.end() )
            return nullptr;

        return item->second;
    }

    // return color if found; if not found, create SGAPPEARANCE
    SGNODE* GetColor( Quantity_Color* colorObj )
    {
        if( nullptr == colorObj )
        {
            if( defaultColor )
                return defaultColor;

            IFSG_APPEARANCE app( true );
            app.SetShininess( 0.05f );
            app.SetSpecular( 0.04f, 0.04f, 0.04f );
            app.SetAmbient( 0.1f, 0.1f, 0.1f );
            app.SetDiffuse( 0.6f, 0.6f, 0.6f );

            defaultColor = app.GetRawPtr();
            return defaultColor;
        }

        Standard_Real id = colorObj->Distance( refColor );
        std::map< Standard_Real, SGNODE* >::iterator item;
        item = colors.find( id );

        if( item != colors.end() )
            return item->second;

        IFSG_APPEARANCE app( true );
        app.SetShininess( 0.1f );
        app.SetSpecular( 0.12f, 0.12f, 0.12f );
        app.SetAmbient( 0.1f, 0.1f, 0.1f );
        app.SetDiffuse( colorObj->Red(), colorObj->Green(), colorObj->Blue() );
        colors.insert( std::pair< Standard_Real, SGNODE* >( id, app.GetRawPtr() ) );

        return app.GetRawPtr();
    }
};


enum FormatType
{
    FMT_NONE = 0,
    FMT_STEP,
    FMT_STPZ,
    FMT_IGES
};


FormatType fileType( const char* aFileName )
{
    wxFileName fname( wxString::FromUTF8Unchecked( aFileName ) );
    wxFFileInputStream ifile( fname.GetFullPath() );

    if( !ifile.IsOk() )
        return FMT_NONE;

    if( fname.GetExt().MakeUpper().EndsWith( "STPZ" ) ||
        fname.GetExt().MakeUpper().EndsWith( "GZ" ) )
        return FMT_STPZ;

    char iline[82];
    memset( iline, 0, 82 );
    ifile.Read( iline, 82 );
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


void getTag( TDF_Label& label, std::string& aTag )
{
    if( label.IsNull() )
        return;

    std::string rtag;   // tag in reverse
    aTag.clear();
    int id = label.Tag();
    std::ostringstream ostr;
    ostr << id;
    rtag = ostr.str();
    ostr.str( "" );
    ostr.clear();

    TDF_Label nlab = label.Father();

    while( !nlab.IsNull() )
    {
        rtag.append( 1, ':' );
        id = nlab.Tag();
        ostr << id;
        rtag.append( ostr.str() );
        ostr.str( "" );
        ostr.clear();
        nlab = nlab.Father();
    };

    std::string::reverse_iterator bI = rtag.rbegin();
    std::string::reverse_iterator eI = rtag.rend();

    while( bI != eI )
    {
        aTag.append( 1, *bI );
        ++bI;
    }
}


bool getColor( DATA& data, TDF_Label label, Quantity_Color& color )
{
    while( true )
    {
        if( data.m_color->GetColor( label, XCAFDoc_ColorGen, color ) )
            return true;
        else if( data.m_color->GetColor( label, XCAFDoc_ColorSurf, color ) )
            return true;
        else if( data.m_color->GetColor( label, XCAFDoc_ColorCurv, color ) )
            return true;

        label = label.Father();

        if( label.IsNull() )
            break;
    };

    return false;
}


void addItems( SGNODE* parent, std::vector< SGNODE* >* lp )
{
    if( nullptr == lp )
        return;

    std::vector< SGNODE* >::iterator sL = lp->begin();
    std::vector< SGNODE* >::iterator eL = lp->end();
    SGNODE* item;

    while( sL != eL )
    {
        item = *sL;

        if( nullptr == S3D::GetSGNodeParent( item ) )
            S3D::AddSGNodeChild( parent, item );
        else
            S3D::AddSGNodeRef( parent, item );

        ++sL;
    }
}


bool readIGES( Handle( TDocStd_Document ) & m_doc, const char* fname )
{
    IGESCAFControl_Reader reader;
    IFSelect_ReturnStatus stat  = reader.ReadFile( fname );
    reader.PrintCheckLoad( Standard_False, IFSelect_ItemsByEntity );

    if( stat != IFSelect_RetDone )
        return false;

    // Enable file-defined shape precision
    if( !Interface_Static::SetIVal( "read.precision.mode", 0 ) )
        return false;

    // set other translation options
    reader.SetColorMode(true);  // use model colors
    reader.SetNameMode(false);  // don't use IGES label names
    reader.SetLayerMode(false); // ignore LAYER data

    if ( !reader.Transfer( m_doc ) )
        return false;

    // are there any shapes to translate?
    if( reader.NbShapes() < 1 )
        return false;

    return true;
}


bool readSTEP( Handle(TDocStd_Document)& m_doc, const char* fname )
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
    reader.SetColorMode( true );  // use model colors
    reader.SetNameMode( false );  // don't use label names
    reader.SetLayerMode( false ); // ignore LAYER data

    if ( !reader.Transfer( m_doc ) )
    {
        m_doc->Close();
        return false;
    }

    // are there any shapes to translate?
    if( reader.NbRootsForTransfer() < 1 )
        return false;

    return true;
}


bool readSTEPZ( Handle(TDocStd_Document)& m_doc, const char* aFileName )
{
    wxFileName fname( wxString::FromUTF8Unchecked( aFileName ) );
    wxFFileInputStream ifile( fname.GetFullPath() );

    wxFileName outFile( fname );

    outFile.SetPath( wxStandardPaths::Get().GetTempDir() );
    outFile.SetExt( "STEP" );

    wxFileOffset size = ifile.GetLength();
    wxBusyCursor busycursor;

    if( size == wxInvalidOffset )
        return false;

    {
        bool success = false;
        wxFFileOutputStream ofile( outFile.GetFullPath() );

        if( !ofile.IsOk() )
            return false;

        char *buffer = new char[size];

        ifile.Read( buffer, size);
        std::string expanded;

        try
        {
            expanded = gzip::decompress( buffer, size );
            success = true;
        }
        catch(...)
        {}

        if( expanded.empty() )
        {
            ifile.Reset();
            ifile.SeekI( 0 );
            wxZipInputStream izipfile( ifile );
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

        if( !success )
            return false;
    }

    bool retval = readSTEP( m_doc, outFile.GetFullPath().mb_str() );

    // Cleanup our temporary file
    wxRemoveFile( outFile.GetFullPath() );

    return retval;
}


SCENEGRAPH* LoadModel( char const* filename )
{
    DATA data;

    Handle(XCAFApp_Application) m_app = XCAFApp_Application::GetApplication();
    m_app->NewDocument( "MDTV-XCAF", data.m_doc );
    FormatType modelFmt = fileType( filename );

    switch( modelFmt )
    {
    case FMT_IGES:
        data.renderBoth = true;

        if( !readIGES( data.m_doc, filename ) )
            return nullptr;

        break;

    case FMT_STEP:
        if( !readSTEP( data.m_doc, filename ) )
            return nullptr;

        break;

    case FMT_STPZ:
        if( !readSTEPZ( data.m_doc, filename ) )
            return nullptr;

        break;


    default:
        return nullptr;
        break;
    }

    data.m_assy = XCAFDoc_DocumentTool::ShapeTool( data.m_doc->Main() );
    data.m_color = XCAFDoc_DocumentTool::ColorTool( data.m_doc->Main() );

    // retrieve all free shapes
    TDF_LabelSequence frshapes;
    data.m_assy->GetFreeShapes( frshapes );

    int nshapes = frshapes.Length();
    int id = 1;
    bool ret = false;

    // create the top level SG node
    IFSG_TRANSFORM topNode( true );
    data.scene = topNode.GetRawPtr();

    while( id <= nshapes )
    {
        TopoDS_Shape shape = data.m_assy->GetShape( frshapes.Value( id ) );

        if ( !shape.IsNull() && processNode( shape, data, data.scene, nullptr ) )
            ret = true;

        ++id;
    };

    if( !ret )
        return nullptr;

    SCENEGRAPH* scene = (SCENEGRAPH*)data.scene;

    // DEBUG: WRITE OUT VRML2 FILE TO CONFIRM STRUCTURE
#if ( defined( DEBUG_OCE ) && DEBUG_OCE > 3 )
    if( data.scene )
    {
        wxFileName fn( wxString::FromUTF8Unchecked( filename ) );
        wxString output;

        if( FMT_STEP == modelFmt )
            output = wxT( "_step-" );
        else
            output = wxT( "_iges-" );

        output.append( fn.GetName() );
        output.append( wxT( ".wrl" ) );
        S3D::WriteVRML( output.ToUTF8(), true, data.scene, true, true );
    }
#endif

    // set to NULL to prevent automatic destruction of the scene data
    data.scene = nullptr;

    return scene;
}


bool processShell( const TopoDS_Shape& shape, DATA& data, SGNODE* parent,
                   std::vector< SGNODE* >* items, Quantity_Color* color )
{
    TopoDS_Iterator it;
    bool ret = false;

    for( it.Initialize( shape, false, false ); it.More(); it.Next() )
    {
        const TopoDS_Face& face = TopoDS::Face( it.Value() );

        if( processFace( face, data, parent, items, color ) )
            ret = true;
    }

    return ret;
}


bool processSolid( const TopoDS_Shape& shape, DATA& data, SGNODE* parent,
                   std::vector< SGNODE* >* items )
{
    TDF_Label label;
    data.hasSolid = true;
    std::string partID;
    Quantity_Color col;
    Quantity_Color* lcolor = nullptr;

    // Search the whole model first to make sure something exists (may or may not have color)
    if( !data.m_assy->Search( shape, label ) )
    {
        static int i = 0;
        std::ostringstream ostr;
        ostr << "KMISC_" << i++;
        partID = ostr.str();
    }
    else
    {
        bool found_color = false;

        if( getColor( data, label, col ) )
        {
            found_color = true;
            lcolor = &col;
        }

        // If the top-level label doesn't have the color information, search components
        if( !found_color )
        {
            if( data.m_assy->Search( shape, label, Standard_False, Standard_True, Standard_True ) &&
                getColor( data, label, col ) )
            {
                found_color = true;
                lcolor = &col;
            }
        }

        // If the components do not have color information, search all components without location
        if( !found_color )
        {
            if( data.m_assy->Search( shape, label, Standard_False, Standard_False,
                                     Standard_True ) &&
                getColor( data, label, col ) )
            {
                found_color = true;
                lcolor = &col;
            }
        }

        // Our last chance to find the color looks for color as a subshape of top-level simple
        // shapes.
        if( !found_color )
        {
            if( data.m_assy->Search( shape, label, Standard_False, Standard_False,
                                     Standard_False ) &&
                getColor( data, label, col ) )
            {
                found_color = true;
                lcolor = &col;
            }
        }

        getTag( label, partID );
    }

    TopoDS_Iterator it;
    IFSG_TRANSFORM childNode( parent );
    SGNODE* pptr = childNode.GetRawPtr();
    const TopLoc_Location& loc = shape.Location();
    bool ret = false;

    if( !loc.IsIdentity() )
    {
        gp_Trsf T = loc.Transformation();
        gp_XYZ coord = T.TranslationPart();
        childNode.SetTranslation( SGPOINT( coord.X(), coord.Y(), coord.Z() ) );
        gp_XYZ axis;
        Standard_Real angle;

        if( T.GetRotation( axis, angle ) )
            childNode.SetRotation( SGVECTOR( axis.X(), axis.Y(), axis.Z() ), angle );
    }

    std::vector< SGNODE* >* component = nullptr;

    if( !partID.empty() )
        data.GetShape( partID, component );

    if( component )
    {
        addItems( pptr, component );

        if( nullptr != items )
            items->push_back( pptr );
    }

    // instantiate the solid
    std::vector< SGNODE* > itemList;

    for( it.Initialize( shape, false, false ); it.More(); it.Next() )
    {
        const TopoDS_Shape& subShape = it.Value();

        if( processShell( subShape, data, pptr, &itemList, lcolor ) )
            ret = true;
    }

    if( !ret )
        childNode.Destroy();
    else if( nullptr != items )
        items->push_back( pptr );

    return ret;
}


bool processComp( const TopoDS_Shape& shape, DATA& data, SGNODE* parent,
                  std::vector< SGNODE* >* items )
{
    TopoDS_Iterator it;
    IFSG_TRANSFORM childNode( parent );
    SGNODE* pptr = childNode.GetRawPtr();
    const TopLoc_Location& loc = shape.Location();
    bool ret = false;

    if( !loc.IsIdentity() )
    {
        gp_Trsf T = loc.Transformation();
        gp_XYZ coord = T.TranslationPart();
        childNode.SetTranslation( SGPOINT( coord.X(), coord.Y(), coord.Z() ) );
        gp_XYZ axis;
        Standard_Real angle;

        if( T.GetRotation( axis, angle ) )
            childNode.SetRotation( SGVECTOR( axis.X(), axis.Y(), axis.Z() ), angle );
    }

    for( it.Initialize( shape, false, false ); it.More(); it.Next() )
    {
        const TopoDS_Shape& subShape = it.Value();
        TopAbs_ShapeEnum stype = subShape.ShapeType();
        data.hasSolid = false;

        switch( stype )
        {
        case TopAbs_COMPOUND:
        case TopAbs_COMPSOLID:
            if( processComp( subShape, data, pptr, items ) )
                ret = true;

            break;

        case TopAbs_SOLID:
            if( processSolid( subShape, data, pptr, items ) )
                ret = true;

            break;

        case TopAbs_SHELL:
            if( processShell( subShape, data, pptr, items, nullptr ) )
                ret = true;

            break;

        case TopAbs_FACE:
            if( processFace( TopoDS::Face( subShape ), data, pptr, items, nullptr ) )
                ret = true;

            break;

        default:
            break;
        }
    }

    if( !ret )
        childNode.Destroy();
    else if( nullptr != items )
        items->push_back( pptr );

    return ret;
}


bool processNode( const TopoDS_Shape& shape, DATA& data, SGNODE* parent,
    std::vector< SGNODE* >* items )
{
    TopAbs_ShapeEnum stype = shape.ShapeType();
    bool ret = false;
    data.hasSolid = false;

    switch( stype )
    {
    case TopAbs_COMPOUND:
    case TopAbs_COMPSOLID:
        if( processComp( shape, data, parent, items ) )
            ret = true;

        break;

    case TopAbs_SOLID:
        if( processSolid( shape, data, parent, items ) )
            ret = true;

        break;

    case TopAbs_SHELL:
        if( processShell( shape, data, parent, items, nullptr ) )
            ret = true;

        break;

    case TopAbs_FACE:
        if( processFace( TopoDS::Face( shape ), data, parent, items, nullptr ) )
            ret = true;

        break;

    default:
        break;
    }

    return ret;
}


bool processFace( const TopoDS_Face& face, DATA& data, SGNODE* parent,
                  std::vector< SGNODE* >* items, Quantity_Color* color )
{
    if( Standard_True == face.IsNull() )
        return false;

    bool reverse = ( face.Orientation() == TopAbs_REVERSED );
    SGNODE* ashape = nullptr;
    std::string partID;
    TDF_Label label;

    bool useBothSides = false;

    // for IGES renderBoth = TRUE; for STEP if a shell or face is not a descendant
    // of a SOLID then hasSolid = false and we must render both sides
    if( data.renderBoth || !data.hasSolid )
        useBothSides = true;

    if( data.m_assy->FindShape( face, label, Standard_False ) )
        getTag( label, partID );

    if( !partID.empty() )
        ashape = data.GetFace( partID );

    if( ashape )
    {
        if( nullptr == S3D::GetSGNodeParent( ashape ) )
            S3D::AddSGNodeChild( parent, ashape );
        else
            S3D::AddSGNodeRef( parent, ashape );

        if( nullptr != items )
            items->push_back( ashape );

        if( useBothSides )
        {
            std::string id2 = partID;
            id2.append( "b" );
            SGNODE* shapeB = data.GetFace( id2 );

            if( nullptr == S3D::GetSGNodeParent( shapeB ) )
                S3D::AddSGNodeChild( parent, shapeB );
            else
                S3D::AddSGNodeRef( parent, shapeB );

            if( nullptr != items )
                items->push_back( shapeB );
        }

        return true;
    }

    TopLoc_Location loc;
    Standard_Boolean isTessellate (Standard_False);
    Handle( Poly_Triangulation ) triangulation = BRep_Tool::Triangulation( face, loc );

    if( triangulation.IsNull() || triangulation->Deflection() > USER_PREC + Precision::Confusion() )
        isTessellate = Standard_True;

    if( isTessellate )
    {
        BRepMesh_IncrementalMesh IM(face, USER_PREC, Standard_False, USER_ANGLE );
        triangulation = BRep_Tool::Triangulation( face, loc );
    }

    if( triangulation.IsNull() == Standard_True )
        return false;

    Quantity_Color lcolor;

    // check for a face color; this has precedence over SOLID colors
    do
    {
        TDF_Label L;

        if( data.m_color->ShapeTool()->Search( face, L ) )
        {
            if( data.m_color->GetColor( L, XCAFDoc_ColorGen, lcolor )
                || data.m_color->GetColor( L, XCAFDoc_ColorCurv, lcolor )
                || data.m_color->GetColor( L, XCAFDoc_ColorSurf, lcolor ) )
                color = &lcolor;
        }
    } while( 0 );

    SGNODE* ocolor = data.GetColor( color );

    // create a SHAPE and attach the color and data,
    // then attach the shape to the parent and return TRUE
    IFSG_SHAPE vshape( true );
    IFSG_FACESET vface( vshape );
    IFSG_COORDS vcoords( vface );
    IFSG_COORDINDEX coordIdx( vface );

    if( nullptr == S3D::GetSGNodeParent( ocolor ) )
        S3D::AddSGNodeChild( vshape.GetRawPtr(), ocolor );
    else
        S3D::AddSGNodeRef( vshape.GetRawPtr(), ocolor );

    const TColgp_Array1OfPnt&    arrPolyNodes = triangulation->Nodes();
    const Poly_Array1OfTriangle& arrTriangles = triangulation->Triangles();

    std::vector< SGPOINT > vertices;
    std::vector< int > indices;
    std::vector< int > indices2;
    gp_Trsf tx;

    for( int i = 1; i <= triangulation->NbNodes(); i++ )
    {
        gp_XYZ v( arrPolyNodes(i).Coord() );
        vertices.emplace_back( v.X(), v.Y(), v.Z() );
    }

    for( int i = 1; i <= triangulation->NbTriangles(); i++ )
    {
        int a, b, c;
        arrTriangles( i ).Get( a, b, c );
        a--;

        if( reverse )
        {
            int tmp = b - 1;
            b = c - 1;
            c = tmp;
        }
        else
        {
            b--;
            c--;
        }

        indices.push_back( a );
        indices.push_back( b );
        indices.push_back( c );

        if( useBothSides )
        {
            indices2.push_back( b );
            indices2.push_back( a );
            indices2.push_back( c );
        }
    }

    vcoords.SetCoordsList( vertices.size(), &vertices[0] );
    coordIdx.SetIndices( indices.size(), &indices[0] );
    vface.CalcNormals( nullptr );
    vshape.SetParent( parent );

    if( !partID.empty() )
        data.faces.insert( std::pair< std::string, SGNODE* >( partID, vshape.GetRawPtr() ) );

    // The outer surface of an IGES model is indeterminate so
    // we must render both sides of a surface.
    if( useBothSides )
    {
        std::string id2 = partID;
        id2.append( "b" );
        IFSG_SHAPE vshape2( true );
        IFSG_FACESET vface2( vshape2 );
        IFSG_COORDS vcoords2( vface2 );
        IFSG_COORDINDEX coordIdx2( vface2 );
        S3D::AddSGNodeRef( vshape2.GetRawPtr(), ocolor );

        vcoords2.SetCoordsList( vertices.size(), &vertices[0] );
        coordIdx2.SetIndices( indices2.size(), &indices2[0] );
        vface2.CalcNormals( nullptr );
        vshape2.SetParent( parent );

        if( !partID.empty() )
            data.faces.insert( std::pair< std::string, SGNODE* >( id2, vshape2.GetRawPtr() ) );
    }

    return true;
}
