/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef WXSTREAM_HELPER_H
#define WXSTREAM_HELPER_H

#include <wx/log.h>
#include <wx/wfstream.h>


static bool CopyStreamData( wxInputStream& inputStream, wxOutputStream& outputStream,
                            wxFileOffset size )
{
    wxChar buf[128 * 1024];
    int readSize = 128 * 1024;
    wxFileOffset copiedData = 0;

    for( ; ; )
    {
        if(size != -1 && copiedData + readSize > size )
            readSize = size - copiedData;

        inputStream.Read( buf, readSize );

        size_t actuallyRead = inputStream.LastRead();
        outputStream.Write( buf, actuallyRead );

        if( outputStream.LastWrite() != actuallyRead )
        {
            wxLogError( _("Failed to output data") );
            //return false;
        }

        if( size == -1 )
        {
            if( inputStream.Eof() )
                break;
        }
        else
        {
            copiedData += actuallyRead;
            if( copiedData >= size )
                break;
        }
    }

    return true;
}


#endif // WXSTREAM_HELPER_H