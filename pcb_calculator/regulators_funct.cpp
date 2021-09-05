/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre.charras
 * Copyright (C) 1992-2021 Kicad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file regulators_funct.cpp
 * Contains the partial functions of PCB_CALCULATOR_FRAME related to regulators
 */


#include <macros.h>

#include <wx/choicdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>

#include "class_regulator_data.h"
#include "pcb_calculator_frame.h"
#include "pcb_calculator_settings.h"

#include "dialogs/dialog_regulator_form.h"

extern double DoubleFromString( const wxString& TextValue );


void PCB_CALCULATOR_FRAME::OnRegulatorCalcButtonClick( wxCommandEvent& event )
{
    RegulatorsSolve();
}


void PCB_CALCULATOR_FRAME::OnRegulatorResetButtonClick( wxCommandEvent& event )
{
    m_RegulR1Value->SetValue( wxT( "10" ) );
    m_RegulR2Value->SetValue( wxT( "10" ) );
    m_RegulVrefValue->SetValue( wxT( "3" ) );
    m_RegulVoutValue->SetValue( wxT( "12" ) );
    m_choiceRegType->SetSelection( 0 );
    m_rbRegulR1->SetValue( 1 );
    m_rbRegulR2->SetValue( 0 );
    m_rbRegulVout->SetValue( 0 );
    RegulatorPageUpdate();
}


void PCB_CALCULATOR_FRAME::RegulatorPageUpdate()
{
    switch( m_choiceRegType->GetSelection() )
    {
    default:
    case 0:
        m_bitmapRegul4pins->Show( true );
        m_bitmapRegul3pins->Show( false );
        m_RegulIadjValue->Enable( false );
        m_RegulFormula->SetLabel( wxT( "Vout = Vref * (R1 + R2) / R2" ) );
        break;

    case 1:
        m_bitmapRegul4pins->Show( false );
        m_bitmapRegul3pins->Show( true );
        m_RegulIadjValue->Enable( true );
        m_RegulFormula->SetLabel( wxT( "Vout = Vref * (R1 + R2) / R1 + Iadj * R2" ) );
        break;
    }

    // The new icon size must be taken in account
    m_panelRegulators->GetSizer()->Layout();

    // Enable/disable buttons:
    bool enbl = m_choiceRegulatorSelector->GetCount() > 0;
    m_buttonEditItem->Enable( enbl );
    m_buttonRemoveItem->Enable( enbl );

    m_panelRegulators->Refresh();
}


void PCB_CALCULATOR_FRAME::OnRegulTypeSelection( wxCommandEvent& event )
{
    RegulatorPageUpdate();
}


void PCB_CALCULATOR_FRAME::OnRegulatorSelection( wxCommandEvent& event )
{
    wxString name = m_choiceRegulatorSelector->GetStringSelection();
    REGULATOR_DATA * item = m_RegulatorList.GetReg( name );

    if( item )
    {
        m_lastSelectedRegulatorName = item->m_Name;
        m_choiceRegType->SetSelection( item->m_Type );
        wxString value;
        value.Printf( wxT( "%g" ), item->m_Vref );
        m_RegulVrefValue->SetValue( value );
        value.Printf( wxT( "%g" ), item->m_Iadj );
        m_RegulIadjValue->SetValue( value );
    }

    // Call RegulatorPageUpdate to enable/disable tools,
    // even if no item selected
    RegulatorPageUpdate();
}


void PCB_CALCULATOR_FRAME::OnDataFileSelection( wxCommandEvent& event )
{
    wxString fullfilename = GetDataFilename();

    wxString wildcard;
    wildcard.Printf( _("PCB Calculator data file" ) + wxT( " (*.%s)|*.%s"),
                     DataFileNameExt, DataFileNameExt );

    wxFileDialog dlg( m_panelRegulators, _("Select PCB Calculator Data File"),
                      wxEmptyString, fullfilename, wildcard, wxFD_OPEN );

    if (dlg.ShowModal() == wxID_CANCEL)
        return;

    fullfilename = dlg.GetPath();

    if( fullfilename == GetDataFilename() )
        return;

    SetDataFilename( fullfilename );

    if( wxFileExists( fullfilename ) && m_RegulatorList.GetCount() > 0 )  // Read file
    {
        if( wxMessageBox( _( "Do you want to load this file and replace current regulator list?" ) )
            != wxID_OK )
            return;
    }

    if( ReadDataFile() )
    {
        m_RegulatorListChanged = false;
        m_choiceRegulatorSelector->Clear();
        m_choiceRegulatorSelector->Append( m_RegulatorList.GetRegList() );
        SelectLastSelectedRegulator();
    }
    else
    {
        wxString msg;
        msg.Printf( _("Unable to read data file '%s'."), fullfilename );
        wxMessageBox( msg );
    }
}


void PCB_CALCULATOR_FRAME::OnAddRegulator( wxCommandEvent& event )
{
    DIALOG_REGULATOR_FORM dlg( this, wxEmptyString );

    if( dlg.ShowModal() != wxID_OK )
        return;

    REGULATOR_DATA* new_item = dlg.BuildRegulatorFromData();

    // Add new item, if not existing
    if( m_RegulatorList.GetReg( new_item->m_Name ) == nullptr )
    {
        // Add item in list
        m_RegulatorList.Add( new_item );
        m_RegulatorListChanged = true;
        m_choiceRegulatorSelector->Clear();
        m_choiceRegulatorSelector->Append( m_RegulatorList.GetRegList() );
        m_lastSelectedRegulatorName = new_item->m_Name;
        SelectLastSelectedRegulator();
    }
    else
    {
        wxMessageBox( _("This regulator is already in list. Aborted") );
        delete new_item;
    }
}


void PCB_CALCULATOR_FRAME::OnEditRegulator( wxCommandEvent& event )
{
    wxString name = m_choiceRegulatorSelector->GetStringSelection();
    REGULATOR_DATA * item  = m_RegulatorList.GetReg( name );

    if( item == nullptr )
        return;

    DIALOG_REGULATOR_FORM dlg( this, name );

    dlg.CopyRegulatorDataToDialog( item );

    if( dlg.ShowModal() != wxID_OK )
        return;

    REGULATOR_DATA * new_item = dlg.BuildRegulatorFromData();
    m_RegulatorList.Replace( new_item );

    m_RegulatorListChanged = true;

    SelectLastSelectedRegulator();
}


void PCB_CALCULATOR_FRAME::OnRemoveRegulator( wxCommandEvent& event )
{
    wxString name = wxGetSingleChoice( _("Remove Regulator"), wxEmptyString,
                                       m_RegulatorList.GetRegList() );
    if( name.IsEmpty() )
        return;

    m_RegulatorList.Remove( name );
    m_RegulatorListChanged = true;
    m_choiceRegulatorSelector->Clear();
    m_choiceRegulatorSelector->Append( m_RegulatorList.GetRegList() );

    if( m_lastSelectedRegulatorName == name )
        m_lastSelectedRegulatorName.Empty();

    SelectLastSelectedRegulator();
}


void PCB_CALCULATOR_FRAME::SelectLastSelectedRegulator()
{
    // Find last selected in regulator list:
    int idx = -1;

    if( !m_lastSelectedRegulatorName.IsEmpty() )
    {
        for( unsigned ii = 0; ii < m_RegulatorList.GetCount(); ii++ )
        {
            if( m_RegulatorList.m_List[ii]->m_Name == m_lastSelectedRegulatorName )
            {
                idx = ii;
                break;
            }
        }
    }

    m_choiceRegulatorSelector->SetSelection( idx );
    wxCommandEvent event;
    OnRegulatorSelection( event );
}


void PCB_CALCULATOR_FRAME::RegulatorsSolve()
{
    int id;

    if( m_rbRegulR1->GetValue() )
    {
        id = 0;     // for R1 calculation
    }
    else if( m_rbRegulR2->GetValue() )
    {
        id = 1;     // for R2 calculation
    }
    else if( m_rbRegulVout->GetValue() )
    {
        id = 2;     // for Vout calculation
    }
    else
    {
        wxMessageBox( wxT("Selection error" ) );
        return;
    }

    double r1, r2, vref, vout;

    wxString txt;

    m_RegulMessage->SetLabel( wxEmptyString);

    // Convert r1 and r2 in ohms
    int r1scale = 1000;
    int r2scale = 1000;

    // Read values from panel:
    txt = m_RegulR1Value->GetValue();
    r1 = DoubleFromString(txt) * r1scale;
    txt = m_RegulR2Value->GetValue();
    r2 = DoubleFromString(txt) * r2scale;
    txt = m_RegulVrefValue->GetValue();
    vref = DoubleFromString(txt);
    txt = m_RegulVoutValue->GetValue();
    vout = DoubleFromString(txt);

    // Some tests:
    if( vout < vref && id != 2 )
    {
        m_RegulMessage->SetLabel( _("Vout must be greater than vref" ) );
        return;
    }

    if( vref == 0.0 )
    {
        m_RegulMessage->SetLabel( _( "Vref set to 0 !" ) );
        return;
    }

    if( ( r1 < 0 && id != 0 ) || ( r2 <= 0 && id != 1 ) )
    {
        m_RegulMessage->SetLabel( _("Incorrect value for R1 R2" ) );
        return;
    }

    // Calculate
    if( m_choiceRegType->GetSelection() == 1)
    {
        // 3 terminal regulator
        txt = m_RegulIadjValue->GetValue();
        double iadj = DoubleFromString(txt);

        // iadj is given in micro amp, so convert it in amp.
        iadj /= 1000000;

        switch( id )
        {
        case 0:
            r1 = vref * r2 / ( vout - vref - ( r2 * iadj ) );
            break;

        case 1:
            r2 = ( vout - vref ) / ( iadj + ( vref / r1 ) );
            break;

        case 2:
            vout = vref * ( r1 + r2 ) / r1;
            vout += r2 * iadj;
            break;
        }
    }
    else
    {   // Standard 4 terminal regulator
        switch( id )
        {
        case 0: r1 = ( vout / vref - 1 ) * r2;  break;
        case 1: r2 = r1 / ( vout / vref - 1 );  break;
        case 2: vout = vref * ( r1 + r2 ) / r2; break;
        }
    }

    // write values to panel:
    txt.Printf( wxT( "%g" ), r1 / r1scale );
    m_RegulR1Value->SetValue( txt );
    txt.Printf( wxT( "%g" ), r2 / r2scale );
    m_RegulR2Value->SetValue( txt );
    txt.Printf( wxT( "%g" ), vref );
    m_RegulVrefValue->SetValue( txt );
    txt.Printf( wxT( "%g" ), vout );
    m_RegulVoutValue->SetValue( txt );
}


void PCB_CALCULATOR_FRAME::Regulators_WriteConfig( PCB_CALCULATOR_SETTINGS* aCfg )
{
    // Save current parameter values in config.
    aCfg->m_Regulators.r1 = m_RegulR1Value->GetValue();
    aCfg->m_Regulators.r2 = m_RegulR2Value->GetValue();
    aCfg->m_Regulators.vref = m_RegulVrefValue->GetValue();
    aCfg->m_Regulators.vout = m_RegulVoutValue->GetValue();
    aCfg->m_Regulators.data_file = GetDataFilename();
    aCfg->m_Regulators.selected_regulator = m_lastSelectedRegulatorName;
    aCfg->m_Regulators.type = m_choiceRegType->GetSelection();

    // Store the parameter selection that was recently calculated (R1, R2 or Vout)
    wxRadioButton * regprms[3] =
    {
        m_rbRegulR1, m_rbRegulR2, m_rbRegulVout
    };

    for( int ii = 0; ii < 3; ii++ )
    {
        if( regprms[ii]->GetValue() )
        {
            aCfg->m_Regulators.last_param = ii;
            break;
        }
    }
}
