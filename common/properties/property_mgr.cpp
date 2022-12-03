/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <properties/property_mgr.h>
#include <properties/property.h>

#include <algorithm>
#include <utility>
#include <wx/wx.h>

static wxString EMPTY_STRING( wxEmptyString );


void PROPERTY_MANAGER::RegisterType( TYPE_ID aType, const wxString& aName )
{
    wxASSERT( m_classNames.count( aType ) == 0 );
    m_classNames.emplace( aType, aName );
}


const wxString& PROPERTY_MANAGER::ResolveType( TYPE_ID aType ) const
{
    auto it = m_classNames.find( aType );
    return it != m_classNames.end() ? it->second : EMPTY_STRING;
}


PROPERTY_BASE* PROPERTY_MANAGER::GetProperty( TYPE_ID aType, const wxString& aProperty ) const
{
    if( m_dirty )
        const_cast<PROPERTY_MANAGER*>( this )->Rebuild();

    auto it = m_classes.find( aType );

    if( it == m_classes.end() )
        return nullptr;

    const CLASS_DESC& classDesc = it->second;

    for( PROPERTY_BASE* property : classDesc.m_allProperties )
    {
        if( !aProperty.CmpNoCase( property->Name() ) )
            return property;
    }

    return nullptr;
}


const PROPERTY_LIST& PROPERTY_MANAGER::GetProperties( TYPE_ID aType ) const
{
    if( m_dirty )
        const_cast<PROPERTY_MANAGER*>( this )->Rebuild();

    static const PROPERTY_LIST empty;
    auto it = m_classes.find( aType );

    if( it == m_classes.end() )
        return empty;

    return it->second.m_allProperties;
}


const PROPERTY_DISPLAY_ORDER& PROPERTY_MANAGER::GetDisplayOrder( TYPE_ID aType ) const
{
    if( m_dirty )
        const_cast<PROPERTY_MANAGER*>( this )->Rebuild();

    static const PROPERTY_DISPLAY_ORDER empty;
    auto it = m_classes.find( aType );

    if( it == m_classes.end() )
        return empty;

    return it->second.m_displayOrder;
}


const std::vector<wxString>& PROPERTY_MANAGER::GetGroupDisplayOrder( TYPE_ID aType ) const
{
    if( m_dirty )
        const_cast<PROPERTY_MANAGER*>( this )->Rebuild();

    static const std::vector<wxString> empty;
    auto it = m_classes.find( aType );

    if( it == m_classes.end() )
        return empty;

    return it->second.m_groupDisplayOrder;
}


const void* PROPERTY_MANAGER::TypeCast( const void* aSource, TYPE_ID aBase, TYPE_ID aTarget ) const
{
    if( aBase == aTarget )
        return aSource;

    auto classDesc = m_classes.find( aBase );

    if( classDesc == m_classes.end() )
        return aSource;

    auto& converters = classDesc->second.m_typeCasts;
    auto converter = converters.find( aTarget );

    if( converter == converters.end() )     // explicit type cast not found
        return IsOfType( aBase, aTarget ) ? aSource : nullptr;

    return (*converter->second)( aSource );
}


void PROPERTY_MANAGER::AddProperty( PROPERTY_BASE* aProperty, const wxString& aGroup )
{
    const wxString& name = aProperty->Name();
    TYPE_ID hash = aProperty->OwnerHash();
    CLASS_DESC& classDesc = getClass( hash );
    classDesc.m_ownProperties.emplace( name, aProperty );

    aProperty->SetGroup( aGroup );

    if( !classDesc.m_groups.count( aGroup ) )
    {
        classDesc.m_groupDisplayOrder.emplace_back( aGroup );
        classDesc.m_groups.insert( aGroup );
    }

    m_dirty = true;
}


void PROPERTY_MANAGER::ReplaceProperty( size_t aBase, const wxString& aName, PROPERTY_BASE* aNew,
                                        const wxString& aGroup )
{
    wxASSERT( aBase == aNew->BaseHash() || IsOfType( aNew->OwnerHash(), aBase ) );
    CLASS_DESC& classDesc = getClass( aNew->OwnerHash() );
    classDesc.m_replaced.insert( std::make_pair( aBase, aName ) );
    AddProperty( aNew, aGroup );
}


void PROPERTY_MANAGER::AddTypeCast( TYPE_CAST_BASE* aCast )
{
    TYPE_ID derivedHash = aCast->DerivedHash();
    CLASS_DESC& classDesc = getClass( aCast->BaseHash() );
    auto& typeCasts = classDesc.m_typeCasts;
    wxASSERT_MSG( typeCasts.count( derivedHash ) == 0, "Such converter already exists" );
    typeCasts.emplace( derivedHash, aCast );
}


void PROPERTY_MANAGER::InheritsAfter( TYPE_ID aDerived, TYPE_ID aBase )
{
    wxASSERT_MSG( aDerived != aBase, "Class cannot inherit from itself" );

    CLASS_DESC& derived = getClass( aDerived );
    CLASS_DESC& base = getClass( aBase );
    derived.m_bases.push_back( base );
    m_dirty = true;

    wxASSERT_MSG( derived.m_bases.size() == 1 || derived.m_typeCasts.count( aBase ) == 1,
                  "You need to add a TYPE_CAST for classes inheriting from multiple bases" );
}


void PROPERTY_MANAGER::Mask( TYPE_ID aDerived, TYPE_ID aBase, const wxString& aName )
{
    wxASSERT_MSG( aDerived != aBase, "Class cannot mask from itself" );

    CLASS_DESC& derived = getClass( aDerived );
    derived.m_maskedBaseProperties.insert( std::make_pair( aBase, aName ) );
    m_dirty = true;
}


bool PROPERTY_MANAGER::IsOfType( TYPE_ID aDerived, TYPE_ID aBase ) const
{
    if( aDerived == aBase )
        return true;

    auto derived = m_classes.find( aDerived );
    wxCHECK( derived != m_classes.end(), false );   // missing class description

    // traverse the hierarchy seeking for the base class
    for( auto& base : derived->second.m_bases )
    {
        if( IsOfType( base.get().m_id, aBase ) )
            return true;
    }

    return false;
}


void PROPERTY_MANAGER::Rebuild()
{
    for( std::pair<const TYPE_ID, CLASS_DESC>& classEntry : m_classes )
        classEntry.second.rebuild();

    m_dirty = false;
}


PROPERTY_MANAGER::CLASS_DESC& PROPERTY_MANAGER::getClass( TYPE_ID aTypeId )
{
    auto it = m_classes.find( aTypeId );

    if( it == m_classes.end() )
        tie( it, std::ignore ) = m_classes.emplace( aTypeId, CLASS_DESC( aTypeId ) );

    return it->second;
}


void PROPERTY_MANAGER::CLASS_DESC::rebuild()
{
    PROPERTY_SET replaced( m_replaced );
    m_allProperties.clear();
    collectPropsRecur( m_allProperties, replaced, m_displayOrder, m_maskedBaseProperties );
    // We need to keep properties sorted to be able to use std::set_* functions
    sort( m_allProperties.begin(), m_allProperties.end() );

    std::vector<wxString> displayOrder;
    std::set<wxString> groups;

    auto collectGroups =
            [&]( std::set<wxString>& aSet, std::vector<wxString>& aResult )
    {
        auto collectGroupsRecursive =
                []( auto& aSelf, std::set<wxString>& aSetR, std::vector<wxString>& aResultR,
                    const CLASS_DESC& aClassR ) -> void
        {
            for( const wxString& group : aClassR.m_groups )
            {
                if( !aSetR.count( group ) )
                {
                    aSetR.insert( group );
                    aResultR.emplace_back( group );
                }
            }

            for( const CLASS_DESC& base : aClassR.m_bases )
                aSelf( aSelf, aSetR, aResultR, base );
        };

        collectGroupsRecursive( collectGroupsRecursive, aSet, aResult, *this );
    };

    // TODO(JE): This currently relies on rebuild() happening after all properties are added
    // separate out own groups vs. all groups to fix
    collectGroups( groups, displayOrder );
    m_groupDisplayOrder = displayOrder;
}


void PROPERTY_MANAGER::CLASS_DESC::collectPropsRecur( PROPERTY_LIST& aResult,
                                                      PROPERTY_SET& aReplaced,
                                                      PROPERTY_DISPLAY_ORDER& aDisplayOrder,
                                                      const PROPERTY_SET& aMasked ) const
{
    for( const std::pair<size_t, wxString>& replacedEntry : m_replaced )
        aReplaced.emplace( replacedEntry );

    /*
     * We want to insert our own properties in forward order, but earlier than anything already in
     * the list (which will have been added by a subclass of us)
     */
    int displayOrderStart = 0;

    if( !aDisplayOrder.empty() )
    {
        int firstSoFar = std::min_element( aDisplayOrder.begin(), aDisplayOrder.end(),
                                           []( const std::pair<PROPERTY_BASE*, int>& aFirst,
                                               const std::pair<PROPERTY_BASE*, int>& aSecond )
                                           {
                                               return aFirst.second < aSecond.second;
                                           } )->second;

        displayOrderStart = firstSoFar - m_ownProperties.size();
    }

    int idx = 0;

    for( const std::pair<const wxString, std::unique_ptr<PROPERTY_BASE>>& prop : m_ownProperties )
    {
        PROPERTY_BASE* property = prop.second.get();
        PROPERTY_SET::key_type propertyKey = std::make_pair( property->OwnerHash(),
                                                             property->Name() );
        // Do not store replaced properties
        if( aReplaced.count( propertyKey ) )
            continue;

        // Do not store masked properties
        if( aMasked.count( propertyKey ) )
            continue;

        aDisplayOrder[property] = displayOrderStart + idx++;
        aResult.push_back( property );
    }

    for( const std::reference_wrapper<CLASS_DESC>& base : m_bases )
        base.get().collectPropsRecur( aResult, aReplaced, aDisplayOrder, aMasked );
}


std::vector<TYPE_ID> PROPERTY_MANAGER::GetMatchingClasses( PROPERTY_BASE* aProperty )
{
    std::vector<TYPE_ID> ids;

/*
    for( auto& cls : m_classes )
    {
        CLASS_INFO info;

        for( auto prop : cls.second.m_allProperties )
            info.properties.push_back(prop);


    }
 */

    return ids;
}


PROPERTY_MANAGER::CLASSES_INFO PROPERTY_MANAGER::GetAllClasses()
{
    CLASSES_INFO rv;

    for( std::pair<const TYPE_ID, CLASS_DESC>& classEntry : m_classes )
    {
        CLASS_INFO info;

        info.type = classEntry.first;
        info.name = m_classNames[classEntry.first];

        for( PROPERTY_BASE* prop : classEntry.second.m_allProperties )
            info.properties.push_back( prop );

        rv.push_back( info );
    }

    return rv;
}
