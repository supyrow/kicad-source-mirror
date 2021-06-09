/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _PCB_CALCULATOR_SETTINGS_H
#define _PCB_CALCULATOR_SETTINGS_H

#include <array>
#include <unordered_map>
#include <settings/app_settings.h>

class PCB_CALCULATOR_SETTINGS : public APP_SETTINGS_BASE
{
public:
    struct ATTENUATOR
    {
        double attenuation;
        double zin;
        double zout;
    };

    struct ATTENUATORS
    {
        int type;
        std::unordered_map<std::string, ATTENUATOR> attenuators;
    };

    struct ELECTRICAL
    {
        int spacing_units;
        wxString spacing_voltage;
    };

    struct REGULATORS
    {
        wxString r1;
        wxString r2;
        wxString vref;
        wxString vout;
        wxString data_file;
        wxString selected_regulator;
        int type;
        int last_param;
    };

    struct TRACK_WIDTH
    {
        wxString current;
        wxString delta_tc;
        wxString track_len;
        int      track_len_units;
        wxString resistivity;
        wxString ext_track_width;
        int      ext_track_width_units;
        wxString ext_track_thickness;
        int      ext_track_thickness_units;
        wxString int_track_width;
        int      int_track_width_units;
        wxString int_track_thickness;
        int      int_track_thickness_units;
    };

    /// Map of TRANSLINE_PRM id to value
    typedef std::map<std::string, double> TL_PARAM_MAP;

    /// Map of TRANSLINE_PRM id to units selection
    typedef std::map<std::string, int> TL_PARAM_UNITS_MAP;

    struct TRANSMISSION_LINE
    {
        int type;

        /// Transline parameters, per transline type
        std::map<std::string, TL_PARAM_MAP> param_values;

        /// Transline parameter units selections, per transline type
        std::map<std::string, TL_PARAM_UNITS_MAP> param_units;
    };

    struct VIA_SIZE
    {
        wxString hole_diameter;
        int      hole_diameter_units;
        wxString thickness;
        int      thickness_units;
        wxString length;
        int      length_units;
        wxString pad_diameter;
        int      pad_diameter_units;
        wxString clearance_diameter;
        int      clearance_diameter_units;
        wxString characteristic_impedance;
        int      characteristic_impedance_units;
        wxString applied_current;
        wxString plating_resistivity;
        wxString permittivity;
        wxString temp_rise;
        wxString pulse_rise_time;
    };

    PCB_CALCULATOR_SETTINGS();

    virtual ~PCB_CALCULATOR_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

protected:
    virtual std::string getLegacyFrameName() const override { return "pcb_calculator"; }

public:
    ATTENUATORS m_Attenuators;

    int m_BoardClassUnits;

    int m_ColorCodeTolerance;

    ELECTRICAL m_Electrical;

    int m_LastPage;

    REGULATORS m_Regulators;

    TRACK_WIDTH m_TrackWidth;

    TRANSMISSION_LINE m_TransLine;

    VIA_SIZE m_ViaSize;
};

#endif
