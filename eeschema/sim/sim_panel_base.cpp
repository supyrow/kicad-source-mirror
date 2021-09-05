/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Sylwester Kocjan <s.kocjan@o2.pl>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "confirm.h"
#include "sim_panel_base.h"

#include "sim_plot_frame.h"
#include "netlist_exporter_pspice_sim.h"


SIM_PANEL_BASE::SIM_PANEL_BASE() : m_simCommand( wxEmptyString )
{
}


SIM_PANEL_BASE::SIM_PANEL_BASE( const wxString& aCommand ) : m_simCommand( aCommand )
{
}


SIM_PANEL_BASE::SIM_PANEL_BASE( const wxString& aCommand, wxWindow* parent, wxWindowID id,
                                const wxPoint& pos, const wxSize& size, long style,
                                const wxString& name ) :
        wxWindow( parent, id, pos, size, style, name ),
        m_simCommand( aCommand )
{
}


SIM_PANEL_BASE::~SIM_PANEL_BASE()
{
}


bool SIM_PANEL_BASE::IsPlottable( SIM_TYPE aSimType )
{
    switch( aSimType )
    {
    case ST_AC:
    case ST_DC:
    case ST_TRANSIENT:
        return true;

    default:
        return false;
    }
}


SIM_TYPE SIM_PANEL_BASE::GetType() const
{
    return NETLIST_EXPORTER_PSPICE_SIM::CommandToSimType( m_simCommand );
}


SIM_NOPLOT_PANEL::SIM_NOPLOT_PANEL( const wxString& aCommand, wxWindow* parent, wxWindowID id,
                                    const wxPoint& pos, const wxSize& size, long style,
                                    const wxString& name ) :
        SIM_PANEL_BASE( aCommand, parent, id, pos, size, style, name )
{
    m_sizer = new wxBoxSizer( wxVERTICAL );
    m_sizer->Add( 0, 1, 1, wxEXPAND, 5 );

    m_textInfo = new wxStaticText( dynamic_cast<wxWindow*>( this ), wxID_ANY, "", wxDefaultPosition,
            wxDefaultSize, wxALL | wxEXPAND | wxALIGN_CENTER_HORIZONTAL );
    m_textInfo->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
    m_textInfo->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
    m_textInfo->SetLabel(
            _( "This simulation provide no plots. Please refer to console window for results" ) );

    m_sizer->Add( m_textInfo, 1, wxALL | wxEXPAND, 5 );
    m_sizer->Add( 0, 1, 1, wxEXPAND, 5 );

    dynamic_cast<wxWindow*>( this )->SetSizer( m_sizer );
}


SIM_NOPLOT_PANEL::~SIM_NOPLOT_PANEL()
{
}
