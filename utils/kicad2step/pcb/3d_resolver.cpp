/*
 * This program source code file is part kicad2mcad
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/thread.h>
#include <wx/utils.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include "kicadpcb.h"

#include "3d_resolver.h"

// configuration file version
#define CFGFILE_VERSION 1
#define S3D_RESOLVER_CONFIG "ExportPaths.cfg"

// flag bits used to track different one-off messages to users
#define ERRFLG_ALIAS    (1)
#define ERRFLG_RELPATH  (2)
#define ERRFLG_ENVPATH  (4)


/**
 * Flag to enable plugin loader trace output.
 *
 * @ingroup trace_env_vars
 */
const wxChar* const trace3dResolver = wxT( "KICAD_3D_RESOLVER" );


static std::mutex mutex3D_resolver;


static bool getHollerith( const std::string& aString, size_t& aIndex, wxString& aResult );


S3D_RESOLVER::S3D_RESOLVER()
{
    m_errflags = 0;
}


bool S3D_RESOLVER::Set3DConfigDir( const wxString& aConfigDir )
{
    createPathList();

    return true;
}


bool S3D_RESOLVER::SetProjectDir( const wxString& aProjDir, bool* flgChanged )
{
    if( aProjDir.empty() )
        return false;

    wxFileName projdir( aProjDir, "" );
    projdir.Normalize();

    if( false == projdir.DirExists() )
        return false;

    m_curProjDir = projdir.GetPath();
    wxSetEnv( "KIPRJMOD", m_curProjDir );

    if( flgChanged )
        *flgChanged = false;

    if( m_Paths.empty() )
    {
        SEARCH_PATH al;
        al.m_Alias = "${KIPRJMOD}";
        al.m_Pathvar = "${KIPRJMOD}";
        al.m_Pathexp = m_curProjDir;
        m_Paths.push_back( al );
        m_NameMap.clear();

        if( flgChanged )
            *flgChanged = true;

    }
    else
    {
        if( m_Paths.front().m_Pathexp.Cmp( m_curProjDir ) )
        {
            m_Paths.front().m_Pathexp = m_curProjDir;
            m_NameMap.clear();

            if( flgChanged )
                *flgChanged = true;

        }
        else
        {
            return true;
        }
    }

    wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n"
                                       " * [INFO] changed project dir to '%s'" ),
                __FILE__, __FUNCTION__, __LINE__, m_Paths.front().m_Pathexp );

    return true;
}


wxString S3D_RESOLVER::GetProjectDir( void )
{
    return m_curProjDir;
}


bool S3D_RESOLVER::createPathList( void )
{
    if( !m_Paths.empty() )
        return true;

    wxString kmod;

    // add an entry for the default search path; at this point
    // we cannot set a sensible default so we use an empty string.
    // the user may change this later with a call to SetProjectDir()

    SEARCH_PATH lpath;
    lpath.m_Alias = "${KIPRJMOD}";
    lpath.m_Pathvar = "${KIPRJMOD}";
    lpath.m_Pathexp = m_curProjDir;
    m_Paths.push_back( lpath );
    wxFileName fndummy;
    wxUniChar psep = fndummy.GetPathSeparator();
    bool hasKICAD6_3DMODEL_DIR = false;

    // iterate over the list of internally defined ENV VARs
    // and add existing paths to the resolver
    std::map< wxString, wxString >::const_iterator mS = m_EnvVars.begin();
    std::map< wxString, wxString >::const_iterator mE = m_EnvVars.end();

    while( mS != mE )
    {
        // filter out URLs, template directories, and known system paths
        if( mS->first == wxString( "KICAD_PTEMPLATES" )
            || mS->first == wxString( "KICAD6_FOOTPRINT_DIR" ) )
        {
            ++mS;
            continue;
        }

        if( wxString::npos != mS->second.find( wxString( "://" ) ) )
        {
            ++mS;
            continue;
        }

        fndummy.Assign( mS->second, "" );
        wxString pathVal;

        // ensure system ENV VARs supersede internally defined vars
        if( wxGetEnv( mS->first, &pathVal ) && wxDirExists( pathVal ) )
            fndummy.Assign( pathVal, "" );
        else
            fndummy.Assign( mS->second, "" );

        fndummy.Normalize();

        if( !fndummy.DirExists() )
        {
            ++mS;
            continue;
        }

        wxString tmp( "${" );
        tmp.Append( mS->first );
        tmp.Append( "}" );

        if( tmp == "${KICAD6_3DMODEL_DIR}" )
            hasKICAD6_3DMODEL_DIR = true;

        lpath.m_Alias =  tmp;
        lpath.m_Pathvar = tmp;
        lpath.m_Pathexp = fndummy.GetFullPath();

        if( !lpath.m_Pathexp.empty() && psep == *lpath.m_Pathexp.rbegin() )
            lpath.m_Pathexp.erase( --lpath.m_Pathexp.end() );

        m_Paths.push_back( lpath );

        ++mS;
    }

    // special case: if KICAD6_FOOTPRINT_DIR is not internally defined but is defined by
    // the system, then create an entry here
    wxString envar;

    if( !hasKICAD6_3DMODEL_DIR && wxGetEnv( "KICAD6_3DMODEL_DIR", &envar ) )
    {
        lpath.m_Alias = "${KICAD6_3DMODEL_DIR}";
        lpath.m_Pathvar = "${KICAD6_3DMODEL_DIR}";
        fndummy.Assign( envar, "" );
        fndummy.Normalize();

        if( fndummy.DirExists() )
        {
            lpath.m_Pathexp = fndummy.GetFullPath();

            if( !lpath.m_Pathexp.empty() && psep == *lpath.m_Pathexp.rbegin() )
                lpath.m_Pathexp.erase( --lpath.m_Pathexp.end() );

            if( !lpath.m_Pathexp.empty() )
                m_Paths.push_back( lpath );
        }

    }

    if( !m_ConfigDir.empty() )
        readPathList();

    if( m_Paths.empty() )
        return false;

#ifdef DEBUG
    wxLogTrace( trace3dResolver, " * [3D model] search paths:\n" );

    for( const auto searchPath : m_Paths )
        wxLogTrace( trace3dResolver, "   + '%s'\n", searchPath.m_Pathexp );
#endif

    return true;
}


wxString S3D_RESOLVER::ResolvePath( const wxString& aFileName )
{
    std::lock_guard<std::mutex> lock( mutex3D_resolver );

    if( aFileName.empty() )
        return wxEmptyString;

    if( m_Paths.empty() )
        createPathList();

    // look up the filename in the internal filename map
    std::map< wxString, wxString, S3D::rsort_wxString >::iterator mi;
    mi = m_NameMap.find( aFileName );

    if( mi != m_NameMap.end() )
        return mi->second;

    // first attempt to use the name as specified:
    wxString tname = aFileName;

#ifdef _WIN32
    // translate from KiCad's internal UNIX-like path to MSWin paths
    tname.Replace( "/", "\\" );
#endif

    // Note: variable expansion must preferably be performed via a
    // threadsafe wrapper for the getenv() system call. If we allow the
    // wxFileName::Normalize() routine to perform expansion then
    // we will have a race condition since wxWidgets does not assure
    // a threadsafe wrapper for getenv().
    if( tname.StartsWith( "${" ) || tname.StartsWith( "$(" ) )
        tname = expandVars( tname );

    wxFileName tmpFN( tname );

    // in the case of absolute filenames we don't store a map item
    if( !aFileName.StartsWith( "${" ) && !aFileName.StartsWith( "$(" )
        && !aFileName.StartsWith( ":" ) && tmpFN.IsAbsolute() )
    {
        tmpFN.Normalize();

        if( tmpFN.FileExists() )
            return tmpFN.GetFullPath();

        return wxEmptyString;
    }

    // this case covers full paths, leading expanded vars, and paths
    // relative to the current working directory (which is not necessarily
    // the current project directory)
    if( tmpFN.FileExists() )
    {
        tmpFN.Normalize();
        tname = tmpFN.GetFullPath();
        m_NameMap.insert( std::pair< wxString, wxString > ( aFileName, tname ) );

        // special case: if a path begins with ${ENV_VAR} but is not in the
        // resolver's path list then add it
        if( aFileName.StartsWith( "${" ) || aFileName.StartsWith( "$(" ) )
            checkEnvVarPath( aFileName );

        return tname;
    }

    // if a path begins with ${ENV_VAR}/$(ENV_VAR) and is not resolved then the
    // file either does not exist or the ENV_VAR is not defined
    if( aFileName.StartsWith( "${" ) || aFileName.StartsWith( "$(" ) )
    {
        if( !( m_errflags & ERRFLG_ENVPATH ) )
        {
            m_errflags |= ERRFLG_ENVPATH;
            wxString errmsg = "[3D File Resolver] File \"";
            errmsg << aFileName << "\" not found\n";
            ReportMessage( errmsg );
        }

        return wxEmptyString;
    }

    // at this point aFileName is:
    // a. an aliased shortened name or
    // b. cannot be determined

    std::list< SEARCH_PATH >::const_iterator sPL = m_Paths.begin();
    std::list< SEARCH_PATH >::const_iterator ePL = m_Paths.end();

    // check the path relative to the current project directory;
    // note: this is not necessarily the same as the current working
    // directory, which has already been checked. This case accounts
    // for partial paths which do not contain ${KIPRJMOD}.
    // This check is performed before checking the path relative to
    // ${KICAD6_3DMODEL_DIR} so that users can potentially override a model
    // within ${KICAD6_3DMODEL_DIR}
    if( !sPL->m_Pathexp.empty() && !tname.StartsWith( ":" ) )
    {
        tmpFN.Assign( sPL->m_Pathexp, "" );
        wxString fullPath = tmpFN.GetPathWithSep() + tname;

        if( fullPath.StartsWith( "${" ) || fullPath.StartsWith( "$(" ) )
            fullPath = expandVars( fullPath );

        if( wxFileName::FileExists( fullPath ) )
        {
            tmpFN.Assign( fullPath );
            tmpFN.Normalize();
            tname = tmpFN.GetFullPath();
            m_NameMap.insert( std::pair< wxString, wxString > ( aFileName, tname ) );

            return tname;
        }

    }

    // check the partial path relative to ${KICAD6_3DMODEL_DIR} (legacy behavior)
    if( !tname.StartsWith( ":" ) )
    {
        wxFileName fpath;
        wxString fullPath( "${KICAD6_3DMODEL_DIR}" );
        fullPath.Append( fpath.GetPathSeparator() );
        fullPath.Append( tname );
        fullPath = expandVars( fullPath );
        fpath.Assign( fullPath );

        if( fpath.Normalize() && fpath.FileExists() )
        {
            tname = fpath.GetFullPath();
            m_NameMap.insert( std::pair< wxString, wxString > ( aFileName, tname ) );
            return tname;
        }

    }

    // ${ENV_VAR} paths have already been checked; skip them
    while( sPL != ePL && ( sPL->m_Alias.StartsWith( "${" ) || sPL->m_Alias.StartsWith( "$(" ) ) )
        ++sPL;

    // at this point the filename must contain an alias or else it is invalid
    wxString alias;         // the alias portion of the short filename
    wxString relpath;       // the path relative to the alias

    if( !SplitAlias( tname, alias, relpath ) )
    {
        if( !( m_errflags & ERRFLG_RELPATH ) )
        {
            // this can happen if the file was intended to be relative to
            // ${KICAD6_3DMODEL_DIR} but ${KICAD6_3DMODEL_DIR} not set or incorrect.
            m_errflags |= ERRFLG_RELPATH;
            wxString errmsg = "[3D File Resolver] No such path";
            errmsg.append( "\n" );
            errmsg.append( tname );
            wxLogTrace( trace3dResolver, "%s\n", errmsg.ToUTF8() );
        }

        return wxEmptyString;
    }

    while( sPL != ePL )
    {
        if( !sPL->m_Alias.Cmp( alias ) && !sPL->m_Pathexp.empty() )
        {
            wxFileName fpath( wxFileName::DirName( sPL->m_Pathexp ) );
            wxString fullPath = fpath.GetPathWithSep() + relpath;

            if( fullPath.StartsWith( "${") || fullPath.StartsWith( "$(" ) )
                fullPath = expandVars( fullPath );

            if( wxFileName::FileExists( fullPath ) )
            {
                wxFileName tmp( fullPath );

                if( tmp.Normalize() )
                    tname = tmp.GetFullPath();

                m_NameMap.insert( std::pair< wxString, wxString > ( aFileName, tname ) );
                return tname;
            }
        }

        ++sPL;
    }

    if( !( m_errflags & ERRFLG_ALIAS ) )
    {
        m_errflags |= ERRFLG_ALIAS;
        wxLogTrace( trace3dResolver,
                    wxT( "[3D File Resolver] No such path; ensure the path alias is defined %s" ),
                    tname.substr( 1 ) );
    }

    return wxEmptyString;
}


bool S3D_RESOLVER::addPath( const SEARCH_PATH& aPath )
{
    if( aPath.m_Alias.empty() || aPath.m_Pathvar.empty() )
        return false;

    std::lock_guard<std::mutex> lock( mutex3D_resolver );

    SEARCH_PATH tpath = aPath;

#ifdef _WIN32
    while( tpath.m_Pathvar.EndsWith( "\\" ) )
        tpath.m_Pathvar.erase( tpath.m_Pathvar.length() - 1 );
#else
    while( tpath.m_Pathvar.EndsWith( "/" ) && tpath.m_Pathvar.length() > 1 )
        tpath.m_Pathvar.erase( tpath.m_Pathvar.length() - 1 );
#endif

    wxFileName path( tpath.m_Pathvar, "" );
    path.Normalize();

    if( !path.DirExists() )
    {
        // suppress the message if the missing pathvar is the legacy KICAD6_3DMODEL_DIR variable
        if( aPath.m_Pathvar != "${KICAD6_3DMODEL_DIR}" &&
            aPath.m_Pathvar != "$(KICAD6_3DMODEL_DIR)" )
        {
            wxString msg = _( "The given path does not exist" );
            msg.append( "\n" );
            msg.append( tpath.m_Pathvar );
            wxLogMessage( "%s\n", msg.ToUTF8() );
        }

        tpath.m_Pathexp.clear();
    }
    else
    {
        tpath.m_Pathexp = path.GetFullPath();

#ifdef _WIN32
        while( tpath.m_Pathexp.EndsWith( "\\" ) )
        tpath.m_Pathexp.erase( tpath.m_Pathexp.length() - 1 );
#else
        while( tpath.m_Pathexp.EndsWith( "/" ) && tpath.m_Pathexp.length() > 1 )
            tpath.m_Pathexp.erase( tpath.m_Pathexp.length() - 1 );
#endif
    }

    wxString pname = path.GetPath();
    std::list< SEARCH_PATH >::iterator sPL = m_Paths.begin();
    std::list< SEARCH_PATH >::iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        if( !tpath.m_Alias.Cmp( sPL->m_Alias ) )
        {
            wxString msg = _( "Alias:" ) + wxS( " " );
            msg.append( tpath.m_Alias );
            msg.append( "\n" );
            msg.append( _( "This path:" ) + wxS( " " ) );
            msg.append( tpath.m_Pathvar );
            msg.append( "\n" );
            msg.append( _( "Existing path:" ) + wxS( " " ) );
            msg.append( sPL->m_Pathvar );
            wxMessageBox( msg, _( "Bad alias (duplicate name)" ) );

            return false;
        }

        ++sPL;
    }

    m_Paths.push_back( tpath );
    return true;
}


bool S3D_RESOLVER::readPathList( void )
{
    wxFileName cfgpath( wxStandardPaths::Get().GetTempDir(), S3D_RESOLVER_CONFIG );
    cfgpath.Normalize();
    wxString cfgname = cfgpath.GetFullPath();

    size_t nitems = m_Paths.size();

    std::ifstream cfgFile;
    std::string   cfgLine;

    if( !wxFileName::Exists( cfgname ) )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:d\n"
                                          " * no 3D configuration file '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, cfgname );

        return false;
    }

    cfgFile.open( cfgname.ToUTF8() );

    if( !cfgFile.is_open() )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n"
                                          " * Could not open configuration file '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, cfgname );

        return false;
    }

    int lineno = 0;
    SEARCH_PATH al;
    size_t idx;
    int vnum = 0;           // version number

    while( cfgFile.good() )
    {
        cfgLine.clear();
        std::getline( cfgFile, cfgLine );
        ++lineno;

        if( cfgLine.empty() )
        {
            if( cfgFile.eof() )
                break;

            continue;
        }

        if( 1 == lineno && cfgLine.compare( 0, 2, "#V" ) == 0 )
        {
            // extract the version number and parse accordingly
            if( cfgLine.size() > 2 )
            {
                std::istringstream istr;
                istr.str( cfgLine.substr( 2 ) );
                istr >> vnum;
            }

            continue;
        }

        idx = 0;

        if( !getHollerith( cfgLine, idx, al.m_Alias ) )
            continue;

        // never add on KICAD6_3DMODEL_DIR from a config file
        if( !al.m_Alias.Cmp( "KICAD6_3DMODEL_DIR" ) )
            continue;

        if( !getHollerith( cfgLine, idx, al.m_Pathvar ) )
            continue;

        if( !getHollerith( cfgLine, idx, al.m_Description ) )
            continue;

        addPath( al );
    }

    cfgFile.close();

    if( m_Paths.size() != nitems )
        return true;

    return false;
}


void S3D_RESOLVER::checkEnvVarPath( const wxString& aPath )
{
    bool useParen = false;

    if( aPath.StartsWith( "$(" ) )
        useParen = true;
    else if( !aPath.StartsWith( "${" ) )
        return;

    size_t pEnd;

    if( useParen )
        pEnd = aPath.find( ")" );
    else
        pEnd = aPath.find( "}" );

    if( pEnd == wxString::npos )
        return;

    wxString envar = aPath.substr( 0, pEnd + 1 );

    // check if the alias exists; if not then add it to the end of the
    // env var section of the path list
    std::list< SEARCH_PATH >::iterator sPL = m_Paths.begin();
    std::list< SEARCH_PATH >::iterator ePL = m_Paths.end();

    while( sPL != ePL )
    {
        if( sPL->m_Alias == envar )
            return;

        if( !sPL->m_Alias.StartsWith( "${" ) )
            break;

        ++sPL;
    }

    SEARCH_PATH lpath;
    lpath.m_Alias = envar;
    lpath.m_Pathvar = lpath.m_Alias;
    wxFileName tmpFN( lpath.m_Alias, "" );
    wxUniChar psep = tmpFN.GetPathSeparator();
    tmpFN.Normalize();

    if( !tmpFN.DirExists() )
        return;

    lpath.m_Pathexp = tmpFN.GetFullPath();

    if( !lpath.m_Pathexp.empty() && psep == *lpath.m_Pathexp.rbegin() )
        lpath.m_Pathexp.erase( --lpath.m_Pathexp.end() );

    if( lpath.m_Pathexp.empty() )
        return;

    m_Paths.insert( sPL, lpath );
    return;
}


wxString S3D_RESOLVER::expandVars( const wxString& aPath )
{
    if( aPath.empty() )
        return wxEmptyString;

    wxString result;

    for( const auto& i : m_EnvVars )
    {
        if( !aPath.compare( 2, i.first.length(), i.first ) )
        {
            result = i.second;
            result.append( aPath.substr( 3 + i.first.length() ) );

            if( result.StartsWith( "${" ) || result.StartsWith( "$(" ) )
                result = expandVars( result );

            return result;
        }
    }

    result = wxExpandEnvVars( aPath );

    if( result == aPath )
        return wxEmptyString;

    if( result.StartsWith( "${" ) || result.StartsWith( "$(" ) )
        result = expandVars( result );

    return result;
}


wxString S3D_RESOLVER::ShortenPath( const wxString& aFullPathName )
{
    wxString fname = aFullPathName;

    if( m_Paths.empty() )
        createPathList();

    std::lock_guard<std::mutex> lock( mutex3D_resolver );

    std::list< SEARCH_PATH >::const_iterator sL = m_Paths.begin();
    std::list< SEARCH_PATH >::const_iterator eL = m_Paths.end();
    size_t idx;

    while( sL != eL )
    {
        // undefined paths do not participate in the file name shortening procedure.
        if( sL->m_Pathexp.empty() )
        {
            ++sL;
            continue;
        }

        wxFileName fpath( sL->m_Pathexp, "" );
        wxString fps = fpath.GetPathWithSep();
        wxString tname;

        idx = fname.find( fps );

        if( std::string::npos != idx && 0 == idx  )
        {
            fname = fname.substr( fps.size() );

#ifdef _WIN32
            // ensure only the '/' separator is used in the internal name
            fname.Replace( "\\", "/" );
#endif

            if( sL->m_Alias.StartsWith( "${" ) || sL->m_Alias.StartsWith( "$(" ) )
            {
                // old style ENV_VAR
                tname = sL->m_Alias;
                tname.Append( "/" );
                tname.append( fname );
            }
            else
            {
                // new style alias
                tname = ":";
                tname.append( sL->m_Alias );
                tname.append( ":" );
                tname.append( fname );
            }

            return tname;
        }

        ++sL;
    }

#ifdef _WIN32
    // it is strange to convert an MSWin full path to use the
    // UNIX separator but this is done for consistency and can
    // be helpful even when transferring project files from
    // MSWin to *NIX.
    fname.Replace( "\\", "/" );
#endif

    return fname;
}



const std::list< SEARCH_PATH >* S3D_RESOLVER::GetPaths( void )
{
    return &m_Paths;
}


bool S3D_RESOLVER::SplitAlias( const wxString& aFileName, wxString& anAlias, wxString& aRelPath )
{
    anAlias.clear();
    aRelPath.clear();

    if( !aFileName.StartsWith( ":" ) )
        return false;

    size_t tagpos = aFileName.find( ":", 1 );

    if( wxString::npos ==  tagpos || 1 == tagpos )
        return false;

    if( tagpos + 1 >= aFileName.length() )
        return false;

    anAlias = aFileName.substr( 1, tagpos - 1 );
    aRelPath = aFileName.substr( tagpos + 1 );

    return true;
}


static bool getHollerith( const std::string& aString, size_t& aIndex, wxString& aResult )
{
    aResult.clear();

    if( aIndex >= aString.size() )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n"
                                          " * Bad Hollerith string in line \"%s\"" ),
                    __FILE__, __FUNCTION__, __LINE__, aString );

        return false;
    }

    size_t i2 = aString.find( '"', aIndex );

    if( std::string::npos == i2 )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n"
                                          " * missing opening quote mark in line \"%s\"" ),
                    __FILE__, __FUNCTION__, __LINE__, aString );

        return false;
    }

    ++i2;

    if( i2 >= aString.size() )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n"
                                          " * unexpected end of line in line \"%s\"" ),
                    __FILE__, __FUNCTION__, __LINE__, aString );

        return false;
    }

    std::string tnum;

    while( aString[i2] >= '0' && aString[i2] <= '9' )
        tnum.append( 1, aString[i2++] );

    if( tnum.empty() || aString[i2++] != ':' )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n"
                                          " * Bad Hollerith string in line \"%s\"" ),
                    __FILE__, __FUNCTION__, __LINE__, aString );

        return false;
    }

    std::istringstream istr;
    istr.str( tnum );
    size_t nchars;
    istr >> nchars;

    if( (i2 + nchars) >= aString.size() )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n"
                                          " * unexpected end of line in line \"%s\"\n" ),
                    __FILE__, __FUNCTION__, __LINE__, aString );

        return false;
    }

    if( nchars > 0 )
    {
        aResult = wxString::FromUTF8( aString.substr( i2, nchars ).c_str() );
        i2 += nchars;
    }

    if( i2 >= aString.size() || aString[i2] != '"' )
    {
        wxLogTrace( trace3dResolver, wxT( "%s:%s:%d\n"
                                          " * missing closing quote mark in line \"%s\"" ),
                    __FILE__, __FUNCTION__, __LINE__, aString );

        return false;
    }

    aIndex = i2 + 1;
    return true;
}


bool S3D_RESOLVER::ValidateFileName( const wxString& aFileName, bool& hasAlias )
{
    // Rules:
    // 1. The generic form of an aliased 3D relative path is:
    //    ALIAS:relative/path
    // 2. ALIAS is a UTF string excluding "{}[]()%~<>\"='`;:.,&?/\\|$"
    // 3. The relative path must be a valid relative path for the platform
    hasAlias = false;

    if( aFileName.empty() )
        return false;

    wxString filename = aFileName;
    wxString lpath;
    size_t pos0 = aFileName.find( ':' );

    // ensure that the file separators suit the current platform
#ifdef __WINDOWS__
    filename.Replace( "/", "\\" );

    // if we see the :\ pattern then it must be a drive designator
    if( pos0 != wxString::npos )
    {
        size_t pos1 = aFileName.find( ":\\" );

        if( pos1 != wxString::npos && ( pos1 != pos0 || pos1 != 1 ) )
            return false;

        // if we have a drive designator then we have no alias
        if( pos1 != wxString::npos )
            pos0 = wxString::npos;
    }
#else
    filename.Replace( "\\", "/" );
#endif

    // names may not end with ':'
    if( pos0 == aFileName.length() -1 )
        return false;

    if( pos0 != wxString::npos )
    {
        // ensure the alias component is not empty
        if( pos0 == 0 )
            return false;

        lpath = filename.substr( 0, pos0 );

        // check the alias for restricted characters
        if( wxString::npos != lpath.find_first_of( "{}[]()%~<>\"='`;:.,&?/\\|$" ) )
            return false;

        hasAlias = true;
        lpath = aFileName.substr( pos0 + 1 );
    }
    else
    {
        lpath = aFileName;

        // in the case of ${ENV_VAR}|$(ENV_VAR)/path, strip the
        // environment string before testing
        pos0 = wxString::npos;

        if( aFileName.StartsWith( "${" ) )
            pos0 = aFileName.find( '}' );
        else if( aFileName.StartsWith( "$(" ) )
            pos0 = aFileName.find( ')' );

        if( pos0 != wxString::npos )
            lpath = aFileName.substr( pos0 + 1 );

    }

    if( wxString::npos != lpath.find_first_of( wxFileName::GetForbiddenChars() ) )
        return false;

    return true;
}
