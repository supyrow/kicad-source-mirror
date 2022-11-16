/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef TEARDROP_TYPES_H
#define TEARDROP_TYPES_H

/**
 * define the type of a teardrop: on a via or pad, or a track end
 */
enum class TEARDROP_TYPE
{
    TD_NONE = 0,        // Not a teardrop: just a standard zone
    TD_UNSPECIFIED,     // Not specified/unknown teardrop type
    TD_VIAPAD,          // a teardrop on a via or pad
    TD_TRACKEND         // a teardrop on a track end
                        // (when 2 tracks having different widths have a teardrop on the
                        // end of the largest track)
};


#endif  // ifndef TEARDROP_TYPES_H
