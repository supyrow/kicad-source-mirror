/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_CONSTRAINTS_REPORTER_H
#define DIALOG_CONSTRAINTS_REPORTER_H

#include <dialog_constraints_reporter_base.h>

class PCB_EDIT_FRAME;
class WX_HTML_REPORT_BOX;
class wxHtmlLinkEvent;

class DIALOG_CONSTRAINTS_REPORTER : public DIALOG_CONSTRAINTS_REPORTER_BASE
{
public:
    DIALOG_CONSTRAINTS_REPORTER( PCB_EDIT_FRAME* aParent );

    void FinishInitialization();

    void OnOK( wxCommandEvent& event ) override
    {
        Close();
    }

    void OnErrorLinkClicked( wxHtmlLinkEvent& event );

    void DeleteAllPages();

    WX_HTML_REPORT_BOX* AddPage( const wxString& pageTitle );

    int GetPageCount() const;

protected:
    PCB_EDIT_FRAME* m_frame;
};

#endif // DIALOG_CONSTRAINTS_REPORTER_H
