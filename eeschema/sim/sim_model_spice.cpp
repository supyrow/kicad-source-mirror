/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

// Include simulator headers after wxWidgets headers to avoid conflicts with Windows headers
// (especially on msys2 + wxWidgets 3.0.x)
#include <sim/sim_model_spice.h>
#include <sim/sim_model_raw_spice.h>
#include <sim/spice_model_parser.h>
#include <sim/sim_library_spice.h>

#include <boost/algorithm/string/trim.hpp>


std::string SPICE_GENERATOR_SPICE::Preview( const SPICE_ITEM& aItem ) const
{
    std::string spiceCode = ModelLine( aItem );

    if( spiceCode == "" )
        spiceCode = static_cast<const SIM_MODEL_SPICE&>( m_model ).m_spiceCode;

    if( spiceCode == "" && m_model.GetBaseModel() )
        spiceCode = static_cast<const SIM_MODEL_SPICE*>( m_model.GetBaseModel() )->m_spiceCode;

    SPICE_ITEM item = aItem;
    item.refName = "";
    std::string itemLine = ItemLine( item );

    if( spiceCode != "" )
        spiceCode.append( "\n" );

    spiceCode.append( itemLine );
    return boost::trim_copy( spiceCode );
}


std::unique_ptr<SIM_MODEL_SPICE> SIM_MODEL_SPICE::Create( const SIM_LIBRARY_SPICE& aLibrary,
                                                          const std::string& aSpiceCode )
{
    auto model = dynamic_cast<SIM_MODEL_SPICE*>(
            SIM_MODEL::Create( SPICE_MODEL_PARSER::ReadType( aLibrary, aSpiceCode ) ).release() );

    if( !model )
        THROW_IO_ERROR( "Could not determine Spice model type" );

    model->m_spiceModelParser->ReadModel( aLibrary, aSpiceCode );
    return std::unique_ptr<SIM_MODEL_SPICE>( model );
}


SIM_MODEL_SPICE::SIM_MODEL_SPICE( TYPE aType,
                                  std::unique_ptr<SPICE_GENERATOR> aSpiceGenerator ) :
    SIM_MODEL( aType, std::move( aSpiceGenerator ) ),
    m_spiceModelParser( std::make_unique<SPICE_MODEL_PARSER>( *this ) )
{
}


SIM_MODEL_SPICE::SIM_MODEL_SPICE( TYPE aType,
                                  std::unique_ptr<SPICE_GENERATOR> aSpiceGenerator,
                                  std::unique_ptr<SPICE_MODEL_PARSER> aSpiceModelParser ) :
    SIM_MODEL( aType, std::move( aSpiceGenerator ) ),
    m_spiceModelParser( std::move( aSpiceModelParser ) )
{
}


void SIM_MODEL_SPICE::SetParamValue( int aParamIndex, const SIM_VALUE& aValue )
{
    if( m_spiceCode != "" )
        THROW_IO_ERROR( "Could not change model parameters: library models are immutable" );

    SIM_MODEL::SetParamValue( aParamIndex, aValue );
}


void SIM_MODEL_SPICE::SetParamFromSpiceCode( const std::string& aParamName,
                                             const std::string& aParamValue,
                                             SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    SIM_MODEL::SetParamValue( aParamName, aParamValue, aNotation );
}
