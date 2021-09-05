/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef WX_PROGRESS_REPORTERS_H
#define WX_PROGRESS_REPORTERS_H

#include <wx/progdlg.h>
#include <wx/gauge.h>
#if wxCHECK_VERSION( 3, 1, 0 )
#include <wx/appprogress.h>
#endif

#include <widgets/progress_reporter_base.h>

/**
 * Multi-thread safe progress reporter dialog, intended for use of tasks that parallel reporting
 * back of work status.
 *
 * @see PROGRESS_REPORTER.
 */
class WX_PROGRESS_REPORTER : public PROGRESS_REPORTER_BASE, public wxProgressDialog
{
public:
    /**
     * The #PROGRESS_REPORTER will stay on top of \a aParent.
     *
     * The style is wxPD_AUTO_HIDE | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME.
     *
     * @param aParent is the wxDialog of Frame that manage this.
     * @param aTitle is the dialog progress title
     * @param aNumPhases is the number of "virtual sections" of the progress bar
     *   aNumPhases = 1 is the usual progress bar
     *   aNumPhases = n creates n virtual progress bar zones: a 0 to 100 percent width
     *   of a virtual zone fills 0 to 1/n progress bar full size of the nth virtual zone index
     * @param aCanAbort is true if the abort button should be shown
     * @param aReserveSpaceForMessage will ensure that the dialog is laid out for status messages,
     *        preventing layout issues on Windows when reporting a message after the initial layout
     */
    WX_PROGRESS_REPORTER( wxWindow* aParent, const wxString& aTitle, int aNumPhases,
                          bool aCanAbort = true, bool aReserveSpaceForMessage = true );
    ~WX_PROGRESS_REPORTER();

    /**
     * Change the title displayed on the window caption.
     */
    void SetTitle( const wxString& aTitle ) override
    {
        wxProgressDialog::SetTitle( aTitle );
    }

private:
#if wxCHECK_VERSION( 3, 1, 0 )
    wxAppProgressIndicator m_appProgressIndicator;
#endif

    bool updateUI() override;
};


class GAUGE_PROGRESS_REPORTER : public PROGRESS_REPORTER_BASE, public wxGauge
{
public:
    /**
     * @param aParent is the parent of the wxGauge control
     * @param aNumPhases is the number of "virtual sections" of the progress bar
     *   aNumPhases = 1 is the usual progress bar
     *   aNumPhases = n creates n virtual progress bar zones: a 0 to 100 percent width
     *   of a virtual zone fills 0 to 1/n progress bar full size of the nth virtual zone index
     */
    GAUGE_PROGRESS_REPORTER( wxWindow* aParent, int aNumPhases );

private:
    bool updateUI() override;
};

#endif
