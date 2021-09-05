#ifndef _SCH_IO_MGR_H_
#define _SCH_IO_MGR_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <richio.h>
#include <import_export.h>
#include <map>
#include <enum_vector.h>
#include <reporter.h>


class SCH_SHEET;
class SCH_SCREEN;
class SCH_PLUGIN;
class SCHEMATIC;
class KIWAY;
class LIB_SYMBOL;
class SYMBOL_LIB;
class PROPERTIES;
class PROGRESS_REPORTER;


/**
 * A factory which returns an instance of a #SCH_PLUGIN.
 */
class SCH_IO_MGR
{
public:

    /**
     * A set of file types that the #SCH_IO_MGR knows about, and for which there
     * has been a plugin written.
     */
    DEFINE_ENUM_VECTOR( SCH_FILE_T,
            {
                    SCH_LEGACY, ///< Legacy Eeschema file formats prior to s-expression.
                    SCH_KICAD,  ///< The s-expression version of the schematic file formats.
                    SCH_ALTIUM,  ///< Altium file format
                    SCH_CADSTAR_ARCHIVE, ///< CADSTAR Schematic Archive
                    SCH_EAGLE,  ///< Autodesk Eagle file format

                    // Add your schematic type here.
                    SCH_FILE_UNKNOWN
            } )

    /**
     * Return a #SCH_PLUGIN which the caller can use to import, export, save, or load
     * design documents.
     *
     * The returned #SCH_PLUGIN, may be reference counted, so please call PluginRelease()
     * when you are done using the returned #SCH_PLUGIN.  It may or may not be code running
     * from a DLL/DSO.
     *
     * @param aFileType is from #SCH_FILE_T and tells which plugin to find.
     *
     * @return the plugin corresponding to aFileType or NULL if not found.
     *  Caller owns the returned object, and must call PluginRelease when done using it.
     */
    APIEXPORT
    static SCH_PLUGIN* FindPlugin( SCH_FILE_T aFileType );

    /**
     * Release a #SCH_PLUGIN back to the system, and may cause it to be unloaded from memory.
     *
     * @param aPlugin is the one to be released, and which is no longer usable
     *  after calling this.
     */
    static void ReleasePlugin( SCH_PLUGIN* aPlugin );

    /**
     * Return a brief name for a plugin, given aFileType enum.
     */
    static const wxString ShowType( SCH_FILE_T aFileType );

    /**
     * Return the #SCH_FILE_T from the corresponding plugin type name: "kicad", "legacy", etc.
     */
    static SCH_FILE_T EnumFromStr( const wxString& aFileType );

    /**
     * Return the schematic file extension for \a aFileType.
     *
     * @param aFileType is the #SCH_FILE_T type.
     *
     * @return the file extension for \a aFileType or an empty string if \a aFileType is invalid.
     */
    static const wxString GetFileExtension( SCH_FILE_T aFileType );

    /**
     * Return the symbol library file extension (if any) for \a aFileType.
     *
     * @param aFileType is the #SCH_FILE_T type.
     *
     * @return the file extension for \a aFileType or an empty string if \a aFileType is invalid.
     */
    static const wxString GetLibraryFileExtension( SCH_FILE_T aFileType );

    /**
     * Return a plugin type given a symbol library using the file extension of \a aLibPath.
     */
    static SCH_FILE_T GuessPluginTypeFromLibPath( const wxString& aLibPath );

    /**
     * Return a plugin type given a schematic using the file extension of \a aSchematicPath.
     */
    static SCH_FILE_T GuessPluginTypeFromSchPath( const wxString& aSchematicPath );
};


/**
 * Base class that schematic file and library loading and saving plugins should derive from.
 * Implementations can provide either Load() or Save() functions, or both.
 * SCH_PLUGINs throw exceptions, so it is best that you wrap your calls to these
 * functions in a try catch block.  Plugins throw exceptions because it is illegal
 * for them to have any user interface calls in them whatsoever, i.e. no windowing
 * or screen printing at all.
 *
 * <pre>
 *   try
 *   {
 *        SCH_IO_MGR::Load(...);
 *   or
 *        SCH_IO_MGR::Save(...);
 *   }
 *   catch( const IO_ERROR& ioe )
 *   {
 *        // grab text from ioe, show in error window.
 *   }
 * </pre>
 */
class SCH_PLUGIN
{
public:

    //-----<PUBLIC SCH_PLUGIN API>-------------------------------------------------

    /**
     * Return a brief hard coded name for this SCH_PLUGIN.
     */
    virtual const wxString GetName() const = 0;

    /**
     * Set an optional reporter for warnings/errors.
     */
    virtual void SetReporter( REPORTER* aReporter ) {}

    /**
     * Set an optional progress reporter.
     */
    virtual void SetProgressReporter( PROGRESS_REPORTER* aReporter ) {}

    /**
     * Return the file extension for the #SCH_PLUGIN.
     */
    virtual const wxString GetFileExtension() const = 0;

    /**
     * Return the library file extension for the #SCH_PLUGIN object.
     */
    virtual const wxString GetLibraryFileExtension() const = 0;

    /**
     * Return the modification hash from the library cache.
     *
     * @note This is temporary until the new s-expr file format is implement.  The new file
     *       format will embed symbols instead of referencing them from the library.  This
     *       function can be removed when the new file format is implemented.
     *
     * @return the modification hash of the library cache.
     */
    virtual int GetModifyHash() const = 0;

    virtual void SaveLibrary( const wxString& aFileName, const PROPERTIES* aProperties = nullptr );

    /**
     * Load information from some input file format that this #SCH_PLUGIN implementation
     * knows about, into either a new #SCH_SHEET or an existing one. This may be used to load an
     * entire new #SCH_SHEET, or to augment an existing one if \a aAppendToMe is not NULL.
     *
     * @param aFileName is the name of the file to use as input and may be foreign in
     *                  nature or native in nature.
     *
     * @param aKiway is the #KIWAY object used to access the symbol libraries loaded
     *               by the project.
     *
     * @param aAppendToMe is an existing #SCH_SHEET to append to, but if NULL then this means
     *                    "do not append, rather load anew".
     *
     * @param aProperties is an associative array that can be used to tell the loader how to
     *                    load the file, because it can take any number of additional named
     *                    arguments that the plugin is known to support. These are tuning
     *                    parameters for the import or load.  The caller continues to own
     *                    this object (plugin may not delete it), and plugins should expect
     *                    it to be optionally NULL.
     *
     * @return the successfully loaded schematic, or the same one as \a aAppendToMe
     *         if \a aAppendToMe was not NULL, and the caller owns it.
     *
     * @throw IO_ERROR if there is a problem loading, and its contents should say what went
     *                 wrong, using line number and character offsets of the input file if
     *                 possible.
     */
    virtual SCH_SHEET* Load( const wxString& aFileName, SCHEMATIC* aSchematic,
                             SCH_SHEET* aAppendToMe = nullptr,
                             const PROPERTIES* aProperties = nullptr );

    /**
     * Write \a aSchematic to a storage file in a format that this #SCH_PLUGIN implementation
     * knows about, or it can be used to write a portion of \a aSchematic to a special kind
     * of export file.
     *
     * @param aFileName is the name of a file to save to on disk.
     *
     * @param aSheet     is the class #SCH_SHEET in memory document tree from which to extract
     *                   information when writing to \a aFileName.  The caller continues to
     *                   own the SCHEMATIC, and the plugin should refrain from modifying the
     *                   SCHEMATIC if possible.
     *
     * @param aSchematic is the #SCHEMATIC object used to access any schematic-wide or project
     *                   information needed to save the document.
     *
     * @param aProperties is an associative array that can be used to tell the saver how to
     *                    save the file, because it can take any number of additional named
     *                    tuning arguments that the plugin is known to support.  The caller
     *                    continues to own this object (plugin may not delete it), and plugins
     *                    should expect it to be optionally NULL.  Set the
     *                    #PropSaveCurrentSheetOnly property to only save the current sheet.
     *                    Otherwise, all hierarchical sheets are saved.
     *
     * @throw IO_ERROR if there is a problem saving or exporting.
     */
    virtual void Save( const wxString& aFileName, SCH_SHEET* aSheet, SCHEMATIC* aSchematic,
                       const PROPERTIES* aProperties = nullptr );

    /**
     * Populate a list of #LIB_SYMBOL alias names contained within the library \a aLibraryPath.
     *
     * @param aSymbolNameList is an array to populate with the #LIB_SYMBOL names associated with
     *                        the library.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing one or more #LIB_SYMBOL objects.
     *
     * @param aProperties is an associative array that can be used to tell the plugin anything
     *                    needed about how to perform with respect to \a aLibraryPath.  The
     *                    caller continues to own this object (plugin may not delete it), and
     *                    plugins should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if the library cannot be found, the part library cannot be loaded.
     */
    virtual void EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                                     const PROPERTIES* aProperties = nullptr );

    /**
     * Populate a list of #LIB_SYMBOL aliases contained within the library \a aLibraryPath.
     *
     * @note It is the responsibility of the caller to delete the returned object from the heap.
     *       Failure to do this will result in memory leaks.
     *
     * @param aSymbolList is an array to populate with the #LIB_SYMBOL pointers associated with
     *                    the library.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing one or more #LIB_SYMBOL objects.
     *
     * @param aProperties is an associative array that can be used to tell the plugin anything
     *                    needed about how to perform with respect to \a aLibraryPath.  The
     *                    caller continues to own this object (plugin may not delete it), and
     *                    plugins should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if the library cannot be found, the part library cannot be loaded.
     */
    virtual void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                     const wxString& aLibraryPath,
                                     const PROPERTIES* aProperties = nullptr );

    /**
     * Load a #LIB_SYMBOL object having \a aPartName from the \a aLibraryPath containing
     * a library format that this #SCH_PLUGIN knows about.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing several symbols.
     *
     * @param aPartName is the name of the #LIB_SYMBOL to load.
     *
     * @param aProperties is an associative array that can be used to tell the loader
     *                    implementation to do something special, because it can take
     *                    any number of additional named tuning arguments that the plugin
     *                    is known to support.  The caller continues to own this object
     *                    (plugin may not delete it), and plugins should expect it to be
     *                    optionally NULL.
     *
     * @return the part created on the heap if found caller shares it or NULL if not found.
     *
     * @throw IO_ERROR if the library cannot be found or read.  No exception
     *                 is thrown in the case where aAliasName cannot be found.
     */
    virtual LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aPartName,
                                    const PROPERTIES* aProperties = nullptr );

    /**
     * Write \a aSymbol to an existing library located at \a aLibraryPath.  If a #LIB_SYMBOL
     * by the same name already exists or there are any conflicting alias names, the new
     * #LIB_SYMBOL will silently overwrite any existing aliases and/or part because libraries
     * cannot have duplicate alias names.  It is the responsibility of the caller to check
     * the library for conflicts before saving.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing several symbols.
     *
     * @param aSymbol is what to store in the library.  The library is refreshed and the
     *                caller must update any #LIB_SYMBOL pointers that may have changed.
     *
     * @param aProperties is an associative array that can be used to tell the
     *                    saver how to save the symbol, because it can take any number of
     *                    additional named tuning arguments that the plugin is known to support.
     *                    The caller continues to own this object (plugin may not delete it), and
     *                    plugins should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if there is a problem saving.
     */
    virtual void SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                             const PROPERTIES* aProperties = nullptr );

    /**
     * Delete the entire #LIB_SYMBOL associated with \a aAliasName from the library
     * \a aLibraryPath.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing several symbols.
     *
     * @param aSymbolName is the name of a #LIB_SYMBOL associated with it's root #LIB_SYMBOL
     *                    object to delete from the specified library.
     *
     * @param aProperties is an associative array that can be used to tell the library
     *                    delete function anything special, because it can take any number
     *                    of additional named tuning arguments that the plugin is known to
     *                    support.  The caller continues to own this object (plugin may not
     *                    delete it), and plugins should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if there is a problem finding the alias or the library or deleting it.
     */
    virtual void DeleteSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                               const PROPERTIES* aProperties = nullptr );

    /**
     * Create a new empty symbol library at \a aLibraryPath.  It is an error to attempt
     * to create an existing library or to attempt to create on a "read only" location.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing several footprints.
     *
     * @param aProperties is an associative array that can be used to tell the library
     *                    create function anything special, because it can take any number
     *                    of additional named tuning arguments that the plugin is known to
     *                    support.  The caller continues to own this object (plugin may not
     *                    delete it), and plugins should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if there is a problem finding the library, or creating it.
     */
    virtual void CreateSymbolLib( const wxString& aLibraryPath,
                                  const PROPERTIES* aProperties = nullptr );

    /**
     * Delete an existing symbol library and returns true if successful, or if library
     * does not exist returns false, or throws an exception if library exists but is read
     * only or cannot be deleted for some other reason.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory or file
     *                     which will contain symbols.
     *
     * @param aProperties is an associative array that can be used to tell the library
     *                    delete implementation function anything special, because it can
     *                    take any number of additional named tuning arguments that the
     *                    plugin is known to support.  The caller continues to own this
     *                    object (plugin may not delete it), and plugins should expect
     *                    it to be optionally NULL.
     *
     * @return true if library deleted or false if library did not exist.
     *
     * @throw IO_ERROR if there is a problem deleting an existing library.
     */
    virtual bool DeleteSymbolLib( const wxString& aLibraryPath,
                                  const PROPERTIES* aProperties = nullptr );

    /**
     * Return true if the library at \a aLibraryPath is writable.  (Often
     * system libraries are read only because of where they are installed.)
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing several symbols.
     *
     * @throw IO_ERROR if no library at aLibraryPath exists.
     */
    virtual bool IsSymbolLibWritable( const wxString& aLibraryPath );

    /**
     * Append supported #SCH_PLUGIN options to \a aListToAppenTo along with internationalized
     * descriptions.  Options are typically appended so that a derived SCH_PLUGIN can call
     * its base class function by the same name first, thus inheriting options declared there.
     * (Some base class options could pertain to all Symbol*() functions in all derived
     * SCH_PLUGINs.)  Note that since aListToAppendTo is a PROPERTIES object, all options
     * will be unique and last guy wins.
     *
     * @param aListToAppendTo holds a tuple of
     * <dl>
     *   <dt>option</dt>
     *   <dd>This eventually is what shows up into the fp-lib-table "options"
     *       field, possibly combined with others.</dd>
     *   <dt>internationalized description</dt>
     *   <dd>The internationalized description is displayed in DIALOG_FP_SCH_PLUGIN_OPTIONS.
     *      It may be multi-line and be quite explanatory of the option.</dd>
     *  </dl>
     * <br>
     *  In the future perhaps \a aListToAppendTo evolves to something capable of also
     *  holding a wxValidator for the cells in said dialog:
     *  http://forums.wxwidgets.org/viewtopic.php?t=23277&p=104180.
     *   This would require a 3 column list, and introducing wx GUI knowledge to
     *   #SCH_PLUGIN, which has been avoided to date.
     */
    virtual void SymbolLibOptions( PROPERTIES* aListToAppendTo ) const;

    /**
     * Return true if the first line in @a aFileName begins with the expected header.
     *
     * @param aFileName is the name of the file to use as input
     *
     */
    virtual bool CheckHeader( const wxString& aFileName );

    /**
     * Return an error string to the caller.
     *
     * This is useful for schematic loaders that can load partial schematics where throwing
     * an exception would be problematic such as the KiCad legacy plugin.
     *
     * @return an unformatted string containing errors if any.
     */
    virtual const wxString& GetError() const;

    //-----</PUBLIC SCH_PLUGIN API>------------------------------------------------


    /*  The compiler writes the "zero argument" constructor for a SCH_PLUGIN
        automatically if you do not provide one. If you decide you need to
        provide a zero argument constructor of your own design, that is allowed.
        It must be public, and it is what the SCH_IO_MGR uses.  Parameters may be
        passed into a SCH_PLUGIN via the PROPERTIES variable for any of the public
        API functions which take one.
    */
    virtual ~SCH_PLUGIN() { }


    /**
     * Helper object to release a #SCH_PLUGIN in the context of a potential thrown exception
     * through its destructor.
     */
    class SCH_PLUGIN_RELEASER
    {
        SCH_PLUGIN* plugin;

        // private assignment operator so it's illegal
        SCH_PLUGIN_RELEASER& operator=( SCH_PLUGIN_RELEASER& aOther ) { return *this; }

        // private copy constructor so it's illegal
        SCH_PLUGIN_RELEASER( const SCH_PLUGIN_RELEASER& aOther ) {}

    public:
        SCH_PLUGIN_RELEASER( SCH_PLUGIN* aPlugin = nullptr ) :
            plugin( aPlugin )
        {
        }

        ~SCH_PLUGIN_RELEASER()
        {
            if( plugin )
                release();
        }

        void release()
        {
            SCH_IO_MGR::ReleasePlugin( plugin );
            plugin = nullptr;
        }

        void set( SCH_PLUGIN* aPlugin )
        {
            if( plugin )
                release();
            plugin = aPlugin;
        }

        operator SCH_PLUGIN* () const
        {
            return plugin;
        }

        SCH_PLUGIN* operator -> () const
        {
            return plugin;
        }
    };
};

#endif // _SCH_IO_MGR_H_
