/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <cstdio>
#include <memory>
#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_rtree.h>
#include <pcb_track.h>
#include <pcb_group.h>
#include <geometry/shape_segment.h>
#include <pcb_expr_evaluator.h>
#include <wx/log.h>

#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <connectivity/from_to_cache.h>

#include <drc/drc_engine.h>

bool fromToFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    LIBEVAL::VALUE*   argTo = aCtx->Pop();
    LIBEVAL::VALUE*   argFrom = aCtx->Pop();

    result->Set(0.0);
    aCtx->Push( result );

    if(!item)
        return false;

    auto ftCache = item->GetBoard()->GetConnectivity()->GetFromToCache();

    if( !ftCache )
    {
        wxLogWarning( wxT( "Attempting to call fromTo() with non-existent from-to cache." ) );
        return true;
    }

    if( ftCache->IsOnFromToPath( static_cast<BOARD_CONNECTED_ITEM*>( item ),
                                 argFrom->AsString(), argTo->AsString() ) )
    {
        result->Set(1.0);
    }

    return true;
}


#define MISSING_LAYER_ARG( f ) wxString::Format( _( "Missing layer name argument to %s." ), f )

static void existsOnLayerFunc( LIBEVAL::CONTEXT* aCtx, void *self )
{
    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;

    LIBEVAL::VALUE*   arg = aCtx->Pop();
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !item )
        return;

    if( !arg )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_LAYER_ARG( wxT( "existsOnLayer()" ) ) );

        return;
    }

    result->SetDeferredEval(
            [item, arg, aCtx]() -> double
            {
                const wxString& layerName = arg->AsString();
                wxPGChoices& layerMap = ENUM_MAP<PCB_LAYER_ID>::Instance().Choices();

                if( aCtx->HasErrorCallback())
                {
                    /*
                     * Interpreted version
                     */

                    bool anyMatch = false;

                    for( unsigned ii = 0; ii < layerMap.GetCount(); ++ii )
                    {
                        wxPGChoiceEntry& entry = layerMap[ ii ];

                        if( entry.GetText().Matches( layerName ))
                        {
                            anyMatch = true;

                            if( item->IsOnLayer( ToLAYER_ID( entry.GetValue())))
                                return 1.0;
                        }
                    }

                    if( !anyMatch )
                    {
                        aCtx->ReportError( wxString::Format( _( "Unrecognized layer '%s'" ),
                                                             layerName ) );
                    }
                }
                else
                {
                    /*
                     * Compiled version
                     */

                    BOARD* board = item->GetBoard();
                    std::unique_lock<std::mutex> cacheLock( board->m_CachesMutex );
                    auto i = board->m_LayerExpressionCache.find( layerName );
                    LSET mask;

                    if( i == board->m_LayerExpressionCache.end() )
                    {
                        for( unsigned ii = 0; ii < layerMap.GetCount(); ++ii )
                        {
                            wxPGChoiceEntry& entry = layerMap[ ii ];

                            if( entry.GetText().Matches( layerName ) )
                                mask.set( ToLAYER_ID( entry.GetValue() ) );
                        }

                        board->m_LayerExpressionCache[ layerName ] = mask;
                    }
                    else
                    {
                        mask = i->second;
                    }

                    if( ( item->GetLayerSet() & mask ).any() )
                        return 1.0;
                }

                return 0.0;
            } );
}


static void isPlatedFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( !item )
        return;

    if( item->Type() == PCB_PAD_T && static_cast<PAD*>( item )->GetAttribute() == PAD_ATTRIB::PTH )
        result->Set( 1.0 );
    else if( item->Type() == PCB_VIA_T )
        result->Set( 1.0 );
}


bool collidesWithCourtyard( BOARD_ITEM* aItem, std::shared_ptr<SHAPE>& aItemShape,
                            PCB_EXPR_CONTEXT* aCtx, FOOTPRINT* aFootprint, PCB_LAYER_ID aSide )
{
    SHAPE_POLY_SET footprintCourtyard;

    footprintCourtyard = aFootprint->GetCourtyard( aSide );

    if( !aItemShape )
    {
        // Since rules are used for zone filling we can't rely on the filled shapes.
        // Use the zone outline instead.
        if( ZONE* zone = dynamic_cast<ZONE*>( aItem ) )
            aItemShape.reset( zone->Outline()->Clone() );
        else
            aItemShape = aItem->GetEffectiveShape( aCtx->GetLayer() );
    }

    return footprintCourtyard.Collide( aItemShape.get() );
};


static bool searchFootprints( BOARD* aBoard, const wxString& aArg, PCB_EXPR_CONTEXT* aCtx,
                              std::function<bool( FOOTPRINT* )> aFunc )
{
    if( aArg == wxT( "A" ) )
    {
        FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aCtx->GetItem( 0 ) );

        if( fp && aFunc( fp ) )
            return 1.0;
    }
    else if( aArg == wxT( "B" ) )
    {
        FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aCtx->GetItem( 1 ) );

        if( fp && aFunc( fp ) )
            return 1.0;
    }
    else for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( fp->GetReference().Matches( aArg ) )
        {
            if( aFunc( fp ) )
                return 1.0;
        }
    }

    return 0.0;
}


#define MISSING_FP_ARG( f ) \
    wxString::Format( _( "Missing footprint argument (A, B, or reference designator) to %s." ), f )

static void intersectsCourtyardFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_CONTEXT* context = static_cast<PCB_EXPR_CONTEXT*>( aCtx );
    LIBEVAL::VALUE*   arg = context->Pop();
    LIBEVAL::VALUE*   result = context->AllocValue();

    result->Set( 0.0 );
    context->Push( result );

    if( !arg )
    {
        if( context->HasErrorCallback() )
            context->ReportError( MISSING_FP_ARG( wxT( "intersectsCourtyard()" ) ) );

        return;
    }

    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( context ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg, context]() -> double
            {
                BOARD*                 board = item->GetBoard();
                std::shared_ptr<SHAPE> itemShape;

                if( searchFootprints( board, arg->AsString(), context,
                        [&]( FOOTPRINT* fp )
                        {
                            PTR_PTR_CACHE_KEY            key = { fp, item };
                            std::unique_lock<std::mutex> cacheLock( board->m_CachesMutex );

                            auto i = board->m_IntersectsCourtyardCache.find( key );

                            if( i != board->m_IntersectsCourtyardCache.end() )
                                return i->second;

                            bool res = collidesWithCourtyard( item, itemShape, context, fp, F_Cu )
                                    || collidesWithCourtyard( item, itemShape, context, fp, B_Cu );

                            board->m_IntersectsCourtyardCache[ key ] = res;
                            return res;
                        } ) )
                {
                    return 1.0;
                }

                return 0.0;
            } );
}


static void intersectsFrontCourtyardFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_CONTEXT* context = static_cast<PCB_EXPR_CONTEXT*>( aCtx );
    LIBEVAL::VALUE*   arg = context->Pop();
    LIBEVAL::VALUE*   result = context->AllocValue();

    result->Set( 0.0 );
    context->Push( result );

    if( !arg )
    {
        if( context->HasErrorCallback() )
            context->ReportError( MISSING_FP_ARG( wxT( "intersectsFrontCourtyard()" ) ) );

        return;
    }

    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( context ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg, context]() -> double
            {
                BOARD*                 board = item->GetBoard();
                std::shared_ptr<SHAPE> itemShape;

                if( searchFootprints( board, arg->AsString(), context,
                        [&]( FOOTPRINT* fp )
                        {
                            PTR_PTR_CACHE_KEY            key = { fp, item };
                            std::unique_lock<std::mutex> cacheLock( board->m_CachesMutex );

                            auto i = board->m_IntersectsFCourtyardCache.find( key );

                            if( i != board->m_IntersectsFCourtyardCache.end() )
                                return i->second;

                            bool res = collidesWithCourtyard( item, itemShape, context, fp, F_Cu );

                            board->m_IntersectsFCourtyardCache[ key ] = res;
                            return res;
                        } ) )
                {
                    return 1.0;
                }

                return 0.0;
            } );
}


static void intersectsBackCourtyardFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_CONTEXT* context = static_cast<PCB_EXPR_CONTEXT*>( aCtx );
    LIBEVAL::VALUE*   arg = context->Pop();
    LIBEVAL::VALUE*   result = context->AllocValue();

    result->Set( 0.0 );
    context->Push( result );

    if( !arg )
    {
        if( context->HasErrorCallback() )
            context->ReportError( MISSING_FP_ARG( wxT( "intersectsBackCourtyard()" ) ) );

        return;
    }

    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( context ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg, context]() -> double
            {
                BOARD*                 board = item->GetBoard();
                std::shared_ptr<SHAPE> itemShape;

                if( searchFootprints( board, arg->AsString(), context,
                        [&]( FOOTPRINT* fp )
                        {
                            PTR_PTR_CACHE_KEY            key = { fp, item };
                            std::unique_lock<std::mutex> cacheLock( board->m_CachesMutex );

                            auto i = board->m_IntersectsBCourtyardCache.find( key );

                            if( i != board->m_IntersectsBCourtyardCache.end() )
                                return i->second;

                            bool res = collidesWithCourtyard( item, itemShape, context, fp, B_Cu );

                            board->m_IntersectsBCourtyardCache[ key ] = res;
                            return res;
                        } ) )
                {
                    return 1.0;
                }

                return 0.0;
            } );
}


bool collidesWithArea( BOARD_ITEM* aItem, PCB_EXPR_CONTEXT* aCtx, ZONE* aArea )
{
    BOARD*                 board = aArea->GetBoard();
    BOX2I                  areaBBox = aArea->GetBoundingBox();
    std::shared_ptr<SHAPE> shape;

    // Collisions include touching, so we need to deflate outline by enough to exclude it.
    // This is particularly important for detecting copper fills as they will be exactly
    // touching along the entire exclusion border.
    SHAPE_POLY_SET areaOutline = aArea->Outline()->CloneDropTriangulation();
    areaOutline.Deflate( board->GetDesignSettings().GetDRCEpsilon(), 0,
                         SHAPE_POLY_SET::ALLOW_ACUTE_CORNERS );

    if( aItem->GetFlags() & HOLE_PROXY )
    {
        if( aItem->Type() == PCB_PAD_T )
        {
            return areaOutline.Collide( aItem->GetEffectiveHoleShape().get() );
        }
        else if( aItem->Type() == PCB_VIA_T )
        {
            LSET overlap = aItem->GetLayerSet() & aArea->GetLayerSet();

            /// Avoid buried vias that don't overlap the zone's layers
            if( overlap.any() )
            {
                if( aCtx->GetLayer() == UNDEFINED_LAYER || overlap.Contains( aCtx->GetLayer() ) )
                    return areaOutline.Collide( aItem->GetEffectiveHoleShape().get() );
            }
        }

        return false;
    }

    if( aItem->Type() == PCB_FOOTPRINT_T )
    {
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aItem );

        if( ( footprint->GetFlags() & MALFORMED_COURTYARDS ) != 0 )
        {
            if( aCtx->HasErrorCallback() )
                aCtx->ReportError( _( "Footprint's courtyard is not a single, closed shape." ) );

            return false;
        }

        if( ( aArea->GetLayerSet() & LSET::FrontMask() ).any() )
        {
            const SHAPE_POLY_SET& courtyard = footprint->GetCourtyard( F_CrtYd );

            if( courtyard.OutlineCount() == 0 )
            {
                if( aCtx->HasErrorCallback() )
                    aCtx->ReportError( _( "Footprint has no front courtyard." ) );

                return false;
            }
            else
            {
                return areaOutline.Collide( &courtyard.Outline( 0 ) );
            }
        }

        if( ( aArea->GetLayerSet() & LSET::BackMask() ).any() )
        {
            const SHAPE_POLY_SET& courtyard = footprint->GetCourtyard( B_CrtYd );

            if( courtyard.OutlineCount() == 0 )
            {
                if( aCtx->HasErrorCallback() )
                    aCtx->ReportError( _( "Footprint has no back courtyard." ) );

                return false;
            }
            else
            {
                return areaOutline.Collide( &courtyard.Outline( 0 ) );
            }
        }

        return false;
    }

    if( aItem->Type() == PCB_ZONE_T || aItem->Type() == PCB_FP_ZONE_T )
    {
        ZONE* zone = static_cast<ZONE*>( aItem );

        if( !zone->IsFilled() )
            return false;

        DRC_RTREE* zoneRTree = board->m_CopperZoneRTreeCache[ zone ].get();

        if( zoneRTree )
        {
            for( PCB_LAYER_ID layer : aArea->GetLayerSet().Seq() )
            {
                if( aCtx->GetLayer() == layer || aCtx->GetLayer() == UNDEFINED_LAYER )
                {
                    if( zoneRTree->QueryColliding( areaBBox, &areaOutline, layer ) )
                        return true;
                }
            }
        }

        return false;
    }
    else
    {
        PCB_LAYER_ID layer = aCtx->GetLayer();

        if( layer != UNDEFINED_LAYER && !( aArea->GetLayerSet().Contains( layer ) ) )
            return false;

        if( !shape )
            shape = aItem->GetEffectiveShape( layer );

        return areaOutline.Collide( shape.get() );
    }
}


bool searchAreas( BOARD* aBoard, const wxString& aArg, PCB_EXPR_CONTEXT* aCtx,
                  std::function<bool( ZONE* )> aFunc )
{
    if( aArg == wxT( "A" ) )
    {
        return aFunc( dynamic_cast<ZONE*>( aCtx->GetItem( 0 ) ) );
    }
    else if( aArg == wxT( "B" ) )
    {
        return aFunc( dynamic_cast<ZONE*>( aCtx->GetItem( 1 ) ) );
    }
    else if( KIID::SniffTest( aArg ) )
    {
        KIID target( aArg );

        for( ZONE* area : aBoard->Zones() )
        {
            // Only a single zone can match the UUID; exit once we find a match whether
            // "inside" or not
            if( area->m_Uuid == target )
                return aFunc( area );
        }

        for( FOOTPRINT* footprint : aBoard->Footprints() )
        {
            for( ZONE* area : footprint->Zones() )
            {
                // Only a single zone can match the UUID; exit once we find a match
                // whether "inside" or not
                if( area->m_Uuid == target )
                    return aFunc( area );
            }
        }

        return 0.0;
    }
    else  // Match on zone name
    {
        for( ZONE* area : aBoard->Zones() )
        {
            if( area->GetZoneName().Matches( aArg ) )
            {
                // Many zones can match the name; exit only when we find an "inside"
                if( aFunc( area ) )
                    return true;
            }
        }

        for( FOOTPRINT* footprint : aBoard->Footprints() )
        {
            for( ZONE* area : footprint->Zones() )
            {
                // Many zones can match the name; exit only when we find an "inside"
                if( area->GetZoneName().Matches( aArg ) )
                {
                    if( aFunc( area ) )
                        return true;
                }
            }
        }

        return false;
    }
}


#define MISSING_AREA_ARG( f ) \
    wxString::Format( _( "Missing rule-area argument (A, B, or rule-area name) to %s." ), f )

static void intersectsAreaFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_CONTEXT* context = static_cast<PCB_EXPR_CONTEXT*>( aCtx );
    LIBEVAL::VALUE*   arg = aCtx->Pop();
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_AREA_ARG( wxT( "intersectsArea()" ) ) );

        return;
    }

    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( context ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg, context]() -> double
            {
                BOARD*       board = item->GetBoard();
                PCB_LAYER_ID layer = context->GetLayer();
                BOX2I        itemBBox = item->GetBoundingBox();

                if( searchAreas( board, arg->AsString(), context,
                        [&]( ZONE* aArea )
                        {
                            if( !aArea || aArea == item || aArea->GetParent() == item )
                                return false;

                            if( !( aArea->GetLayerSet() & item->GetLayerSet() ).any() )
                                return false;

                            if( !aArea->GetBoundingBox().Intersects( itemBBox ) )
                                return false;

                            std::unique_lock<std::mutex> cacheLock( board->m_CachesMutex );
                            PTR_PTR_LAYER_CACHE_KEY      key = { aArea, item, layer };

                            auto i = board->m_IntersectsAreaCache.find( key );

                            if( i != board->m_IntersectsAreaCache.end() )
                                return i->second;

                            bool collides = collidesWithArea( item, context, aArea );

                            board->m_IntersectsAreaCache[ key ] = collides;

                            return collides;
                        } ) )
                {
                    return 1.0;
                }

                return 0.0;
            } );
}


static void enclosedByAreaFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_CONTEXT* context = static_cast<PCB_EXPR_CONTEXT*>( aCtx );
    LIBEVAL::VALUE*   arg = aCtx->Pop();
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_AREA_ARG( wxT( "enclosedByArea()" ) ) );

        return;
    }

    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( context ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg, context]() -> double
            {
                BOARD*       board = item->GetBoard();
                int          maxError = board->GetDesignSettings().m_MaxError;
                PCB_LAYER_ID layer = context->GetLayer();
                BOX2I        itemBBox = item->GetBoundingBox();

                if( searchAreas( board, arg->AsString(), context,
                        [&]( ZONE* aArea )
                        {
                            if( !aArea || aArea == item || aArea->GetParent() == item )
                                return false;

                            if( !( aArea->GetLayerSet() & item->GetLayerSet() ).any() )
                                return false;

                            if( !aArea->GetBoundingBox().Intersects( itemBBox ) )
                                return false;

                            std::unique_lock<std::mutex> cacheLock( board->m_CachesMutex );
                            PTR_PTR_LAYER_CACHE_KEY      key = { aArea, item, layer };

                            auto i = board->m_EnclosedByAreaCache.find( key );

                            if( i != board->m_EnclosedByAreaCache.end() )
                                return i->second;

                            SHAPE_POLY_SET itemShape;
                            bool           enclosedByArea;

                            item->TransformShapeToPolygon( itemShape, layer, 0, maxError,
                                                           ERROR_OUTSIDE );

                            if( itemShape.IsEmpty() )
                            {
                                // If it's already empty then our test will have no meaning.
                                enclosedByArea = false;
                            }
                            else
                            {
                                itemShape.BooleanSubtract( *aArea->Outline(),
                                                           SHAPE_POLY_SET::PM_FAST );

                                enclosedByArea = itemShape.IsEmpty();
                            }

                            board->m_EnclosedByAreaCache[ key ] = enclosedByArea;

                            return enclosedByArea;
                        } ) )
                {
                    return 1.0;
                }

                return 0.0;
            } );
}


#define MISSING_GROUP_ARG( f ) \
    wxString::Format( _( "Missing group name argument to %s." ), f )

static void memberOfFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE* arg = aCtx->Pop();
    LIBEVAL::VALUE* result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !arg )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_GROUP_ARG( wxT( "memberOf()" ) ) );

        return;
    }

    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;

    if( !item )
        return;

    result->SetDeferredEval(
            [item, arg]() -> double
            {
                PCB_GROUP* group = item->GetParentGroup();

                if( !group && item->GetParent() && item->GetParent()->Type() == PCB_FOOTPRINT_T )
                    group = item->GetParent()->GetParentGroup();

                while( group )
                {
                    if( group->GetName().Matches( arg->AsString() ) )
                        return 1.0;

                    group = group->GetParentGroup();
                }

                return 0.0;
            } );
}


static void isMicroVia( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    PCB_VIA* via = dyn_cast<PCB_VIA*>( item );

    if( via && via->GetViaType() == VIATYPE::MICROVIA )
        result->Set ( 1.0 );
}


static void isBlindBuriedViaFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_VAR_REF* vref = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    PCB_VIA* via = dyn_cast<PCB_VIA*>( item );

    if( via && via->GetViaType() == VIATYPE::BLIND_BURIED )
        result->Set ( 1.0 );
}


static void isCoupledDiffPairFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    PCB_EXPR_CONTEXT*     context = static_cast<PCB_EXPR_CONTEXT*>( aCtx );
    BOARD_CONNECTED_ITEM* a = dynamic_cast<BOARD_CONNECTED_ITEM*>( context->GetItem( 0 ) );
    BOARD_CONNECTED_ITEM* b = dynamic_cast<BOARD_CONNECTED_ITEM*>( context->GetItem( 1 ) );
    LIBEVAL::VALUE*       result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    result->SetDeferredEval(
            [a, b, context]() -> double
            {
                NETINFO_ITEM* netinfo = a ? a->GetNet() : nullptr;

                if( !netinfo )
                    return 0.0;

                wxString coupledNet;
                wxString dummy;

                if( !DRC_ENGINE::MatchDpSuffix( netinfo->GetNetname(), coupledNet, dummy ) )
                    return 0.0;

                if( context->GetConstraint() == DRC_CONSTRAINT_T::LENGTH_CONSTRAINT
                        || context->GetConstraint() == DRC_CONSTRAINT_T::SKEW_CONSTRAINT )
                {
                    // DRC engine evaluates these singly, so we won't have a B item
                    return 1.0;
                }

                return b && b->GetNetname() == coupledNet;
            } );
}


#define MISSING_DP_ARG( f ) \
    wxString::Format( _( "Missing diff-pair name argument to %s." ), f )

static void inDiffPairFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE*   argv   = aCtx->Pop();
    PCB_EXPR_VAR_REF* vref   = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item   = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    result->Set( 0.0 );
    aCtx->Push( result );

    if( !argv )
    {
        if( aCtx->HasErrorCallback() )
            aCtx->ReportError( MISSING_DP_ARG( wxT( "inDiffPair()" ) ) );

        return;
    }

    if( !item || !item->GetBoard() )
        return;

    result->SetDeferredEval(
            [item, argv]() -> double
            {
                if( item && item->IsConnected() )
                {
                    NETINFO_ITEM* netinfo = static_cast<BOARD_CONNECTED_ITEM*>( item )->GetNet();

                    wxString refName = netinfo->GetNetname();
                    wxString arg = argv->AsString();
                    wxString baseName, coupledNet;
                    int      polarity = DRC_ENGINE::MatchDpSuffix( refName, coupledNet, baseName );

                    if( polarity != 0 && item->GetBoard()->FindNet( coupledNet ) )
                    {
                        if( baseName.Matches( arg ) )
                            return 1.0;

                        if( baseName.EndsWith( "_" ) && baseName.BeforeLast( '_' ).Matches( arg ) )
                            return 1.0;
                    }
                }

                return 0.0;
            } );
}


static void getFieldFunc( LIBEVAL::CONTEXT* aCtx, void* self )
{
    LIBEVAL::VALUE*   arg    = aCtx->Pop();
    PCB_EXPR_VAR_REF* vref   = static_cast<PCB_EXPR_VAR_REF*>( self );
    BOARD_ITEM*       item   = vref ? vref->GetObject( aCtx ) : nullptr;
    LIBEVAL::VALUE*   result = aCtx->AllocValue();

    result->Set( "" );
    aCtx->Push( result );

    if( !arg )
    {
        if( aCtx->HasErrorCallback() )
        {
            aCtx->ReportError( wxString::Format( _( "Missing field name argument to %s." ),
                                                 wxT( "getField()" ) ) );
        }

        return;
    }

    if( !item || !item->GetBoard() )
        return;

    result->SetDeferredEval(
            [item, arg]() -> wxString
            {
                if( item && item->Type() == PCB_FOOTPRINT_T )
                {
                    FOOTPRINT* fp = static_cast<FOOTPRINT*>( item );

                    if( fp->HasProperty( arg->AsString() ) )
                        return fp->GetProperty( arg->AsString() );
                }

                return "";
            } );
}


PCB_EXPR_BUILTIN_FUNCTIONS::PCB_EXPR_BUILTIN_FUNCTIONS()
{
    RegisterAllFunctions();
}


void PCB_EXPR_BUILTIN_FUNCTIONS::RegisterAllFunctions()
{
    m_funcs.clear();

    RegisterFunc( wxT( "existsOnLayer('x')" ), existsOnLayerFunc );

    RegisterFunc( wxT( "isPlated()" ), isPlatedFunc );

    RegisterFunc( wxT( "insideCourtyard('x') DEPRECATED" ), intersectsCourtyardFunc );
    RegisterFunc( wxT( "insideFrontCourtyard('x') DEPRECATED" ), intersectsFrontCourtyardFunc );
    RegisterFunc( wxT( "insideBackCourtyard('x') DEPRECATED" ), intersectsBackCourtyardFunc );
    RegisterFunc( wxT( "intersectsCourtyard('x')" ), intersectsCourtyardFunc );
    RegisterFunc( wxT( "intersectsFrontCourtyard('x')" ), intersectsFrontCourtyardFunc );
    RegisterFunc( wxT( "intersectsBackCourtyard('x')" ), intersectsBackCourtyardFunc );

    RegisterFunc( wxT( "insideArea('x') DEPRECATED" ), intersectsAreaFunc );
    RegisterFunc( wxT( "intersectsArea('x')" ), intersectsAreaFunc );
    RegisterFunc( wxT( "enclosedByArea('x')" ), enclosedByAreaFunc );

    RegisterFunc( wxT( "isMicroVia()" ), isMicroVia );
    RegisterFunc( wxT( "isBlindBuriedVia()" ), isBlindBuriedViaFunc );

    RegisterFunc( wxT( "memberOf('x')" ), memberOfFunc );

    RegisterFunc( wxT( "fromTo('x','y')" ), fromToFunc );
    RegisterFunc( wxT( "isCoupledDiffPair()" ), isCoupledDiffPairFunc );
    RegisterFunc( wxT( "inDiffPair('x')" ), inDiffPairFunc );

    RegisterFunc( wxT( "getField('x')" ), getFieldFunc );
}


BOARD_ITEM* PCB_EXPR_VAR_REF::GetObject( const LIBEVAL::CONTEXT* aCtx ) const
{
    wxASSERT( dynamic_cast<const PCB_EXPR_CONTEXT*>( aCtx ) );

    const PCB_EXPR_CONTEXT* ctx = static_cast<const PCB_EXPR_CONTEXT*>( aCtx );
    BOARD_ITEM*             item  = ctx->GetItem( m_itemIndex );
    return item;
}


class PCB_LAYER_VALUE : public LIBEVAL::VALUE
{
public:
    PCB_LAYER_VALUE( PCB_LAYER_ID aLayer ) :
        LIBEVAL::VALUE( LayerName( aLayer ) ),
        m_layer( aLayer )
    {};

    virtual bool EqualTo( LIBEVAL::CONTEXT* aCtx, const VALUE* b ) const override
    {
        // For boards with user-defined layer names there will be 2 entries for each layer
        // in the ENUM_MAP: one for the canonical layer name and one for the user layer name.
        // We need to check against both.

        wxPGChoices&                 layerMap = ENUM_MAP<PCB_LAYER_ID>::Instance().Choices();
        const wxString&              layerName = b->AsString();
        BOARD*                       board = static_cast<PCB_EXPR_CONTEXT*>( aCtx )->GetBoard();
        std::unique_lock<std::mutex> cacheLock( board->m_CachesMutex );
        auto                         i = board->m_LayerExpressionCache.find( layerName );
        LSET                         mask;

        if( i == board->m_LayerExpressionCache.end() )
        {
            for( unsigned ii = 0; ii < layerMap.GetCount(); ++ii )
            {
                wxPGChoiceEntry& entry = layerMap[ii];

                if( entry.GetText().Matches( layerName ) )
                    mask.set( ToLAYER_ID( entry.GetValue() ) );
            }

            board->m_LayerExpressionCache[ layerName ] = mask;
        }
        else
        {
            mask = i->second;
        }

        return mask.Contains( m_layer );
    }

protected:
    PCB_LAYER_ID m_layer;
};


LIBEVAL::VALUE* PCB_EXPR_VAR_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    PCB_EXPR_CONTEXT* context = static_cast<PCB_EXPR_CONTEXT*>( aCtx );

    if( m_itemIndex == 2 )
        return new PCB_LAYER_VALUE( context->GetLayer() );

    BOARD_ITEM* item = GetObject( aCtx );

    if( !item )
        return new LIBEVAL::VALUE();

    auto it = m_matchingTypes.find( TYPE_HASH( *item ) );

    if( it == m_matchingTypes.end() )
    {
        // Don't force user to type "A.Type == 'via' && A.Via_Type == 'buried'" when the
        // simpler "A.Via_Type == 'buried'" is perfectly clear.  Instead, return an undefined
        // value when the property doesn't appear on a particular object.

        return new LIBEVAL::VALUE();
    }
    else
    {
        if( m_type == LIBEVAL::VT_NUMERIC )
            return new LIBEVAL::VALUE( (double) item->Get<int>( it->second ) );
        else
        {
            wxString str;

            if( !m_isEnum )
            {
                str = item->Get<wxString>( it->second );
                return new LIBEVAL::VALUE( str );
            }
            else
            {
                const wxAny& any = item->Get( it->second );
                bool valid = any.GetAs<wxString>( &str );

                if( valid )
                {
                    if( it->second->Name() == wxT( "Layer" ) )
                        return new PCB_LAYER_VALUE( context->GetBoard()->GetLayerID( str ) );
                    else
                        return new LIBEVAL::VALUE( str );
                }
            }

            return new LIBEVAL::VALUE();
        }
    }
}


LIBEVAL::VALUE* PCB_EXPR_NETCLASS_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    BOARD_CONNECTED_ITEM* item = dynamic_cast<BOARD_CONNECTED_ITEM*>( GetObject( aCtx ) );

    if( !item )
        return new LIBEVAL::VALUE();

    return new LIBEVAL::VALUE( item->GetEffectiveNetClass()->GetName() );
}


LIBEVAL::VALUE* PCB_EXPR_NETNAME_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    BOARD_CONNECTED_ITEM* item = dynamic_cast<BOARD_CONNECTED_ITEM*>( GetObject( aCtx ) );

    if( !item )
        return new LIBEVAL::VALUE();

    return new LIBEVAL::VALUE( item->GetNetname() );
}


LIBEVAL::VALUE* PCB_EXPR_TYPE_REF::GetValue( LIBEVAL::CONTEXT* aCtx )
{
    BOARD_ITEM* item = GetObject( aCtx );

    if( !item )
        return new LIBEVAL::VALUE();

    return new LIBEVAL::VALUE( ENUM_MAP<KICAD_T>::Instance().ToString( item->Type() ) );
}


LIBEVAL::FUNC_CALL_REF PCB_EXPR_UCODE::CreateFuncCall( const wxString& aName )
{
    PCB_EXPR_BUILTIN_FUNCTIONS& registry = PCB_EXPR_BUILTIN_FUNCTIONS::Instance();

    return registry.Get( aName.Lower() );
}


std::unique_ptr<LIBEVAL::VAR_REF> PCB_EXPR_UCODE::CreateVarRef( const wxString& aVar,
                                                                const wxString& aField )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    std::unique_ptr<PCB_EXPR_VAR_REF> vref;

    // Check for a couple of very common cases and compile them straight to "object code".

    if( aField.CmpNoCase( wxT( "NetClass" ) ) == 0 )
    {
        if( aVar == wxT( "A" ) )
            return std::make_unique<PCB_EXPR_NETCLASS_REF>( 0 );
        else if( aVar == wxT( "B" ) )
            return std::make_unique<PCB_EXPR_NETCLASS_REF>( 1 );
        else
            return nullptr;
    }
    else if( aField.CmpNoCase( wxT( "NetName" ) ) == 0 )
    {
        if( aVar == wxT( "A" ) )
            return std::make_unique<PCB_EXPR_NETNAME_REF>( 0 );
        else if( aVar == wxT( "B" ) )
            return std::make_unique<PCB_EXPR_NETNAME_REF>( 1 );
        else
            return nullptr;
    }
    else if( aField.CmpNoCase( wxT( "Type" ) ) == 0 )
    {
        if( aVar == wxT( "A" ) )
            return std::make_unique<PCB_EXPR_TYPE_REF>( 0 );
        else if( aVar == wxT( "B" ) )
            return std::make_unique<PCB_EXPR_TYPE_REF>( 1 );
        else
            return nullptr;
    }

    if( aVar == wxT( "A" ) || aVar == wxT( "AB" ) )
        vref = std::make_unique<PCB_EXPR_VAR_REF>( 0 );
    else if( aVar == wxT( "B" ) )
        vref = std::make_unique<PCB_EXPR_VAR_REF>( 1 );
    else if( aVar == wxT( "L" ) )
        vref = std::make_unique<PCB_EXPR_VAR_REF>( 2 );
    else
        return nullptr;

    if( aField.length() == 0 ) // return reference to base object
    {
        return std::move( vref );
    }

    wxString field( aField );
    field.Replace( wxT( "_" ),  wxT( " " ) );

    for( const PROPERTY_MANAGER::CLASS_INFO& cls : propMgr.GetAllClasses() )
    {
        if( propMgr.IsOfType( cls.type, TYPE_HASH( BOARD_ITEM ) ) )
        {
            PROPERTY_BASE* prop = propMgr.GetProperty( cls.type, field );

            if( prop )
            {
                vref->AddAllowedClass( cls.type, prop );

                if( prop->TypeHash() == TYPE_HASH( int ) )
                {
                    vref->SetType( LIBEVAL::VT_NUMERIC );
                }
                else if( prop->TypeHash() == TYPE_HASH( wxString ) )
                {
                    vref->SetType( LIBEVAL::VT_STRING );
                }
                else if ( prop->HasChoices() )
                {   // it's an enum, we treat it as string
                    vref->SetType( LIBEVAL::VT_STRING );
                    vref->SetIsEnum ( true );
                }
                else
                {
                    wxFAIL_MSG( wxT( "PCB_EXPR_UCODE::createVarRef: Unknown property type." ) );
                }
            }
        }
    }

    if( vref->GetType() == LIBEVAL::VT_UNDEFINED )
        vref->SetType( LIBEVAL::VT_PARSE_ERROR );

    return std::move( vref );
}


BOARD* PCB_EXPR_CONTEXT::GetBoard() const
{
    if( m_items[0] )
        return m_items[0]->GetBoard();

    return nullptr;
}


class PCB_UNIT_RESOLVER : public LIBEVAL::UNIT_RESOLVER
{
public:
    virtual ~PCB_UNIT_RESOLVER()
    {
    }

    virtual const std::vector<wxString>& GetSupportedUnits() const override
    {
        static const std::vector<wxString> pcbUnits = { wxT( "mil" ), wxT( "mm" ), wxT( "in" ) };

        return pcbUnits;
    }

    virtual wxString GetSupportedUnitsMessage() const override
    {
        return _( "must be mm, in, or mil" );
    }

    virtual double Convert( const wxString& aString, int unitId ) const override
    {
        double v = wxAtof( aString );

        switch( unitId )
        {
        case 0: return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MILS, aString );
        case 1: return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MILLIMETRES, aString );
        case 2: return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::INCHES, aString );
        default: return v;
        }
    };
};


PCB_EXPR_COMPILER::PCB_EXPR_COMPILER()
{
    m_unitResolver = std::make_unique<PCB_UNIT_RESOLVER>();
}


PCB_EXPR_EVALUATOR::PCB_EXPR_EVALUATOR() :
    m_result( 0 ),
    m_compiler(),
    m_ucode(),
    m_errorStatus()
{
}


PCB_EXPR_EVALUATOR::~PCB_EXPR_EVALUATOR()
{
}


bool PCB_EXPR_EVALUATOR::Evaluate( const wxString& aExpr )
{
    PCB_EXPR_UCODE   ucode;
    PCB_EXPR_CONTEXT preflightContext( NULL_CONSTRAINT, F_Cu );

    if( !m_compiler.Compile( aExpr.ToUTF8().data(), &ucode, &preflightContext ) )
        return false;

    PCB_EXPR_CONTEXT evaluationContext( NULL_CONSTRAINT, F_Cu );
    LIBEVAL::VALUE*  result = ucode.Run( &evaluationContext );

    if( result->GetType() == LIBEVAL::VT_NUMERIC )
        m_result = KiROUND( result->AsDouble() );

    return true;
}

