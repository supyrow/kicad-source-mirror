/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers.
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

#include <common.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <pad.h>
#include <zone.h>

/*
    "Disallow" test. Goes through all items, matching types/conditions drop errors.
    Errors generated:
    - DRCE_ALLOWED_ITEMS
    - DRCE_TEXT_ON_EDGECUTS
*/

class DRC_TEST_PROVIDER_DISALLOW : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_DISALLOW()
    {
    }

    virtual ~DRC_TEST_PROVIDER_DISALLOW()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "disallow";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests for disallowed items (e.g. keepouts)";
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;
};


bool DRC_TEST_PROVIDER_DISALLOW::Run()
{
    if( !reportPhase( _( "Checking keepouts & disallow constraints..." ) ) )
        return false;   // DRC cancelled

    // Zones can be expensive (particularly when multi-layer), so we run the progress bar on them
    BOARD* board = m_drcEngine->GetBoard();
    int    boardZoneCount = board->Zones().size();
    int    delta = std::max( 1, boardZoneCount / board->GetCopperLayerCount() );
    int    ii = 0;

    auto doCheckItem =
            [&]( BOARD_ITEM* item )
            {
                auto constraint = m_drcEngine->EvalRules( DISALLOW_CONSTRAINT, item, nullptr,
                                                          UNDEFINED_LAYER );

                if( constraint.m_DisallowFlags )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ALLOWED_ITEMS );

                    m_msg.Printf( drcItem->GetErrorText() + wxS( " (%s)" ),
                                  constraint.GetName() );

                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, item->GetPosition() );
                }
            };

    auto checkItem =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_ON_EDGECUTS )
                        && item->GetLayer() == Edge_Cuts )
                {
                    switch( item->Type() )
                    {
                    case PCB_TEXT_T:
                    case PCB_DIM_ALIGNED_T:
                    case PCB_DIM_CENTER_T:
                    case PCB_DIM_ORTHOGONAL_T:
                    case PCB_DIM_LEADER_T:
                    {
                        std::shared_ptr<DRC_ITEM> drc = DRC_ITEM::Create( DRCE_TEXT_ON_EDGECUTS );
                        drc->SetItems( item );
                        reportViolation( drc, item->GetPosition() );
                    }
                        break;

                    default:
                        break;
                    }
                }

                if( m_drcEngine->IsErrorLimitExceeded( DRCE_ALLOWED_ITEMS ) )
                    return false;

                if( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T )
                {
                    ZONE* zone = static_cast<ZONE*>( item );

                    if( zone->GetIsRuleArea() )
                        return true;

                    if( item->Type() == PCB_ZONE_T )
                    {
                        if( !reportProgress( ii++, boardZoneCount, delta ) )
                            return false;   // DRC cancelled
                    }
                }

                item->ClearFlags( HOLE_PROXY );
                doCheckItem( item );

                bool hasHole;

                switch( item->Type() )
                {
                case PCB_VIA_T: hasHole = true;                                           break;
                case PCB_PAD_T: hasHole = static_cast<PAD*>( item )->GetDrillSizeX() > 0; break;
                default:        hasHole = false;                                          break;
                }

                if( hasHole )
                {
                    item->SetFlags( HOLE_PROXY );
                    doCheckItem( item );
                    item->ClearFlags( HOLE_PROXY );
                }

                return true;
            };

    forEachGeometryItem( {}, LSET::AllLayersMask(), checkItem );

    reportRuleStatistics();

    return true;
}


int DRC_TEST_PROVIDER_DISALLOW::GetNumPhases() const
{
    return 1;
}


std::set<DRC_CONSTRAINT_T> DRC_TEST_PROVIDER_DISALLOW::GetConstraintTypes() const
{
    return { DISALLOW_CONSTRAINT };
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_DISALLOW> dummy;
}
