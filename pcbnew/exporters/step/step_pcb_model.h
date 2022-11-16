/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef OCE_VIS_OCE_UTILS_H
#define OCE_VIS_OCE_UTILS_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <BRepBuilderAPI_MakeWire.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>

#include <math/vector2d.h>
#include <math/vector3.h>
#include <geometry/shape_poly_set.h>

///< Default minimum distance between points to treat them as separate ones (mm)
static constexpr double STEPEXPORT_MIN_DISTANCE = 0.01;
static constexpr double STEPEXPORT_MIN_ACCEPTABLE_DISTANCE = 0.001;

class PAD;

typedef std::pair< std::string, TDF_Label > MODEL_DATUM;
typedef std::map< std::string, TDF_Label > MODEL_MAP;

extern void ReportMessage( const wxString& aMessage );

class STEP_PCB_MODEL
{
public:
    STEP_PCB_MODEL( const wxString& aPcbName );
    virtual ~STEP_PCB_MODEL();

    // add a pad hole or slot (must be in final position)
    bool AddPadHole( const PAD* aPad, const VECTOR2D& aOrigin );

    // add a component at the given position and orientation
    bool AddComponent( const std::string& aFileName, const std::string& aRefDes, bool aBottom,
                       VECTOR2D aPosition, double aRotation, VECTOR3D aOffset,
                       VECTOR3D aOrientation, VECTOR3D aScale, bool aSubstituteModels = true );

    void SetBoardColor( double r, double g, double b );

    // set the thickness of the PCB (mm); the top of the PCB shall be at Z = aThickness
    // aThickness < 0.0 == use default thickness
    // aThickness <= THICKNESS_MIN == use THICKNESS_MIN
    // aThickness > THICKNESS_MIN == use aThickness
    void SetPCBThickness( double aThickness );

    // Set the minimum distance (in mm) to consider 2 points have the same coordinates
    void SetMinDistance( double aDistance );

    void SetMaxError( int aMaxError ) { m_maxError = aMaxError; }

    // create the PCB model using the current outlines and drill holes
    bool CreatePCB( SHAPE_POLY_SET& aOutline, VECTOR2D aOrigin );

    bool MakeShape( TopoDS_Shape& aShape, const SHAPE_LINE_CHAIN& chain, double aThickness,
                    const VECTOR2D& aOrigin );

#ifdef SUPPORTS_IGES
    // write the assembly model in IGES format
    bool WriteIGES( const wxString& aFileName );
#endif

    // write the assembly model in STEP format
    bool WriteSTEP( const wxString& aFileName );

private:
    /**
     * @return true if the board(s) outline is valid. False otherwise
     */
    bool isBoardOutlineValid();

    /** create one solid board using current outline and drill holes set
     * @param aIdx is the main outline index
     * @param aOutline is the set of outlines with holes
     * @param aOrigin is the coordinate origin for 3 view
     */
    bool createOneBoard( int aIdx, SHAPE_POLY_SET& aOutline, VECTOR2D aOrigin );

    /**
     * Load a 3D model data.
     *
     * @param aFileNameUTF8 is the filename encoded UTF8 (different formats allowed)
     * but for WRML files a model data can be loaded instead of the vrml data,
     * not suitable in a step file.
     * @param aScale is the X,Y,Z scaling factors.
     * @param aLabel is the TDF_Label to store the data.
     * @param aSubstituteModels = true to allows data substitution, false to disallow.
     * @param aErrorMessage (can be nullptr) is an error message to be displayed on error.
     * @return true if successfully loaded, false on error.
     */
    bool getModelLabel( const std::string& aFileNameUTF8, VECTOR3D aScale, TDF_Label& aLabel,
                        bool aSubstituteModels, wxString* aErrorMessage = nullptr );

    bool getModelLocation( bool aBottom, VECTOR2D aPosition, double aRotation, VECTOR3D aOffset,
                           VECTOR3D aOrientation, TopLoc_Location& aLocation );

    bool readIGES( Handle( TDocStd_Document )& m_doc, const char* fname );
    bool readSTEP( Handle( TDocStd_Document )& m_doc, const char* fname );

    TDF_Label transferModel( Handle( TDocStd_Document )& source, Handle( TDocStd_Document ) & dest,
                             VECTOR3D aScale );

    Handle( XCAFApp_Application )   m_app;
    Handle( TDocStd_Document )      m_doc;
    Handle( XCAFDoc_ShapeTool )     m_assy;
    TDF_Label                       m_assy_label;
    bool                            m_hasPCB;       // set true if CreatePCB() has been invoked
    std::vector<TDF_Label>          m_pcb_labels;   // labels for the PCB model (one by main outline)
    MODEL_MAP                       m_models;       // map of file names to model labels
    int                             m_components;   // number of successfully loaded components;
    double                          m_precision;    // model (length unit) numeric precision
    double                          m_angleprec;    // angle numeric precision
    double                          m_boardColor[3];// RGB values
    double                          m_thickness;    // PCB thickness, mm

    double                          m_minx;         // leftmost curve point
    double                          m_minDistance2; // minimum squared distance between items (mm)

    std::vector<TopoDS_Shape>       m_cutouts;

    /// Name of the PCB, which will most likely be the file name of the path.
    wxString                        m_pcbName;

    int                             m_maxError;
};

#endif // OCE_VIS_OCE_UTILS_H
