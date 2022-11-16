/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file common_mocks.cpp
 * @brief Mock objects for unit tests
 */

#include <mock_kiface_base.h>
#include <mock_pgm_base.h>


PGM_BASE& Pgm()
{
    static MOCK_PGM_BASE program;
    return program;
}


// Similar to PGM_BASE& Pgm(), but return nullptr when a *.ki_face is run from
// a python script or something else.
// Therefore here return always nullptr
PGM_BASE* PgmOrNull()
{
    return nullptr;
}


KIFACE_BASE& Kiface()
{
    static MOCK_KIFACE_BASE kiface;
    return kiface;
}
