/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#ifndef _SCH_ALTIUM_PLUGIN_H_
#define _SCH_ALTIUM_PLUGIN_H_

#include <memory>
#include <sch_io_mgr.h>
#include <wx/filename.h>
#include <wx/gdicmn.h>

#include "altium_parser_sch.h"


class SCH_SYMBOL;
class SCH_SHEET;
class TITLE_BLOCK;

namespace CFB
{
class CompoundFileReader;
}

/**
 * SCH_ALTIUM_PLUGIN
 * is a #SCH_PLUGIN derivation for loading Altium .SchDoc schematic files.
 *
 * As with all SCH_PLUGINs there is no UI dependencies i.e. windowing calls allowed.
 */
class SCH_ALTIUM_PLUGIN : public SCH_PLUGIN
{
public:
    SCH_ALTIUM_PLUGIN();
    ~SCH_ALTIUM_PLUGIN();

    const wxString GetName() const override;

    void SetReporter( REPORTER* aReporter ) override { m_reporter = aReporter; }

    const wxString GetFileExtension() const override;

    const wxString GetLibraryFileExtension() const override;

    int GetModifyHash() const override;

    SCH_SHEET* Load( const wxString& aFileName, SCHEMATIC* aSchematic,
                     SCH_SHEET* aAppendToMe = nullptr,
                     const PROPERTIES* aProperties = nullptr ) override;

    bool CheckHeader( const wxString& aFileName ) override;

    // unimplemented functions. Will trigger a not_implemented IO error.
    //void SaveLibrary( const wxString& aFileName, const PROPERTIES* aProperties = NULL ) override;

    //void Save( const wxString& aFileName, SCH_SCREEN* aSchematic, KIWAY* aKiway,
    //           const PROPERTIES* aProperties = NULL ) override;

    //void EnumerateSymbolLib( wxArrayString& aAliasNameList, const wxString& aLibraryPath,
    //                         const PROPERTIES* aProperties = NULL ) override;

    //LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
    //                      const PROPERTIES* aProperties = NULL ) override;

    //void SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
    //                 const PROPERTIES* aProperties = NULL ) override;

    //void DeleteAlias( const wxString& aLibraryPath, const wxString& aAliasName,
    //                  const PROPERTIES* aProperties = NULL ) override;

    //void DeleteSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
    //                   const PROPERTIES* aProperties = NULL ) override;

    //void CreateSymbolLib( const wxString& aLibraryPath,
    //                      const PROPERTIES* aProperties = NULL ) override;

    // bool DeleteSymbolLib( const wxString& aLibraryPath,
    //                      const PROPERTIES* aProperties = NULL ) override;

    //bool IsSymbolLibWritable( const wxString& aLibraryPath ) override;

    //void SymbolLibOptions( PROPERTIES* aListToAppendTo ) const override;

    wxString   getLibName();
    wxFileName getLibFileName();

    void ParseAltiumSch( const wxString& aFileName );
    void ParseStorage( const CFB::CompoundFileReader& aReader );
    void ParseFileHeader( const CFB::CompoundFileReader& aReader );

private:
    bool IsComponentPartVisible( int aOwnerindex, int aOwnerpartdisplaymode ) const;
    const ASCH_STORAGE_FILE* GetFileFromStorage( const wxString& aFilename ) const;

    void ParseComponent( int aIndex, const std::map<wxString, wxString>& aProperties );
    void ParsePin( const std::map<wxString, wxString>& aProperties );
    void ParseLabel( const std::map<wxString, wxString>& aProperties );
    void ParseTextFrame( const std::map<wxString, wxString>& aProperties );
    void ParseNote( const std::map<wxString, wxString>& aProperties );
    void ParseBezier( const std::map<wxString, wxString>& aProperties );
    void ParsePolyline( const std::map<wxString, wxString>& aProperties );
    void ParsePolygon( const std::map<wxString, wxString>& aProperties );
    void ParseRoundRectangle( const std::map<wxString, wxString>& aProperties );
    void ParseArc( const std::map<wxString, wxString>& aProperties );
    void ParseLine( const std::map<wxString, wxString>& aProperties );
    void ParseRectangle( const std::map<wxString, wxString>& aProperties );
    void ParseSheetSymbol( int aIndex, const std::map<wxString, wxString>& aProperties );
    void ParseSheetEntry( const std::map<wxString, wxString>& aProperties );
    void ParsePowerPort( const std::map<wxString, wxString>& aProperties );
    void ParsePort( const ASCH_PORT& aElem );
    void ParseNoERC( const std::map<wxString, wxString>& aProperties );
    void ParseNetLabel( const std::map<wxString, wxString>& aProperties );
    void ParseBus( const std::map<wxString, wxString>& aProperties );
    void ParseWire( const std::map<wxString, wxString>& aProperties );
    void ParseJunction( const std::map<wxString, wxString>& aProperties );
    void ParseImage( const std::map<wxString, wxString>& aProperties );
    void ParseSheet( const std::map<wxString, wxString>& aProperties );
    void ParseSheetName( const std::map<wxString, wxString>& aProperties );
    void ParseFileName( const std::map<wxString, wxString>& aProperties );
    void ParseDesignator( const std::map<wxString, wxString>& aProperties );
    void ParseBusEntry( const std::map<wxString, wxString>& aProperties );
    void ParseParameter( const std::map<wxString, wxString>& aProperties );
    void ParseImplementationList( int aIndex, const std::map<wxString, wxString>& aProperties );
    void ParseImplementation( const std::map<wxString, wxString>& aProperties );

private:
    REPORTER*                       m_reporter;          // current reporter for warnings/errors

    SCH_SHEET* m_rootSheet;      // The root sheet of the schematic being loaded..
    SCH_SHEET* m_currentSheet;   // The current sheet of the schematic being loaded..
    SCHEMATIC* m_schematic;      // Passed to Load(), the schematic object being loaded
    wxString   m_libName;        // Library name to save symbols

    SCH_PLUGIN::SCH_PLUGIN_RELEASER m_pi;                // Plugin to create KiCad symbol library.
    std::unique_ptr<PROPERTIES>     m_properties;        // Library plugin properties.

    std::unique_ptr<TITLE_BLOCK>    m_currentTitleBlock; // Will be assigned at the end of parsing
                                                         // a sheet

    wxPoint                         m_sheetOffset;
    std::unique_ptr<ASCH_SHEET>     m_altiumSheet;
    std::map<int, SCH_SYMBOL*>      m_symbols;
    std::map<int, SCH_SHEET*>       m_sheets;
    std::map<int, LIB_SYMBOL*>      m_libSymbols;        // every symbol has its unique lib_symbol

    std::map<wxString, LIB_SYMBOL*> m_powerSymbols;
    std::vector<ASCH_STORAGE_FILE>  m_altiumStorage;

    std::map<int, ASCH_SYMBOL>      m_altiumComponents;
    std::map<int, int>              m_altiumImplementationList;
    std::vector<ASCH_PORT>          m_altiumPortsCurrentSheet; // we require all connections first
};

#endif // _SCH_ALTIUM_PLUGIN_H_
