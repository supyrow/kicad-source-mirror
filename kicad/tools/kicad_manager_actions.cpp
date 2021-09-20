/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/tool_action.h>
#include <bitmaps.h>
#include <tools/kicad_manager_actions.h>
#include <frame_type.h>


// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

TOOL_ACTION KICAD_MANAGER_ACTIONS::newProject( "kicad.Control.newProject",
        AS_GLOBAL,
        MD_CTRL + 'N', LEGACY_HK_NAME( "New Project" ),
        _( "New Project..." ), _( "Create new blank project" ),
        BITMAPS::new_project );

TOOL_ACTION KICAD_MANAGER_ACTIONS::newFromTemplate( "kicad.Control.newFromTemplate",
        AS_GLOBAL,
        MD_CTRL + 'T', LEGACY_HK_NAME( "New Project From Template" ),
        _( "New Project from Template..." ), _( "Create new project from template" ) );

TOOL_ACTION KICAD_MANAGER_ACTIONS::openDemoProject( "kicad.Control.openDemoProject",
        AS_GLOBAL,
        0, LEGACY_HK_NAME( "Open Demo Project" ),
        _( "Open Demo Project..." ), _( "Open a demo project" ) );

TOOL_ACTION KICAD_MANAGER_ACTIONS::openProject( "kicad.Control.openProject",
        AS_GLOBAL,
        MD_CTRL + 'O', LEGACY_HK_NAME( "Open Project" ),
        _( "Open Project..." ), _( "Open an existing project" ),
        BITMAPS::directory_open );

TOOL_ACTION KICAD_MANAGER_ACTIONS::closeProject( "kicad.Control.closeProject",
        AS_GLOBAL,
        0, LEGACY_HK_NAME( "Close Project" ),
        _( "Close Project" ), _( "Close the current project" ),
        BITMAPS::project_close );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editSchematic( "kicad.Control.editSchematic",
        AS_GLOBAL,
        MD_CTRL + 'E', LEGACY_HK_NAME( "Run Eeschema" ),
        _( "Schematic Editor" ), _( "Edit schematic" ),
        BITMAPS::icon_eeschema_24, AF_NONE, (void*) FRAME_SCH );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editSymbols( "kicad.Control.editSymbols",
        AS_GLOBAL,
        MD_CTRL + 'L', LEGACY_HK_NAME( "Run LibEdit" ),
        _( "Symbol Editor" ), _( "Edit schematic symbols" ),
        BITMAPS::icon_libedit_24, AF_NONE, (void*) FRAME_SCH_SYMBOL_EDITOR );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editPCB( "kicad.Control.editPCB",
        AS_GLOBAL,
        MD_CTRL + 'P', LEGACY_HK_NAME( "Run Pcbnew" ),
        _( "PCB Editor" ), _( "Edit PCB" ),
        BITMAPS::icon_pcbnew_24, AF_NONE, (void*) FRAME_PCB_EDITOR );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editFootprints( "kicad.Control.editFootprints",
        AS_GLOBAL,
        MD_CTRL + 'F', LEGACY_HK_NAME( "Run FpEditor" ),
        _( "Footprint Editor" ), _( "Edit PCB footprints" ),
        BITMAPS::icon_modedit_24, AF_NONE, (void*) FRAME_FOOTPRINT_EDITOR );

TOOL_ACTION KICAD_MANAGER_ACTIONS::viewGerbers( "kicad.Control.viewGerbers",
        AS_GLOBAL,
        MD_CTRL + 'G', LEGACY_HK_NAME( "Run Gerbview" ),
        _( "Gerber Viewer" ), _( "Preview Gerber output files" ),
        BITMAPS::icon_gerbview_24 );

TOOL_ACTION KICAD_MANAGER_ACTIONS::convertImage( "kicad.Control.convertImage",
        AS_GLOBAL,
        MD_CTRL + 'B', LEGACY_HK_NAME( "Run Bitmap2Component" ),
        _( "Image Converter" ), _( "Convert bitmap images to schematic or PCB components" ),
        BITMAPS::icon_bitmap2component_24 );

TOOL_ACTION KICAD_MANAGER_ACTIONS::showCalculator( "kicad.Control.showCalculator",
        AS_GLOBAL, 0, LEGACY_HK_NAME( "Run PcbCalculator" ),
        _( "Calculator Tools" ), _( "Run component calculations, track width calculations, etc." ),
        BITMAPS::icon_pcbcalculator_24 );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editDrawingSheet( "kicad.Control.editDrawingSheet",
        AS_GLOBAL,
        MD_CTRL + 'Y', LEGACY_HK_NAME( "Run PlEditor" ),
        _( "Drawing Sheet Editor" ), _( "Edit drawing sheet borders and title block" ),
        BITMAPS::icon_pagelayout_editor_24 );

#ifdef PCM
TOOL_ACTION KICAD_MANAGER_ACTIONS::showPluginManager( "kicad.Control.pluginContentManager",
        AS_GLOBAL,
        MD_CTRL + 'M', "",
        _( "Plugin and Content Manager" ), _( "Run Plugin and Content Manager" ),
        BITMAPS::icon_pcm_24 );
#endif

TOOL_ACTION KICAD_MANAGER_ACTIONS::openTextEditor( "kicad.Control.openTextEditor",
        AS_GLOBAL,
        0, "",
        _( "Open Text Editor" ), _( "Launch preferred text editor" ),
        BITMAPS::editor );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editOtherSch( "kicad.Control.editOtherSch",
        AS_GLOBAL );

TOOL_ACTION KICAD_MANAGER_ACTIONS::editOtherPCB( "kicad.Control.editOtherPCB",
        AS_GLOBAL );


