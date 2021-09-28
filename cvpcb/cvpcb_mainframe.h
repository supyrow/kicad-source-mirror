/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef _CVPCB_MAINFRAME_H_
#define _CVPCB_MAINFRAME_H_

#include <config_params.h>
#include <kiway_player.h>
#include <netlist_reader/pcb_netlist.h>

#include <auto_associate.h>
#include <cvpcb_association.h>
#include <listboxes.h>

/*  Forward declarations */
class ACTION_TOOLBAR;
class ACTION_MENU;
class TOOL_DISPATCHER;
class wxAuiToolBar;
class wxStaticText;
class DISPLAY_FOOTPRINTS_FRAME;

namespace CV { struct IFACE; }

// The undo/redo list is composed of vectors of associations
typedef std::vector< CVPCB_ASSOCIATION >       CVPCB_UNDO_REDO_ENTRIES;

// The undo list is a vector of undo entries
typedef std::vector< CVPCB_UNDO_REDO_ENTRIES > CVPCB_UNDO_REDO_LIST;


/**
 * The CvPcb application main window.
 */
class CVPCB_MAINFRAME : public KIWAY_PLAYER
{
public:
    ~CVPCB_MAINFRAME();

    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl=0 ) override;

    void KiwayMailIn( KIWAY_EXPRESS& aEvent ) override;

    /**
     * The action to apply to a footprint filter when it is modified.
     */
    enum CVPCB_FILTER_ACTION
    {
        FILTER_DISABLE,     ///< Turn off the filter
        FILTER_ENABLE,      ///< Turn on the filter
        FILTER_TOGGLE       ///< Toggle the filter state
    };

    /**
     * The type of the controls present in the application.
     */
    enum CONTROL_TYPE
    {
        CONTROL_NONE,            ///< No controls have focus
        CONTROL_LIBRARY,         ///< Library listbox
        CONTROL_COMPONENT,       ///< Component listbox
        CONTROL_FOOTPRINT        ///< Footprint listbox
    };

    /**
     * Directions to rotate the focus through the listboxes is.
     */
    enum FOCUS_DIR
    {
        CHANGE_FOCUS_RIGHT,
        CHANGE_FOCUS_LEFT
    };

    /**
     * Directions to move when selecting items.
     */
    enum ITEM_DIR
    {
        ITEM_NEXT,  ///< The next item
        ITEM_PREV   ///< The previous item
    };

    /**
     * Get if the current associations have been modified but not saved.
     *
     * @return true if the any changes have not been saved
     */
    bool IsContentModified() const override
    {
        return m_modified;
    }

    /**
     * @return a pointer on the Footprint Viewer frame, if exists, or NULL
     */
    DISPLAY_FOOTPRINTS_FRAME* GetFootprintViewerFrame() const;

    wxWindow* GetToolCanvas() const override;

    /**
     * Find out which control currently has focus.
     *
     * @return the control that currently has focus
     */
    CVPCB_MAINFRAME::CONTROL_TYPE GetFocusedControl() const;

    /**
     * Get a pointer to the currently focused control
     *
     * @return the control that currently has focus
     */
    wxControl* GetFocusedControlObject() const;

    /**
     * Set the focus to a specific control.
     *
     * @param aControl the control to set focus to
     */
    void SetFocusedControl( CVPCB_MAINFRAME::CONTROL_TYPE aControl );

    /**
     * Called when clicking on a component in component list window:
     * * Updates the filtered footprint list, if the filtered list option is selected
     * * Updates the current selected footprint in footprint list
     * * Updates the footprint shown in footprint display window (if opened)
     */
    void OnSelectComponent( wxListEvent& event );

    bool canCloseWindow( wxCloseEvent& aCloseEvent ) override;
    void doCloseWindow() override;

    /*
     * Functions to rebuild the toolbars and menubars
     */
    void ReCreateHToolbar();
    void ReCreateMenuBar() override;

    void ShowChangedLanguage() override;

    /**
     * Called by the automatic association button
     * Read *.equ files to try to find corresponding footprint
     * for each component that is not already linked to a footprint ( a "free"
     * component )
     * format of a line:
     * 'cmp_ref' 'footprint_name'
     */
    void AutomaticFootprintMatching();

    /**
     * Set a filter criteria to either on/off or toggle the criteria.
     *
     * @param aFilter The filter to modify
     * @param aAction What action (on, off or toggle) to take
     */
    void SetFootprintFilter( FOOTPRINTS_LISTBOX::FP_FILTER_T aFilter,
                             CVPCB_MAINFRAME::CVPCB_FILTER_ACTION aAction );

    /**
     * Called each time the text of m_tcFilterString is changed.
     */
    void OnEnterFilteringText( wxCommandEvent& event );

    /**
     * Undo the most recent associations that were performed.
     */
    void UndoAssociation();

    /**
     * Redo the most recently undone association.
     */
    void RedoAssociation();

    int GetUndoCommandCount() const override
    {
        return m_undoList.size();
    }

    int GetRedoCommandCount() const override
    {
        return m_redoList.size();
    }

    /**
     * Associate a footprint with a specific component in the list.
     *
     * Associations can be chained into a single undo/redo step by setting aNewEntry to false
     * for every association other than the first one. This will create a new list entry for
     * the first association, and add the subsequent associations to that list.
     *
     * @param aAssociation is the association to perform
     * @param aNewEntry specifies if this association should be a new entry in the undo list
     * @param aAddUndoItem specifies if an undo item should be created for this association
     */
    void AssociateFootprint( const CVPCB_ASSOCIATION& aAssociation, bool aNewEntry = true,
                             bool aAddUndoItem = true );

    /*
     * Functions to build the listboxes and their contents
     */
    void BuildSymbolsListBox();
    void BuildFootprintsListBox();
    void BuildLibrariesListBox();

    /**
     * Save the edits that the user has done by sending them back to Eeschema via the kiway.
     *
     * Optionally saves the schematic to disk as well.
     */
    bool SaveFootprintAssociation( bool doSaveSchematic );

    /**
     * Load the netlist file built on the fly by Eeschema and loads footprint libraries from
     * fp lib tables.
     *
     * @param aNetlist is the netlist from Eeschema in KiCad s-expr format.
     * (see CVPCB_MAINFRAME::KiwayMailIn() to know how to get this netlist)
     */
    bool ReadNetListAndFpFiles( const std::string& aNetlist );

    /**
     * Read the netlist (.net) file built on the fly by Eeschema.
     *
     * @param aNetlist is the netlist buffer filled by Eeschema, in KiCad s-expr format.
     * It is the same netlist as the .net file created by Eeschema.
     * (This method is called by ReadNetListAndFpFiles)
     */
    int ReadSchematicNetlist( const std::string& aNetlist );

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;

    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    /**
     * Update the information displayed on the status bar at bottom of the main frame.
     *
     * When the library or component list controls have the focus, the footprint assignment
     * status of the components is displayed in the first status bar pane and the list of
     * filters for the selected component is displayed in the second status bar pane.  When
     * the footprint list control has the focus, the description of the selected footprint is
     * displayed in the first status bar pane and the key words for the selected footprint are
     * displayed in the second status bar pane.  The third status bar pane always displays the
     * current footprint list filtering.
     */
    void DisplayStatus();

    /**
     * Read the list of footprint (*.mod files) and generate the list of footprints.
     *
     * For each footprint the following items are stored:
     *  - the footprint name
     *  - documentation string
     *  - associated keywords
     *
     * @return true if libraries are found, false otherwise.
     */
    bool LoadFootprintFiles();

    /**
     * Send a remote command to Eeschema via a socket,
     *
     * Commands are:
     * $PART: "reference" put cursor on component anchor
     *
     * @param aClearHighligntOnly use true if the message to send is only "clear highlight"
     *                            (used when exiting CvPcb)
     */
    void SendMessageToEESCHEMA( bool aClearHighligntOnly = false );

    /**
     * Get the selected component from the component listbox.
     *
     * @return the selected component
     */
    COMPONENT* GetSelectedComponent();

    /**
     * Set the currently selected component in the components listbox
     *
     * @param aIndex the index of the component to select, -1 to clear selection
     * @param aSkipUpdate skips running the OnSelectComponent event to update the other windows
     */
    void SetSelectedComponent( int aIndex, bool aSkipUpdate = false );

    /**
     * Criteria to use to identify sets of components
     */
    enum CRITERIA
    {
        ALL_COMPONENTS,     ///< All components
        SEL_COMPONENTS,     ///< Selected components
        NA_COMPONENTS,      ///< Not associated components
        ASSOC_COMPONENTS    ///< Associated components
    };

    /**
     * Get the indices for all the components meeting the specified criteria in the components
     * listbox.
     *
     * @param aCriteria is the criteria to use for finding the indices
     * @return a vector containing all the indices
     */
    std::vector<unsigned int> GetComponentIndices(
            CVPCB_MAINFRAME::CRITERIA aCriteria = CVPCB_MAINFRAME::ALL_COMPONENTS );

    /**
     * @return the LIB_ID of the selected footprint in footprint listview
     * or a empty string if no selection
     */
    wxString GetSelectedFootprint();

    void SetStatusText( const wxString& aText, int aNumber = 0 ) override;

protected:
    CVPCB_MAINFRAME( KIWAY* aKiway, wxWindow* aParent );

    void setupUIConditions() override;

private:
    wxString formatSymbolDesc( int idx, const wxString& aReference, const wxString& aValue,
                               const wxString& aFootprint );

    /**
     * Setup the tool system for the CVPCB main frame.
     */
    void setupTools();

    /**
     * Setup event handlers
     */
    void setupEventHandlers();

    /**
     * Read the .equ files and populate the list of equivalents.
     *
     * @param aList the list to populate
     * @param aErrorMessages is a pointer to a wxString to store error messages
     *  (can be NULL)
     * @return the error count ( 0 = no error)
     */
    int buildEquivalenceList( FOOTPRINT_EQUIVALENCE_LIST& aList,
                              wxString* aErrorMessages = nullptr );

    void refreshAfterSymbolSearch( COMPONENT* aSymbol );

public:
    FOOTPRINT_LIST* m_FootprintsList;

protected:
    bool            m_modified;
    bool            m_skipComponentSelect;  // skip component selection event during
                                            // automatic selection/deletion of associations

    bool            m_initialized;          // true after creating widgets.
    bool            m_cannotClose;          // true when the cvpcb frame cannot be closed
                                            // (mainly during reading a netlist sent by Eeschema)

private:
    friend struct CV::IFACE;

    NETLIST                   m_netlist;
    int                       m_filteringOptions;
    ACTION_TOOLBAR*           m_mainToolBar;
    FOOTPRINTS_LISTBOX*       m_footprintListBox;
    LIBRARY_LISTBOX*          m_librariesListBox;
    SYMBOLS_LISTBOX*       m_symbolsListBox;
    wxTextCtrl*               m_tcFilterString;
    wxStaticText*             m_statusLine1;
    wxStaticText*             m_statusLine2;
    wxStaticText*             m_statusLine3;
    wxButton*                 m_saveAndContinue;

    // Tool dispatcher
    TOOL_DISPATCHER* m_toolDispatcher;

    // Context menus for the list boxes
    ACTION_MENU* m_footprintContextMenu;
    ACTION_MENU* m_symbolsContextMenu;

    // Undo/Redo item lists
    CVPCB_UNDO_REDO_LIST    m_undoList;
    CVPCB_UNDO_REDO_LIST    m_redoList;
};

#endif  //#ifndef _CVPCB_MAINFRAME_H_
