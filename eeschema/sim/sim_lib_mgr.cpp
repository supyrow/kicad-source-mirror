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

#include <sim/sim_lib_mgr.h>
#include <sch_symbol.h>
#include <sim/sim_library.h>
#include <sim/sim_model.h>
#include <pgm_base.h>
#include <string>


SIM_LIB_MGR::SIM_LIB_MGR( const PROJECT& aPrj ) : m_project( aPrj )
{
}


SIM_LIBRARY& SIM_LIB_MGR::CreateLibrary( const std::string& aLibraryPath )
{
    auto it = m_libraries.try_emplace( aLibraryPath, SIM_LIBRARY::Create( aLibraryPath ) ).first;
    return *it->second;
}


SIM_MODEL& SIM_LIB_MGR::CreateModel( SIM_MODEL::TYPE aType, int aSymbolPinCount )
{
    m_models.push_back( SIM_MODEL::Create( aType, aSymbolPinCount ) );
    return *m_models.back();
}


SIM_MODEL& SIM_LIB_MGR::CreateModel( const SIM_MODEL& aBaseModel, int aSymbolPinCount )
{
    m_models.push_back( SIM_MODEL::Create( aBaseModel, aSymbolPinCount ) );
    return *m_models.back();
}


SIM_LIBRARY::MODEL SIM_LIB_MGR::CreateModel( SCH_SYMBOL& aSymbol )
{
    return CreateModel( aSymbol.GetFields(), static_cast<int>( aSymbol.GetLibPins().size() ) );
}


template SIM_LIBRARY::MODEL SIM_LIB_MGR::CreateModel( const std::vector<SCH_FIELD>& aFields,
                                                      int aSymbolPinCount );
template SIM_LIBRARY::MODEL SIM_LIB_MGR::CreateModel( const std::vector<LIB_FIELD>& aFields,
                                                      int aSymbolPinCount );

template <typename T>
SIM_LIBRARY::MODEL SIM_LIB_MGR::CreateModel( const std::vector<T>& aFields, int aSymbolPinCount )
{
    std::string libraryPath = SIM_MODEL::GetFieldValue( &aFields, SIM_LIBRARY::LIBRARY_FIELD );
    std::string baseModelName;

    if( libraryPath != "" )
    {
        std::string  absolutePath = std::string( m_project.AbsolutePath( libraryPath ).ToUTF8() );
        SIM_LIBRARY* library = nullptr;

        try
        {
            auto it = m_libraries.try_emplace( libraryPath,
                    SIM_LIBRARY::Create( absolutePath ) ).first;
            library = &*it->second;
        }
        catch( const IO_ERROR& e )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "Error loading simulation model library '%s': %s" ),
                                      absolutePath,
                                      e.What() ) );
        }

        baseModelName = SIM_MODEL::GetFieldValue( &aFields, SIM_LIBRARY::NAME_FIELD );

        if( baseModelName == "" )
        {
            THROW_IO_ERROR( wxString::Format( _( "Error loading simulation model: no '%s' field" ),
                                              SIM_LIBRARY::NAME_FIELD ) );
        }

        SIM_MODEL* baseModel = library->FindModel( baseModelName );

        if( !baseModel )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "Error loading simulation model: could not find base model '%s' in library '%s'" ),
                                      baseModelName,
                                      absolutePath ) );
        }

        m_models.push_back( SIM_MODEL::Create( *baseModel, aSymbolPinCount, aFields ) );
    }
    else
        m_models.push_back( SIM_MODEL::Create( aSymbolPinCount, aFields ) );

    return { baseModelName, *m_models.back() };
}


std::map<std::string, std::reference_wrapper<const SIM_LIBRARY>> SIM_LIB_MGR::GetLibraries() const
{
    std::map<std::string, std::reference_wrapper<const SIM_LIBRARY>> libraries;

    for( auto& [path, library] : m_libraries )
        libraries.try_emplace( path, *library );

    return libraries;
}


std::vector<std::reference_wrapper<SIM_MODEL>> SIM_LIB_MGR::GetModels() const
{
    std::vector<std::reference_wrapper<SIM_MODEL>> models;

    for( const std::unique_ptr<SIM_MODEL>& model : m_models )
        models.emplace_back( *model );

    return models;
}
