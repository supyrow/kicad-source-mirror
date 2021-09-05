/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_PCM_PROGRESS_H_
#define DIALOG_PCM_PROGRESS_H_


#include "dialog_pcm_progress_base.h"
#include "reporter.h"
#include <atomic>
#if wxCHECK_VERSION( 3, 1, 0 )
#include <wx/appprogress.h>
#endif


/**
 * @brief Progress dialog for PCM system
 *
 * This dialog is designed to work with PCM_TASK_MANAGER's threading system.
 * Some of it's methods are safe to call from a non-UI thread.
 *
 * SetOverallProgress(), SetOverallProgressPhases() and AdvanceOverallProgressPhase()
 * are meant to be called from the same thread.
 */
class DIALOG_PCM_PROGRESS : public DIALOG_PCM_PROGRESS_BASE
{
protected:
    // Handlers for DIALOG_PCM_PROGRESS_BASE events.
    void OnCancelClicked( wxCommandEvent& event ) override;
    void OnCloseClicked( wxCommandEvent& event ) override;

public:
    /** Constructor */
    DIALOG_PCM_PROGRESS( wxWindow* parent, bool aShowDownloadSection = true );

    ///< Thread safe. Adds a message to detailed report window.
    void Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED );

    ///< Thread safe. Sets current download progress gauge and text.
    void SetDownloadProgress( uint64_t aDownloaded, uint64_t aTotal );

    ///< Safe to call from non-UI thread. Sets current overall progress gauge.
    void SetOverallProgress( uint64_t aProgress, uint64_t aTotal );

    ///< Safe to call from non-UI thread. Sets number of phases for overall progress gauge.
    void SetOverallProgressPhases( int aPhases );

    ///< Safe to call from non-UI thread. Increments current phase by one.
    void AdvanceOverallProgressPhase();

    ///< Thread safe. Updates download text.
    void SetDownloadsFinished();

    ///< Thread safe. Disables cancel button, enables close button.
    void SetFinished();

    ///< Thread safe. Return true if cancel was clicked.
    bool IsCancelled();

private:
    static uint64_t  toKb( uint64_t aValue );
    int              m_currentPhase;
    int              m_overallPhases;
    std::atomic_bool m_cancelled;
#if wxCHECK_VERSION( 3, 1, 0 )
    wxAppProgressIndicator m_appProgressIndicator;
#endif
};

#endif // DIALOG_PCM_PROGRESS_H_
