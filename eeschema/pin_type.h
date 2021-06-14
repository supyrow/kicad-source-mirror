/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
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

#ifndef PIN_TYPE_H_
#define PIN_TYPE_H_

#include <wx/arrstr.h>
#include <vector>

enum class BITMAPS : unsigned int;

/**
 * The symbol library pin object electrical types used in ERC tests.
 */
enum class ELECTRICAL_PINTYPE
{
    PT_INPUT,         ///< usual pin input: must be connected
    PT_OUTPUT,        ///< usual output
    PT_BIDI,          ///< input or output (like port for a microprocessor)
    PT_TRISTATE,      ///< tris state bus pin
    PT_PASSIVE,       ///< pin for passive symbols: must be connected, and can be connected to any pin
    PT_NIC,           ///< not internally connected (may be connected to anything)
    PT_UNSPECIFIED,   ///< unknown electrical properties: creates always a warning when connected
    PT_POWER_IN,      ///< power input (GND, VCC for ICs). Must be connected to a power output.
    PT_POWER_OUT,     ///< output of a regulator: intended to be connected to power input pins
    PT_OPENCOLLECTOR, ///< pin type open collector
    PT_OPENEMITTER,   ///< pin type open emitter
    PT_NC,            ///< not connected (must be left open)

    PT_LAST_OPTION = PT_NC ///< sentinel value, set to last usable enum option
};

#define ELECTRICAL_PINTYPES_TOTAL ( static_cast<int>( ELECTRICAL_PINTYPE::PT_LAST_OPTION ) + 1 )

enum class GRAPHIC_PINSHAPE
{
    LINE,
    INVERTED,
    CLOCK,
    INVERTED_CLOCK,
    INPUT_LOW,
    CLOCK_LOW,
    OUTPUT_LOW,
    FALLING_EDGE_CLOCK,
    NONLOGIC,

    LAST_OPTION = NONLOGIC ///< this is the sentinel value, must be set to last enum value
};

#define GRAPHIC_PINSHAPES_TOTAL ( static_cast<int>( GRAPHIC_PINSHAPE::LAST_OPTION ) + 1 )


// UI

wxString PinShapeGetText( GRAPHIC_PINSHAPE shape );
BITMAPS PinShapeGetBitmap( GRAPHIC_PINSHAPE shape );

wxString ElectricalPinTypeGetText( ELECTRICAL_PINTYPE );
BITMAPS ElectricalPinTypeGetBitmap( ELECTRICAL_PINTYPE );

wxString PinOrientationName( unsigned aPinOrientationCode );
int PinOrientationCode( int index );
int PinOrientationIndex( int code );

const wxArrayString& PinTypeNames();
const std::vector<BITMAPS>& PinTypeIcons();

const wxArrayString& PinShapeNames();
const std::vector<BITMAPS>& PinShapeIcons();

const wxArrayString& PinOrientationNames();
const std::vector<BITMAPS>& PinOrientationIcons();

#endif
