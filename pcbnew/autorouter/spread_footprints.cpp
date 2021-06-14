/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file spread_footprints.cpp
 * @brief functions to spread footprints on free areas outside a board.
 * this is useful after reading a netlist, when new footprints are loaded
 * and stacked at 0,0 coordinate.
 * Often, spread them on a free area near the board being edited make more easy
 * their selection.
 */

#include <algorithm>
#include <convert_to_biu.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <footprint.h>
#include <rect_placement/rect_placement.h>

struct TSubRect : public CRectPlacement::TRect
{
    int n;      // Original index of this subrect, before sorting

    TSubRect() : TRect(),
        n( 0 )
    {
    }

    TSubRect( int _w, int _h, int _n ) :
        TRect( 0, 0, _w, _h ), n( _n ) { }
};

typedef std::vector<TSubRect> CSubRectArray;

// Use 0.01 mm units to calculate placement, to avoid long calculation time
const int scale = (int)(0.01 * IU_PER_MM);

const int PADDING = (int)(1 * IU_PER_MM);

// Populates a list of rectangles, from a list of footprints
void fillRectList( CSubRectArray& vecSubRects, std::vector <FOOTPRINT*>& aFootprintList )
{
    vecSubRects.clear();

    for( unsigned ii = 0; ii < aFootprintList.size(); ii++ )
    {
        EDA_RECT fpBox = aFootprintList[ii]->GetBoundingBox( false, false );
        TSubRect fpRect( ( fpBox.GetWidth() + PADDING ) / scale,
                         ( fpBox.GetHeight() + PADDING ) / scale, ii );
        vecSubRects.push_back( fpRect );
    }
}

// Populates a list of rectangles, from a list of EDA_RECT
void fillRectList( CSubRectArray& vecSubRects, std::vector <EDA_RECT>& aRectList )
{
    vecSubRects.clear();

    for( unsigned ii = 0; ii < aRectList.size(); ii++ )
    {
        EDA_RECT& rect = aRectList[ii];
        TSubRect fpRect( rect.GetWidth()/scale, rect.GetHeight()/scale, ii );
        vecSubRects.push_back( fpRect );
    }
}



// Spread a list of rectangles inside a placement area
void spreadRectangles( CRectPlacement& aPlacementArea,
                       CSubRectArray& vecSubRects,
                       int areaSizeX, int areaSizeY )
{
    areaSizeX/= scale;
    areaSizeY/= scale;

    // Sort the subRects based on dimensions, larger dimension goes first.
    std::sort( vecSubRects.begin(), vecSubRects.end(), CRectPlacement::TRect::Greater );

    // gives the initial size to the area
    aPlacementArea.Init( areaSizeX, areaSizeY );

    // Add all subrects
    CSubRectArray::iterator it;

    for( it = vecSubRects.begin(); it != vecSubRects.end(); )
    {
        CRectPlacement::TRect r( 0, 0, it->w, it->h );

        bool bPlaced = aPlacementArea.AddAtEmptySpotAutoGrow( &r, areaSizeX, areaSizeY );

        if( !bPlaced )   // No room to place the rectangle: enlarge area and retry
        {
            bool retry = false;

            if( areaSizeX < INT_MAX/2 )
            {
                retry = true;
                areaSizeX = areaSizeX * 1.2;
            }

            if( areaSizeX < INT_MAX/2 )
            {
                retry = true;
                areaSizeY = areaSizeY * 1.2;
            }

            if( retry )
            {
                aPlacementArea.Init( areaSizeX, areaSizeY );
                it = vecSubRects.begin();
                continue;
            }
        }

        // When correctly placed in a placement area, the coords are returned in r.x and r.y
        // Store them.
        it->x   = r.x;
        it->y   = r.y;

        it++;
    }
}


void moveFootprintsInArea( CRectPlacement& aPlacementArea, std::vector <FOOTPRINT*>& aFootprintList,
                           EDA_RECT& aFreeArea, bool aFindAreaOnly )
{
    CSubRectArray   vecSubRects;

    fillRectList( vecSubRects, aFootprintList );
    spreadRectangles( aPlacementArea, vecSubRects, aFreeArea.GetWidth(), aFreeArea.GetHeight() );

    if( aFindAreaOnly )
        return;

    for( unsigned it = 0; it < vecSubRects.size(); ++it )
    {
        wxPoint pos( vecSubRects[it].x, vecSubRects[it].y );
        pos.x *= scale;
        pos.y *= scale;

        FOOTPRINT* footprint = aFootprintList[vecSubRects[it].n];

        EDA_RECT fpBBox = footprint->GetBoundingBox( false, false );
        wxPoint mod_pos = pos + ( footprint->GetPosition() - fpBBox.GetOrigin() )
                          + aFreeArea.GetOrigin();

        footprint->Move( mod_pos - footprint->GetPosition() );
    }
}

static bool sortFootprintsbySheetPath( FOOTPRINT* ref, FOOTPRINT* compare );


/**
 * Footprints (after loaded by reading a netlist for instance) are moved
 * to be in a small free area (outside the current board) without overlapping.
 * @param aBoard is the board to edit.
 * @param aFootprints: a list of footprints to be spread out.
 * @param aSpreadAreaPosition the position of the upper left corner of the
 *        area allowed to spread footprints
 */
void SpreadFootprints( std::vector<FOOTPRINT*>* aFootprints, wxPoint aSpreadAreaPosition )
{
    // Build candidate list
    // calculate also the area needed by these footprints
    std::vector <FOOTPRINT*> footprintList;

    for( FOOTPRINT* footprint : *aFootprints )
    {
        if( footprint->IsLocked() )
            continue;

        footprintList.push_back( footprint );
    }

    if( footprintList.empty() )
        return;

    // sort footprints by sheet path. we group them later by sheet
    sort( footprintList.begin(), footprintList.end(), sortFootprintsbySheetPath );

    // Extract and place footprints by sheet
    std::vector <FOOTPRINT*> footprintListBySheet;
    std::vector <EDA_RECT>   placementSheetAreas;
    double                   subsurface;
    double                   placementsurface = 0.0;

    // The placement uses 2 passes:
    // the first pass creates the rectangular areas to place footprints
    // each sheet in schematic creates one rectangular area.
    // the second pass moves footprints inside these areas
    for( int pass = 0; pass < 2; pass++ )
    {
        int subareaIdx = 0;
        footprintListBySheet.clear();
        subsurface = 0.0;

        int fp_max_width = 0;
        int fp_max_height = 0;

        for( unsigned ii = 0; ii < footprintList.size(); ii++ )
        {
            FOOTPRINT* footprint = footprintList[ii];
            bool       islastItem = false;

            if( ii == footprintList.size() - 1 ||
                ( footprintList[ii]->GetPath().AsString().BeforeLast( '/' ) !=
                  footprintList[ii+1]->GetPath().AsString().BeforeLast( '/' ) ) )
                islastItem = true;

            footprintListBySheet.push_back( footprint );
            subsurface += footprint->GetArea( PADDING );

            // Calculate min size of placement area:
            EDA_RECT bbox = footprint->GetBoundingBox( false, false );
            fp_max_width = std::max( fp_max_width, bbox.GetWidth() );
            fp_max_height = std::max( fp_max_height, bbox.GetHeight() );

            if( islastItem )
            {
                // end of the footprint sublist relative to the same sheet path
                // calculate placement of the current sublist
                EDA_RECT freeArea;
                int Xsize_allowed = (int) ( sqrt( subsurface ) * 4.0 / 3.0 );
                Xsize_allowed = std::max( fp_max_width, Xsize_allowed );

                int Ysize_allowed = (int) ( subsurface / Xsize_allowed );
                Ysize_allowed = std::max( fp_max_height, Ysize_allowed );

                freeArea.SetWidth( Xsize_allowed );
                freeArea.SetHeight( Ysize_allowed );
                CRectPlacement placementArea;

                if( pass == 1 )
                {
                    wxPoint areapos = placementSheetAreas[subareaIdx].GetOrigin()
                                      + aSpreadAreaPosition;
                    freeArea.SetOrigin( areapos );
                }

                bool findAreaOnly = pass == 0;
                moveFootprintsInArea( placementArea, footprintListBySheet, freeArea, findAreaOnly );

                if( pass == 0 )
                {
                    // Populate sheet placement areas list
                    EDA_RECT sub_area;
                    sub_area.SetWidth( placementArea.GetW()*scale );
                    sub_area.SetHeight( placementArea.GetH()*scale );
                    // Add a margin around the sheet placement area:
                    sub_area.Inflate( Millimeter2iu( 1.5 ) );

                    placementSheetAreas.push_back( sub_area );

                    placementsurface += (double) sub_area.GetWidth()*
                                        sub_area.GetHeight();
                }

                // Prepare buffers for next sheet
                subsurface  = 0.0;
                footprintListBySheet.clear();
                subareaIdx++;
            }
        }

        // End of pass:
        // At the end of the first pass, we have to find position of each sheet
        // placement area
        if( pass == 0 )
        {
            int Xsize_allowed = (int) ( sqrt( placementsurface ) * 4.0 / 3.0 );

            if( Xsize_allowed < 0 || Xsize_allowed > INT_MAX/2 )
                Xsize_allowed = INT_MAX/2;

            int Ysize_allowed = (int) ( placementsurface / Xsize_allowed );

            if( Ysize_allowed < 0 || Ysize_allowed > INT_MAX/2 )
                Ysize_allowed = INT_MAX/2;

            CRectPlacement placementArea;
            CSubRectArray  vecSubRects;
            fillRectList( vecSubRects, placementSheetAreas );
            spreadRectangles( placementArea, vecSubRects, Xsize_allowed, Ysize_allowed );

            for( unsigned it = 0; it < vecSubRects.size(); ++it )
            {
                TSubRect& srect = vecSubRects[it];
                wxPoint pos( srect.x*scale, srect.y*scale );
                wxSize size( srect.w*scale, srect.h*scale );

                // Avoid too large coordinates: Overlapping components
                // are better than out of screen components
                if( (uint64_t)pos.x + (uint64_t)size.x > INT_MAX/2 )
                    pos.x = 0;

                if( (uint64_t)pos.y + (uint64_t)size.y > INT_MAX/2 )
                    pos.y = 0;

                placementSheetAreas[srect.n].SetOrigin( pos );
                placementSheetAreas[srect.n].SetSize( size );
            }
        }
    }   // End pass
}


// Sort function, used to group footprints by sheet.
// Footprints are sorted by their sheet path.
// (the full sheet path restricted to the time stamp of the sheet itself,
// without the time stamp of the footprint ).
static bool sortFootprintsbySheetPath( FOOTPRINT* ref, FOOTPRINT* compare )
{
    return ref->GetPath() < compare->GetPath();
}
