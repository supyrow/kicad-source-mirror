/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016-2022 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <zone.h>
#include "pns_node.h"
#include "pns_item.h"
#include "pns_line.h"
#include "pns_router.h"

typedef VECTOR2I::extended_type ecoord;

namespace PNS {

bool ITEM::collideSimple( const ITEM* aOther, const NODE* aNode, bool aDifferentNetsOnly, int aOverrideClearance ) const
{
    const ROUTER_IFACE* iface = ROUTER::GetInstance()->GetInterface();
    const SHAPE*        shapeA = Shape();
    const SHAPE*        holeA = Hole();
    int                 lineWidthA = 0;
    const SHAPE*        shapeB = aOther->Shape();
    const SHAPE*        holeB = aOther->Hole();
    int                 lineWidthB = 0;

    // Sadly collision routines ignore SHAPE_POLY_LINE widths so we have to pass them in as part
    // of the clearance value.
    if( m_kind == LINE_T )
        lineWidthA = static_cast<const LINE*>( this )->Width() / 2;

    if( aOther->m_kind == LINE_T )
        lineWidthB = static_cast<const LINE*>( aOther )->Width() / 2;

    // same nets? no collision!
    if( aDifferentNetsOnly && m_net == aOther->m_net && m_net >= 0 && aOther->m_net >= 0 )
        return false;

    // a pad associated with a "free" pin (NIC) doesn't have a net until it has been used
    if( aDifferentNetsOnly && ( IsFreePad() || aOther->IsFreePad() ) )
        return false;

    // check if we are not on completely different layers first
    if( !m_layers.Overlaps( aOther->m_layers ) )
        return false;

    auto checkKeepout =
            []( const ZONE* aKeepout, const BOARD_ITEM* aOther )
            {
                if( aKeepout->GetDoNotAllowTracks() && aOther->IsType( { PCB_ARC_T, PCB_TRACE_T } ) )
                    return true;

                if( aKeepout->GetDoNotAllowVias() && aOther->Type() == PCB_VIA_T )
                    return true;

                if( aKeepout->GetDoNotAllowPads() && aOther->Type() == PCB_PAD_T )
                    return true;

                // Incomplete test, but better than nothing:
                if( aKeepout->GetDoNotAllowFootprints() && aOther->Type() == PCB_PAD_T )
                {
                    return !aKeepout->GetParentFootprint()
                            || aKeepout->GetParentFootprint() != aOther->GetParentFootprint();
                }

                return false;
            };

    const ZONE* zoneA = dynamic_cast<ZONE*>( Parent() );
    const ZONE* zoneB = dynamic_cast<ZONE*>( aOther->Parent() );

    if( zoneA && aOther->Parent() && !checkKeepout( zoneA, aOther->Parent() ) )
        return false;

    if( zoneB && Parent() && !checkKeepout( zoneB, Parent() ) )
        return false;

    bool thisNotFlashed  = !iface->IsFlashedOnLayer( this, aOther->Layer() );
    bool otherNotFlashed = !iface->IsFlashedOnLayer( aOther, Layer() );

    if( ( aNode->GetCollisionQueryScope() == NODE::CQS_ALL_RULES
          || ( thisNotFlashed || otherNotFlashed ) )
        && ( holeA || holeB ) )
    {
        int holeClearance = aNode->GetHoleClearance( this, aOther );

        if( holeClearance >= 0 && holeA && holeA->Collide( shapeB, holeClearance + lineWidthB ) )
        {
            Mark( Marker() | MK_HOLE );
            return true;
        }

        if( holeB && holeClearance >= 0 && holeB->Collide( shapeA, holeClearance + lineWidthA ) )
        {
            aOther->Mark( aOther->Marker() | MK_HOLE );
            return true;
        }

        if( holeA && holeB )
        {
            int holeToHoleClearance = aNode->GetHoleToHoleClearance( this, aOther );

            if( holeToHoleClearance >= 0 && holeA->Collide( holeB, holeToHoleClearance ) )
            {
                Mark( Marker() | MK_HOLE );
                aOther->Mark( aOther->Marker() | MK_HOLE );
                return true;
            }
        }
    }

    if( !aOther->Layers().IsMultilayer() && thisNotFlashed )
        return false;

    if( !Layers().IsMultilayer() && otherNotFlashed )
        return false;

    int clearance = aOverrideClearance >= 0 ? aOverrideClearance : aNode->GetClearance( this, aOther );

    if( clearance >= 0 )
    {
        bool checkCastellation = ( m_parent && m_parent->GetLayer() == Edge_Cuts );
        bool checkNetTie = aNode->GetRuleResolver()->IsInNetTie( this );

        if( checkCastellation || checkNetTie )
        {
            // Slow method
            int      actual;
            VECTOR2I pos;

            if( shapeA->Collide( shapeB, clearance + lineWidthA, &actual, &pos ) )
            {
                if( checkCastellation && aNode->QueryEdgeExclusions( pos ) )
                    return false;

                if( checkNetTie && aNode->GetRuleResolver()->IsNetTieExclusion( aOther, pos, this ) )
                    return false;

                return true;
            }
        }
        else
        {
            // Fast method
            if( shapeA->Collide( shapeB, clearance + lineWidthA + lineWidthB ) )
                return true;
        }
    }

    return false;
}


bool ITEM::Collide( const ITEM* aOther, const NODE* aNode, bool aDifferentNetsOnly, int aOverrideClearance ) const
{
    if( collideSimple( aOther, aNode, aDifferentNetsOnly, aOverrideClearance ) )
        return true;

    // Special cases for "head" lines with vias attached at the end.  Note that this does not
    // support head-line-via to head-line-via collisions, but you can't route two independent
    // tracks at once so it shouldn't come up.

    if( m_kind == LINE_T )
    {
        const LINE* line = static_cast<const LINE*>( this );

        if( line->EndsWithVia() && line->Via().collideSimple( aOther, aNode, aDifferentNetsOnly, aOverrideClearance ) )
            return true;
    }

    if( aOther->m_kind == LINE_T )
    {
        const LINE* line = static_cast<const LINE*>( aOther );

        if( line->EndsWithVia() && line->Via().collideSimple( this, aNode, aDifferentNetsOnly, aOverrideClearance ) )
            return true;
    }

    return false;
}


std::string ITEM::KindStr() const
{
    switch( m_kind )
    {
    case ARC_T:       return "arc";
    case LINE_T:      return "line";
    case SEGMENT_T:   return "segment";
    case VIA_T:       return "via";
    case JOINT_T:     return "joint";
    case SOLID_T:     return "solid";
    case DIFF_PAIR_T: return "diff-pair";
    default:          return "unknown";
    }
}


ITEM::~ITEM()
{
}

const std::string ITEM::Format() const
{
    std::stringstream ss;
    ss << KindStr() << " ";
    ss << "net " << m_net << " ";
    ss << "layers " << m_layers.Start() << " " << m_layers.End();
    return ss.str();
}

}
