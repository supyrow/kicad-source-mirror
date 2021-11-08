/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file gestfich.cpp
 * @brief Functions for file management
 */

#include <wx/mimetype.h>
#include <wx/filename.h>
#include <wx/dir.h>

#include <pgm_base.h>
#include <confirm.h>
#include <core/arraydim.h>
#include <gestfich.h>
#include <string_utils.h>
#include <launch_ext.h>

void QuoteString( wxString& string )
{
    if( !string.StartsWith( wxT( "\"" ) ) )
    {
        string.Prepend ( wxT( "\"" ) );
        string.Append ( wxT( "\"" ) );
    }
}


wxString FindKicadFile( const wxString& shortname )
{
    // Test the presence of the file in the directory shortname of
    // the KiCad binary path.
#ifndef __WXMAC__
    wxString fullFileName = Pgm().GetExecutablePath() + shortname;
#else
    wxString fullFileName = Pgm().GetExecutablePath() + wxT( "Contents/MacOS/" ) + shortname;
#endif
    if( wxFileExists( fullFileName ) )
        return fullFileName;

    // Test the presence of the file in the directory shortname
    // defined by the environment variable KiCad.
    if( Pgm().IsKicadEnvVariableDefined() )
    {
        fullFileName = Pgm().GetKicadEnvVariable() + shortname;

        if( wxFileExists( fullFileName ) )
            return fullFileName;
    }

#if defined( __WINDOWS__ )
    // kicad can be installed highly portably on Windows, anywhere and concurrently
    // either the "kicad file" is immediately adjacent to the exe or it's not a valid install
    return shortname;
#endif

    // Path list for KiCad binary files
    const static wxChar* possibilities[] = {
#if defined( __WXMAC__ )
        // all internal paths are relative to main bundle kicad.app
        wxT( "Contents/Applications/pcbnew.app/Contents/MacOS/" ),
        wxT( "Contents/Applications/eeschema.app/Contents/MacOS/" ),
        wxT( "Contents/Applications/gerbview.app/Contents/MacOS/" ),
        wxT( "Contents/Applications/bitmap2component.app/Contents/MacOS/" ),
        wxT( "Contents/Applications/pcb_calculator.app/Contents/MacOS/" ),
        wxT( "Contents/Applications/pl_editor.app/Contents/MacOS/" ),
#else
        wxT( "/usr/bin/" ),
        wxT( "/usr/local/bin/" ),
        wxT( "/usr/local/kicad/bin/" ),
#endif
    };

    // find binary file from possibilities list:
    for( unsigned i=0;  i<arrayDim(possibilities);  ++i )
    {
#ifndef __WXMAC__
        fullFileName = possibilities[i] + shortname;
#else
        // make relative paths absolute
        fullFileName = Pgm().GetExecutablePath() + possibilities[i] + shortname;
#endif

        if( wxFileExists( fullFileName ) )
            return fullFileName;
    }

    return shortname;
}


int ExecuteFile( const wxString& aEditorName, const wxString& aFileName, wxProcess *aCallback )
{
    wxString fullEditorName;
    wxString param;

#ifdef __UNIX__
    int space = aEditorName.Find( ' ' );

    if( space > 0 && !aEditorName.Contains( "\"" ) && !aEditorName.Contains( "\'" ) )
    {
        fullEditorName = FindKicadFile( aEditorName.Mid( 0, space ) );
        param = aEditorName.Mid( space + 1 );
    }
    else
#endif
    {
        fullEditorName = FindKicadFile( aEditorName );
    }

    if( wxFileExists( fullEditorName ) )
    {
        int i = 0;
        const wchar_t* args[4];

        args[i++] = fullEditorName.wc_str();

        if( !param.IsEmpty() )
            args[i++] = param.wc_str();

        if( !aFileName.IsEmpty() )
            args[i++] = aFileName.wc_str();

        args[i] = nullptr;

        return wxExecute( const_cast<wchar_t**>( args ), wxEXEC_ASYNC, aCallback );
    }

    wxString msg;
    msg.Printf( _( "Command '%s' could not be found." ), fullEditorName );
    DisplayError( nullptr, msg, 20 );
    return -1;
}


bool OpenPDF( const wxString& file )
{
    wxString msg;
    wxString filename = file;

    Pgm().ReadPdfBrowserInfos();

    if( Pgm().UseSystemPdfBrowser() )
    {
        if( !LaunchExternal( filename ) )
        {
            msg.Printf( _( "Unable to find a PDF viewer for '%s'." ), filename );
            DisplayError( nullptr, msg );
            return false;
        }
    }
    else
    {
        const wchar_t* args[3];

        args[0] = Pgm().GetPdfBrowserName().wc_str();
        args[1] = filename.wc_str();
        args[2] = nullptr;

        if( wxExecute( const_cast<wchar_t**>( args ) ) == -1 )
        {
            msg.Printf( _( "Problem while running the PDF viewer '%s'." ), args[0] );
            DisplayError( nullptr, msg );
            return false;
        }
    }

    return true;
}


void OpenFile( const wxString& file )
{
    wxFileName  fileName( file );
    wxFileType* filetype = wxTheMimeTypesManager->GetFileTypeFromExtension( fileName.GetExt() );

    if( !filetype )
        return;

    wxString    command;
    wxFileType::MessageParameters params( file );

    filetype->GetOpenCommand( &command, params );
    delete filetype;

    if( !command.IsEmpty() )
        wxExecute( command );
}


bool doPrintFile( const wxString& file, bool aDryRun )
{
    wxString ext = wxFileName( file ).GetExt();

#ifdef __WXMSW__
    wxFileType* filetype = wxTheMimeTypesManager->GetFileTypeFromExtension( ext );

    if( filetype )
    {
        wxString                      printCommand;
        wxFileType::MessageParameters params( file );

        filetype->GetPrintCommand( &printCommand, params );

        delete filetype;

        if( !printCommand.IsEmpty() )
        {
            if( !aDryRun )
                wxExecute( printCommand );

            return true;
        }
    }
#endif

#ifdef __WXMAC__
    wxString app;

    if( ext == "ps" || ext == "pdf" )
        app = "Preview";
    else if( ext == "csv" )
        app = "Numbers";
    else if( ext == "txt" )
        app = "TextEdit";
    else if( ext == "rpt" || ext == "pos" || ext == "cmp" || ext == "net" || ext == "kicad_dru" )
        app = "TextEdit";

    if( !app.IsEmpty() )
    {
        if( !aDryRun )
        {
            wxString ascript = "osascript ";
            ascript += "-e 'tell app \"" + app + "\"' ";
            ascript += "-e '   set srcFileRef to (open POSIX file \"" + EscapeString( file, CTX_QUOTED_STR ) + "\")' ";
            ascript += "-e '   activate' ";
            ascript += "-e '   print srcFileRef print dialog true' ";
            ascript += "-e 'end tell' ";

            wxExecute( ascript );
        }

        return true;
    }
#endif

#ifdef __WXGTK__
    if( ext == "ps" || ext == "pdf"
            || ext == "csv"
            || ext == "txt"
            || ext == "rpt" || ext == "pos" || ext == "cmp" || ext == "net" || ext == "kicad_dru" )
    {
        if( !aDryRun )
        {
            const wchar_t* args[3];
            args[0] = wxT( "lp" );
            args[1] = file.wc_str();
            args[2] = nullptr;

            wxExecute( const_cast<wchar_t**>( args ) );
        }

        return true;
    }
#endif

    if( !aDryRun )
    {
        DisplayError( nullptr, wxString::Format( _( "Cannot print '%s'.\n\nUnknown file type." ),
                                                 file ) );
    }

    return false;
}


void PrintFile( const wxString& file )
{
    doPrintFile( file, false );
}


bool CanPrintFile( const wxString& file )
{
    return doPrintFile( file, true );
}


void KiCopyFile( const wxString& aSrcPath, const wxString& aDestPath, wxString& aErrors )
{
    if( !wxCopyFile( aSrcPath, aDestPath ) )
    {
        wxString msg;

        if( !aErrors.IsEmpty() )
            aErrors += "\n";

        msg.Printf( _( "Cannot copy file '%s'." ), aDestPath );
        aErrors += msg;
    }
}


wxString QuoteFullPath( wxFileName& fn, wxPathFormat format )
{
    return wxT( "\"" ) + fn.GetFullPath( format ) + wxT( "\"" );
}
