/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Mikołaj Wielgus <wielgusmikolaj@gmail.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __SIM_WORKBOOK__
#define __SIM_WORKBOOK__

#include <dialog_sim_settings.h>
#include <sim/sim_panel_base.h>
#include <sim/sim_plot_panel.h>


class SIM_WORKBOOK : public wxAuiNotebook
{
public:
    SIM_WORKBOOK();
    SIM_WORKBOOK( wxWindow* aParent, wxWindowID aId=wxID_ANY, const wxPoint&
            aPos=wxDefaultPosition, const wxSize& aSize=wxDefaultSize, long
            aStyle=wxAUI_NB_DEFAULT_STYLE );

    // Methods from wxAuiNotebook
    
    bool AddPage( wxWindow* aPage, const wxString& aCaption, bool aSelect=false, const wxBitmap& aBitmap=wxNullBitmap );
    bool AddPage( wxWindow* aPage, const wxString& aText, bool aSelect, int aImageId ) override;

    bool DeleteAllPages() override; 
    bool DeletePage( size_t aPage ) override;

    // Custom methods

    bool AddTrace( SIM_PLOT_PANEL* aPlotPanel, const wxString& aTitle, const wxString& aName,
                   int aPoints, const double* aX, const double* aY, SIM_PLOT_TYPE aType,
                   const wxString& aParam );
    bool DeleteTrace( SIM_PLOT_PANEL* aPlotPanel, const wxString& aName );
    
    void SetSimCommand( SIM_PANEL_BASE* aPlotPanel, const wxString& aSimCommand )
    {
        aPlotPanel->setSimCommand( aSimCommand );
        setModified();
    }

    const wxString& GetSimCommand( const SIM_PANEL_BASE* aPlotPanel )
    {
        return aPlotPanel->getSimCommand();
    }

    void ClrModified();
    bool IsModified() const { return m_modified; }

private:
    void setModified( bool value = true );

    ///< Dirty bit, indicates something in the workbook has changed
    bool m_modified;
};

wxDECLARE_EVENT( EVT_WORKBOOK_MODIFIED, wxCommandEvent );
wxDECLARE_EVENT( EVT_WORKBOOK_CLR_MODIFIED, wxCommandEvent );

#endif // __SIM_WORKBOOK__
