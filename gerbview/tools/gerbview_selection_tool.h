/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GERBVIEW_SELECTION_TOOL_H
#define GERBVIEW_SELECTION_TOOL_H

#include <memory>
#include <math/vector2d.h>
#include <tool/tool_interactive.h>
#include <tool/action_menu.h>
#include <tool/selection_conditions.h>
#include <tool/selection_tool.h>
#include <tool/tool_menu.h>
#include <tools/gerbview_selection.h>
#include <gerbview_frame.h>

class SELECTION_AREA;
class GERBER_COLLECTOR;

namespace KIGFX
{
    class GAL;
}


/**
 * Selection tool for GerbView, based on the one in Pcbnew
 */
class GERBVIEW_SELECTION_TOOL : public SELECTION_TOOL, public TOOL_INTERACTIVE
{
public:
    GERBVIEW_SELECTION_TOOL();
    ~GERBVIEW_SELECTION_TOOL();

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    // called to rebuild a CONDITIONAL_MENU before opening it:
    int UpdateMenu( const TOOL_EVENT& aEvent );

    /**
     * The main loop.
     */
    int Main( const TOOL_EVENT& aEvent );

    /**
     * Return the set of currently selected items.
     */
    GERBVIEW_SELECTION& GetSelection();

    int ClearSelection( const TOOL_EVENT& aEvent );

    int SelectItem( const TOOL_EVENT& aEvent );
    int SelectItems( const TOOL_EVENT& aEvent );

    int UnselectItem( const TOOL_EVENT& aEvent );
    int UnselectItems( const TOOL_EVENT& aEvent );

    ///< Sets up handlers for various events.
    void setTransitions() override;

private:
    /**
     * Select an item pointed by the parameter aWhere. If there is more than one item at that
     * place, there is a menu displayed that allows one to choose the item.
     *
     * @param aWhere is the place where the item should be selected.
     * @param aAllowDisambiguation decides what to do in case of disambiguation. If true, then
     * a menu is shown, otherwise function finishes without selecting anything.
     * @return True if an item was selected, false otherwise.
     */
    bool selectPoint( const VECTOR2I& aWhere, bool aOnDrag = false );

    /**
     * Select an item under the cursor unless there is something already selected or
     * \a aSelectAlways is true.
     *
     * @param aSelectAlways forces to select an item even if there is an item already selected.
     * @return true if eventually there is an item selected, false otherwise.
     */
    bool selectCursor( bool aSelectAlways = false );

    /**
     * Clear the current selection.
     */
    void clearSelection();

    /**
     * Handle the menu that allows one to select one of many items in case
     * there is more than one item at the selected point (@see selectCursor()).
     *
     * @param aItems contains list of items that are displayed to the user.
     */
    EDA_ITEM* disambiguationMenu( GERBER_COLLECTOR* aItems );

    /**
     * Check conditions for an item to be selected.
     *
     * @return True if the item fulfills conditions to be selected.
     */
    bool selectable( const EDA_ITEM* aItem ) const;

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
     * Mark item as selected, but does not add it to the #ITEMS_PICKED_LIST.
     *
     * @param aItem is an item to be be marked.
     */
    void selectVisually( EDA_ITEM* aItem );

    /**
     * Mark item as selected, but does not add it to the #ITEMS_PICKED_LIST.
     *
     * @param aItem is an item to be be marked.
     */
    void unselectVisually( EDA_ITEM* aItem );

    GERBVIEW_FRAME* m_frame;        // Pointer to the parent frame.
    GERBVIEW_SELECTION m_selection; // Current state of selection.

    bool m_preliminary;             // Determines if the selection is preliminary or final.
};

#endif
