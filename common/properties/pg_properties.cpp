/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.TXT for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/dc.h>
#include <wx/propgrid/propgrid.h>
#include <wx/regex.h>

#include <common.h>
#include <macros.h>
#include <validators.h>
#include <eda_units.h>
#include <properties/eda_angle_variant.h>
#include <properties/pg_properties.h>
#include <properties/property_mgr.h>
#include <properties/property.h>
#include <widgets/color_swatch.h>

// reg-ex describing a signed valid value with a unit
static const wxChar REGEX_SIGNED_DISTANCE[] = wxT( "([-+]?[0-9]+[\\.?[0-9]*) *(mm|in|mils)*" );
static const wxChar REGEX_UNSIGNED_DISTANCE[] = wxT( "([0-9]+[\\.?[0-9]*) *(mm|in|mils)*" );


class wxAnyToEDA_ANGLE_VARIANTRegistrationImpl : public wxAnyToVariantRegistration
{
public:
    wxAnyToEDA_ANGLE_VARIANTRegistrationImpl( wxVariantDataFactory factory )
        : wxAnyToVariantRegistration( factory )
    {
    }

public:
    static bool IsSameClass(const wxAnyValueType* otherType)
    {
        return AreSameClasses( *s_instance.get(), *otherType );
    }

    static wxAnyValueType* GetInstance()
    {
        return s_instance.get();
    }

    virtual wxAnyValueType* GetAssociatedType() override
    {
        return wxAnyToEDA_ANGLE_VARIANTRegistrationImpl::GetInstance();
    }
private:
    static bool AreSameClasses(const wxAnyValueType& a, const wxAnyValueType& b)
    {
        return wxTypeId(a) == wxTypeId(b);
    }

    static wxAnyValueTypeScopedPtr s_instance;
};


wxAnyValueTypeScopedPtr wxAnyToEDA_ANGLE_VARIANTRegistrationImpl::s_instance( new wxAnyValueTypeImpl<EDA_ANGLE>() );

static wxAnyToEDA_ANGLE_VARIANTRegistrationImpl s_wxAnyToEDA_ANGLE_VARIANTRegistration( &EDA_ANGLE_VARIANT_DATA::VariantDataFactory );


wxPGProperty* PGPropertyFactory( const PROPERTY_BASE* aProperty )
{
    wxPGProperty* ret = nullptr;
    PROPERTY_DISPLAY display = aProperty->Display();

    switch( display )
    {
    case PROPERTY_DISPLAY::PT_SIZE:
        ret = new PGPROPERTY_SIZE();
        ret->SetEditor( wxT( "UnitEditor" ) );
        break;

    case PROPERTY_DISPLAY::PT_COORD:
        ret = new PGPROPERTY_COORD();
        static_cast<PGPROPERTY_COORD*>( ret )->SetCoordType( aProperty->CoordType() );
        ret->SetEditor( wxT( "UnitEditor" ) );
        break;

    case PROPERTY_DISPLAY::PT_DECIDEGREE:
    case PROPERTY_DISPLAY::PT_DEGREE:
    {
        PGPROPERTY_ANGLE* prop = new PGPROPERTY_ANGLE();

        if( display == PROPERTY_DISPLAY::PT_DECIDEGREE )
            prop->SetScale( 10.0 );

        ret = prop;
        break;
    }

    default:
        wxFAIL;
        KI_FALLTHROUGH;
        /* fall through */
    case PROPERTY_DISPLAY::PT_DEFAULT:
    {
        // Create a corresponding wxPGProperty
        size_t typeId = aProperty->TypeHash();

        // Enum property
        if( aProperty->HasChoices() )
        {
            // I do not know why enum property takes a non-const reference to wxPGChoices..
            ret = new wxEnumProperty( wxPG_LABEL, wxPG_LABEL,
                    const_cast<wxPGChoices&>( aProperty->Choices() ) );
        }
        else if( typeId == TYPE_HASH( int ) || typeId == TYPE_HASH( long ) )
        {
            ret = new wxIntProperty();
        }
        else if( typeId == TYPE_HASH( unsigned int ) || typeId == TYPE_HASH( unsigned long ) )
        {
            ret = new wxUIntProperty();
        }
        else if( typeId == TYPE_HASH( float ) || typeId == TYPE_HASH( double ) )
        {
            ret = new wxFloatProperty();
        }
        else if( typeId == TYPE_HASH( bool ) )
        {
            ret = new wxBoolProperty();
            ret->SetAttribute( wxPG_BOOL_USE_CHECKBOX, true );
        }
        else if( typeId == TYPE_HASH( wxString ) )
        {
            ret = new wxStringProperty();
        }
        else
        {
            wxFAIL_MSG( wxString::Format( "Property '" + aProperty->Name() +
                        "' is not supported by PGPropertyFactory" ) );
            ret = new wxPropertyCategory();
            ret->Enable( false );
        }
        break;
    }
    }

    if( ret )
    {
        ret->SetLabel( aProperty->Name() );
        ret->SetName( aProperty->Name() );
        ret->Enable( !aProperty->IsReadOnly() );
        ret->SetClientData( const_cast<PROPERTY_BASE*>( aProperty ) );
    }

    return ret;
}


PGPROPERTY_DISTANCE::PGPROPERTY_DISTANCE( const wxString& aRegEx,
                                          ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) :
        m_coordType( aCoordType )
{
    m_regExValidator.reset( new REGEX_VALIDATOR( aRegEx ) );
}


PGPROPERTY_DISTANCE::~PGPROPERTY_DISTANCE()
{
}


bool PGPROPERTY_DISTANCE::StringToDistance( wxVariant& aVariant, const wxString& aText,
                                            int aArgFlags ) const
{
    // TODO(JE): Are there actual use cases for this?
    wxCHECK_MSG( false, false, wxT( "PGPROPERTY_DISTANCE::StringToDistance should not be used." ) );
}


wxString PGPROPERTY_DISTANCE::DistanceToString( wxVariant& aVariant, int aArgFlags ) const
{
    wxCHECK( aVariant.GetType() == wxPG_VARIANT_TYPE_LONG, wxEmptyString );
    // TODO(JE) This should be handled by UNIT_BINDER

    long distanceIU = aVariant.GetLong();

    ORIGIN_TRANSFORMS* transforms = PROPERTY_MANAGER::Instance().GetTransforms();

    if( transforms )
        distanceIU = transforms->ToDisplay( static_cast<long long int>( distanceIU ), m_coordType );

    switch( PROPERTY_MANAGER::Instance().GetUnits() )
    {
        case EDA_UNITS::INCHES:
            return wxString::Format( wxT( "%d in" ), pcbIUScale.IUToMils( distanceIU ) / 1000.0 );

        case EDA_UNITS::MILS:
            return wxString::Format( wxT( "%d mils" ), pcbIUScale.IUToMils( distanceIU ) );

        case EDA_UNITS::MILLIMETRES:
            return wxString::Format( wxT( "%g mm" ), pcbIUScale.IUTomm( distanceIU ) );

        case EDA_UNITS::UNSCALED:
            return wxString::Format( wxT( "%li" ), distanceIU );

        default:
            // DEGREEs are handled by PGPROPERTY_ANGLE
            break;
    }

    wxFAIL;
    return wxEmptyString;
}


PGPROPERTY_SIZE::PGPROPERTY_SIZE( const wxString& aLabel, const wxString& aName,
        long aValue )
    : wxUIntProperty( aLabel, aName, aValue ), PGPROPERTY_DISTANCE( REGEX_UNSIGNED_DISTANCE )
{
}


wxValidator* PGPROPERTY_SIZE::DoGetValidator() const
{
    //return m_regExValidator.get();
            return nullptr;
}


PGPROPERTY_COORD::PGPROPERTY_COORD( const wxString& aLabel, const wxString& aName,
                                    long aValue, ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) :
        wxIntProperty( aLabel, aName, aValue ),
        PGPROPERTY_DISTANCE( REGEX_SIGNED_DISTANCE, aCoordType )
{
}


wxValidator* PGPROPERTY_COORD::DoGetValidator() const
{
    //return m_regExValidator.get();
    return nullptr;
}


bool PGPROPERTY_ANGLE::StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags ) const
{
    double value = 0.0;

    if( !aText.ToDouble( &value ) )
    {
        aVariant.MakeNull();
        return true;
    }

    value *= m_scale;

    if( aVariant.IsNull() || aVariant.GetDouble() != value )
    {
        aVariant = value;
        return true;
    }

    return false;
}


wxString PGPROPERTY_ANGLE::ValueToString( wxVariant& aVariant, int aArgFlags ) const
{
    if( aVariant.GetType() == wxPG_VARIANT_TYPE_DOUBLE )
    {
        // TODO(JE) Is this still needed?
        return wxString::Format( wxT( "%g\u00B0" ), aVariant.GetDouble() / m_scale );
    }
    else if( aVariant.GetType() == wxT( "EDA_ANGLE" ) )
    {
        wxString ret;
        static_cast<EDA_ANGLE_VARIANT_DATA*>( aVariant.GetData() )->Write( ret );
        return ret;
    }
    else
    {
        wxCHECK_MSG( false, wxEmptyString, "Unexpected variant type in PGPROPERTY_ANGLE" );
    }
}


wxSize PGPROPERTY_COLORENUM::OnMeasureImage( int aItem ) const
{
    // TODO(JE) calculate size from window metrics?
    return wxSize( 16, 12 );
}


void PGPROPERTY_COLORENUM::OnCustomPaint( wxDC& aDC, const wxRect& aRect,
                                          wxPGPaintData& aPaintData )
{
    int index = aPaintData.m_choiceItem;

    if( index < 0 )
        index = GetIndex();

    // GetIndex can return -1 when the control hasn't been set up yet
    if( index < 0 || index >= static_cast<int>( GetChoices().GetCount() ) )
        return;

    wxString layer = GetChoices().GetLabel( index );
    wxColour color = GetColor( layer );

    if( color == wxNullColour )
        return;

    aDC.SetPen( *wxTRANSPARENT_PEN );
    aDC.SetBrush( wxBrush( color ) );
    aDC.DrawRectangle( aRect );

    aPaintData.m_drawnWidth = aRect.width;
}
