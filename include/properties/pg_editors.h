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

#ifndef KICAD_PG_EDITORS_H
#define KICAD_PG_EDITORS_H

#include <memory>

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/editors.h>

class EDA_DRAW_FRAME;
class PROPERTY_EDITOR_UNIT_BINDER;

class PG_UNIT_EDITOR : public wxPGTextCtrlEditor
{
public:
    static const wxString EDITOR_NAME;

    PG_UNIT_EDITOR( EDA_DRAW_FRAME* aFrame );

    virtual ~PG_UNIT_EDITOR();

    wxString GetName() const override { return EDITOR_NAME; }

    wxPGWindowList CreateControls( wxPropertyGrid* aPropGrid, wxPGProperty* aProperty,
                                   const wxPoint& aPos, const wxSize& aSize ) const override;

    bool GetValueFromControl( wxVariant& aVariant, wxPGProperty* aProperty,
                              wxWindow* aCtrl ) const override;

    void UpdateControl( wxPGProperty* aProperty, wxWindow* aCtrl ) const override;

    bool OnEvent( wxPropertyGrid* aPropGrid, wxPGProperty* aProperty, wxWindow* aCtrl,
                  wxEvent& aEvent ) const override;

    /**
     * When restarting an editor, the instance of PG_UNIT_EDITOR may be the same
     * but the referenced frame is different.  This re-binds the frame to the editor
     * and associated controls
     * @param aFrame New frame to bind
     */
    void UpdateFrame( EDA_DRAW_FRAME* aFrame );

protected:
    EDA_DRAW_FRAME* m_frame;

    std::unique_ptr<PROPERTY_EDITOR_UNIT_BINDER> m_unitBinder;
};

#endif //KICAD_PG_EDITORS_H
