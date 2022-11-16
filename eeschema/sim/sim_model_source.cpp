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

#include <sim/sim_model_source.h>

#include <fmt/core.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>


namespace SIM_MODEL_SOURCE_PARSER
{
    using namespace SIM_MODEL_SOURCE_GRAMMAR;

    template <typename Rule> struct pwlValuesSelector : std::false_type {};
    template <> struct pwlValuesSelector<number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>>
        : std::true_type {};
}


std::string SPICE_GENERATOR_SOURCE::ModelLine( const SPICE_ITEM& aItem ) const
{
    return "";
}


std::string SPICE_GENERATOR_SOURCE::ItemLine( const SPICE_ITEM& aItem ) const
{
    SPICE_ITEM item = aItem;
    item.modelName = "";

    std::string ac = m_model.FindParam( "ac" )->value->ToSpiceString();
    std::string ph = m_model.FindParam( "ph" )->value->ToSpiceString();

    if( ac != "" )
        item.modelName.append( fmt::format( "AC {} {} ", ac, ph ) );

    if( m_model.GetSpiceInfo().inlineTypeString != "" )
    {
        std::string args = "";
        
        switch( m_model.GetType() )
        {
        case SIM_MODEL::TYPE::V_PWL:
        case SIM_MODEL::TYPE::I_PWL:
        {
            tao::pegtl::string_input<> in( m_model.GetParam( 0 ).value->ToString(),
                                           "from_content" );
            std::unique_ptr<tao::pegtl::parse_tree::node> root;

            try
            {
                root = tao::pegtl::parse_tree::parse<SIM_MODEL_SOURCE_PARSER::pwlValuesGrammar,
                                                     SIM_MODEL_SOURCE_PARSER::pwlValuesSelector>
                    ( in );
            }
            catch( const tao::pegtl::parse_error& )
            {
                break;
            }

            if( root )
            {
                for( const auto& node : root->children )
                {
                    if( node->is_type<SIM_MODEL_SOURCE_PARSER::number<SIM_VALUE::TYPE_FLOAT,
                                                                      SIM_VALUE::NOTATION::SI>>() )
                    {
                        std::unique_ptr<SIM_VALUE> value = SIM_VALUE::Create( SIM_VALUE::TYPE_FLOAT,
                                                                              node->string() );
                        args.append( value->ToString( SIM_VALUE::NOTATION::SPICE ) + " " );
                    }
                }
            }
        }
            break;

        // TODO: dt should be tstep by default.

        case SIM_MODEL::TYPE::V_WHITENOISE:
        case SIM_MODEL::TYPE::I_WHITENOISE:
            args.append( getParamValueString( "rms", "0" ) + " " );
            args.append( getParamValueString( "dt", "0" ) + " " );
            args.append( "0 0 0 0 0 " );
            break;

        case SIM_MODEL::TYPE::V_PINKNOISE:
        case SIM_MODEL::TYPE::I_PINKNOISE:
            args.append( "0 " );
            args.append( getParamValueString( "dt", "0" ) + " " );
            args.append( getParamValueString( "slope", "0" ) + " " );
            args.append( getParamValueString( "rms", "0" ) + " " );
            args.append( "0 0 0 " );
            break;

        case SIM_MODEL::TYPE::V_BURSTNOISE:
        case SIM_MODEL::TYPE::I_BURSTNOISE:
            args.append( "0 0 0 0 " );
            args.append( getParamValueString( "ampl", "0" ) + " " );
            args.append( getParamValueString( "tcapt", "0" ) + " " );
            args.append( getParamValueString( "temit", "0" ) + " " );
            break;
        
        case SIM_MODEL::TYPE::V_RANDUNIFORM:
        case SIM_MODEL::TYPE::I_RANDUNIFORM:
        {
            args.append( "1 " );
            args.append( getParamValueString( "dt", "0" ) + " " );
            args.append( getParamValueString( "td", "0" ) + " " );

            auto min = dynamic_cast<SIM_VALUE_FLOAT&>( *m_model.FindParam( "max" )->value );
            auto max = dynamic_cast<SIM_VALUE_FLOAT&>( *m_model.FindParam( "min" )->value );
            SIM_VALUE_FLOAT range = max - min;
            SIM_VALUE_FLOAT offset = ( max + min ) / SIM_VALUE_FLOAT( 2 );

            args.append( range.ToSpiceString() + " " );
            args.append( offset.ToSpiceString() + " " );
        }
            break;

        case SIM_MODEL::TYPE::V_RANDNORMAL:
        case SIM_MODEL::TYPE::I_RANDNORMAL:
            args.append( "2 " );
            args.append( getParamValueString( "dt", "0" ) + " " );
            args.append( getParamValueString( "td", "0" ) + " " );
            args.append( getParamValueString( "stddev", "0" ) + " " );
            args.append( getParamValueString( "mean", "0" ) + " " );
            break;

        case SIM_MODEL::TYPE::V_RANDEXP:
        case SIM_MODEL::TYPE::I_RANDEXP:
            args.append( "3 " );
            args.append( getParamValueString( "dt", "0" ) + " " );
            args.append( getParamValueString( "td", "0" ) + " " );
            args.append( getParamValueString( "mean", "0" ) + " " );
            args.append( getParamValueString( "offset", "0" ) + " " );
            break;

        /*case SIM_MODEL::TYPE::V_RANDPOISSON:
        case SIM_MODEL::TYPE::I_RANDPOISSON:
            args.append( "4 " );
            args.append( FindParam( "dt" )->value->ToSpiceString() + " " );
            args.append( FindParam( "td" )->value->ToSpiceString() + " " );
            args.append( FindParam( "lambda" )->value->ToSpiceString() + " " );
            args.append( FindParam( "offset" )->value->ToSpiceString() + " " );
            break;*/

        default:
            for( const SIM_MODEL::PARAM& param : m_model.GetParams() )
            {
                std::string argStr = param.value->ToString( SIM_VALUE_GRAMMAR::NOTATION::SPICE );

                if( argStr != "" )
                    args.append( argStr + " " );
            }
            break;
        }

        item.modelName.append( fmt::format( "{}( {})", m_model.GetSpiceInfo().inlineTypeString, args ) );
    }
    else
        item.modelName.append( m_model.GetParam( 0 ).value->ToSpiceString() );

    return SPICE_GENERATOR::ItemLine( item );
}


std::string SPICE_GENERATOR_SOURCE::getParamValueString( const std::string& aParamName,
                                                         const std::string& aDefaultValue ) const
{
    std::string result = m_model.FindParam( aParamName )->value->ToSpiceString();

    if( result == "" )
        result = aDefaultValue;

    return result;
}


std::string SIM_SERDE_SOURCE::GenerateParamValuePair( const SIM_MODEL::PARAM& aParam ) const
{
    if( aParam.value->ToString() == "0" )
        return "";

    return SIM_SERDE::GenerateParamValuePair( aParam );
}


SIM_MODEL_SOURCE::SIM_MODEL_SOURCE( TYPE aType )
    : SIM_MODEL( aType,
                 std::make_unique<SPICE_GENERATOR_SOURCE>( *this ),
                 std::make_unique<SIM_SERDE_SOURCE>( *this ) )
{
    for( const SIM_MODEL::PARAM::INFO& paramInfo : makeParamInfos( aType ) )
        AddParam( paramInfo );
}


void SIM_MODEL_SOURCE::WriteDataSchFields( std::vector<SCH_FIELD>& aFields ) const
{
    SIM_MODEL::WriteDataSchFields( aFields );

    if( IsInferred() )
        inferredWriteDataFields( aFields );
}


void SIM_MODEL_SOURCE::WriteDataLibFields( std::vector<LIB_FIELD>& aFields ) const
{
    SIM_MODEL::WriteDataLibFields( aFields );

    if( IsInferred() )
        inferredWriteDataFields( aFields );
}


void SIM_MODEL_SOURCE::SetParamValue( int aParamIndex, const SIM_VALUE& aValue )
{
    // Sources are special. All preceding parameter values must be filled. If they are not, fill
    // them out automatically. If a value is nulled, delete everything after it.
    if( !aValue.HasValue() )
    {
        for( int paramIndex = static_cast<int>( aParamIndex );
             paramIndex < GetParamCount();
             ++paramIndex )
        {
            m_params.at( aParamIndex ).value->FromString( "" );
        }
    }
    else
    {
        for( int paramIndex = 0; paramIndex < aParamIndex; ++paramIndex )
        {
            if( GetParam( paramIndex ).value->ToString() == "" )
            {
                m_params.at( aParamIndex ).value->FromString( "0" );
                SIM_MODEL::SetParamValue( paramIndex, "0" );
            }
        }
    }

    return SIM_MODEL::SetParamValue( aParamIndex, aValue );
}


template <typename T>
void SIM_MODEL_SOURCE::inferredWriteDataFields( std::vector<T>& aFields ) const
{
    std::string value;

    if( GetTypeInfo().fieldValue != "" )
    {
        value = fmt::format( "{} {}",
                             GetTypeInfo().fieldValue,
                             GetFieldValue( &aFields, PARAMS_FIELD ) );
    }
    else
        value = fmt::format( "{}", GetFieldValue( &aFields, PARAMS_FIELD ) );

    if( value == "" )
        value = GetDeviceTypeInfo().fieldValue;

    WriteInferredDataFields( aFields, value );
}


const std::vector<SIM_MODEL::PARAM::INFO>& SIM_MODEL_SOURCE::makeParamInfos( TYPE aType )
{
    static std::vector<PARAM::INFO> vdc = makeDcParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> idc = makeDcParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vsin = makeSinParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> isin = makeSinParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vpulse = makePulseParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> ipulse = makePulseParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vexp = makeExpParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> iexp = makeExpParamInfos( "y", "A" );

    /*static std::vector<PARAM::INFO> vsfam = makeSfamParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> isfam = makeSfamParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vsffm = makeSffmParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> isffm = makeSffmParamInfos( "y", "A" );*/

    static std::vector<PARAM::INFO> vpwl = makePwlParamInfos( "y", "Voltage", "V" );
    static std::vector<PARAM::INFO> ipwl = makePwlParamInfos( "y", "Current", "A" );

    static std::vector<PARAM::INFO> vwhitenoise = makeWhiteNoiseParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> iwhitenoise = makeWhiteNoiseParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vpinknoise = makePinkNoiseParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> ipinknoise = makePinkNoiseParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vburstnoise = makeBurstNoiseParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> iburstnoise = makeBurstNoiseParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vrandomuniform = makeRandomUniformParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> irandomuniform = makeRandomUniformParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vrandomnormal = makeRandomNormalParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> irandomnormal = makeRandomNormalParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vrandomexp = makeRandomExpParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> irandomexp = makeRandomExpParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vrandompoisson = makeRandomPoissonParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> irandompoisson = makeRandomPoissonParamInfos( "y", "A" );

    switch( aType )
    {
    case TYPE::V:             return vdc;
    case TYPE::I:             return idc;
    case TYPE::V_SIN:         return vsin;
    case TYPE::I_SIN:         return isin;
    case TYPE::V_PULSE:       return vpulse;
    case TYPE::I_PULSE:       return ipulse;
    case TYPE::V_EXP:         return vexp;
    case TYPE::I_EXP:         return iexp;
    /*case TYPE::V_SFAM:        return vsfam;
    case TYPE::I_SFAM:        return isfam;
    case TYPE::V_SFFM:        return vsffm;
    case TYPE::I_SFFM:        return isffm;*/
    case TYPE::V_PWL:         return vpwl;
    case TYPE::I_PWL:         return ipwl;
    case TYPE::V_WHITENOISE:  return vwhitenoise;
    case TYPE::I_WHITENOISE:  return iwhitenoise;
    case TYPE::V_PINKNOISE:   return vpinknoise;
    case TYPE::I_PINKNOISE:   return ipinknoise;
    case TYPE::V_BURSTNOISE:  return vburstnoise;
    case TYPE::I_BURSTNOISE:  return iburstnoise;
    case TYPE::V_RANDUNIFORM: return vrandomuniform;
    case TYPE::I_RANDUNIFORM: return irandomuniform;
    case TYPE::V_RANDNORMAL:  return vrandomnormal;
    case TYPE::I_RANDNORMAL:  return irandomnormal;
    case TYPE::V_RANDEXP:     return vrandomexp;
    case TYPE::I_RANDEXP:     return irandomexp;
    //case TYPE::V_RANDPOISSON: return vrandompoisson;
    //case TYPE::I_RANDPOISSON: return irandompoisson;
    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_SOURCE" );
        static std::vector<SIM_MODEL::PARAM::INFO> empty;
        return empty;
    }
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeDcParamInfos( std::string aPrefix,
                                                                        std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "DC value";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeSinParamInfos( std::string aPrefix,
                                                                         std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "ampl";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "f";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1/tstop";
    paramInfo.description = "Frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "theta";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "1/s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Damping factor";
    paramInfos.push_back( paramInfo );

    // "phase" is not needed. "td" is enough.

    /*paramInfo.name = "phase";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "°";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Phase";
    paramInfos.push_back( paramInfo );*/

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makePulseParamInfos( std::string aPrefix,
                                                                           std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "1";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Initial value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix + "2";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Pulsed value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tr";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Rise time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tf";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Fall time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tw"; // Ngspice calls it "pw".
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstop";
    paramInfo.description = "Pulse width";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "per";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstop";
    paramInfo.description = "Period";
    paramInfos.push_back( paramInfo );

    // "phase" is not needed. "td" is enough.

    /*paramInfo.name = "phase";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "°";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Phase";
    paramInfos.push_back( paramInfo );*/

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeExpParamInfos( std::string aPrefix,
                                                                         std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "1";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Initial value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix + "2";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Pulsed value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td1";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Rise delay time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tau1";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Rise time constant";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td2";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "td1+tstep";
    paramInfo.description = "Fall delay time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tau2";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Fall time constant";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}


/*std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeSfamParamInfos( std::string aPrefix, std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";

    paramInfo.name = "ampl";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Amplitude";

    paramInfos.push_back( paramInfo );
    paramInfo.name = "mo";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Modulating signal offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fc";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Carrier frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "mf";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Modulating frequency";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeSffmParamInfos( std::string aPrefix, std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "ampl";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fc";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1/tstop";
    paramInfo.description = "Carrier frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "mdi";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Modulation index";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fs";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1/tstop";
    paramInfo.description = "Signal frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phasec";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "°";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Carrier phase";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phases";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "°";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Signal phase";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}*/


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makePwlParamInfos( std::string aPrefix,
                                                                         std::string aQuantity,
                                                                         std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "pwl";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "s," + aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = aUnit == "V" ? "Time-voltage points" : "Time-current points";
    paramInfos.push_back( paramInfo );

    // TODO: Ngspice doesn't support "td" and "r" for current sources, so let's disable that for
    // now.

    /*paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = aUnit == "V" ? "Time-voltage points" : "Time-current points";
    paramInfo.isSpiceInstanceParam = true;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "repeat";
    paramInfo.type = SIM_VALUE::TYPE_BOOL;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Repeat forever";
    paramInfo.isSpiceInstanceParam = true;
    paramInfo.spiceInstanceName = "r";
    paramInfos.push_back( paramInfo );*/

    /*paramInfo.name = "t";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT_VECTOR;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Time vector";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix;
    paramInfo.type = SIM_VALUE::TYPE_FLOAT_VECTOR;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = aQuantity + " vector";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "repeat";
    paramInfo.type = SIM_VALUE::TYPE_BOOL;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Repeat forever";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );*/

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeWhiteNoiseParamInfos( std::string aPrefix,
                                                                                std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "rms";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "White noise RMS amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "dt";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makePinkNoiseParamInfos( std::string aPrefix, 
                                                                               std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "rms";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "1/f noise RMS amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "slope";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "1/f noise exponent";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "dt";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeBurstNoiseParamInfos( std::string aPrefix,
                                                                                std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "ampl";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Burst noise amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tcapt";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Burst noise trap capture time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "temit";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Burst noise trap emission time";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeRandomUniformParamInfos( std::string aPrefix,
                                                                                   std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "min";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "-0.5";
    paramInfo.description = "Min. value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "max";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0.5";
    paramInfo.description = "Max. value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "dt";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeRandomNormalParamInfos( std::string aPrefix,
                                                                                  std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "mean";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Mean";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "stddev";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Standard deviation";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "dt";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeRandomExpParamInfos( std::string aPrefix,
                                                                               std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "offset";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "mean";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Mean";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "dt";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeRandomPoissonParamInfos( std::string aPrefix,
                                                                                   std::string aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "offset";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "lambda";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Mean";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "dt";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    return paramInfos;
}

void SIM_MODEL_SOURCE::appendAcParamInfos( std::vector<PARAM::INFO>& aParamInfos, std::string aUnit )
{
    PARAM::INFO paramInfo;

    paramInfo.name = "ac";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::AC;
    paramInfo.defaultValue = "0";
    paramInfo.description = "AC magnitude";
    aParamInfos.push_back( paramInfo );

    paramInfo.name = "ph";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "°";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::AC;
    paramInfo.defaultValue = "0";
    paramInfo.description = "AC phase";
    aParamInfos.push_back( paramInfo );
}
