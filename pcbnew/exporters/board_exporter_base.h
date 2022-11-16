/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#ifndef __BOARD_EXPORTER_BASE_H
#define __BOARD_EXPORTER_BASE_H

#include <string_utf8_map.h>
#include <wx/file.h>

class BOARD;
class REPORTER;
class PROGRESS_REPORTER;

class wxDialog;

class BOARD_EXPORTER_BASE
{
public:
    BOARD_EXPORTER_BASE()
    {
    };

    virtual ~BOARD_EXPORTER_BASE()
    {
    };

    void SetOutputFilename( const wxFileName& aPath )
    {
        m_outputFilePath = aPath;
    }

    void SetBoard( BOARD* aBoard )
    {
        m_board = aBoard;
    }

    void SetReporter( REPORTER* aReporter )
    {
        m_reporter = aReporter;
    }

    void SetProgressReporter( PROGRESS_REPORTER *aProgressReporter )
    {
        m_progressReporter = aProgressReporter;
    }

    virtual bool Run() = 0;

protected:
    STRING_UTF8_MAP    m_properties;
    BOARD*             m_board = nullptr;
    wxFileName         m_outputFilePath;
    REPORTER*          m_reporter = nullptr;
    PROGRESS_REPORTER* m_progressReporter = nullptr;
};

#endif
