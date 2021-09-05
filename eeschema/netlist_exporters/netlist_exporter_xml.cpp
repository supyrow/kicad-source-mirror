/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013-2017 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include "netlist_exporter_xml.h"

#include <build_version.h>
#include <common.h>     // for ExpandTextVars
#include <sch_base_frame.h>
#include <symbol_library.h>
#include <string_utils.h>
#include <connection_graph.h>
#include <string_utils.h>
#include <wx/wfstream.h>
#include <xnode.h>      // also nests: <wx/xml/xml.h>

#include <symbol_lib_table.h>

#include <set>

static bool sortPinsByNumber( LIB_PIN* aPin1, LIB_PIN* aPin2 );

bool NETLIST_EXPORTER_XML::WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions )
{
    // output the XML format netlist.

    // declare the stream ourselves to use the buffered FILE api
    // instead of letting wx use the syscall variant
    wxFFileOutputStream stream( aOutFileName );

    if( !stream.IsOk() )
        return false;

    wxXmlDocument       xdoc;
    xdoc.SetRoot( makeRoot( GNL_ALL | aNetlistOptions ) );

    return xdoc.Save( stream, 2 /* indent bug, today was ignored by wxXml lib */ );
}


XNODE* NETLIST_EXPORTER_XML::makeRoot( unsigned aCtl )
{
    XNODE*      xroot = node( "export" );

    xroot->AddAttribute( "version", "E" );

    if( aCtl & GNL_HEADER )
        // add the "design" header
        xroot->AddChild( makeDesignHeader() );

    if( aCtl & GNL_SYMBOLS )
        xroot->AddChild( makeSymbols( aCtl ) );

    if( aCtl & GNL_PARTS )
        xroot->AddChild( makeLibParts() );

    if( aCtl & GNL_LIBRARIES )
        // must follow makeGenericLibParts()
        xroot->AddChild( makeLibraries() );

    if( aCtl & GNL_NETS )
        xroot->AddChild( makeListOfNets( aCtl ) );

    return xroot;
}


/// Holder for multi-unit symbol fields


void NETLIST_EXPORTER_XML::addSymbolFields( XNODE* aNode, SCH_SYMBOL* aSymbol,
                                            SCH_SHEET_PATH* aSheet )
{
    wxString                     value;
    wxString                     datasheet;
    wxString                     footprint;
    std::map<wxString, wxString> userFields;

    if( aSymbol->GetUnitCount() > 1 )
    {
        // Sadly, each unit of a symbol can have its own unique fields. This
        // block finds the unit with the lowest number having a non blank field
        // value and records it.  Therefore user is best off setting fields
        // into only the first unit.  But this scavenger algorithm will find
        // any non blank fields in all units and use the first non-blank field
        // for each unique field name.

        wxString ref = aSymbol->GetRef( aSheet );

        SCH_SHEET_LIST sheetList = m_schematic->GetSheets();
        int minUnit = aSymbol->GetUnit();

        for( unsigned i = 0;  i < sheetList.size();  i++ )
        {
            for( auto item : sheetList[i].LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol2 = (SCH_SYMBOL*) item;

                wxString ref2 = symbol2->GetRef( &sheetList[i] );

                if( ref2.CmpNoCase( ref ) != 0 )
                    continue;

                int unit = symbol2->GetUnit();

                // The lowest unit number wins.  User should only set fields in any one unit.
                // remark: IsVoid() returns true for empty strings or the "~" string (empty
                // field value)
                if( !symbol2->GetValue( &sheetList[i], m_resolveTextVars ).IsEmpty()
                        && ( unit < minUnit || value.IsEmpty() ) )
                {
                    value = symbol2->GetValue( &sheetList[i], m_resolveTextVars );
                }

                if( !symbol2->GetFootprint( &sheetList[i], m_resolveTextVars ).IsEmpty()
                        && ( unit < minUnit || footprint.IsEmpty() ) )
                {
                    footprint = symbol2->GetFootprint( &sheetList[i], m_resolveTextVars );
                }

                if( !symbol2->GetField( DATASHEET_FIELD )->IsVoid()
                        && ( unit < minUnit || datasheet.IsEmpty() ) )
                {
                    if( m_resolveTextVars )
                        datasheet = symbol2->GetField( DATASHEET_FIELD )->GetShownText();
                    else
                        datasheet = symbol2->GetField( DATASHEET_FIELD )->GetText();
                }

                for( int ii = MANDATORY_FIELDS; ii < symbol2->GetFieldCount(); ++ii )
                {
                    const SCH_FIELD& f = symbol2->GetFields()[ ii ];

                    if( f.GetText().size()
                        && ( unit < minUnit || userFields.count( f.GetName() ) == 0 ) )
                    {
                        if( m_resolveTextVars )
                            userFields[ f.GetName() ] = f.GetShownText();
                        else
                            userFields[ f.GetName() ] = f.GetText();
                    }
                }

                minUnit = std::min( unit, minUnit );
            }
        }
    }
    else
    {
        value = aSymbol->GetValue( aSheet, m_resolveTextVars );
        footprint = aSymbol->GetFootprint( aSheet, m_resolveTextVars );

        if( m_resolveTextVars )
            datasheet = aSymbol->GetField( DATASHEET_FIELD )->GetShownText();
        else
            datasheet = aSymbol->GetField( DATASHEET_FIELD )->GetText();

        for( int ii = MANDATORY_FIELDS; ii < aSymbol->GetFieldCount(); ++ii )
        {
            const SCH_FIELD& f = aSymbol->GetFields()[ ii ];

            if( f.GetText().size() )
            {
                if( m_resolveTextVars )
                    userFields[ f.GetName() ] = f.GetShownText();
                else
                    userFields[ f.GetName() ] = f.GetText();
            }
        }
    }

    // Do not output field values blank in netlist:
    if( value.size() )
        aNode->AddChild( node( "value", UnescapeString( value ) ) );
    else    // value field always written in netlist
        aNode->AddChild( node( "value", "~" ) );

    if( footprint.size() )
        aNode->AddChild( node( "footprint", UnescapeString( footprint ) ) );

    if( datasheet.size() )
        aNode->AddChild( node( "datasheet", UnescapeString( datasheet ) ) );

    if( userFields.size() )
    {
        XNODE* xfields;
        aNode->AddChild( xfields = node( "fields" ) );

        // non MANDATORY fields are output alphabetically
        for( const std::pair<const wxString, wxString>& f : userFields )
        {
            XNODE* xfield = node( "field", UnescapeString( f.second ) );
            xfield->AddAttribute( "name", UnescapeString( f.first ) );
            xfields->AddChild( xfield );
        }
    }
}


XNODE* NETLIST_EXPORTER_XML::makeSymbols( unsigned aCtl )
{
    XNODE* xcomps = node( "components" );

    m_referencesAlreadyFound.Clear();
    m_libParts.clear();

    SCH_SHEET_LIST sheetList = m_schematic->GetSheets();

    // Output is xml, so there is no reason to remove spaces from the field values.
    // And XML element names need not be translated to various languages.

    for( unsigned ii = 0; ii < sheetList.size(); ii++ )
    {
        SCH_SHEET_PATH sheet = sheetList[ii];
        m_schematic->SetCurrentSheet( sheet );

        auto cmp = [sheet]( SCH_SYMBOL* a, SCH_SYMBOL* b )
                   {
                       return ( StrNumCmp( a->GetRef( &sheet, false ),
                                           b->GetRef( &sheet, false ), true ) < 0 );
                   };

        std::set<SCH_SYMBOL*, decltype( cmp )> ordered_symbols( cmp );
        std::multiset<SCH_SYMBOL*, decltype( cmp )> extra_units( cmp );

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            auto        test = ordered_symbols.insert( symbol );

            if( !test.second )
            {
                if( ( *( test.first ) )->m_Uuid > symbol->m_Uuid )
                {
                    extra_units.insert( *( test.first ) );
                    ordered_symbols.erase( test.first );
                    ordered_symbols.insert( symbol );
                }
                else
                {
                    extra_units.insert( symbol );
                }
            }
        }

        for( EDA_ITEM* item : ordered_symbols )
        {
            SCH_SYMBOL* symbol = findNextSymbol( item, &sheet );

            if( !symbol
               || ( ( aCtl & GNL_OPT_BOM ) && !symbol->GetIncludeInBom() )
               || ( ( aCtl & GNL_OPT_KICAD ) && !symbol->GetIncludeOnBoard() ) )
            {
                continue;
            }

            // Output the symbol's elements in order of expected access frequency. This may
            // not always look best, but it will allow faster execution under XSL processing
            // systems which do sequential searching within an element.

            XNODE* xcomp;  // current symbol being constructed
            xcomps->AddChild( xcomp = node( "comp" ) );

            xcomp->AddAttribute( "ref", symbol->GetRef( &sheet ) );
            addSymbolFields( xcomp, symbol, &sheetList[ ii ] );

            XNODE*  xlibsource;
            xcomp->AddChild( xlibsource = node( "libsource" ) );

            // "logical" library name, which is in anticipation of a better search algorithm
            // for parts based on "logical_lib.part" and where logical_lib is merely the library
            // name minus path and extension.
            wxString libName;
            wxString partName;

            if( symbol->UseLibIdLookup() )
            {
                libName = symbol->GetLibId().GetLibNickname();
                partName = symbol->GetLibId().GetLibItemName();
            }
            else
            {
                partName = symbol->GetSchSymbolLibraryName();
            }

            xlibsource->AddAttribute( "lib", libName );

            // We only want the symbol name, not the full LIB_ID.
            xlibsource->AddAttribute( "part", partName );

            xlibsource->AddAttribute( "description", symbol->GetDescription() );

            XNODE* xproperty;

            std::vector<SCH_FIELD>& fields = symbol->GetFields();

            for( size_t jj = MANDATORY_FIELDS; jj < fields.size(); ++jj )
            {
                xcomp->AddChild( xproperty = node( "property" ) );
                xproperty->AddAttribute( "name", fields[jj].GetCanonicalName() );
                xproperty->AddAttribute( "value", fields[jj].GetText() );
            }

            for( const SCH_FIELD& sheetField : sheet.Last()->GetFields() )
            {
                xcomp->AddChild( xproperty = node( "property" ) );
                xproperty->AddAttribute( "name", sheetField.GetCanonicalName() );
                xproperty->AddAttribute( "value", sheetField.GetText() );
            }

            if( !symbol->GetIncludeInBom() )
            {
                xcomp->AddChild( xproperty = node( "property" ) );
                xproperty->AddAttribute( "name", "exclude_from_bom" );
            }

            if( !symbol->GetIncludeOnBoard() )
            {
                xcomp->AddChild( xproperty = node( "property" ) );
                xproperty->AddAttribute( "name", "exclude_from_board" );
            }

            XNODE* xsheetpath;
            xcomp->AddChild( xsheetpath = node( "sheetpath" ) );

            xsheetpath->AddAttribute( "names", sheet.PathHumanReadable() );
            xsheetpath->AddAttribute( "tstamps", sheet.PathAsString() );

            XNODE* xunits; // Node for extra units
            xcomp->AddChild( xunits = node( "tstamps" ) );

            auto range = extra_units.equal_range( symbol );

            // Output a series of children with all UUIDs associated with the REFDES
            for( auto it = range.first; it != range.second; ++it )
            {
                wxString uuid = ( *it )->m_Uuid.AsString();

                // Add a space between UUIDs, if not in KICAD mode (i.e.
                // using wxXmlDocument::Save()).  KICAD MODE has its own XNODE::Format function.
                if( !( aCtl & GNL_OPT_KICAD ) )     // i.e. for .xml format
                    uuid += ' ';

                xunits->AddChild( new XNODE( wxXML_TEXT_NODE, wxEmptyString, uuid ) );
            }

            // Output the primary UUID
            xunits->AddChild(
                    new XNODE( wxXML_TEXT_NODE, wxEmptyString, symbol->m_Uuid.AsString() ) );
        }
    }

    return xcomps;
}


XNODE* NETLIST_EXPORTER_XML::makeDesignHeader()
{
    SCH_SCREEN* screen;
    XNODE*      xdesign = node( "design" );
    XNODE*      xtitleBlock;
    XNODE*      xsheet;
    XNODE*      xcomment;
    XNODE*      xtextvar;
    wxString    sheetTxt;
    wxFileName  sourceFileName;

    // the root sheet is a special sheet, call it source
    xdesign->AddChild( node( "source", m_schematic->GetFileName() ) );

    xdesign->AddChild( node( "date", DateAndTime() ) );

    // which Eeschema tool
    xdesign->AddChild( node( "tool", wxString( "Eeschema " ) + GetBuildVersion() ) );

    const std::map<wxString, wxString>& properties = m_schematic->Prj().GetTextVars();

    for( const std::pair<const wxString, wxString>& prop : properties )
    {
        xdesign->AddChild( xtextvar = node( "textvar", prop.second ) );
        xtextvar->AddAttribute( "name", prop.first );
    }

    /*
     *  Export the sheets information
     */
    SCH_SHEET_LIST sheetList = m_schematic->GetSheets();

    for( unsigned i = 0;  i < sheetList.size();  i++ )
    {
        screen = sheetList[i].LastScreen();

        xdesign->AddChild( xsheet = node( "sheet" ) );

        // get the string representation of the sheet index number.
        // Note that sheet->GetIndex() is zero index base and we need to increment the
        // number by one to make it human readable
        sheetTxt.Printf( "%u", i + 1 );
        xsheet->AddAttribute( "number", sheetTxt );
        xsheet->AddAttribute( "name", sheetList[i].PathHumanReadable() );
        xsheet->AddAttribute( "tstamps", sheetList[i].PathAsString() );

        TITLE_BLOCK tb = screen->GetTitleBlock();
        PROJECT*    prj = &m_schematic->Prj();

        xsheet->AddChild( xtitleBlock = node( "title_block" ) );

        xtitleBlock->AddChild( node( "title", ExpandTextVars( tb.GetTitle(), prj ) ) );
        xtitleBlock->AddChild( node( "company", ExpandTextVars( tb.GetCompany(), prj ) ) );
        xtitleBlock->AddChild( node( "rev", ExpandTextVars( tb.GetRevision(), prj ) ) );
        xtitleBlock->AddChild( node( "date", ExpandTextVars( tb.GetDate(), prj ) ) );

        // We are going to remove the fileName directories.
        sourceFileName = wxFileName( screen->GetFileName() );
        xtitleBlock->AddChild( node( "source", sourceFileName.GetFullName() ) );

        xtitleBlock->AddChild( xcomment = node( "comment" ) );
        xcomment->AddAttribute( "number", "1" );
        xcomment->AddAttribute( "value", ExpandTextVars( tb.GetComment( 0 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( "comment" ) );
        xcomment->AddAttribute( "number", "2" );
        xcomment->AddAttribute( "value", ExpandTextVars( tb.GetComment( 1 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( "comment" ) );
        xcomment->AddAttribute( "number", "3" );
        xcomment->AddAttribute( "value", ExpandTextVars( tb.GetComment( 2 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( "comment" ) );
        xcomment->AddAttribute( "number", "4" );
        xcomment->AddAttribute( "value", ExpandTextVars( tb.GetComment( 3 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( "comment" ) );
        xcomment->AddAttribute( "number", "5" );
        xcomment->AddAttribute( "value", ExpandTextVars( tb.GetComment( 4 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( "comment" ) );
        xcomment->AddAttribute( "number", "6" );
        xcomment->AddAttribute( "value", ExpandTextVars( tb.GetComment( 5 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( "comment" ) );
        xcomment->AddAttribute( "number", "7" );
        xcomment->AddAttribute( "value", ExpandTextVars( tb.GetComment( 6 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( "comment" ) );
        xcomment->AddAttribute( "number", "8" );
        xcomment->AddAttribute( "value", ExpandTextVars( tb.GetComment( 7 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( "comment" ) );
        xcomment->AddAttribute( "number", "9" );
        xcomment->AddAttribute( "value", ExpandTextVars( tb.GetComment( 8 ), prj ) );
    }

    return xdesign;
}


XNODE* NETLIST_EXPORTER_XML::makeLibraries()
{
    XNODE*            xlibs = node( "libraries" );     // auto_ptr
    SYMBOL_LIB_TABLE* symbolLibTable = m_schematic->Prj().SchSymbolLibTable();

    for( std::set<wxString>::iterator it = m_libraries.begin(); it!=m_libraries.end();  ++it )
    {
        wxString    libNickname = *it;
        XNODE*      xlibrary;

        if( symbolLibTable->HasLibrary( libNickname ) )
        {
            xlibs->AddChild( xlibrary = node( "library" ) );
            xlibrary->AddAttribute( "logical", libNickname );
            xlibrary->AddChild( node( "uri", symbolLibTable->GetFullURI( libNickname ) ) );
        }

        // @todo: add more fun stuff here
    }

    return xlibs;
}


XNODE* NETLIST_EXPORTER_XML::makeLibParts()
{
    XNODE*                  xlibparts = node( "libparts" );   // auto_ptr

    LIB_PINS                pinList;
    std::vector<LIB_FIELD*> fieldList;

    m_libraries.clear();

    for( auto lcomp : m_libParts )
    {
        wxString libNickname = lcomp->GetLibId().GetLibNickname();;

        // The library nickname will be empty if the cache library is used.
        if( !libNickname.IsEmpty() )
            m_libraries.insert( libNickname );  // inserts symbol's library if unique

        XNODE* xlibpart;
        xlibparts->AddChild( xlibpart = node( "libpart" ) );
        xlibpart->AddAttribute( "lib", libNickname );
        xlibpart->AddAttribute( "part", lcomp->GetName()  );

        //----- show the important properties -------------------------
        if( !lcomp->GetDescription().IsEmpty() )
            xlibpart->AddChild( node( "description", lcomp->GetDescription() ) );

        if( !lcomp->GetDatasheetField().GetText().IsEmpty() )
            xlibpart->AddChild( node( "docs",  lcomp->GetDatasheetField().GetText() ) );

        // Write the footprint list
        if( lcomp->GetFPFilters().GetCount() )
        {
            XNODE*  xfootprints;
            xlibpart->AddChild( xfootprints = node( "footprints" ) );

            for( unsigned i = 0; i < lcomp->GetFPFilters().GetCount(); ++i )
                xfootprints->AddChild( node( "fp", lcomp->GetFPFilters()[i] ) );
        }

        //----- show the fields here ----------------------------------
        fieldList.clear();
        lcomp->GetFields( fieldList );

        XNODE*     xfields;
        xlibpart->AddChild( xfields = node( "fields" ) );

        for( const LIB_FIELD* field : fieldList )
        {
            if( !field->GetText().IsEmpty() )
            {
                XNODE*     xfield;
                xfields->AddChild( xfield = node( "field", field->GetText() ) );
                xfield->AddAttribute( "name", field->GetCanonicalName() );
            }
        }

        //----- show the pins here ------------------------------------
        pinList.clear();
        lcomp->GetPins( pinList, 0, 0 );

        /* we must erase redundant Pins references in pinList
         * These redundant pins exist because some pins
         * are found more than one time when a symbol has
         * multiple parts per package or has 2 representations (DeMorgan conversion)
         * For instance, a 74ls00 has DeMorgan conversion, with different pin shapes,
         * and therefore each pin  appears 2 times in the list.
         * Common pins (VCC, GND) can also be found more than once.
         */
        sort( pinList.begin(), pinList.end(), sortPinsByNumber );
        for( int ii = 0; ii < (int)pinList.size()-1; ii++ )
        {
            if( pinList[ii]->GetNumber() == pinList[ii+1]->GetNumber() )
            {   // 2 pins have the same number, remove the redundant pin at index i+1
                pinList.erase(pinList.begin() + ii + 1);
                ii--;
            }
        }

        if( pinList.size() )
        {
            XNODE*     pins;

            xlibpart->AddChild( pins = node( "pins" ) );
            for( unsigned i=0; i<pinList.size();  ++i )
            {
                XNODE*     pin;

                pins->AddChild( pin = node( "pin" ) );
                pin->AddAttribute( "num", pinList[i]->GetShownNumber() );
                pin->AddAttribute( "name", pinList[i]->GetShownName() );
                pin->AddAttribute( "type", pinList[i]->GetCanonicalElectricalTypeName() );

                // caution: construction work site here, drive slowly
            }
        }
    }

    return xlibparts;
}


XNODE* NETLIST_EXPORTER_XML::makeListOfNets( unsigned aCtl )
{
    XNODE*      xnets = node( "nets" );      // auto_ptr if exceptions ever get used.
    wxString    netCodeTxt;
    wxString    netName;
    wxString    ref;

    XNODE*      xnet = nullptr;

    /*  output:
        <net code="123" name="/cfcard.sch/WAIT#">
            <node ref="R23" pin="1"/>
            <node ref="U18" pin="12"/>
        </net>
    */

    struct NET_NODE
    {
        NET_NODE( SCH_PIN* aPin, const SCH_SHEET_PATH& aSheet, bool aNoConnect ) :
                m_Pin( aPin ),
                m_Sheet( aSheet ),
                m_NoConnect( aNoConnect )
        {}

        SCH_PIN*       m_Pin;
        SCH_SHEET_PATH m_Sheet;
        bool           m_NoConnect;
    };

    struct NET_RECORD
    {
        NET_RECORD( const wxString& aName ) :
            m_Name( aName )
        {};

        wxString              m_Name;
        std::vector<NET_NODE> m_Nodes;
    };

    std::vector<NET_RECORD*> nets;

    for( const auto& it : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        wxString    net_name  = it.first.first;
        auto        subgraphs = it.second;
        NET_RECORD* net_record;

        if( subgraphs.empty() )
            continue;

        nets.emplace_back( new NET_RECORD( net_name ) );
        net_record = nets.back();

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            bool nc = subgraph->m_no_connect && subgraph->m_no_connect->Type() == SCH_NO_CONNECT_T;
            const SCH_SHEET_PATH& sheet = subgraph->m_sheet;

            for( SCH_ITEM* item : subgraph->m_items )
            {
                if( item->Type() == SCH_PIN_T )
                {
                    SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                    SCH_SYMBOL* symbol = pin->GetParentSymbol();

                    if( !symbol
                       || ( ( aCtl & GNL_OPT_BOM ) && !symbol->GetIncludeInBom() )
                       || ( ( aCtl & GNL_OPT_KICAD ) && !symbol->GetIncludeOnBoard() ) )
                    {
                        continue;
                    }

                    net_record->m_Nodes.emplace_back( pin, sheet, nc );
                }
            }
        }
    }

    // Netlist ordering: Net name, then ref des, then pin name
    std::sort( nets.begin(), nets.end(),
               []( const NET_RECORD* a, const NET_RECORD*b )
               {
                   return StrNumCmp( a->m_Name, b->m_Name ) < 0;
               } );

    for( int i = 0; i < (int) nets.size(); ++i )
    {
        NET_RECORD* net_record = nets[i];
        bool        added = false;
        XNODE*      xnode;

        // Netlist ordering: Net name, then ref des, then pin name
        std::sort( net_record->m_Nodes.begin(), net_record->m_Nodes.end(),
                   []( const NET_NODE& a, const NET_NODE& b )
                   {
                       wxString refA = a.m_Pin->GetParentSymbol()->GetRef( &a.m_Sheet );
                       wxString refB = b.m_Pin->GetParentSymbol()->GetRef( &b.m_Sheet );

                       if( refA == refB )
                           return a.m_Pin->GetShownNumber() < b.m_Pin->GetShownNumber();

                       return refA < refB;
                   } );

        // Some duplicates can exist, for example on multi-unit parts with duplicated
        // pins across units.  If the user connects the pins on each unit, they will
        // appear on separate subgraphs.  Remove those here:
        net_record->m_Nodes.erase(
                std::unique( net_record->m_Nodes.begin(), net_record->m_Nodes.end(),
                        []( const NET_NODE& a, const NET_NODE& b )
                        {
                            wxString refA = a.m_Pin->GetParentSymbol()->GetRef( &a.m_Sheet );
                            wxString refB = b.m_Pin->GetParentSymbol()->GetRef( &b.m_Sheet );

                            return refA == refB
                                        && a.m_Pin->GetShownNumber() == b.m_Pin->GetShownNumber();
                        } ),
                net_record->m_Nodes.end() );

        for( const NET_NODE& netNode : net_record->m_Nodes )
        {
            wxString refText = netNode.m_Pin->GetParentSymbol()->GetRef( &netNode.m_Sheet );
            wxString pinText = netNode.m_Pin->GetShownNumber();

            // Skip power symbols and virtual symbols
            if( refText[0] == wxChar( '#' ) )
                continue;

            if( !added )
            {
                netCodeTxt.Printf( "%d", i + 1 );

                xnets->AddChild( xnet = node( "net" ) );
                xnet->AddAttribute( "code", netCodeTxt );
                xnet->AddAttribute( "name", net_record->m_Name );

                added = true;
            }

            xnet->AddChild( xnode = node( "node" ) );
            xnode->AddAttribute( "ref", refText );
            xnode->AddAttribute( "pin", pinText );

            wxString pinName = netNode.m_Pin->GetShownName();
            wxString pinType = netNode.m_Pin->GetCanonicalElectricalTypeName();

            if( !pinName.IsEmpty() )
                xnode->AddAttribute( "pinfunction", pinName );

            if( netNode.m_NoConnect )
                pinType += "+no_connect";

            xnode->AddAttribute( "pintype", pinType );
        }
    }

    for( NET_RECORD* record : nets )
        delete record;

    return xnets;
}


XNODE* NETLIST_EXPORTER_XML::node( const wxString& aName,
                                   const wxString& aTextualContent /* = wxEmptyString*/ )
{
    XNODE* n = new XNODE( wxXML_ELEMENT_NODE, aName );

    if( aTextualContent.Len() > 0 )     // excludes wxEmptyString, the parameter's default value
        n->AddChild( new XNODE( wxXML_TEXT_NODE, wxEmptyString, aTextualContent ) );

    return n;
}


static bool sortPinsByNumber( LIB_PIN* aPin1, LIB_PIN* aPin2 )
{
    // return "lhs < rhs"
    return StrNumCmp( aPin1->GetShownNumber(), aPin2->GetShownNumber(), true ) < 0;
}
