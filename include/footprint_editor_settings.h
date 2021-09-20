/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef FOOTPRINT_EDITOR_SETTINGS_H_
#define FOOTPRINT_EDITOR_SETTINGS_H_

#include <board_design_settings.h>
#include <settings/app_settings.h>
#include <pcbnew_settings.h>
#include <project/board_project_settings.h>


class FOOTPRINT_EDITOR_SETTINGS : public APP_SETTINGS_BASE
{
public:
    struct USER_GRID
    {
        double size_x;
        double size_y;
        int units;
    };

    FOOTPRINT_EDITOR_SETTINGS();

    virtual ~FOOTPRINT_EDITOR_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    /// Only some of these settings are actually used for footprint editing
    // TODO: factor out the relevant stuff so the whole BDS doesn't have to be here
    BOARD_DESIGN_SETTINGS m_DesignSettings;

    // Only the magneticPads element is used
    MAGNETIC_SETTINGS m_MagneticItems;

    PCB_DISPLAY_OPTIONS m_Display;

    USER_GRID m_UserGrid;

    bool m_PolarCoords;

    bool m_Use45Limit;

    int m_LibWidth;

    wxString m_LastImportExportPath;

    wxString m_FootprintTextShownColumns;

    SELECTION_FILTER_OPTIONS m_SelectionFilter;

    std::vector<LAYER_PRESET> m_LayerPresets;

    wxString m_ActiveLayerPreset;

protected:

    virtual std::string getLegacyFrameName() const override { return "ModEditFrame"; }

private:

    bool migrateSchema0to1();
};


#endif
