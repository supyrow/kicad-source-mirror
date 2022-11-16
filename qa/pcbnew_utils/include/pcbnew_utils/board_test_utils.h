/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef QA_PCBNEW_BOARD_TEST_UTILS__H
#define QA_PCBNEW_BOARD_TEST_UTILS__H

#include <memory>
#include <string>
#include <wx/string.h>
#include <mutex>
#include <map>
#include <reporter.h>

class BOARD;
class BOARD_ITEM;
class FOOTPRINT;
class FP_TEXT;
class FP_SHAPE;
class FP_ZONE;
class PAD;
class SHAPE_POLY_SET;
class SETTINGS_MANAGER;


namespace KI_TEST
{
/**
 * A helper that contains logic to assist in dumping boards to
 * disk depending on some environment variables.
 *
 * This is useful when setting up or verifying unit tests that work on BOARD
 * objects.
 *
 * To dump files set the KICAD_TEST_DUMP_BOARD_FILES environment variable.
 * Files will be written to the system temp directory (/tmp on Linux, or as set
 * by $TMP and friends).
 */
class BOARD_DUMPER
{
public:
    BOARD_DUMPER();

    void DumpBoardToFile( BOARD& aBoard, const std::string& aName ) const;

    const bool m_dump_boards;
};


class CONSOLE_LOG
{
public:
    enum COLOR
    {
        RED = 0,
        GREEN,
        DEFAULT
    };

    CONSOLE_LOG(){};

    void PrintProgress( const wxString& aMessage )
    {
        if( m_lastLineIsProgressBar )
            eraseLastLine();

        printf( "%s", (const char*) aMessage.c_str() );
        fflush( stdout );

        m_lastLineIsProgressBar = true;
    }


    void Print( const wxString& aMessage )
    {
        if( m_lastLineIsProgressBar )
            eraseLastLine();

        printf( "%s", (const char*) aMessage.c_str() );
        fflush( stdout );

        m_lastLineIsProgressBar = false;
    }


    void SetColor( COLOR color )
    {
        std::map<COLOR, wxString> colorMap = { { RED, "\033[0;31m" },
                                               { GREEN, "\033[0;32m" },
                                               { DEFAULT, "\033[0;37m" } };

        printf( "%s", (const char*) colorMap[color].c_str() );
        fflush( stdout );
    }


private:
    void eraseLastLine()
    {
        printf( "\r\033[K" );
        fflush( stdout );
    }

    bool       m_lastLineIsProgressBar = false;
    std::mutex m_lock;
};


class CONSOLE_MSG_REPORTER : public REPORTER
{
public:
    CONSOLE_MSG_REPORTER( CONSOLE_LOG* log ) : m_log( log ){};
    ~CONSOLE_MSG_REPORTER(){};


    virtual REPORTER& Report( const wxString& aText,
                              SEVERITY        aSeverity = RPT_SEVERITY_UNDEFINED ) override
    {
        switch( aSeverity )
        {
        case RPT_SEVERITY_ERROR:
            m_log->SetColor( CONSOLE_LOG::RED );
            m_log->Print( "ERROR | " );
            break;

        default: m_log->SetColor( CONSOLE_LOG::DEFAULT ); m_log->Print( "      | " );
        }

        m_log->SetColor( CONSOLE_LOG::DEFAULT );
        m_log->Print( aText + "\n" );
        return *this;
    }

    virtual bool HasMessage() const override { return true; }

private:
    CONSOLE_LOG* m_log;
};


void LoadBoard( SETTINGS_MANAGER& aSettingsManager, const wxString& aRelPath,
                std::unique_ptr<BOARD>& aBoard );

void FillZones( BOARD* m_board );


/**
 * Helper method to check if two footprints are semantically the same.
 */
void CheckFootprint( const FOOTPRINT* expected, const FOOTPRINT* fp );

void CheckFpPad( const PAD* expected, const PAD* pad );

void CheckFpText( const FP_TEXT* expected, const FP_TEXT* text );

void CheckFpShape( const FP_SHAPE* expected, const FP_SHAPE* shape );

void CheckFpZone( const FP_ZONE* expected, const FP_ZONE* zone );

void CheckShapePolySet( const SHAPE_POLY_SET* expected, const SHAPE_POLY_SET* polyset );

} // namespace KI_TEST

#endif // QA_PCBNEW_BOARD_TEST_UTILS__H
