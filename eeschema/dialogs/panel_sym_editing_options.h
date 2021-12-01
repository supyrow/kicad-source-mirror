/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_SYM_EDITING_OPTIONS_H
#define PANEL_SYM_EDITING_OPTIONS_H

#include <widgets/unit_binder.h>
#include "panel_sym_editing_options_base.h"

class SYMBOL_EDIT_FRAME;


class PANEL_SYM_EDITING_OPTIONS : public PANEL_SYM_EDITING_OPTIONS_BASE
{
public:
    PANEL_SYM_EDITING_OPTIONS( SYMBOL_EDIT_FRAME* aFrame, wxWindow* aWindow );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    SYMBOL_EDIT_FRAME* m_frame;

    UNIT_BINDER        m_lineWidth;
    UNIT_BINDER        m_textSize;

    UNIT_BINDER        m_pinLength;
    UNIT_BINDER        m_pinNameSize;
    UNIT_BINDER        m_pinNumberSize;
};


#endif //PANEL_SYM_EDITING_OPTIONS_H
