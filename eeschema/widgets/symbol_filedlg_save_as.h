/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SYMBOL_FILEDLG_SAVE_AS_
#define SYMBOL_FILEDLG_SAVE_AS_

#include <symbol_editor/symbol_saveas_type.h>
#include <wx/filedlgcustomize.h>

class SYMBOL_FILEDLG_SAVE_AS : public wxFileDialogCustomizeHook
{
public:
    SYMBOL_FILEDLG_SAVE_AS( SYMBOL_SAVEAS_TYPE aOption ) : m_option( aOption ){};

    virtual void AddCustomControls( wxFileDialogCustomize& customizer ) override
    {
        m_simpleSaveAs = customizer.AddRadioButton( _( "Do not update library tables" ) );
        m_replaceTableEntry =
                customizer.AddRadioButton( _( "Update existing library table entry" ) );
        m_addGlobalTableEntry =
                customizer.AddRadioButton( _( "Add new global library table entry" ) );
        m_addProjectTableEntry =
                customizer.AddRadioButton(  _( "Add new project library table entry" ) );

        // Note, due to windows api, wx does not actually support calling SetValue( false ) (it asserts)
        if( m_option == SYMBOL_SAVEAS_TYPE::NORMAL_SAVE_AS )
            m_simpleSaveAs->SetValue( true );

        if( m_option == SYMBOL_SAVEAS_TYPE::REPLACE_TABLE_ENTRY )
            m_replaceTableEntry->SetValue( true );

        if( m_option == SYMBOL_SAVEAS_TYPE::ADD_GLOBAL_TABLE_ENTRY )
            m_addGlobalTableEntry->SetValue( true );

        if( m_option == SYMBOL_SAVEAS_TYPE::ADD_PROJECT_TABLE_ENTRY )
            m_addProjectTableEntry->SetValue( true );
    }

    virtual void TransferDataFromCustomControls() override
    {
        if( m_replaceTableEntry->GetValue() )
            m_option = SYMBOL_SAVEAS_TYPE::REPLACE_TABLE_ENTRY;
        else if( m_addGlobalTableEntry->GetValue() )
            m_option = SYMBOL_SAVEAS_TYPE::ADD_GLOBAL_TABLE_ENTRY;
        else if( m_addProjectTableEntry->GetValue() )
            m_option = SYMBOL_SAVEAS_TYPE::ADD_PROJECT_TABLE_ENTRY;
        else
            m_option = SYMBOL_SAVEAS_TYPE::NORMAL_SAVE_AS;
    }

    SYMBOL_SAVEAS_TYPE GetOption() const { return m_option; }

private:
    SYMBOL_SAVEAS_TYPE m_option;

    wxFileDialogRadioButton* m_simpleSaveAs;
    wxFileDialogRadioButton* m_replaceTableEntry;
    wxFileDialogRadioButton* m_addGlobalTableEntry;
    wxFileDialogRadioButton* m_addProjectTableEntry;

    wxDECLARE_NO_COPY_CLASS( SYMBOL_FILEDLG_SAVE_AS );
};

#endif