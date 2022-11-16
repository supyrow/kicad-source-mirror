/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef COMMAND_EXPORT_PCB_BASE_H
#define COMMAND_EXPORT_PCB_BASE_H

#include "command.h"
#include <layer_ids.h>

namespace CLI
{
#define ARG_OUTPUT "--output"
#define ARG_INPUT "input"
#define ARG_BLACKANDWHITE "--black-and-white"
#define ARG_LAYERS "--layers"
#define ARG_INCLUDE_REFDES "--include-refdes"
#define ARG_INCLUDE_VALUE "--include-value"
#define ARG_THEME "--theme"
#define ARG_INCLUDE_BORDER_TITLE "--include-border-title"

struct EXPORT_PCB_BASE_COMMAND : public COMMAND
{
    EXPORT_PCB_BASE_COMMAND( std::string aName );

    int Perform( KIWAY& aKiway ) override;

protected:
    LSET convertLayerStringList( wxString& aLayerString ) const;
    void addLayerArg( bool aRequire );

    std::map<std::string, LSET> m_layerMasks;
    LSET                        m_selectedLayers;

    bool m_requireLayers;
};
} // namespace CLI

#endif