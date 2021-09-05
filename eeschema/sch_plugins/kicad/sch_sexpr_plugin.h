#ifndef _SCH_SEXPR_PLUGIN_H_
#define _SCH_SEXPR_PLUGIN_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <memory>
#include <sch_io_mgr.h>
#include <sch_file_versions.h>
#include <stack>


class KIWAY;
class LINE_READER;
class SCH_SCREEN;
class SCH_SHEET;
struct SCH_SHEET_INSTANCE;
class SCH_SHEET_PATH;
class SCH_BITMAP;
class SCH_JUNCTION;
class SCH_NO_CONNECT;
class SCH_LINE;
class SCH_BUS_ENTRY_BASE;
class SCH_TEXT;
class SCH_SYMBOL;
class SCH_FIELD;
struct SYMBOL_INSTANCE_REFERENCE;
class PROPERTIES;
class EE_SELECTION;
class SCH_SEXPR_PLUGIN_CACHE;
class LIB_SYMBOL;
class SYMBOL_LIB;
class BUS_ALIAS;

/**
 * A #SCH_PLUGIN derivation for loading schematic files using the new s-expression
 * file format.
 *
 * As with all SCH_PLUGINs there is no UI dependencies i.e. windowing calls allowed.
 */
class SCH_SEXPR_PLUGIN : public SCH_PLUGIN
{
public:

    SCH_SEXPR_PLUGIN();
    virtual ~SCH_SEXPR_PLUGIN();

    const wxString GetName() const override
    {
        return wxT( "Eeschema s-expression" );
    }

    const wxString GetFileExtension() const override
    {
        return wxT( "kicad_sch" );
    }

    const wxString GetLibraryFileExtension() const override
    {
        return wxT( "kicad_sym" );
    }

    void SetProgressReporter( PROGRESS_REPORTER* aReporter ) override
    {
        m_progressReporter = aReporter;
    }

    /**
     * The property used internally by the plugin to enable cache buffering which prevents
     * the library file from being written every time the cache is changed.  This is useful
     * when writing the schematic cache library file or saving a library to a new file name.
     */
    static const char* PropBuffering;

    int GetModifyHash() const override;

    SCH_SHEET* Load( const wxString& aFileName, SCHEMATIC* aSchematic,
                     SCH_SHEET* aAppendToMe = nullptr,
                     const PROPERTIES* aProperties = nullptr ) override;

    void LoadContent( LINE_READER& aReader, SCH_SHEET* aSheet,
                      int aVersion = SEXPR_SCHEMATIC_FILE_VERSION );

    void Save( const wxString& aFileName, SCH_SHEET* aSheet, SCHEMATIC* aSchematic,
               const PROPERTIES* aProperties = nullptr ) override;

    void Format( SCH_SHEET* aSheet );

    void Format( EE_SELECTION* aSelection, SCH_SHEET_PATH* aSelectionPath,
                 SCH_SHEET_LIST* aFullSheetHierarchy, OUTPUTFORMATTER* aFormatter );

    void EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                             const wxString&   aLibraryPath,
                             const PROPERTIES* aProperties = nullptr ) override;
    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                             const wxString&           aLibraryPath,
                             const PROPERTIES*         aProperties = nullptr ) override;
    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const PROPERTIES* aProperties = nullptr ) override;
    void SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                     const PROPERTIES* aProperties = nullptr ) override;
    void DeleteSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                       const PROPERTIES* aProperties = nullptr ) override;
    void CreateSymbolLib( const wxString& aLibraryPath,
                          const PROPERTIES* aProperties = nullptr ) override;
    bool DeleteSymbolLib( const wxString& aLibraryPath,
                          const PROPERTIES* aProperties = nullptr ) override;
    void SaveLibrary( const wxString& aLibraryPath,
                      const PROPERTIES* aProperties = nullptr ) override;

    bool CheckHeader( const wxString& aFileName ) override;
    bool IsSymbolLibWritable( const wxString& aLibraryPath ) override;

    const wxString& GetError() const override { return m_error; }

    static LIB_SYMBOL* ParseLibSymbol( LINE_READER& aReader,
                                       int aVersion = SEXPR_SCHEMATIC_FILE_VERSION );
    static void FormatLibSymbol( LIB_SYMBOL* aPart, OUTPUTFORMATTER& aFormatter );

private:
    void loadHierarchy( SCH_SHEET* aSheet );
    void loadFile( const wxString& aFileName, SCH_SHEET* aSheet );

    void saveSymbol( SCH_SYMBOL* aSymbol, SCH_SHEET_PATH* aSheetPath, int aNestLevel );
    void saveField( SCH_FIELD* aField, int aNestLevel );
    void saveBitmap( SCH_BITMAP* aBitmap, int aNestLevel );
    void saveSheet( SCH_SHEET* aSheet, int aNestLevel );
    void saveJunction( SCH_JUNCTION* aJunction, int aNestLevel );
    void saveNoConnect( SCH_NO_CONNECT* aNoConnect, int aNestLevel );
    void saveBusEntry( SCH_BUS_ENTRY_BASE* aBusEntry, int aNestLevel );
    void saveLine( SCH_LINE* aLine, int aNestLevel );
    void saveText( SCH_TEXT* aText, int aNestLevel );
    void saveBusAlias( std::shared_ptr<BUS_ALIAS> aAlias, int aNestLevel );
    void saveInstances( const std::vector<SCH_SHEET_INSTANCE>&        aSheets,
                        const std::vector<SYMBOL_INSTANCE_REFERENCE>& aSymbols, int aNestLevel );

    void cacheLib( const wxString& aLibraryFileName, const PROPERTIES* aProperties );
    bool isBuffering( const PROPERTIES* aProperties );

protected:
    int                     m_version;          ///< Version of file being loaded.
    int                     m_nextFreeFieldId;

    wxString                m_error;            ///< For throwing exceptions or errors on partial
                                                ///<  loads.
    PROGRESS_REPORTER*      m_progressReporter;

    wxString                m_path;             ///< Root project path for loading child sheets.
    std::stack<wxString>    m_currentPath;      ///< Stack to maintain nested sheet paths
    SCH_SHEET*              m_rootSheet;        ///< The root sheet of the schematic being loaded.
    SCHEMATIC*              m_schematic;
    OUTPUTFORMATTER*        m_out;              ///< The formatter for saving SCH_SCREEN objects.
    SCH_SEXPR_PLUGIN_CACHE* m_cache;

    /// initialize PLUGIN like a constructor would.
    void init( SCHEMATIC* aSchematic, const PROPERTIES* aProperties = nullptr );
};

#endif  // _SCH_SEXPR_PLUGIN_H_
