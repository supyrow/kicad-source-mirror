/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <qa/pcbnew/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_marker.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>


struct DRC_REGRESSION_TEST_FIXTURE
{
    DRC_REGRESSION_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCCustomRuleSeverityTest, DRC_REGRESSION_TEST_FIXTURE )
{
    // This board has two edge-connectors.  There is a custom DRC rule which conditionally
    // applies to one of them and sets the edge-clearance severity to "ignore".  It should
    // therefore only generate edge-clearance violations for the other edge connector.

    KI_TEST::LoadBoard( m_settingsManager, "severities", m_board );
    KI_TEST::FillZones( m_board.get(), 6 );

    std::vector<DRC_ITEM>  violations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )
            {
                PCB_MARKER temp( aItem, aPos );

                if( bds.m_DrcExclusions.find( temp.Serialize() ) == bds.m_DrcExclusions.end() )
                    violations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MILLIMETRES, true, false );

    if( violations.size() == 8 )
    {
        BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
        BOOST_TEST_MESSAGE( "Custom rule severity test passed" );
    }
    else
    {
        BOOST_CHECK_EQUAL( violations.size(), 8 );

        std::map<KIID, EDA_ITEM*> itemMap;
        m_board->FillItemMap( itemMap );

        for( const DRC_ITEM& item : violations )
            BOOST_TEST_MESSAGE( item.ShowReport( EDA_UNITS::INCHES, RPT_SEVERITY_ERROR, itemMap ) );

        BOOST_ERROR( "Custom rule severity test failed" );
    }
}
