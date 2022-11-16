/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2022 KiCad Developers.
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


// WARNING - this Tom's crappy PNS hack tool code. Please don't complain about its quality
// (unless you want to improve it).

#include "pns_log_file.h"

#include <router/pns_segment.h>

#include <board_design_settings.h>

#include <pcbnew/plugins/kicad/pcb_plugin.h>
#include <pcbnew/drc/drc_engine.h>

#include <../../unittests/common/console_log.h>

BOARD_CONNECTED_ITEM* PNS_LOG_FILE::ItemById( const PNS_LOG_FILE::EVENT_ENTRY& evt )
{
    BOARD_CONNECTED_ITEM* parent = nullptr;

    for( auto item : m_board->AllConnectedItems() )
    {
        if( item->m_Uuid == evt.uuid )
        {
            parent = item;
            break;
        };
    }

    return parent;
}

static const wxString readLine( FILE* f )
{
    char str[16384];
    fgets( str, sizeof( str ) - 1, f );
    return wxString( str );
}


PNS_LOG_FILE::PNS_LOG_FILE() : 
    m_mode( PNS::ROUTER_MODE::PNS_MODE_ROUTE_SINGLE )
{
    m_routerSettings.reset( new PNS::ROUTING_SETTINGS( nullptr, "" ) );
}

static std::shared_ptr<SHAPE> parseShape( SHAPE_TYPE expectedType, wxStringTokenizer& aTokens )
{
    SHAPE_TYPE type = static_cast<SHAPE_TYPE> ( wxAtoi( aTokens.GetNextToken() ) );

    if( type == SHAPE_TYPE::SH_SEGMENT )
    {
        std::shared_ptr<SHAPE_SEGMENT> sh(  new SHAPE_SEGMENT );
        VECTOR2I a,b;
        a.x = wxAtoi( aTokens.GetNextToken() );
        a.y = wxAtoi( aTokens.GetNextToken() );
        b.x = wxAtoi( aTokens.GetNextToken() );
        b.y = wxAtoi( aTokens.GetNextToken() );
        int width = wxAtoi( aTokens.GetNextToken() );
        sh->SetSeg( SEG( a, b ));
        sh->SetWidth( width );
        return sh;
    }
    else if ( type == SHAPE_TYPE::SH_CIRCLE )
    {
        
            std::shared_ptr<SHAPE_CIRCLE> sh(  new SHAPE_CIRCLE );
        VECTOR2I a;
        a.x = wxAtoi( aTokens.GetNextToken() );
        a.y = wxAtoi( aTokens.GetNextToken() );
        int radius = wxAtoi( aTokens.GetNextToken() );
        sh->SetCenter( a );
        sh->SetRadius( radius );
        return sh;
    }

    return nullptr;
}

bool parseCommonPnsProps( PNS::ITEM* aItem, const wxString& cmd, wxStringTokenizer& aTokens )
{
        if( cmd == "net" )
        {
                aItem->SetNet( wxAtoi( aTokens.GetNextToken() ) );
                return true;
        } else if ( cmd == "layers" )
            {
                int start = wxAtoi( aTokens.GetNextToken() );
                int end = wxAtoi( aTokens.GetNextToken() );
                aItem->SetLayers( LAYER_RANGE( start, end ));
                return true;
            }
    return false;

}

static PNS::SEGMENT* parsePnsSegmentFromString( PNS::SEGMENT* aSeg, wxStringTokenizer& aTokens )
{
    PNS::SEGMENT* seg = new ( PNS::SEGMENT );

    while( aTokens.CountTokens() )
    {
           wxString cmd = aTokens.GetNextToken();
            if( !parseCommonPnsProps( seg, cmd, aTokens ) )
            {
                if ( cmd == "shape" )
                {
                    auto sh = parseShape( SH_SEGMENT, aTokens );
                    
                    if(!sh)
                        return nullptr;
        
                    seg->SetShape( *static_cast<SHAPE_SEGMENT*>(sh.get()) );
                    
                }
            }
    }

    return seg;
}

static PNS::VIA* parsePnsViaFromString( PNS::VIA* aSeg, wxStringTokenizer& aTokens )
{
    PNS::VIA* via = new ( PNS::VIA );

    while( aTokens.CountTokens() )
    {
           wxString cmd = aTokens.GetNextToken();
            if( !parseCommonPnsProps( via, cmd, aTokens ) )
            {
                if ( cmd == "shape" )
                {
                    auto sh = parseShape( SH_CIRCLE, aTokens );
                    
                    if(!sh)
                        return nullptr;
        
                    auto *sc = static_cast<SHAPE_CIRCLE*>( sh.get() );

                    via->SetPos( sc->GetCenter() );
                    via->SetDiameter( 2 * sc->GetRadius() );
                }
                else if ( cmd == "drill" )
                {
                    via->SetDrill( wxAtoi( aTokens.GetNextToken() ) );
                }
            }
    }

    return via;
}


static PNS::ITEM* parseItemFromString( wxStringTokenizer& aTokens )
{
    wxString type = aTokens.GetNextToken();

    if( type == "segment" )
    {
        auto seg = new PNS::SEGMENT();
        return parsePnsSegmentFromString( seg, aTokens );
    }
    else if( type == "via" )
    {
        auto seg = new PNS::VIA();
        return parsePnsViaFromString( seg, aTokens );
    }

    return nullptr;
}

bool comparePnsItems( const PNS::ITEM*a , const PNS::ITEM* b )
{
    if( a->Kind() != b->Kind() )
        return false;
    
    if( a->Net() != b->Net() )
        return false;

    if( a->Layers() != b->Layers() )
        return false;

    if( a->Kind() == PNS::ITEM::VIA_T )
    {
        auto va = static_cast<const PNS::VIA*>(a);
        auto vb = static_cast<const PNS::VIA*>(b);

        if( va->Diameter() != vb->Diameter() )
            return false;

        if( va->Drill() != vb->Drill() )
            return false;

        if( va->Pos() != vb->Pos() )
            return false;

    }
    else if ( a->Kind() == PNS::ITEM::SEGMENT_T )
    {
        auto sa = static_cast<const PNS::SEGMENT*>(a);
        auto sb = static_cast<const PNS::SEGMENT*>(b);

        if( sa->Seg() != sb->Seg() )
            return false;

        if( sa->Width() != sb->Width() )
            return false;
    }
    
    return true;
}


bool PNS_LOG_FILE::COMMIT_STATE::Compare( const PNS_LOG_FILE::COMMIT_STATE& aOther )
{
    COMMIT_STATE check( aOther );

    //printf("pre-compare: %d/%d\n", check.m_addedItems.size(), check.m_removedIds.size() );
    //printf("pre-compare (log): %d/%d\n", m_addedItems.size(), m_removedIds.size() );

    for( auto uuid : m_removedIds )
    {
        if( check.m_removedIds.find( uuid ) != check.m_removedIds.end() )
        {
            check.m_removedIds.erase( uuid );
        }
        else
        {
            return false; // removed twice? wtf
        }
    }

    for( auto item : m_addedItems )
    {
        for( auto chk : check.m_addedItems )
        {
            if( comparePnsItems( item, chk ) )
            {
                check.m_addedItems.erase( chk );
                break;
            }
        }
    }

    //printf("post-compare: %d/%d\n", check.m_addedItems.size(), check.m_removedIds.size() );

    return check.m_addedItems.empty() && check.m_removedIds.empty();
}


bool PNS_LOG_FILE::Load( const wxFileName& logFileName, REPORTER* aRpt )
{
    wxFileName fname_log( logFileName );
    fname_log.SetExt( wxT( "log" ) );

    wxFileName fname_dump( logFileName );
    fname_dump.SetExt( wxT( "dump" ) );

    wxFileName fname_project( logFileName );
    fname_project.SetExt( wxT( "kicad_pro" ) );
    fname_project.MakeAbsolute();

    wxFileName fname_settings( logFileName );
    fname_settings.SetExt( wxT( "settings" ) );


    FILE* f = fopen( fname_log.GetFullPath().c_str(), "rb" );

    aRpt->Report( wxString::Format( "Loading log from '%s'", fname_log.GetFullPath() ) );

    if( !f )
        return false;

    while( !feof( f ) )
    {
        wxStringTokenizer tokens( readLine( f ) );

        if( !tokens.CountTokens() )
            continue;

        wxString cmd = tokens.GetNextToken();

        if( cmd == wxT("mode") )
        {
            m_mode = static_cast<PNS::ROUTER_MODE>( wxAtoi( tokens.GetNextToken() ) );
        }
        else if( cmd == wxT("event") )
        {
            EVENT_ENTRY evt;
            evt.p.x = wxAtoi( tokens.GetNextToken() );
            evt.p.y = wxAtoi( tokens.GetNextToken() );
            evt.type = (PNS::LOGGER::EVENT_TYPE) wxAtoi( tokens.GetNextToken() );
            evt.uuid = KIID( tokens.GetNextToken() );
            m_events.push_back( evt );
        }
        else if ( cmd == wxT("added") )
        {
            auto item = parseItemFromString( tokens );
            m_commitState.m_addedItems.insert( item );
        }
        else if ( cmd == wxT("removed") )
        {
            m_commitState.m_removedIds.insert( KIID( tokens.GetNextToken() ) );
        }
    }

    fclose( f );

    aRpt->Report( wxString::Format( wxT("Loading router settings from '%s'"), fname_settings.GetFullPath() ) );
    
    bool ok = m_routerSettings->LoadFromRawFile( fname_settings.GetFullPath() );

    if( !ok )
    {
        aRpt->Report( wxString::Format( wxT("Failed to load routing settings. Usign defaults.")) , RPT_SEVERITY_WARNING );
    }

    aRpt->Report( wxString::Format( wxT("Loading project settings from '%s'"), fname_settings.GetFullPath() ) );
    
    m_settingsMgr.reset( new SETTINGS_MANAGER ( true ) );
    m_settingsMgr->LoadProject( fname_project.GetFullPath() );

    try
    {
        PCB_PLUGIN io;
        aRpt->Report( wxString::Format( wxT("Loading board snapshot from '%s'"), fname_dump.GetFullPath() ) );

        m_board.reset( io.Load( fname_dump.GetFullPath(), nullptr, nullptr ) );
        m_board->SetProject( m_settingsMgr->GetProject( fname_project.GetFullPath() ) );

        std::shared_ptr<DRC_ENGINE> drcEngine( new DRC_ENGINE );

        CONSOLE_LOG            consoleLog;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        bds.m_DRCEngine = drcEngine;

        m_board->SynchronizeNetsAndNetClasses();

        drcEngine->SetBoard( m_board.get() );
        drcEngine->SetDesignSettings( &bds );
        drcEngine->SetLogReporter( new CONSOLE_MSG_REPORTER( &consoleLog ) );
        drcEngine->InitEngine( wxFileName() );
    }
    catch( const PARSE_ERROR& parse_error )
    {
        aRpt->Report( wxString::Format( "parse error : %s (%s)\n", parse_error.Problem(),
                parse_error.What() ), RPT_SEVERITY_ERROR );

        return false;
    }

    return true;
}
