/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.TXT for contributors.
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
 * @file test_altium_pcblib_import.cpp
 * Test suite for import of *.PcbLib libraries
 */

#include <board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/plugins/altium/altium_designer_plugin.h>
#include <pcbnew/plugins/kicad/pcb_plugin.h>

#include <footprint.h>
#include <fp_shape.h>
#include <fp_text.h>
#include <pad.h>
#include <zone.h>


#define CHECK_ENUM_CLASS_EQUAL( L, R )                                                             \
    BOOST_CHECK_EQUAL( static_cast<int>( L ), static_cast<int>( R ) )


struct ALTIUM_PCBLIB_IMPORT_FIXTURE
{
    ALTIUM_PCBLIB_IMPORT_FIXTURE() {}

    ALTIUM_DESIGNER_PLUGIN altiumPlugin;
    PCB_PLUGIN             kicadPlugin;
};


/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( AltiumPcbLibImport, ALTIUM_PCBLIB_IMPORT_FIXTURE )


/**
 * Compare all footprints declared in a *.PcbLib file with their KiCad reference footprint
 */
BOOST_AUTO_TEST_CASE( AltiumPcbLibImport )
{
    std::vector<std::pair<wxString, wxString>> tests = {
        { "TracksTest.PcbLib", "TracksTest.pretty" },
        { "Espressif ESP32-WROOM-32.PcbLib", "Espressif ESP32-WROOM-32.pretty" }
    };

    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/altium/pcblib/";

    for( const std::pair<wxString, wxString>& libName : tests )
    {
        wxString altiumLibraryPath = dataPath + libName.first;
        wxString kicadLibraryPath = dataPath + libName.second;

        wxArrayString altiumFootprintNames;
        altiumPlugin.FootprintEnumerate( altiumFootprintNames, altiumLibraryPath, true, nullptr );
        wxArrayString kicadFootprintNames;
        kicadPlugin.FootprintEnumerate( kicadFootprintNames, kicadLibraryPath, true, nullptr );
        BOOST_CHECK_EQUAL( altiumFootprintNames.GetCount(), kicadFootprintNames.GetCount() );

        for( size_t i = 0; i < altiumFootprintNames.GetCount(); i++ )
        {
            wxString footprintName = altiumFootprintNames[i];

            BOOST_TEST_CONTEXT( wxString::Format( wxT( "Import '%s' from '%s'" ), footprintName,
                                                  libName.first ) )
            {
                FOOTPRINT* altiumFp = altiumPlugin.FootprintLoad( altiumLibraryPath, footprintName,
                                                                  false, nullptr );
                BOOST_CHECK( altiumFp );

                BOOST_CHECK_EQUAL( wxT( "REF**" ), altiumFp->GetReference() );
                BOOST_CHECK_EQUAL( footprintName, altiumFp->GetValue() );

                FOOTPRINT* kicadFp = kicadPlugin.FootprintLoad( kicadLibraryPath, footprintName,
                                                                false, nullptr );
                BOOST_CHECK( kicadFp );

                KI_TEST::CheckFootprint( kicadFp, altiumFp );
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
