/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <sim/kibis/kibis.h>
#include <sim/sim_model_kibis.h>
#include <sim/sim_library_kibis.h>
#include <paths.h>
#include <fmt/core.h>
#include <wx/filename.h>
#include <kiway.h>

std::string SPICE_GENERATOR_KIBIS::ModelName( const SPICE_ITEM& aItem ) const
{
    return fmt::format( "{}.{}", aItem.refName, aItem.baseModelName );
}


std::string SPICE_GENERATOR_KIBIS::ModelLine( const SPICE_ITEM& aItem ) const
{
    return "";
}

std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> SPICE_GENERATOR_KIBIS::GetInstanceParams() const
{
    std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> vec;
    return vec;
}


std::vector<std::string> SPICE_GENERATOR_KIBIS::CurrentNames( const SPICE_ITEM& aItem ) const
{
    std::vector<std::string> currentNames;

    for( const SIM_MODEL::PIN& pin : GetPins() )
        currentNames.push_back( fmt::format( "I({}:{})", ItemName( aItem ), pin.name ) );

    return currentNames;
}


std::string SPICE_GENERATOR_KIBIS::IbisDevice( const SPICE_ITEM& aItem, const std::string aCwd,
                                               const std::string aCacheDir ) const
{
    std::string ibisLibFilename = SIM_MODEL::GetFieldValue( aItem.fields, SIM_LIBRARY::LIBRARY_FIELD );
    std::string ibisCompName    = SIM_MODEL::GetFieldValue( aItem.fields, SIM_LIBRARY::NAME_FIELD  );
    std::string ibisPinName     = SIM_MODEL::GetFieldValue( aItem.fields, SIM_LIBRARY_KIBIS::PIN_FIELD );
    std::string ibisModelName   = SIM_MODEL::GetFieldValue( aItem.fields, SIM_LIBRARY_KIBIS::MODEL_FIELD );
    bool diffMode = SIM_MODEL::GetFieldValue( aItem.fields, SIM_LIBRARY_KIBIS::DIFF_FIELD ) == "1";

    wxFileName libPath = wxFileName( wxString( ibisLibFilename ) );

    if( !libPath.IsAbsolute() )
        libPath.MakeAbsolute( aCwd );

    KIBIS kibis( std::string( libPath.GetFullPath().c_str() ) );
    kibis.m_cacheDir = aCacheDir;

    if( !kibis.m_valid )
    {
        THROW_IO_ERROR( wxString::Format( _( "Invalid IBIS file '%s'" ),
                                          ibisLibFilename ) );
    }

    KIBIS_COMPONENT* kcomp = kibis.GetComponent( std::string( ibisCompName ) );

    if( !kcomp )
        THROW_IO_ERROR( wxString::Format( _( "Could not find IBIS component '%s'" ), ibisCompName ) );

    KIBIS_PIN* kpin = kcomp->GetPin( ibisPinName );


    if( !kcomp->m_valid )
    {
        THROW_IO_ERROR( wxString::Format( _( "Invalid IBIS component '%s'" ),
                                          ibisCompName ) );
    }

    if( !kpin )
    {
        THROW_IO_ERROR( wxString::Format( _( "Could not find IBIS pin '%s' in component '%s'" ),
                                          ibisPinName,
                                          ibisCompName ) );
    }

    if( !kpin->m_valid )
    {
        THROW_IO_ERROR( wxString::Format( _( "Invalid IBIS pin '%s' in component '%s'" ),
                                          ibisPinName,
                                          ibisCompName ) );
    }

    KIBIS_MODEL* kmodel = kibis.GetModel( ibisModelName );

    if( !kmodel )
        THROW_IO_ERROR( wxString::Format( _( "Could not find IBIS model '%s'" ), ibisModelName ) );

    if( !kmodel->m_valid )
        THROW_IO_ERROR( wxString::Format( _( "Invalid IBIS model '%s'" ), ibisModelName ) );

    KIBIS_PARAMETER kparams;

    kparams.SetCornerFromString( kparams.m_supply, m_model.FindParam( "vcc" )->value->ToString() );
    kparams.SetCornerFromString( kparams.m_Rpin, m_model.FindParam( "rpin" )->value->ToString() );
    kparams.SetCornerFromString( kparams.m_Lpin, m_model.FindParam( "lpin" )->value->ToString() );
    kparams.SetCornerFromString( kparams.m_Cpin, m_model.FindParam( "cpin" )->value->ToString() );
    //kparams.SetCornerFromString( kparams.m_Ccomp, FindParam( "ccomp" )->value->ToString() );

    std::string result;

    switch( m_model.GetType() )
    {
    case SIM_MODEL::TYPE::KIBIS_DEVICE:
        if( diffMode )
            kpin->writeSpiceDiffDevice( &result, aItem.modelName, *kmodel, kparams );
        else
            kpin->writeSpiceDevice( &result, aItem.modelName, *kmodel, kparams );
        break;

    case SIM_MODEL::TYPE::KIBIS_DRIVER_DC:
    {
        std::string paramValue = m_model.FindParam( "dc" )->value->ToString();

        if( paramValue == "hi-Z" )
        {
            kparams.m_waveform =
                    static_cast<KIBIS_WAVEFORM*>( new KIBIS_WAVEFORM_HIGH_Z() );
        }
        else if( paramValue == "low" )
        {
            kparams.m_waveform =
                    static_cast<KIBIS_WAVEFORM*>( new KIBIS_WAVEFORM_STUCK_LOW() );
        }
        else if( paramValue == "high" )
        {
            kparams.m_waveform =
                    static_cast<KIBIS_WAVEFORM*>( new KIBIS_WAVEFORM_STUCK_HIGH() );
        }

        if( diffMode )
            kpin->writeSpiceDiffDriver( &result, aItem.modelName, *kmodel, kparams );
        else
            kpin->writeSpiceDriver( &result, aItem.modelName, *kmodel, kparams );
        break;
    }

    case SIM_MODEL::TYPE::KIBIS_DRIVER_RECT:
    {
        KIBIS_WAVEFORM_RECTANGULAR* waveform = new KIBIS_WAVEFORM_RECTANGULAR();

        if ( m_model.FindParam( "ton" ) )
            waveform->m_ton = static_cast<SIM_VALUE_FLOAT&>( *m_model.FindParam( "ton" )->value ).Get().value_or( 1 );

        if ( m_model.FindParam( "toff" ) )
            waveform->m_toff = static_cast<SIM_VALUE_FLOAT&>( *m_model.FindParam( "toff" )->value ).Get().value_or( 1 );

        if ( m_model.FindParam( "delay" ) )
            waveform->m_delay = static_cast<SIM_VALUE_FLOAT&>( *m_model.FindParam( "delay" )->value ).Get().value_or( 0 );

        if ( m_model.FindParam( "cycles" ) )
            waveform->m_cycles = static_cast<SIM_VALUE_FLOAT&>( *m_model.FindParam( "cycles" )->value ).Get().value_or( 0 );

        kparams.m_waveform = waveform;

        if( diffMode )
            kpin->writeSpiceDiffDriver( &result, aItem.modelName, *kmodel, kparams );
        else
            kpin->writeSpiceDriver( &result, aItem.modelName, *kmodel, kparams );
        break;
    }

    case SIM_MODEL::TYPE::KIBIS_DRIVER_PRBS:
    {
        KIBIS_WAVEFORM_PRBS* waveform = new KIBIS_WAVEFORM_PRBS();

        if ( m_model.FindParam( "f0" ) )
            waveform->m_bitrate = static_cast<SIM_VALUE_FLOAT&>( *m_model.FindParam( "f0" )->value ).Get().value_or( 0 );

        if ( m_model.FindParam( "bits" ) )
        waveform->m_bits = static_cast<SIM_VALUE_FLOAT&>( *m_model.FindParam( "bits" )->value ).Get().value_or( 0 );

        if ( m_model.FindParam( "delay" ) )
        waveform->m_delay = static_cast<SIM_VALUE_FLOAT&>( *m_model.FindParam( "delay" )->value ).Get().value_or( 0 );

        kparams.m_waveform = waveform;

        if( diffMode )
            kpin->writeSpiceDiffDriver( &result, aItem.modelName, *kmodel, kparams );
        else
            kpin->writeSpiceDriver( &result, aItem.modelName, *kmodel, kparams );
        break;
    }

    default:
        wxFAIL_MSG( "Unknown IBIS model type" );
        return "";
    }

    return result;
}


SIM_MODEL_KIBIS::SIM_MODEL_KIBIS( TYPE aType ) :
        SIM_MODEL( aType, std::make_unique<SPICE_GENERATOR_KIBIS>( *this ) ),
        m_enableDiff( false ),
        m_sourceModel( nullptr )
{
    static std::vector<PARAM::INFO> device = makeParamInfos( TYPE::KIBIS_DEVICE );
    static std::vector<PARAM::INFO> dcDriver = makeParamInfos( TYPE::KIBIS_DRIVER_DC );
    static std::vector<PARAM::INFO> rectDriver = makeParamInfos( TYPE::KIBIS_DRIVER_RECT );
    static std::vector<PARAM::INFO> prbsDriver = makeParamInfos( TYPE::KIBIS_DRIVER_PRBS );

    std::vector<PARAM::INFO>* paramInfos = nullptr;

    switch( aType )
    {
    case SIM_MODEL::TYPE::KIBIS_DEVICE:      paramInfos = &device;     break;
    case SIM_MODEL::TYPE::KIBIS_DRIVER_DC:   paramInfos = &dcDriver;   break;
    case SIM_MODEL::TYPE::KIBIS_DRIVER_RECT: paramInfos = &rectDriver; break;
    case SIM_MODEL::TYPE::KIBIS_DRIVER_PRBS: paramInfos = &prbsDriver; break;

    default:
        wxFAIL;
        return;
    }

    for( const PARAM::INFO& paramInfo : *paramInfos )
        AddParam( paramInfo );

    SwitchSingleEndedDiff( false );
}


void SIM_MODEL_KIBIS::SwitchSingleEndedDiff( bool aDiff )
{
    DeletePins();

    if( aDiff )
    {
        AddPin( { "GND", "1" } );
        AddPin( { "+", "2" } );
        AddPin( { "-", "3" } );
    }
    else
    {
        AddPin( { "GND", "1" } );
        AddPin( { "IN/OUT", "2" } );
    }
}

SIM_MODEL_KIBIS::SIM_MODEL_KIBIS( TYPE aType, const SIM_MODEL_KIBIS& aSource ) : SIM_MODEL_KIBIS( aType )
{
    for( PARAM& param1 : m_params )
    {
        for( auto& param2refwrap : aSource.GetParams() )
        {
            const PARAM& param2 = param2refwrap.get();

            if( param1.info.name == param2.info.name )
                *( param1.value ) = *( param2.value );
        }
    }

    m_componentName = aSource.m_componentName;

    m_ibisPins = aSource.GetIbisPins();
    m_ibisModels = aSource.GetIbisModels();
    
    m_enableDiff = aSource.CanDifferential();
}

SIM_MODEL_KIBIS::SIM_MODEL_KIBIS( TYPE aType, SIM_MODEL_KIBIS& aSource,
                                  const std::vector<LIB_FIELD>& aFields ) :
        SIM_MODEL_KIBIS( aType, aSource )
{
    ReadDataFields( 2, &aFields );
}

SIM_MODEL_KIBIS::SIM_MODEL_KIBIS( TYPE aType, SIM_MODEL_KIBIS& aSource,
                                  const std::vector<SCH_FIELD>& aFields ) :
        SIM_MODEL_KIBIS( aType, aSource )
{
}


void SIM_MODEL_KIBIS::CreatePins( unsigned aSymbolPinCount )
{
    SIM_MODEL::CreatePins( aSymbolPinCount );

    // Reset the pins to Not Connected. Linear order is not as common, and reordering the pins is
    // more effort in the GUI than assigning them from scratch.
    for( int pinIndex = 0; pinIndex < GetPinCount(); ++pinIndex )
        SetPinSymbolPinNumber( pinIndex, "" );
}


bool SIM_MODEL_KIBIS::ChangePin( SIM_LIBRARY_KIBIS& aLib, std::string aPinNumber )
{
    KIBIS_COMPONENT* kcomp = aLib.m_kibis.GetComponent( std::string( GetComponentName() ) );

    if( !kcomp )
        return false;

    KIBIS_PIN* kpin = kcomp->GetPin( std::string( aPinNumber.c_str() ) );

    if( !kpin )
        return false;

    m_ibisModels.clear();

    for( KIBIS_MODEL* kmodel : kpin->m_models )
        m_ibisModels.push_back( kmodel->m_name );

    return true;
}


void SIM_MODEL_KIBIS::SetBaseModel( const SIM_MODEL& aBaseModel )
{
    // Actual base models can only be of the same type, which is not the case here, as in addition
    // to IBIS device model type we have multiple types of drivers available for the same sourced
    // model. And we don't want to inherit the default values anyway. So we just store these models
    // and use the only for Spice code generation.
    m_sourceModel = dynamic_cast<const SIM_MODEL_KIBIS*>( &aBaseModel );
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_KIBIS::makeParamInfos( TYPE aType )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO              paramInfo;

    paramInfo.name = "vcc";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "TYP";
    paramInfo.description = _( "Power supply" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "TYP", "MIN", "MAX" };
    paramInfos.push_back( paramInfo );

    paramInfo.name = "rpin";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "TYP";
    paramInfo.description = _( "Parasitic Resistance" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "TYP", "MIN", "MAX" };
    paramInfos.push_back( paramInfo );

    paramInfo.name = "lpin";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "TYP";
    paramInfo.description = _( "Parasitic Pin Inductance" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "TYP", "MIN", "MAX" };
    paramInfos.push_back( paramInfo );

    paramInfo.name = "cpin";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "TYP";
    paramInfo.description = _( "Parasitic Pin Capacitance" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "TYP", "MIN", "MAX" };
    paramInfos.push_back( paramInfo );

    std::vector<PARAM::INFO> dc = makeDcWaveformParamInfos();
    std::vector<PARAM::INFO> rect = makeRectWaveformParamInfos();
    std::vector<PARAM::INFO> prbs = makePrbsWaveformParamInfos();

    switch( aType )
    {
    case TYPE::KIBIS_DRIVER_DC:
        for( const PARAM::INFO& param : makeDcWaveformParamInfos() )
            paramInfos.push_back( param );
        break;

    case TYPE::KIBIS_DRIVER_RECT:
        for( const PARAM::INFO& param : makeRectWaveformParamInfos() )
            paramInfos.push_back( param );
        break;

    case TYPE::KIBIS_DRIVER_PRBS:
        for( const PARAM::INFO& param : makePrbsWaveformParamInfos() )
            paramInfos.push_back( param );
        break;

    default:
        break;
    }

    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_KIBIS::makeDcWaveformParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO              paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "hi-Z";
    paramInfo.description = _( "DC Value" );
    paramInfo.enumValues = { "hi-Z", "low", "high" };
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_KIBIS::makeRectWaveformParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO              paramInfo;

    paramInfo.name = "ton";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "";
    paramInfo.description = _( "ON time" );
    paramInfos.push_back( paramInfo );

    paramInfo.name = "toff";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "";
    paramInfo.description = _( "OFF time" );
    paramInfos.push_back( paramInfo );

    paramInfo.name = "delay";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "0";
    paramInfo.description = _( "Delay" );
    paramInfos.push_back( paramInfo );

    paramInfo.name = "cycles";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "1";
    paramInfo.description = _( "cycles" );
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_KIBIS::makePrbsWaveformParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO              paramInfo;

    paramInfo.name = "f0";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "";
    paramInfo.description = _( "Bitrate" );
    paramInfos.push_back( paramInfo );

    paramInfo.name = "bits";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "";
    paramInfo.description = _( "Number of bits" );
    paramInfos.push_back( paramInfo );

    return paramInfos;
}

void SIM_MODEL_KIBIS::ReadDataSchFields( unsigned aSymbolPinCount, const std::vector<SCH_FIELD>* aFields )
{
    bool diffMode = SIM_MODEL::GetFieldValue( aFields, SIM_LIBRARY_KIBIS::DIFF_FIELD ) == "1";
    SwitchSingleEndedDiff( diffMode );

    SIM_MODEL::ReadDataSchFields( aSymbolPinCount, aFields );
}


void SIM_MODEL_KIBIS::ReadDataLibFields( unsigned aSymbolPinCount, const std::vector<LIB_FIELD>* aFields )
{
    bool diffMode = SIM_MODEL::GetFieldValue( aFields, SIM_LIBRARY_KIBIS::DIFF_FIELD ) == "1";
    SwitchSingleEndedDiff( diffMode );

    SIM_MODEL::ReadDataLibFields( aSymbolPinCount, aFields );
}