/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#ifndef KICAD_SCH_SELECTION_TOOL_H
#define KICAD_SCH_SELECTION_TOOL_H

#include <tool/selection_tool.h>
#include <tool/action_menu.h>
#include <tool/tool_menu.h>
#include <tools/ee_selection.h>
#include <ee_collectors.h>
#include <sch_symbol.h>
#include <gal/cursors.h>

class SCH_BASE_FRAME;
class SCH_ITEM;

namespace KIGFX
{
    class GAL;
}


class EE_CONDITIONS : public SELECTION_CONDITIONS
{
public:
    static SELECTION_CONDITION SingleSymbol;
    static SELECTION_CONDITION SingleSymbolOrPower;
    static SELECTION_CONDITION SingleDeMorganSymbol;
    static SELECTION_CONDITION SingleMultiUnitSymbol;
};


class EE_SELECTION_TOOL : public SELECTION_TOOL, public TOOL_INTERACTIVE
{
public:
    EE_SELECTION_TOOL();
    ~EE_SELECTION_TOOL();

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int UpdateMenu( const TOOL_EVENT& aEvent );

    /**
     * The main loop.
     */
    int Main( const TOOL_EVENT& aEvent );

    void OnIdle( wxIdleEvent& aEvent );

    /**
     * Return the set of currently selected items.
     */
    EE_SELECTION& GetSelection();

    /**
     * Return either an existing selection (filtered), or the selection at the current
     * cursor if the existing selection is empty.
     */
    EE_SELECTION& RequestSelection( const KICAD_T* aFilterList = EE_COLLECTOR::AllItems );

    /**
     * This overload of SelectPoint will create an EE_COLLECTOR and collect hits at location aWhere
     * before calling the primary SelectPoint method.
     *
     * @param aWhere is the location where the item(s) should be collected
     * @param aItem is set to the newly selected item if only one was selected, otherwise is
     *              unchanged.
     * @param aSelectionCancelledFlag allows the function to inform its caller that a selection
     *                                was canceled (for instance, by clicking outside of the
     *                                disambiguation menu).
     * @param aCheckLocked indicates if locked items should be excluded.
     * @param aAdd indicates if found item(s) should be added to the selection
     * @param aSubtract indicates if found item(s) should be subtracted from the selection
     * @param aExclusiveOr indicates if found item(s) should be toggle in the selection
     */
    bool SelectPoint( const VECTOR2I& aWhere, const KICAD_T* aFilterList = EE_COLLECTOR::AllItems,
                      EDA_ITEM** aItem = nullptr, bool* aSelectionCancelledFlag = nullptr,
                      bool aCheckLocked = false, bool aAdd = false, bool aSubtract = false,
                      bool aExclusiveOr = false );

    int AddItemToSel( const TOOL_EVENT& aEvent );
    void AddItemToSel( EDA_ITEM* aItem, bool aQuietMode = false );
    int AddItemsToSel( const TOOL_EVENT& aEvent );
    void AddItemsToSel( EDA_ITEMS* aList, bool aQuietMode = false );

    int RemoveItemFromSel( const TOOL_EVENT& aEvent );
    void RemoveItemFromSel( EDA_ITEM* aItem, bool aQuietMode = false );
    int RemoveItemsFromSel( const TOOL_EVENT& aEvent );
    void RemoveItemsFromSel( EDA_ITEMS* aList, bool aQuietMode = false );

    /**
     * A safer version of RemoveItemsFromSel( EDA_ITEMS ) which doesn't require the items to
     * still exist.
     */
    void RemoveItemsFromSel( std::vector<KIID>* aList, bool aQuietMode = false );

    void BrightenItem( EDA_ITEM* aItem );
    void UnbrightenItem( EDA_ITEM* aItem );

    void SelectHighlightItem( EDA_ITEM* aItem ) { highlight( aItem, SELECTED ); }

    ///< Find (but don't select) node under cursor
    EDA_ITEM* GetNode( VECTOR2I aPosition );

    ///< Select node under cursor
    int SelectNode( const TOOL_EVENT& aEvent );

    ///< If node selected then expand to connection, otherwise select connection under cursor
    int SelectConnection( const TOOL_EVENT& aEvent );

    ///< Clear current selection event handler.
    int ClearSelection( const TOOL_EVENT& aEvent );

    ///< Select all visible items in sheet
    int SelectAll( const TOOL_EVENT& aEvent );

    void ClearSelection();

    /**
     * Check conditions for an item to be selected.
     *
     * @return True if the item fulfills conditions to be selected.
     */
    bool Selectable( const EDA_ITEM* aItem, const VECTOR2I* aPos = nullptr,
                     bool checkVisibilityOnly = false ) const;

    /**
     * Apply heuristics to try and determine a single object when multiple are found under the
     * cursor.
     */
    void GuessSelectionCandidates( EE_COLLECTOR& collector, const VECTOR2I& aPos );

    /**
     * Show a popup menu to trim the COLLECTOR passed as aEvent's parameter down to a single
     * item.
     *
     * @note This routine **does not** modify the selection.
     */
    int SelectionMenu( const TOOL_EVENT& aEvent );

    /**
     * Rebuild the selection from the EDA_ITEMs' selection flags.
     *
     * Commonly called after rolling back an undo state to make sure there aren't any stale
     * pointers.
     */
    void RebuildSelection();

    /**
     * Select one or more items at the location given by parameter aWhere.
     *
     * This method does not attempt to disambiguate multiple items and is simply "collecting"
     *
     * @param aCollector is the collector object that will store found item(s)
     * @param aWhere is the place where the item should be selected.
     * @param aFilterList is a list of items that are acceptable for collection
     * @param aCheckLocked indicates if locked items should be excluded.
     */
    bool CollectHits( EE_COLLECTOR& aCollector, const VECTOR2I& aWhere,
                      const KICAD_T* aFilterList = EE_COLLECTOR::AllItems );

private:
    /**
     * Apply rules to narrow the collection down to selectable objects, and then heuristics
     * to try and narrow it to a single object.
     */
    void narrowSelection( EE_COLLECTOR& collector, const VECTOR2I& aWhere, bool aCheckLocked );

    /**
     * This is the primary SelectPoint method that will prompt the user with a menu to disambiguate
     * multiple selections and then finish by adding, subtracting or toggling the item(s) to the
     * actual selection group.
     *
     * @param aCollector is an EE_COLLECTOR that already has collected items
     * @param aItem is set to the newly selected item if only one was selected, otherwise is
     *              unchanged.
     * @param aSelectionCancelledFlag allows the function to inform its caller that a selection
     *                                was canceled (for instance, by clicking outside of the
     *                                disambiguation menu).
     * @param aAdd indicates if found item(s) should be added to the selection
     * @param aSubtract indicates if found item(s) should be subtracted from the selection
     * @param aExclusiveOr indicates if found item(s) should be toggle in the selection
     */
    bool selectPoint( EE_COLLECTOR& aCollector, EDA_ITEM** aItem = nullptr,
                      bool* aSelectionCancelledFlag = nullptr, bool aAdd = false,
                      bool aSubtract = false, bool aExclusiveOr = false );

    /**
     * Handle drawing a selection box that allows one to select many items at the same time.
     *
     * @return true if the function was canceled (i.e. CancelEvent was received).
     */
    bool selectMultiple();

    /**
     * Allow the selection of a single item from a list via pop-up menu.  The items are
     * highlighted on the canvas when hovered in the menu.  The collector is trimmed to
     * the picked item.
     *
     * @return true if an item was picked
     */
    bool doSelectionMenu( EE_COLLECTOR* aItems );

    /**
     * Start the process to show our disambiguation menu once the user has kept
     * the mouse down for the minimum time
     * @param aEvent
     */
    void onDisambiguationExpire( wxTimerEvent& aEvent );

    /**
     * Handle disambiguation actions including displaying the menu.
     */
    int disambiguateCursor( const TOOL_EVENT& aEvent );

    /**
     * Take necessary action mark an item as selected.
     *
     * @param aItem is an item to be selected.
     */
    void select( EDA_ITEM* aItem );

    /**
     * Take necessary action mark an item as unselected.
     *
     * @param aItem is an item to be unselected.
     */
    void unselect( EDA_ITEM* aItem );

    /**
     * Highlight the item visually.
     *
     * @param aItem is an item to be be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to add the item to in the BRIGHTENED mode.
     */
    void highlight( EDA_ITEM* aItem, int aHighlightMode, EE_SELECTION* aGroup = nullptr );

    /**
     * Unhighlight the item visually.
     *
     * @param aItem is an item to be be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to remove the item from.
     */
    void unhighlight( EDA_ITEM* aItem, int aHighlightMode, EE_SELECTION* aGroup = nullptr );

    /**
     * Set the reference point to the anchor of the top-left item.
     */
    void updateReferencePoint();

    /**
     * @return True if the given point is contained in any of selected items' bounding box.
     */
    bool selectionContains( const VECTOR2I& aPoint ) const;

    ///< Set up handlers for various events.
    void setTransitions() override;

private:

    SCH_BASE_FRAME* m_frame;             // Pointer to the parent frame
    EE_SELECTION    m_selection;         // Current state of selection

    KICURSOR        m_nonModifiedCursor; // Cursor in the absence of shift/ctrl/alt

    bool            m_isSymbolEditor;    // True when the symbol editor is the parent frame
    bool            m_isSymbolViewer;    // True when the symbol browser is the parent frame
    int             m_unit;              // Fixed unit filter (for symbol editor)
    int             m_convert;           // Fixed DeMorgan filter (for symbol editor)
};

#endif //KICAD_SCH_SELECTION_TOOL_H
