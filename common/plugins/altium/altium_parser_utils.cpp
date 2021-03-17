/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "altium_parser_utils.h"

#include <kicad_string.h>
#include <lib_id.h>


LIB_ID AltiumToKiCadLibID( wxString aLibName, wxString aLibReference )
{
    ReplaceIllegalFileNameChars( aLibName, '_' );
    ReplaceIllegalFileNameChars( aLibReference, '_' );

    wxString key = !aLibName.empty() ? ( aLibName + ":" + aLibReference ) : aLibReference;

    LIB_ID libId;
    libId.Parse( key, true );

    return libId;
}

// https://www.altium.com/documentation/altium-designer/sch-obj-textstringtext-string-ad#!special-strings
wxString AltiumSpecialStringsToKiCadVariables( const wxString&                     aString,
                                               const std::map<wxString, wxString>& aOverride )
{
    if( aString.IsEmpty() || aString.at( 0 ) != '=' )
    {
        return aString;
    }

    wxString result;

    size_t start = 1;
    size_t delemiter = 0;
    do
    {
        delemiter = aString.find( "+", start );

        wxString specialString = aString.substr( start, delemiter - start ).Trim( true );

        if( !specialString.IsEmpty() )
        {
            auto variableOverride = aOverride.find( specialString );
            if( variableOverride == aOverride.end() )
            {
                result += wxString::Format( wxT( "${%s}" ), specialString );
            }
            else
            {
                result += variableOverride->second;
            }
        }

        start = delemiter + 1;
    } while( delemiter != wxString::npos );

    return result;
}