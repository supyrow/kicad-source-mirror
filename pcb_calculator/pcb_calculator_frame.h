/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
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

#ifndef PCB_CALCULATOR_FRAME_H_
#define PCB_CALCULATOR_FRAME_H_

#include <calculator_panels/calculator_panel.h>
#include <kiway_player.h>

class wxMenuBar;
class wxNotebook;
class wxBoxSizer;

class APP_SETTINGS_BASE;
class KIWAY;
class PANEL_TRANSLINE;


/**
 * PCB calculator the main frame.
 */
class PCB_CALCULATOR_FRAME : public KIWAY_PLAYER
{
public:
    PCB_CALCULATOR_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~PCB_CALCULATOR_FRAME();

    // Pcb calculator doesn't host a tool framework
    wxWindow* GetToolCanvas() const override
    {
        return nullptr;
    }

    /*
     * Return the panel of given type or nullptr if there is no such panel exists.
     */
    template<typename T>
    T* GetCalculator()
    {
        std::map<std::size_t, CALCULATOR_PANEL*>::iterator panel = m_panelTypes.find( typeid( T ).hash_code() );

        if( panel != m_panelTypes.end() )
            return static_cast<T*>( panel->second );

        return nullptr;
    }

    void AddCalculator( CALCULATOR_PANEL *aPanel, const wxString& panelUIName );

private:
    // Event handlers
    void OnClosePcbCalc( wxCloseEvent& event );

    void OnUpdateUI( wxUpdateUIEvent& event );

    void onThemeChanged( wxSysColourChangedEvent& aEvent );

    // Config read-write, virtual from EDA_BASE_FRAME
    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

private:
    wxMenuBar*  m_menubar;
    wxNotebook* m_notebook;
    wxBoxSizer* m_mainSizer;

    int         m_lastNotebookPage;
    bool        m_macHack;

    std::vector<CALCULATOR_PANEL*>           m_panels;
    std::map<std::size_t, CALCULATOR_PANEL*> m_panelTypes;


};


extern const wxString DataFileNameExt;

#endif // PCB_CALCULATOR_H
