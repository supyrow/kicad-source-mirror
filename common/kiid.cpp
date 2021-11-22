/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney@ieee.org>
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <kiid.h>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/functional/hash.hpp>

#if BOOST_VERSION >= 106700
#include <boost/uuid/entropy_error.hpp>
#endif

#include <mutex>

#include <wx/log.h>

// boost:mt19937 is not thread-safe
static std::mutex rng_mutex;

// Create only once, as seeding is *very* expensive
static boost::mt19937 rng;
static boost::uuids::basic_random_generator<boost::mt19937> randomGenerator( rng );

// These don't have the same performance penalty, but might as well be consistent
static boost::uuids::string_generator stringGenerator;
static boost::uuids::nil_generator    nilGenerator;

// Global nil reference
KIID niluuid( 0 );

// When true, always create nil uuids for performance, when valid ones aren't needed
static bool createNilUuids = false;


// For static initialization
KIID& NilUuid()
{
    static KIID nil( 0 );
    return nil;
}


KIID::KIID()
{
    m_cached_timestamp = 0;

#if BOOST_VERSION >= 106700
    try
    {
#endif

        if( createNilUuids )
        {
            m_uuid = nilGenerator();
        }
        else
        {
            std::lock_guard<std::mutex> lock( rng_mutex );
            m_uuid = randomGenerator();
        }

#if BOOST_VERSION >= 106700
    }
    catch( const boost::uuids::entropy_error& )
    {
        wxLogFatalError( "A Boost UUID entropy exception was thrown in %s:%s.",
                         __FILE__, __FUNCTION__ );
    }
#endif
}


KIID::KIID( int null ) : m_uuid( nilGenerator() ), m_cached_timestamp( 0 )
{
    wxASSERT( null == 0 );
}


KIID::KIID( const wxString& aString ) : m_uuid(), m_cached_timestamp( 0 )
{
    if( aString.length() == 8 )
    {
        // A legacy-timestamp-based UUID has only the last 4 octets filled in.
        // Convert them individually to avoid stepping in the little-endian/big-endian
        // doo-doo.
        for( int i = 0; i < 4; ++i )
        {
            wxString octet      = aString.substr( i * 2, 2 );
            m_uuid.data[i + 12] = strtol( octet.data(), nullptr, 16 );
        }

        m_cached_timestamp = strtol( aString.c_str(), nullptr, 16 );
    }
    else
    {
        try
        {
            m_uuid = stringGenerator( aString.wc_str() );

            if( IsLegacyTimestamp() )
                m_cached_timestamp = strtol( aString.substr( 28 ).c_str(), nullptr, 16 );
        }
        catch( ... )
        {
            // Failed to parse string representation; best we can do is assign a new
            // random one.
#if BOOST_VERSION >= 106700
            try
            {
#endif

                m_uuid = randomGenerator();

#if BOOST_VERSION >= 106700
            }
            catch( const boost::uuids::entropy_error& )
            {
                wxLogFatalError( "A Boost UUID entropy exception was thrown in %s:%s.",
                                 __FILE__, __FUNCTION__ );
            }
#endif
        }
    }
}


bool KIID::SniffTest( const wxString& aCandidate )
{
    static wxString niluuidStr = niluuid.AsString();

    if( aCandidate.Length() != niluuidStr.Length() )
        return false;

    for( wxChar c : aCandidate )
    {
        if( c >= '0' && c <= '9' )
            continue;

        if( c >= 'a' && c <= 'f' )
            continue;

        if( c >= 'A' && c <= 'F' )
            continue;

        if( c == '-' )
            continue;

        return false;
    }

    return true;
}


KIID::KIID( timestamp_t aTimestamp )
{
    m_cached_timestamp = aTimestamp;

    // A legacy-timestamp-based UUID has only the last 4 octets filled in.
    // Convert them individually to avoid stepping in the little-endian/big-endian
    // doo-doo.
    wxString str = AsLegacyTimestampString();

    for( int i = 0; i < 4; ++i )
    {
        wxString octet      = str.substr( i * 2, 2 );
        m_uuid.data[i + 12] = strtol( octet.data(), nullptr, 16 );
    }
}


bool KIID::IsLegacyTimestamp() const
{
    return !m_uuid.data[8] && !m_uuid.data[9] && !m_uuid.data[10] && !m_uuid.data[11];
}


timestamp_t KIID::AsLegacyTimestamp() const
{
    return m_cached_timestamp;
}


size_t KIID::Hash() const
{
    size_t hash = 0;

    // Note: this is NOT little-endian/big-endian safe, but as long as it's just used
    // at runtime it won't matter.

    for( int i = 0; i < 4; ++i )
        boost::hash_combine( hash, reinterpret_cast<const uint32_t*>( m_uuid.data )[i] );

    return hash;
}


void KIID::Clone( const KIID& aUUID )
{
    m_uuid             = aUUID.m_uuid;
    m_cached_timestamp = aUUID.m_cached_timestamp;
}


wxString KIID::AsString() const
{
    return boost::uuids::to_string( m_uuid );
}


wxString KIID::AsLegacyTimestampString() const
{
    return wxString::Format( "%8.8lX", (unsigned long) AsLegacyTimestamp() );
}


void KIID::ConvertTimestampToUuid()
{
    if( !IsLegacyTimestamp() )
        return;

    m_cached_timestamp = 0;
    m_uuid             = randomGenerator();
}


void KIID::CreateNilUuids( bool aNil )
{
    createNilUuids = aNil;
}


void KIID::SeedGenerator( unsigned int aSeed )
{
    rng.seed( aSeed );
}


KIID_PATH::KIID_PATH( const wxString& aString )
{
    for( const wxString& pathStep : wxSplit( aString, '/' ) )
    {
        if( !pathStep.empty() )
            emplace_back( KIID( pathStep ) );
    }
}


bool KIID_PATH::MakeRelativeTo( const KIID_PATH& aPath )
{
    KIID_PATH copy = *this;
    clear();

    if( aPath.size() > copy.size() )
        return false; // this path is not contained within aPath

    for( size_t i = 0; i < aPath.size(); ++i )
    {
        if( copy.at( i ).AsString() != aPath.at( i ).AsString() )
            return false; // this path is not contained within aPath
    }

    for( size_t i = aPath.size(); i < copy.size(); ++i )
        push_back( copy.at( i ) );

    return true;
}


wxString KIID_PATH::AsString() const
{
    wxString path;

    for( const KIID& pathStep : *this )
        path += '/' + pathStep.AsString();

    return path;
}
