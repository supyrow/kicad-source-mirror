/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "exporter_step.h"
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <fp_lib_table.h>

#include <pgm_base.h>
#include <base_units.h>
#include <filename_resolver.h>

#include <Message.hxx>                // OpenCascade messenger
#include <Message_PrinterOStream.hxx> // OpenCascade output messenger
#include <Standard_Failure.hxx>       // In open cascade

#include <Standard_Version.hxx>

#include <wx/crt.h>

#define OCC_VERSION_MIN 0x070500

#if OCC_VERSION_HEX < OCC_VERSION_MIN
#include <Message_Messenger.hxx>
#endif

#define DEFAULT_BOARD_THICKNESS 1.6

void ReportMessage( const wxString& aMessage )
{
    wxPrintf( aMessage );
    fflush( stdout ); // Force immediate printing (needed on mingw)
}

class KiCadPrinter : public Message_Printer
{
public:
    KiCadPrinter( EXPORTER_STEP* aConverter ) : m_converter( aConverter ) {}

protected:
#if OCC_VERSION_HEX < OCC_VERSION_MIN
    virtual void Send( const TCollection_ExtendedString& theString,
                       const Message_Gravity theGravity,
                       const Standard_Boolean theToPutEol ) const override
    {
        Send( TCollection_AsciiString( theString ), theGravity, theToPutEol );
    }

    virtual void Send( const TCollection_AsciiString& theString,
                       const Message_Gravity theGravity,
                       const Standard_Boolean theToPutEol ) const override
#else
    virtual void send( const TCollection_AsciiString& theString,
                        const Message_Gravity theGravity ) const override
#endif
    {
      if( theGravity >= Message_Info )
      {
          ReportMessage( theString.ToCString() );

#if OCC_VERSION_HEX < OCC_VERSION_MIN
          if( theToPutEol )
              ReportMessage( wxT( "\n" ) );
#else
          ReportMessage( wxT( "\n" ) );
#endif
      }

      if( theGravity >= Message_Alarm )
          m_converter->SetError();

      if( theGravity == Message_Fail )
          m_converter->SetFail();
    }

private:
    EXPORTER_STEP* m_converter;
};


EXPORTER_STEP::EXPORTER_STEP( BOARD* aBoard, const EXPORTER_STEP_PARAMS& aParams ) :
    m_params( aParams ),
    m_error( false ),
    m_fail( false ),
    m_hasDrillOrigin( false ),
    m_hasGridOrigin( false ),
    m_board( aBoard ),
    m_pcbModel( nullptr ),
    m_pcbName(),
    m_minDistance( STEPEXPORT_MIN_DISTANCE ),
    m_boardThickness( DEFAULT_BOARD_THICKNESS )
{
    m_solderMaskColor = COLOR4D( 0.08, 0.20, 0.14, 0.83 );

    m_resolver = std::make_unique<FILENAME_RESOLVER>();
    m_resolver->Set3DConfigDir( wxT( "" ) );
    m_resolver->SetProgramBase( &Pgm() );
}


EXPORTER_STEP::~EXPORTER_STEP()
{
}


bool EXPORTER_STEP::composePCB( FOOTPRINT* aFootprint, VECTOR2D aOrigin )
{
    bool hasdata = false;

    if( ( aFootprint->GetAttributes() & FP_EXCLUDE_FROM_BOM ) && !m_params.m_includeExcludedBom )
    {
        return hasdata;
    }

    // Prefetch the library for this footprint
    // In case we need to resolve relative footprint paths
    wxString libraryName = aFootprint->GetFPID().GetLibNickname();
    wxString footprintBasePath = wxEmptyString;

    double posX = aFootprint->GetPosition().x - aOrigin.x;
    double posY = (aFootprint->GetPosition().y) - aOrigin.y;

    if( m_board->GetProject() )
    {
        try
        {
            // FindRow() can throw an exception
            const FP_LIB_TABLE_ROW* fpRow =
                    m_board->GetProject()->PcbFootprintLibs()->FindRow( libraryName, false );

            if( fpRow )
                footprintBasePath = fpRow->GetFullURI( true );
        }
        catch( ... )
        {
            // Do nothing if the libraryName is not found in lib table
        }
    }

    // Dump the pad holes into the PCB
    for( PAD* pad : aFootprint->Pads() )
    {
        if( m_pcbModel->AddPadHole( pad, aOrigin ) )
            hasdata = true;
    }

    // Exit early if we don't want to include footprint models
    if( m_params.m_boardOnly )
    {
        return hasdata;
    }

    VECTOR2D newpos( pcbIUScale.IUTomm( posX ), pcbIUScale.IUTomm( posY ) );

    for( const FP_3DMODEL& fp_model : aFootprint->Models() )
    {
        if( !fp_model.m_Show || fp_model.m_Filename.empty() )

            continue;

        std::vector<wxString> searchedPaths;
        wxString mname = m_resolver->ResolvePath( fp_model.m_Filename, wxEmptyString );


        if( !wxFileName::FileExists( mname ) )
        {
            ReportMessage( wxString::Format( wxT( "Could not add 3D model to %s.\n"
                                                  "File not found: %s\n" ),
                                             aFootprint->GetReference(), mname ) );
            continue;
        }

        std::string fname( mname.ToUTF8() );
        std::string refName( aFootprint->GetReference().ToUTF8() );
        try
        {
            bool bottomSide = aFootprint->GetLayer() == B_Cu;

            // the rotation is stored in degrees but opencascade wants radians
            VECTOR3D modelRot = fp_model.m_Rotation;
            modelRot *= M_PI;
            modelRot /= 180.0;

            if( m_pcbModel->AddComponent( fname, refName, bottomSide,
                                          newpos,
                                          aFootprint->GetOrientation().AsRadians(),
                                          fp_model.m_Offset, modelRot,
                                          fp_model.m_Scale, m_params.m_substModels ) )
            {
                hasdata = true;
            }
        }
        catch( const Standard_Failure& e )
        {
            ReportMessage( wxString::Format( wxT( "Could not add 3D model to %s.\n"
                                                  "OpenCASCADE error: %s\n" ),
                                             aFootprint->GetReference(), e.GetMessageString() ) );
        }

    }

    return hasdata;
}


bool EXPORTER_STEP::composePCB()
{
    if( m_pcbModel )
        return true;

    SHAPE_POLY_SET pcbOutlines; // stores the board main outlines

    if( !m_board->GetBoardPolygonOutlines( pcbOutlines ) )
    {
        wxLogWarning( _( "Board outline is malformed. Run DRC for a full analysis." ) );
    }

    VECTOR2D origin;

    // Determine the coordinate system reference:
    // Precedence of reference point is Drill Origin > Grid Origin > User Offset
    if( m_params.m_useDrillOrigin )
        origin = m_board->GetDesignSettings().GetAuxOrigin();
    else if( m_params.m_useGridOrigin )
        origin = m_board->GetDesignSettings().GetGridOrigin();
    else
        origin = m_params.m_origin;

    m_pcbModel = std::make_unique<STEP_PCB_MODEL>( m_pcbName );

    // TODO: Handle when top & bottom soldermask colours are different...
    m_pcbModel->SetBoardColor( m_solderMaskColor.r, m_solderMaskColor.g, m_solderMaskColor.b );

    m_pcbModel->SetPCBThickness( m_boardThickness );
    m_pcbModel->SetMinDistance(
            std::max( m_params.m_minDistance, STEPEXPORT_MIN_ACCEPTABLE_DISTANCE ) );

    m_pcbModel->SetMaxError( m_board->GetDesignSettings().m_MaxError );

    for( FOOTPRINT* i : m_board->Footprints() )
        composePCB( i, origin );

    ReportMessage( wxT( "Create PCB solid model\n" ) );

    if( !m_pcbModel->CreatePCB( pcbOutlines, origin ) )
    {
        ReportMessage( wxT( "could not create PCB solid model\n" ) );
        return false;
    }

    return true;
}


void EXPORTER_STEP::determinePcbThickness()
{
    m_boardThickness = DEFAULT_BOARD_THICKNESS;

    const BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    if( bds.GetStackupDescriptor().GetCount() )
    {
        int thickness = 0;

        for( BOARD_STACKUP_ITEM* item : bds.GetStackupDescriptor().GetList() )
        {
            switch( item->GetType() )
            {
            case BS_ITEM_TYPE_DIELECTRIC:
                thickness += item->GetThickness();
                break;

            case BS_ITEM_TYPE_COPPER:
                if( item->IsEnabled() )
                    thickness += item->GetThickness();

                break;

            default:
                break;
            }
        }

        m_boardThickness = pcbIUScale.IUTomm( thickness );
    }
}


bool EXPORTER_STEP::Export()
{
    // setup opencascade message log
    Message::DefaultMessenger()->RemovePrinters( STANDARD_TYPE( Message_PrinterOStream ) );
    Message::DefaultMessenger()->AddPrinter( new KiCadPrinter( this ) );

    ReportMessage( _( "Determining PCB data\n" ) );
    determinePcbThickness();

    try
    {
        ReportMessage( _( "Build STEP data\n" ) );

        if( !composePCB() )
        {
            ReportMessage( _( "\n** Error building STEP board model. Export aborted. **\n" ) );
            return false;
        }

        ReportMessage( _( "Writing STEP file\n" ) );

        if( !m_pcbModel->WriteSTEP( m_outputFile ) )
        {
            ReportMessage( _( "\n** Error writing STEP file. **\n" ) );
            return false;
        }
        else
        {
            ReportMessage( wxString::Format( _( "\nSTEP file '%s' created.\n" ), m_outputFile ) );
        }
    }
    catch( const Standard_Failure& e )
    {
        ReportMessage( e.GetMessageString() );
        ReportMessage( _( "\n** Error exporting STEP file. Export aborted. **\n" ) );
        return false;
    }
    catch( ... )
    {
        ReportMessage( _( "\n** Error exporting STEP file. Export aborted. **\n" ) );
        return false;
    }

    if( m_fail || m_error )
    {
        wxString msg;

        if( m_fail )
        {
            msg = _( "Unable to create STEP file.\n"
                     "Check that the board has a valid outline and models." );
        }
        else if( m_error )
        {
            msg = _( "STEP file has been created, but there are warnings." );
        }

        ReportMessage( msg );
    }

    return true;
}