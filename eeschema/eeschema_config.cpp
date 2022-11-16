/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <mutex>

#include <symbol_library.h>
#include <confirm.h>
#include <dialogs/dialog_schematic_setup.h>
#include <kiway.h>
#include <symbol_edit_frame.h>
#include <dialogs/panel_gal_display_options.h>
#include <pgm_base.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <schematic.h>
#include <hierarch.h>
#include <settings/app_settings.h>
#include <settings/settings_manager.h>
#include <symbol_lib_table.h>
#include <wildcards_and_files_ext.h>
#include <drawing_sheet/ds_data_model.h>
#include <zoom_defines.h>


/// Helper for all the old plotting/printing code while it still exists
COLOR4D GetLayerColor( SCH_LAYER_ID aLayer )
{
    return Pgm().GetSettingsManager().GetColorSettings()->GetColor( aLayer );
}


bool SCH_EDIT_FRAME::LoadProjectSettings()
{
    SCHEMATIC_SETTINGS& settings = Schematic().Settings();
    settings.m_JunctionSize = GetSchematicJunctionSize();

    GetRenderSettings()->SetDefaultPenWidth( settings.m_DefaultLineWidth );
    GetRenderSettings()->m_LabelSizeRatio  = settings.m_LabelSizeRatio;
    GetRenderSettings()->m_TextOffsetRatio = settings.m_TextOffsetRatio;
    GetRenderSettings()->m_PinSymbolSize   = settings.m_PinSymbolSize;

    GetRenderSettings()->SetDashLengthRatio( settings.m_DashedLineDashRatio );
    GetRenderSettings()->SetGapLengthRatio( settings.m_DashedLineGapRatio );

    // Verify some values, because the config file can be edited by hand, and have bad values:
    LIB_SYMBOL::SetSubpartIdNotation( LIB_SYMBOL::GetSubpartIdSeparator(),
                                      LIB_SYMBOL::GetSubpartFirstId() );

    // Load the drawing sheet from the filename stored in BASE_SCREEN::m_DrawingSheetFileName.
    // If empty, or not existing, the default drawing sheet is loaded.
    wxString filename = DS_DATA_MODEL::ResolvePath( BASE_SCREEN::m_DrawingSheetFileName,
                                                    Prj().GetProjectPath() );

    if( !DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( filename ) )
        ShowInfoBarError( _( "Error loading drawing sheet." ), true );

    return true;
}


void SCH_EDIT_FRAME::ShowSchematicSetupDialog( const wxString& aInitialPage )
{
    DIALOG_SCHEMATIC_SETUP dlg( this );

    if( !aInitialPage.IsEmpty() )
        dlg.SetInitialPage( aInitialPage, wxEmptyString );

    if( dlg.ShowQuasiModal() == wxID_OK )
    {
        SaveProjectSettings();

        Kiway().CommonSettingsChanged( false, true );

        GetRenderSettings()->SetDefaultPenWidth( Schematic().Settings().m_DefaultLineWidth );
        GetRenderSettings()->m_LabelSizeRatio  = Schematic().Settings().m_LabelSizeRatio;
        GetRenderSettings()->m_TextOffsetRatio = Schematic().Settings().m_TextOffsetRatio;
        GetRenderSettings()->m_PinSymbolSize   = Schematic().Settings().m_PinSymbolSize;

        GetRenderSettings()->SetDashLengthRatio( Schematic().Settings().m_DashedLineDashRatio );
        GetRenderSettings()->SetGapLengthRatio( Schematic().Settings().m_DashedLineGapRatio );

        GetCanvas()->GetView()->MarkDirty();
        GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
        GetCanvas()->Refresh();
    }
}


int SCH_EDIT_FRAME::GetSchematicJunctionSize()
{
    std::vector<double>& sizeMultipliers = eeconfig()->m_Drawing.junction_size_mult_list;

    const std::shared_ptr<NET_SETTINGS>& netSettings = Prj().GetProjectFile().NetSettings();
    int sizeChoice = Schematic().Settings().m_JunctionSizeChoice;
    int dotSize = netSettings->m_DefaultNetClass->GetWireWidth() * sizeMultipliers[ sizeChoice ];

    return std::max( dotSize, 1 );
}


void SCH_EDIT_FRAME::SaveProjectSettings()
{
    wxFileName fn = Schematic().RootScreen()->GetFileName();  //ConfigFileName

    fn.SetExt( ProjectFileExtension );

    if( !fn.HasName() || !IsWritable( fn, false ) )
        return;

    RecordERCExclusions();

    GetSettingsManager()->SaveProject( fn.GetFullPath() );
}


void SCH_EDIT_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    // For now, axes are forced off in Eeschema even if turned on in config
    eeconfig()->m_Window.grid.axes_enabled = false;

    SCH_BASE_FRAME::LoadSettings( eeconfig() );

    SCH_SEARCH_DATA* searchData = dynamic_cast<SCH_SEARCH_DATA*>( m_findReplaceData.get() );

    if( searchData )
    {
        searchData->replaceReferences = eeconfig()->m_FindReplaceExtra.replace_references;
        searchData->searchAllFields = eeconfig()->m_FindReplaceExtra.search_all_fields;
        searchData->searchAllPins = eeconfig()->m_FindReplaceExtra.search_all_pins;
        searchData->searchCurrentSheetOnly = eeconfig()->m_FindReplaceExtra.search_current_sheet_only;
    }

    GetRenderSettings()->m_ShowPinsElectricalType = false;
    GetRenderSettings()->m_ShowPinNumbers = false;
    GetRenderSettings()->SetDefaultFont( eeconfig()->m_Appearance.default_font );
}


void SCH_EDIT_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    SCH_BASE_FRAME::SaveSettings( eeconfig() );
    wxAuiPaneInfo& hierarchy_pane = m_auimgr.GetPane( SchematicHierarchyPaneName() );

    if( eeconfig() )
    {
        eeconfig()->m_System.units = static_cast<int>( GetUserUnits() );
        eeconfig()->m_AuiPanels.show_schematic_hierarchy = hierarchy_pane.IsShown();
        eeconfig()->m_AuiPanels.schematic_hierarchy_float = hierarchy_pane.IsFloating();
        // Other parameters (hierarchy_panel_float_width, hierarchy_panel_float_height,
        // and hierarchy_panel_docked_width should have been updated when resizing the
        // hierarchy panel

        SCH_SEARCH_DATA* searchData = dynamic_cast<SCH_SEARCH_DATA*>( m_findReplaceData.get() );

        if( searchData )
        {
            eeconfig()->m_FindReplaceExtra.replace_references = searchData->replaceReferences;
            eeconfig()->m_FindReplaceExtra.search_all_fields = searchData->searchAllFields;
            eeconfig()->m_FindReplaceExtra.search_all_pins = searchData->searchAllPins;
            eeconfig()->m_FindReplaceExtra.search_current_sheet_only =
                    searchData->searchCurrentSheetOnly;
        }
    }
}


void SCH_BASE_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    wxCHECK_RET( aCfg, "Call to SCH_BASE_FRAME::LoadSettings with null settings" );

    EDA_DRAW_FRAME::LoadSettings( aCfg );

    // Currently values read from config file are not used because the user cannot
    // change this config
    // if( aCfg->m_Window.grid.sizes.empty() )  // Will be probably never enabled
    {
        /*
         * Do NOT add others values (mainly grid values in mm), because they can break the
         * schematic: Because wires and pins are considered as connected when the are to the
         * same coordinate we cannot mix coordinates in mils (internal units) and mm (that
         * cannot exactly converted in mils in many cases).  In fact schematic must only use
         * 50 and 25 mils to place labels, wires and symbols others values are useful only
         * for graphic items (mainly in library editor) so use integer values in mils only.
         * The 100 mil grid is added to help conform to the KiCad Library Convention which
         * states: "Using a 100mil grid, pin ends and origin must lie on grid nodes IEC-60617"
         */
        aCfg->m_Window.grid.sizes = { "100 mil",
                                      "50 mil",
                                      "25 mil",
                                      "10 mil",
                                      "5 mil",
                                      "2 mil",
                                      "1 mil" };
    }

    // Currently values read from config file are not used because the user cannot
    // change this config
    // if( aCfg->m_Window.zoom_factors.empty() )
    {
        aCfg->m_Window.zoom_factors = { ZOOM_LIST_EESCHEMA };
    }
}


void SCH_BASE_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    wxCHECK_RET( aCfg, "Call to SCH_BASE_FRAME::SaveSettings with null settings" );

    EDA_DRAW_FRAME::SaveSettings( aCfg );
}


static std::mutex s_symbolTableMutex;


SYMBOL_LIB_TABLE* PROJECT::SchSymbolLibTable()
{
    std::lock_guard<std::mutex> lock( s_symbolTableMutex );

    // This is a lazy loading function, it loads the project specific table when
    // that table is asked for, not before.
    SYMBOL_LIB_TABLE* tbl = (SYMBOL_LIB_TABLE*) GetElem( ELEM_SYMBOL_LIB_TABLE );

    // its gotta be NULL or a SYMBOL_LIB_TABLE, or a bug.
    wxASSERT( !tbl || tbl->Type() == SYMBOL_LIB_TABLE_T );

    if( !tbl )
    {
        // Stack the project specific SYMBOL_LIB_TABLE overlay on top of the global table.
        // ~SYMBOL_LIB_TABLE() will not touch the fallback table, so multiple projects may
        // stack this way, all using the same global fallback table.
        tbl = new SYMBOL_LIB_TABLE( &SYMBOL_LIB_TABLE::GetGlobalLibTable() );

        SetElem( ELEM_SYMBOL_LIB_TABLE, tbl );

        wxString prjPath;

        wxGetEnv( PROJECT_VAR_NAME, &prjPath );

        if( !prjPath.IsEmpty() )
        {
            wxFileName fn( prjPath, SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

            try
            {
                tbl->Load( fn.GetFullPath() );
            }
            catch( const IO_ERROR& ioe )
            {
                wxString msg;
                msg.Printf( _( "Error loading the symbol library table '%s'." ),
                            fn.GetFullPath() );
                DisplayErrorMessage( nullptr, msg, ioe.What() );
            }
        }
    }

    return tbl;
}
