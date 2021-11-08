/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
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

#include "footprint_edit_frame.h"
#include "pcbnew_id.h"
#include <bitmaps.h>
#include <menus_helpers.h>
#include <tool/actions.h>
#include <tool/action_menu.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <widgets/wx_menubar.h>


void FOOTPRINT_EDIT_FRAME::ReCreateMenuBar()
{
    PCB_SELECTION_TOOL* selTool = m_toolManager->GetTool<PCB_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    //-- File menu ----------------------------------------------------------
    //
    ACTION_MENU* fileMenu = new ACTION_MENU( false, selTool );

    fileMenu->Add( ACTIONS::newLibrary );
    fileMenu->Add( ACTIONS::addLibrary );
    fileMenu->Add( PCB_ACTIONS::newFootprint );
    fileMenu->Add( PCB_ACTIONS::createFootprint );

    fileMenu->AppendSeparator();

    fileMenu->Add( ACTIONS::save );
    fileMenu->Add( ACTIONS::saveAs );
    fileMenu->Add( ACTIONS::revert );

    fileMenu->AppendSeparator();

    ACTION_MENU* submenuImport = new ACTION_MENU( false, selTool );
    submenuImport->SetTitle( _( "Import" ) );
    submenuImport->SetIcon( BITMAPS::import );

    submenuImport->Add( PCB_ACTIONS::importFootprint,        ACTION_MENU::NORMAL, _( "Footprint..." ) );
    submenuImport->Add( PCB_ACTIONS::placeImportedGraphics,  ACTION_MENU::NORMAL, _( "Graphics..." ) );

    fileMenu->Add( submenuImport );

    ACTION_MENU* submenuExport = new ACTION_MENU( false, selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( BITMAPS::export_file );

    submenuExport->Add( PCB_ACTIONS::exportFootprint, ACTION_MENU::NORMAL, _( "Footprint..." ) );
    submenuExport->Add( _( "View as &PNG..." ),
                        _( "Create a PNG file from the current view" ),
                        ID_FPEDIT_SAVE_PNG,
                        BITMAPS::export_png );

    fileMenu->Add( submenuExport );

    fileMenu->AppendSeparator();
    fileMenu->Add( ACTIONS::print );

    fileMenu->AppendSeparator();
    fileMenu->AddClose( _( "Footprint Editor" ) );


    //-- Edit menu -------------------------------------------------------
    //
    ACTION_MENU* editMenu = new ACTION_MENU( false, selTool );

    editMenu->Add( ACTIONS::undo );
    editMenu->Add( ACTIONS::redo );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::cut );
    editMenu->Add( ACTIONS::copy );
    editMenu->Add( ACTIONS::paste );
    editMenu->Add( ACTIONS::doDelete );
    editMenu->Add( ACTIONS::duplicate );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::selectAll );

    editMenu->AppendSeparator();
    editMenu->Add( PCB_ACTIONS::footprintProperties );
    editMenu->Add( PCB_ACTIONS::defaultPadProperties );

    editMenu->AppendSeparator();
    editMenu->Add( PCB_ACTIONS::cleanupGraphics );


    //-- View menu -------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, selTool );

    viewMenu->Add( ACTIONS::showFootprintBrowser );
    viewMenu->Add( ACTIONS::show3DViewer );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomTool );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::toggleGrid,                 ACTION_MENU::CHECK );
    viewMenu->Add( ACTIONS::gridProperties );
    viewMenu->Add( PCB_ACTIONS::togglePolarCoords,      ACTION_MENU::CHECK );

    // Units submenu
    ACTION_MENU* unitsSubMenu = new ACTION_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( BITMAPS::unit_mm );
    unitsSubMenu->Add( ACTIONS::inchesUnits,                ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::milsUnits,                  ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::millimetersUnits,           ACTION_MENU::CHECK );
    viewMenu->Add( unitsSubMenu );

    viewMenu->Add( ACTIONS::toggleCursorStyle,              ACTION_MENU::CHECK );

    viewMenu->AppendSeparator();
    // Drawing Mode Submenu
    ACTION_MENU* drawingModeSubMenu = new ACTION_MENU( false, selTool );
    drawingModeSubMenu->SetTitle( _( "&Drawing Mode" ) );
    drawingModeSubMenu->SetIcon( BITMAPS::add_zone );

    drawingModeSubMenu->Add( PCB_ACTIONS::padDisplayMode,   ACTION_MENU::CHECK );
    drawingModeSubMenu->Add( PCB_ACTIONS::graphicsOutlines, ACTION_MENU::CHECK );
    drawingModeSubMenu->Add( PCB_ACTIONS::textOutlines,     ACTION_MENU::CHECK );
    viewMenu->Add( drawingModeSubMenu );

    // Contrast Mode Submenu
    ACTION_MENU* contrastModeSubMenu = new ACTION_MENU( false, selTool );
    contrastModeSubMenu->SetTitle( _( "&Contrast Mode" ) );
    contrastModeSubMenu->SetIcon( BITMAPS::contrast_mode );

    contrastModeSubMenu->Add( ACTIONS::highContrastMode,    ACTION_MENU::CHECK );
    contrastModeSubMenu->Add( PCB_ACTIONS::layerAlphaDec );
    contrastModeSubMenu->Add( PCB_ACTIONS::layerAlphaInc );
    viewMenu->Add( contrastModeSubMenu );

    viewMenu->Add( PCB_ACTIONS::flipBoard,                  ACTION_MENU::CHECK );

    viewMenu->AppendSeparator();
    viewMenu->Add( PCB_ACTIONS::showFootprintTree,          ACTION_MENU::CHECK );
    viewMenu->Add( PCB_ACTIONS::showLayersManager,          ACTION_MENU::CHECK );


    //-- Place menu -------------------------------------------------------
    //
    ACTION_MENU* placeMenu = new ACTION_MENU( false, selTool );

    placeMenu->Add( PCB_ACTIONS::placePad );

    placeMenu->AppendSeparator();
    placeMenu->Add( PCB_ACTIONS::placeText );
    placeMenu->Add( PCB_ACTIONS::drawLine );
    placeMenu->Add( PCB_ACTIONS::drawArc );
    placeMenu->Add( PCB_ACTIONS::drawRectangle );
    placeMenu->Add( PCB_ACTIONS::drawCircle );
    placeMenu->Add( PCB_ACTIONS::drawPolygon );
    placeMenu->Add( PCB_ACTIONS::drawRuleArea );

    placeMenu->AppendSeparator();
    placeMenu->Add( PCB_ACTIONS::setAnchor );
    placeMenu->Add( ACTIONS::gridSetOrigin );


    //-- Inspect menu ------------------------------------------------------
    //
    ACTION_MENU* inspectMenu = new ACTION_MENU( false, selTool );

    inspectMenu->Add( ACTIONS::measureTool );

    inspectMenu->AppendSeparator();
    inspectMenu->Add( PCB_ACTIONS::checkFootprint );


    //-- Tools menu --------------------------------------------------------
    //
    ACTION_MENU* toolsMenu = new ACTION_MENU( false, selTool );

    toolsMenu->Add( _( "&Load Footprint from PCB..." ),
                    _( "Load a footprint from the current board into the editor" ),
                    ID_LOAD_FOOTPRINT_FROM_BOARD,
                    BITMAPS::load_module_board );

    toolsMenu->Add( _( "&Insert Footprint on PCB" ),
                    _( "Insert footprint onto current board" ),
                    ID_ADD_FOOTPRINT_TO_BOARD,
                    BITMAPS::insert_module_board );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( PCB_ACTIONS::repairFootprint );


    //-- Preferences menu -------------------------------------------------
    //
    ACTION_MENU* prefsMenu = new ACTION_MENU( false, selTool );

    prefsMenu->Add( ACTIONS::configurePaths );
    prefsMenu->Add( ACTIONS::showFootprintLibTable );

    // We can't use ACTIONS::showPreferences yet because wxWidgets moves this on
    // Mac, and it needs the wxID_PREFERENCES id to find it.
    prefsMenu->Add( _( "Preferences..." ) + "\tCtrl+,",
                    _( "Show preferences for all open tools" ),
                    wxID_PREFERENCES,
                    BITMAPS::preference );

    prefsMenu->AppendSeparator();
    AddMenuLanguageList( prefsMenu, selTool );

    //--MenuBar -----------------------------------------------------------
    //
    menuBar->Append( fileMenu,    _( "&File" ) );
    menuBar->Append( editMenu,    _( "&Edit" ) );
    menuBar->Append( viewMenu,    _( "&View" ) );
    menuBar->Append( placeMenu,   _( "&Place" ) );
    menuBar->Append( inspectMenu, _( "&Inspect" ) );
    menuBar->Append( toolsMenu,   _( "&Tools" ) );
    menuBar->Append( prefsMenu,   _( "P&references" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
