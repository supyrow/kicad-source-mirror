/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __ACTIONS_H
#define __ACTIONS_H

#include <tool/tool_action.h>
#include <tool/tool_event.h>

#define LEGACY_HK_NAME( x ) x

/**
 * Gather all the actions that are shared by tools.
 *
 * The instance of a subclass of ACTIONS is created inside of #ACTION_MANAGER object that
 * registers the actions.
 */
class ACTIONS
{
public:

    virtual ~ACTIONS() {};

    // Generic document actions
    static TOOL_ACTION doNew;           // sadly 'new' is a reserved word
    static TOOL_ACTION newLibrary;
    static TOOL_ACTION addLibrary;
    static TOOL_ACTION open;
    static TOOL_ACTION save;
    static TOOL_ACTION saveAs;
    static TOOL_ACTION saveCopyAs;
    static TOOL_ACTION saveAll;
    static TOOL_ACTION revert;
    static TOOL_ACTION pageSettings;
    static TOOL_ACTION print;
    static TOOL_ACTION plot;
    static TOOL_ACTION quit;

    // Generic edit actions
    static TOOL_ACTION cancelInteractive;
    static TOOL_ACTION showContextMenu;
    static TOOL_ACTION undo;
    static TOOL_ACTION redo;
    static TOOL_ACTION cut;
    static TOOL_ACTION copy;
    static TOOL_ACTION paste;
    static TOOL_ACTION pasteSpecial;
    static TOOL_ACTION selectAll;
    static TOOL_ACTION duplicate;
    static TOOL_ACTION doDelete;        // sadly 'delete' is a reserved word
    static TOOL_ACTION deleteTool;

    // Find and Replace
    static TOOL_ACTION find;
    static TOOL_ACTION findAndReplace;
    static TOOL_ACTION findNext;
    static TOOL_ACTION findNextMarker;
    static TOOL_ACTION replaceAndFindNext;
    static TOOL_ACTION replaceAll;
    static TOOL_ACTION updateFind;

    // RC Lists
    static TOOL_ACTION prevMarker;
    static TOOL_ACTION nextMarker;
    static TOOL_ACTION excludeMarker;

    // View controls
    static TOOL_ACTION zoomRedraw;
    static TOOL_ACTION zoomIn;
    static TOOL_ACTION zoomOut;
    static TOOL_ACTION zoomInCenter;
    static TOOL_ACTION zoomOutCenter;
    static TOOL_ACTION zoomCenter;
    static TOOL_ACTION zoomFitScreen;
    static TOOL_ACTION zoomFitObjects; // Zooms to bbox of items on screen (except page border)
    static TOOL_ACTION zoomPreset;
    static TOOL_ACTION zoomTool;
    static TOOL_ACTION centerContents;
    static TOOL_ACTION toggleCursor;
    static TOOL_ACTION toggleCursorStyle;
    static TOOL_ACTION highContrastMode;
    static TOOL_ACTION highContrastModeCycle;

    static TOOL_ACTION refreshPreview;      // Similar to a synthetic mouseMoved event, but also
                                            // used after a rotate, mirror, etc.

    static TOOL_ACTION pinLibrary;
    static TOOL_ACTION unpinLibrary;

    /// Cursor control with keyboard
    static TOOL_ACTION cursorUp;
    static TOOL_ACTION cursorDown;
    static TOOL_ACTION cursorLeft;
    static TOOL_ACTION cursorRight;

    static TOOL_ACTION cursorUpFast;
    static TOOL_ACTION cursorDownFast;
    static TOOL_ACTION cursorLeftFast;
    static TOOL_ACTION cursorRightFast;

    static TOOL_ACTION cursorClick;
    static TOOL_ACTION cursorDblClick;

    // Panning with keyboard
    static TOOL_ACTION panUp;
    static TOOL_ACTION panDown;
    static TOOL_ACTION panLeft;
    static TOOL_ACTION panRight;

    // Grid control
    static TOOL_ACTION gridFast1;
    static TOOL_ACTION gridFast2;
    static TOOL_ACTION gridNext;
    static TOOL_ACTION gridPrev;
    static TOOL_ACTION gridSetOrigin;
    static TOOL_ACTION gridResetOrigin;
    static TOOL_ACTION gridPreset;
    static TOOL_ACTION toggleGrid;
    static TOOL_ACTION gridProperties;

    // Units
    static TOOL_ACTION inchesUnits;
    static TOOL_ACTION milsUnits;
    static TOOL_ACTION millimetersUnits;
    static TOOL_ACTION updateUnits;
    static TOOL_ACTION toggleUnits;
    static TOOL_ACTION togglePolarCoords;
    static TOOL_ACTION resetLocalCoords;

    // Common Tools
    static TOOL_ACTION selectionTool;
    static TOOL_ACTION measureTool;
    static TOOL_ACTION pickerTool;

    // Misc
    static TOOL_ACTION show3DViewer;
    static TOOL_ACTION showSymbolBrowser;
    static TOOL_ACTION showSymbolEditor;
    static TOOL_ACTION showFootprintBrowser;
    static TOOL_ACTION showFootprintEditor;
    static TOOL_ACTION updatePcbFromSchematic;
    static TOOL_ACTION updateSchematicFromPcb;

    // Internal
    static TOOL_ACTION updateMenu;
    static TOOL_ACTION activatePointEditor;
    static TOOL_ACTION changeEditMethod;
    static TOOL_ACTION updatePreferences;

    // Suite
    static TOOL_ACTION openPreferences;
    static TOOL_ACTION configurePaths;
    static TOOL_ACTION showSymbolLibTable;
    static TOOL_ACTION showFootprintLibTable;
    static TOOL_ACTION gettingStarted;
    static TOOL_ACTION help;
    static TOOL_ACTION listHotKeys;
    static TOOL_ACTION donate;
    static TOOL_ACTION getInvolved;
    static TOOL_ACTION reportBug;

    ///< Cursor control event types
    enum CURSOR_EVENT_TYPE { CURSOR_NONE, CURSOR_UP, CURSOR_DOWN, CURSOR_LEFT, CURSOR_RIGHT,
                             CURSOR_CLICK, CURSOR_DBL_CLICK, CURSOR_RIGHT_CLICK,
                             CURSOR_FAST_MOVE = 0x8000 };

    ///< Remove event modifier flags
    enum class REMOVE_FLAGS { NORMAL = 0x00, ALT = 0x01, CUT = 0x02 };
};


/**
 * Gather all the events that are shared by tools.
 */
class EVENTS
{
public:
    const static TOOL_EVENT SelectedEvent;
    const static TOOL_EVENT UnselectedEvent;
    const static TOOL_EVENT ClearedEvent;

    ///< Selected item had a property changed (except movement)
    const static TOOL_EVENT SelectedItemsModified;

    ///< Selected items were moved, this can be very high frequency on the canvas, use with care
    const static TOOL_EVENT SelectedItemsMoved;

    ///< Used to inform tools that the selection should temporarily be non-editable
    const static TOOL_EVENT InhibitSelectionEditing;
    const static TOOL_EVENT UninhibitSelectionEditing;

    ///< Used to inform tool that it should display the disambiguation menu
    const static TOOL_EVENT DisambiguatePoint;
};

#endif // __ACTIONS_H


