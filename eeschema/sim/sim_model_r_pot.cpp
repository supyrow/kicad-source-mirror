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

#include <sim/sim_model_r_pot.h>
#include <fmt/core.h>


std::string SPICE_GENERATOR_R_POT::ModelLine( const SPICE_ITEM& aItem ) const
{
    std::string r = m_model.FindParam( "r" )->value->ToSpiceString();
    std::string position = m_model.FindParam( "pos" )->value->ToSpiceString();

    if( position != "" )
    {
        return fmt::format( ".model {} potentiometer( r={} position={} )\n", aItem.modelName, r,
                            position );
    }
    else
        return fmt::format( ".model {} potentiometer( r={} )\n", aItem.modelName, r );
}


std::string SPICE_GENERATOR_R_POT::ItemLine( const SPICE_ITEM& aItem ) const
{
    // Swap pin order so that pos=1 is all +, and pos=0 is all -.
    SPICE_ITEM item = aItem;

    // FIXME: This `if` is there because Preview() calls this function with empty pinNetNames vector.
    // This shouldn't be necessary.
    if( item.pinNetNames.size() >= 3 )
        std::swap( item.pinNetNames.at( 0 ), item.pinNetNames.at( 2 ) );

    return SPICE_GENERATOR::ItemLine( item );
}


std::string SPICE_GENERATOR_R_POT::TunerCommand( const SPICE_ITEM& aItem,
                                                 const SIM_VALUE_FLOAT& aValue ) const
{
    return fmt::format( "altermod @{}[position]={}",
                        aItem.model->SpiceGenerator().ItemName( aItem ), aValue.ToSpiceString() );
}


SIM_MODEL_R_POT::SIM_MODEL_R_POT() :
    SIM_MODEL( TYPE::R_POT, std::make_unique<SPICE_GENERATOR_R_POT>( *this ) )
{
    static std::vector<PARAM::INFO> paramInfos = makeParamInfos();

    for( const SIM_MODEL::PARAM::INFO& paramInfo : paramInfos )
        AddParam( paramInfo );
}


void SIM_MODEL_R_POT::WriteDataSchFields( std::vector<SCH_FIELD>& aFields ) const
{
    SIM_MODEL::WriteDataSchFields( aFields );

    if( IsInferred() )
        WriteInferredDataFields( aFields );
}


void SIM_MODEL_R_POT::WriteDataLibFields( std::vector<LIB_FIELD>& aFields ) const
{
    SIM_MODEL::WriteDataLibFields( aFields );

    if( IsInferred() )
        WriteInferredDataFields( aFields );
}


const std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_R_POT::makeParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "r";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "Ω";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Resistance";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "pos";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0.5";
    paramInfo.description = "Wiper position";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}
