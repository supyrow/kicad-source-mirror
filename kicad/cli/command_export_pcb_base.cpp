/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "command_export_pcb_base.h"
#include <cli/exit_codes.h>
#include <kiface_base.h>
#include <bitset>
#include <layer_ids.h>

#include <macros.h>
#include <wx/tokenzr.h>
#include <wx/crt.h>

CLI::EXPORT_PCB_BASE_COMMAND::EXPORT_PCB_BASE_COMMAND( std::string aName ) : COMMAND( aName )
{
    m_argParser.add_argument( "-o", ARG_OUTPUT )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Output file name" ) ) );

    m_argParser.add_argument( ARG_INPUT ).help( UTF8STDSTR( _( "Input file" ) ) );

    for( int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        std::string untranslated = TO_UTF8( wxString( LSET::Name( PCB_LAYER_ID( layer ) ) ) );

        //m_layerIndices[untranslated] = PCB_LAYER_ID( layer );
        m_layerMasks[untranslated] = LSET( PCB_LAYER_ID( layer ) );
    }

    m_layerMasks["*"] = LSET::AllLayersMask();
    m_layerMasks["*.Cu"] = LSET::AllCuMask();
    m_layerMasks["*In.Cu"] = LSET::InternalCuMask();
    m_layerMasks["F&B.Cu"] = LSET( 2, F_Cu, B_Cu );
    m_layerMasks["*.Adhes"] = LSET( 2, B_Adhes, F_Adhes );
    m_layerMasks["*.Paste"] = LSET( 2, B_Paste, F_Paste );
    m_layerMasks["*.Mask"] = LSET( 2, B_Mask, F_Mask );
    m_layerMasks["*.SilkS"] = LSET( 2, B_SilkS, F_SilkS );
    m_layerMasks["*.Fab"] = LSET( 2, B_Fab, F_Fab );
    m_layerMasks["*.CrtYd"] = LSET( 2, B_CrtYd, F_CrtYd );
}


LSET CLI::EXPORT_PCB_BASE_COMMAND::convertLayerStringList( wxString& aLayerString ) const
{
    LSET layerMask;

    if( !aLayerString.IsEmpty() )
    {
        layerMask.reset();
        wxStringTokenizer layerTokens( aLayerString, "," );
        while( layerTokens.HasMoreTokens() )
        {
            std::string token = TO_UTF8( layerTokens.GetNextToken() );
            if( m_layerMasks.count( token ) )
            {
                layerMask |= m_layerMasks.at(token);
            }
            else
            {
                wxFprintf( stderr, _( "Invalid layer name \"%s\"\n" ), token );
            }
        }
    }

    return layerMask;
}


void CLI::EXPORT_PCB_BASE_COMMAND::addLayerArg( bool aRequire )
{
    m_argParser.add_argument( "-l", ARG_LAYERS )
            .default_value( std::string() )
            .help( UTF8STDSTR(
                    _( "Comma separated list of untranslated layer names to include such as "
                       "F.Cu,B.Cu" ) ) );

    m_requireLayers = aRequire;
}


int CLI::EXPORT_PCB_BASE_COMMAND::Perform( KIWAY& aKiway )
{
    if( m_requireLayers )
    {
        wxString layers = FROM_UTF8( m_argParser.get<std::string>( ARG_LAYERS ).c_str() );

        LSET layerMask = convertLayerStringList( layers );
        if( layerMask.Seq().size() < 1 )
        {
            wxFprintf( stderr, _( "At least one or more layers must be specified\n" ) );
            return EXIT_CODES::ERR_ARGS;
        }

        m_selectedLayers = layerMask;
    }

    return EXIT_CODES::OK;
}