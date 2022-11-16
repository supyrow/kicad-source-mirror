/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SIM_PROPERTY_H
#define SIM_PROPERTY_H

#include <sim/sim_model.h>
#include <wx/window.h>
#include <wx/notebook.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/manager.h>
#include <wx/propgrid/props.h>


class SIM_VALIDATOR : public wxValidator
{
private:
    void navigate( int flags );

    void onKeyDown( wxKeyEvent& aEvent );

    wxDECLARE_EVENT_TABLE();
};


class SIM_STRING_VALIDATOR : public SIM_VALIDATOR
{
public:
    SIM_STRING_VALIDATOR( SIM_VALUE::TYPE aValueType, SIM_VALUE_GRAMMAR::NOTATION aNotation );
    SIM_STRING_VALIDATOR( const SIM_STRING_VALIDATOR& aValidator ) = default;

    wxObject* Clone() const override;

    bool Validate( wxWindow* aParent ) override;
    bool TransferToWindow() override;
    bool TransferFromWindow() override;

private:
    bool isValid( const wxString& aString );

    wxTextEntry* getTextEntry();

    SIM_VALUE::TYPE             m_valueType;
    SIM_VALUE_GRAMMAR::NOTATION m_notation;
};


class SIM_BOOL_VALIDATOR : public SIM_VALIDATOR
{
public:
    SIM_BOOL_VALIDATOR() : SIM_VALIDATOR() {}
    SIM_BOOL_VALIDATOR( const SIM_BOOL_VALIDATOR& aValidator ) = default;

    bool Validate( wxWindow* aParent ) override;

private:
    wxDECLARE_EVENT_TABLE();
};


class SIM_PROPERTY
{
public:
    SIM_PROPERTY( SIM_MODEL& aModel, int aParamIndex );

    void Disable();

    const SIM_MODEL::PARAM& GetParam() const { return m_model.GetParam( m_paramIndex ); }

protected:
    SIM_MODEL&                   m_model;
    int                          m_paramIndex;
    bool                         m_disabled; // If true, never access the models.
};


class SIM_BOOL_PROPERTY : public wxBoolProperty, public SIM_PROPERTY
{
public:
    SIM_BOOL_PROPERTY( const wxString& aLabel, const wxString& aName,
                       SIM_MODEL& aModel,
                       int aParamIndex );

    wxValidator* DoGetValidator() const override;

    void OnSetValue() override;
};


class SIM_STRING_PROPERTY : public wxStringProperty, public SIM_PROPERTY
{
public:
    // We pass shared_ptrs because we need to make sure they are destroyed only after the last time
    // SIM_PROPERTY uses them.
    SIM_STRING_PROPERTY( const wxString& aLabel, const wxString& aName,
                         SIM_MODEL& aModel,
                         int aParamIndex,
                         SIM_VALUE::TYPE aValueType = SIM_VALUE::TYPE_FLOAT,
                         SIM_VALUE_GRAMMAR::NOTATION aNotation = SIM_VALUE_GRAMMAR::NOTATION::SI );

    wxValidator* DoGetValidator() const override;

    bool StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags = 0 )
        const override;

protected:
    SIM_VALUE::TYPE              m_valueType;
    SIM_VALUE_GRAMMAR::NOTATION  m_notation;
};


class SIM_ENUM_PROPERTY : public wxEnumProperty, public SIM_PROPERTY
{
public:
    SIM_ENUM_PROPERTY( const wxString& aLabel, const wxString& aName,
                       SIM_MODEL& aModel,
                       int aParamIndex,
                       SIM_VALUE::TYPE aValueType = SIM_VALUE::TYPE_FLOAT,
                       SIM_VALUE_GRAMMAR::NOTATION aNotation = SIM_VALUE_GRAMMAR::NOTATION::SI );

    bool IntToValue( wxVariant& aVariant, int aNumber, int aArgFlags = 0 ) const override;
};

#endif // SIM_PROPERTY_H
