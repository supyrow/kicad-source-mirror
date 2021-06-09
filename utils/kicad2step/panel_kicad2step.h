/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file panel_kicad2step.h
 * Declare the main PCB object.
 */

#ifndef PANEL_KICAD2STEP_H
#define PANEL_KICAD2STEP_H

#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/panel.h>

class KICAD2MCAD_PRMS       // A small class to handle parameters of conversion
{
public:
    KICAD2MCAD_PRMS();

    ///< Return file extension for the selected output format
    wxString getOutputExt() const;

#ifdef SUPPORTS_IGES
    bool     m_fmtIGES;
#endif
    bool     m_overwrite;
    bool     m_useGridOrigin;
    bool     m_useDrillOrigin;
    bool     m_includeVirtual;
    bool     m_substModels;
    wxString m_filename;
    wxString m_outputFile;
    double   m_xOrigin;
    double   m_yOrigin;
    double   m_minDistance;

};

class PANEL_KICAD2STEP: public wxPanel
{
public:
    PANEL_KICAD2STEP( wxWindow* parent, wxWindowID id = wxID_ANY,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxSize( 500,300 ),
                      long style = wxTAB_TRAVERSAL );

    /**
     * Run the KiCad to STEP converter.
     */
    int RunConverter();

    /**
     * Add a message to m_tcMessages.
     */
    void AppendMessage( const wxString& aMessage );

    KICAD2MCAD_PRMS m_params;

private:
    wxTextCtrl* m_tcMessages;
};

#endif      // #ifndef PANEL_KICAD2STEP_H
