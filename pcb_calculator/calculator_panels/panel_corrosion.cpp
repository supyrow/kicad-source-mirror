/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2022 Kicad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <calculator_panels/panel_corrosion.h>
#include <pcb_calculator_settings.h>
#include <widgets/unit_selector.h>
#include <math/util.h>      // for KiROUND

extern double DoubleFromString( const wxString& TextValue );


CORROSION_TABLE_ENTRY::CORROSION_TABLE_ENTRY( wxString aName, wxString aSymbol, double aPot )
{
    m_potential = aPot;
    m_name = aName;
    m_symbol = aSymbol;
}

PANEL_CORROSION::PANEL_CORROSION( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                  const wxSize& size, long style, const wxString& name ) :
        PANEL_CORROSION_BASE( parent, id, pos, size, style, name )
{
    m_entries.clear();
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Platinum" ), "Pt", -0.57 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Gold" ), "Au", -0.44 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Titanium" ), "Ti", -0.32 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Stainless steel 18-9" ), "X8CrNiS18-9",
                                                   -0.32 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Silver" ), "Ag", -0.22 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Mercury" ), "Hg", -0.22 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Nickel" ), "Ni", -0.14 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Copper" ), "Cu", 0.0 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Copper-Aluminium" ), "CuAl10", 0.03 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Brass" ), "CuZn39Pb", 0.08 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Bronze" ), "CuSn12", 0.2 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Tin" ), "Sn", 0.23 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Lead" ), "Pb", 0.27 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Aluminium-Copper" ), "AlCu4Mg", 0.37 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Cast iron" ), "", 0.38 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Carbon steel" ), "", 0.43 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Aluminium" ), "Al", 0.52 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Cadmium" ), "Cd", 0.53 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Iron" ), "Fe", 0.535 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Chrome" ), "Cr", 0.63 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Zinc" ), "Zn", 0.83 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Manganese" ), "Mn", 0.9 ) );
    m_entries.emplace_back( CORROSION_TABLE_ENTRY( _( "Magnesium" ), "Mg", 1.38 ) );

    // Resize the table

    m_table->DeleteCols( 0, m_table->GetNumberCols() );
    m_table->DeleteRows( 0, m_table->GetNumberRows() );
    m_table->AppendCols( m_entries.size() );
    m_table->AppendRows( m_entries.size() );

    m_corFilterValue = 0;
    FillTable();
    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


PANEL_CORROSION::~PANEL_CORROSION()
{
}

void PANEL_CORROSION::ThemeChanged()
{
}


void PANEL_CORROSION::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_corFilterCtrl->SetValue( aCfg->m_CorrosionTable.threshold_voltage );
    m_corFilterValue = DoubleFromString( m_corFilterCtrl->GetValue() );
}


void PANEL_CORROSION::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    aCfg->m_CorrosionTable.threshold_voltage = wxString( "" ) << m_corFilterValue;
}


void PANEL_CORROSION::OnCorFilterChange( wxCommandEvent& aEvent )
{
    m_corFilterValue = DoubleFromString( m_corFilterCtrl->GetValue() );
    FillTable();
}


void PANEL_CORROSION::FillTable()
{
    // Fill the table with data
    int i = 0;

    wxColour color_ok( 122, 166, 194 );

    wxColour color_text( 0, 0, 0 );
    wxString value;


    for( CORROSION_TABLE_ENTRY entryA : m_entries )
    {
        int j = 0;

        wxString label = entryA.m_name;

        if( entryA.m_symbol.size() > 0 )
        {
            label += " (" + entryA.m_symbol + ")";
        }
        m_table->SetRowLabelValue( i, label );
        m_table->SetColLabelValue( i, label );

        for( CORROSION_TABLE_ENTRY entryB : m_entries )
        {
            double diff = entryA.m_potential - entryB.m_potential;
            int diff_temp = KiROUND( abs( diff * 100 ) );
            value = "";
            value << diff * 1000; // Let's display it in mV instead of V.
            m_table->SetCellValue( i, j, value );

            // Overide anything that could come from a dark them
            m_table->SetCellTextColour( i, j, color_text );

            if( ( abs( diff ) ) == 0 )
            {
                m_table->SetCellBackgroundColour( i, j, wxColor( 193, 231, 255 ) );
            }
            else if( ( KiROUND( abs( diff * 1000 ) ) ) > m_corFilterValue )
            {
                m_table->SetCellBackgroundColour(
                        i, j, wxColour( 202 - diff_temp, 206 - diff_temp, 225 - diff_temp ) );
            }
            else
            {
                m_table->SetCellBackgroundColour( i, j, color_ok );
            }
            m_table->SetReadOnly( i, j, true );
            j++;
        }
        i++;
    }

    m_table->SetColLabelTextOrientation( wxVERTICAL );

    m_table->SetColLabelSize( wxGRID_AUTOSIZE );
    m_table->SetRowLabelSize( wxGRID_AUTOSIZE );
    m_table->AutoSizeColumns();
    m_table->AutoSizeRows();
    Refresh();
}
