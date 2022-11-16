/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2018-2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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


#ifndef PCBNEW_CONNECTIVITY_ITEMS_H
#define PCBNEW_CONNECTIVITY_ITEMS_H

#include <board.h>
#include <pad.h>
#include <footprint.h>
#include <pcb_track.h>
#include <zone.h>

#include <geometry/shape_poly_set.h>

#include <memory>
#include <algorithm>
#include <functional>
#include <vector>
#include <deque>
#include <intrusive_list.h>

#include <connectivity/connectivity_rtree.h>
#include <connectivity/connectivity_data.h>

class CN_ITEM;
class CN_CLUSTER;


/**
 * CN_ANCHOR represents a physical location that can be connected: a pad or a track/arc/via
 * endpoint.
 */
class CN_ANCHOR
{
public:
    CN_ANCHOR()
    {
        m_item = nullptr;
    }

    CN_ANCHOR( const VECTOR2I& aPos, CN_ITEM* aItem )
    {
        m_pos   = aPos;
        m_item  = aItem;
        assert( m_item );
    }

    bool Valid() const;

    CN_ITEM* Item() const
    {
        return m_item;
    }

    BOARD_CONNECTED_ITEM* Parent() const;

    const VECTOR2I& Pos() const
    {
        return m_pos;
    }

    void Move( const VECTOR2I& aPos )
    {
        m_pos += aPos;
    }

    const unsigned int Dist( const CN_ANCHOR& aSecond )
    {
        return ( m_pos - aSecond.Pos() ).EuclideanNorm();
    }

    ///< Return tag, common identifier for connected nodes.
    inline int GetTag() const
    {
        return m_tag;
    }

    ///< Set tag, common identifier for connected nodes.
    inline void SetTag( int aTag )
    {
        m_tag = aTag;
    }

    ///< Decide whether this node can be a ratsnest line target.
    inline void SetNoLine( bool aEnable )
    {
        m_noline = aEnable;
    }

    ///< Return true if this node can be a target for ratsnest lines.
    inline const bool& GetNoLine() const
    {
        return m_noline;
    }

    inline void SetCluster( std::shared_ptr<CN_CLUSTER>& aCluster )
    {
        m_cluster = aCluster;
    }

    inline const std::shared_ptr<CN_CLUSTER>& GetCluster() const
    {
        return m_cluster;
    }

    /**
     * The anchor point is dangling if the parent is a track and this anchor point is not
     * connected to another item ( track, vias pad or zone) or if the parent is a via and
     * this anchor point is connected to only one track and not to another item.
     *
     * @return true if this anchor is dangling.
     */
    bool IsDangling() const;

    /**
     * @return the count of tracks and vias connected to this anchor.
     */
    int ConnectedItemsCount() const;

    // Tag used for unconnected items.
    static const int TAG_UNCONNECTED = -1;

private:
    VECTOR2I m_pos;              ///< Position of the anchor.
    CN_ITEM* m_item = nullptr;   ///< Pad or track/arc/via owning the anchor.
    int      m_tag = -1;         ///< Tag for quick connection resolution.
    bool     m_noline = false;   ///< Whether it the node can be a target for ratsnest lines.

    std::shared_ptr<CN_CLUSTER> m_cluster;     ///< Cluster to which the anchor belongs.
};



/**
 * CN_ITEM represents a BOARD_CONNETED_ITEM in the connectivity system (ie: a pad, track/arc/via,
 * or zone).
 */
class CN_ITEM
{
public:
    void Dump();

    CN_ITEM( BOARD_CONNECTED_ITEM* aParent, bool aCanChangeNet, int aAnchorCount = 2 )
    {
        m_parent = aParent;
        m_canChangeNet = aCanChangeNet;
        m_visited = false;
        m_valid = true;
        m_dirty = true;
        m_anchors.reserve( std::max( 6, aAnchorCount ) );
        m_layers = LAYER_RANGE( 0, PCB_LAYER_ID_COUNT );
        m_connected.reserve( 8 );
    }

    virtual ~CN_ITEM() {};

    void AddAnchor( const VECTOR2I& aPos )
    {
        m_anchors.emplace_back( std::make_shared<CN_ANCHOR>( aPos, this ) );
    }

    std::vector<std::shared_ptr<CN_ANCHOR>>& Anchors() { return m_anchors; }

    void SetValid( bool aValid ) { m_valid = aValid; }
    bool Valid() const { return m_valid; }

    void SetDirty( bool aDirty ) { m_dirty = aDirty; }
    bool Dirty() const { return m_dirty;  }

    /**
     * Set the layers spanned by the item to aLayers.
     */
    void SetLayers( const LAYER_RANGE& aLayers ) { m_layers = aLayers;  }

    /**
     * Set the layers spanned by the item to a single layer aLayer.
     */
    void SetLayer( int aLayer ) { m_layers = LAYER_RANGE( aLayer, aLayer ); }

    /**
     * Return the contiguous set of layers spanned by the item.
     */
    const LAYER_RANGE& Layers() const { return m_layers; }

    /**
     * Return the item's layer, for single-layered items only.
     */
    virtual int Layer() const
    {
        return Layers().Start();
    }

    const BOX2I& BBox()
    {
        if( m_dirty && m_valid )
            m_bbox = m_parent->GetBoundingBox();

        return m_bbox;
    }

    BOARD_CONNECTED_ITEM* Parent() const { return m_parent; }

    const std::vector<CN_ITEM*>& ConnectedItems() const { return m_connected; }
    void ClearConnections() { m_connected.clear(); }

    void SetVisited( bool aVisited ) { m_visited = aVisited; }
    bool Visited() const { return m_visited; }

    bool CanChangeNet() const { return m_canChangeNet; }

    void Connect( CN_ITEM* b )
    {
        std::lock_guard<std::mutex> lock( m_listLock );

        auto i = std::lower_bound( m_connected.begin(), m_connected.end(), b );

        if( i != m_connected.end() && *i == b )
          return;

        m_connected.insert( i, b );
    }

    void RemoveInvalidRefs();

    virtual int AnchorCount() const;
    virtual const VECTOR2I GetAnchor( int n ) const;

    int GetAnchorItemCount() const { return m_anchors.size(); }
    std::shared_ptr<CN_ANCHOR> GetAnchorItem( int n ) const { return m_anchors[n]; }

    int Net() const
    {
        return ( !m_parent || !m_valid ) ? -1 : m_parent->GetNetCode();
    }
                                     ///< allow parallel connection threads
protected:
    bool            m_dirty;         ///< used to identify recently added item not yet
                                     ///< scanned into the connectivity search
    LAYER_RANGE     m_layers;        ///< layer range over which the item exists
    BOX2I           m_bbox;          ///< bounding box for the item

private:
    BOARD_CONNECTED_ITEM*                    m_parent;

    std::vector<CN_ITEM*>                    m_connected;   ///< list of physically touching items
    std::vector<std::shared_ptr<CN_ANCHOR>>  m_anchors;

    bool            m_canChangeNet;  ///< can the net propagator modify the netcode?

    bool            m_visited;       ///< visited flag for the BFS scan
    bool            m_valid;         ///< used to identify garbage items (we use lazy removal)

    std::mutex      m_listLock;      ///< mutex protecting this item's connected_items set to
};


typedef std::shared_ptr<CN_ITEM> CN_ITEM_PTR;


class CN_ZONE_LAYER : public CN_ITEM
{
public:
    CN_ZONE_LAYER( ZONE* aParent, PCB_LAYER_ID aLayer, int aSubpolyIndex ) :
            CN_ITEM( aParent, false ),
            m_subpolyIndex( aSubpolyIndex ),
            m_layer( aLayer )
    {
        m_triangulatedPoly = aParent->GetFilledPolysList( aLayer );
        SetLayers( aLayer );
    }

    void BuildRTree()
    {
        for( unsigned int ii = 0; ii < m_triangulatedPoly->TriangulatedPolyCount(); ++ii )
        {
            const auto* triangleSet = m_triangulatedPoly->TriangulatedPolygon( ii );

            if( triangleSet->GetSourceOutlineIndex() != m_subpolyIndex )
                continue;

            for( const SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRI& tri : triangleSet->Triangles() )
            {
                BOX2I     bbox = tri.BBox();
                const int mmin[2] = { bbox.GetX(), bbox.GetY() };
                const int mmax[2] = { bbox.GetRight(), bbox.GetBottom() };

                m_rTree.Insert( mmin, mmax, &tri );
            }
        }
    }

    int SubpolyIndex() const { return m_subpolyIndex; }

    PCB_LAYER_ID GetLayer() const { return m_layer; }

    bool ContainsPoint( const VECTOR2I& p ) const
    {
        int  min[2] = { p.x, p.y };
        int  max[2] = { p.x, p.y };
        bool collision = false;

        auto visitor =
                [&]( const SHAPE* aShape ) -> bool
                {
                    if( aShape->Collide( p ) )
                    {
                        collision = true;
                        return false;
                    }

                    return true;
                };

        m_rTree.Search( min, max, visitor );

        return collision;
    }

    PCB_LAYER_ID GetLayer() { return m_layer; }

    virtual int AnchorCount() const override;
    virtual const VECTOR2I GetAnchor( int n ) const override;

    const SHAPE_LINE_CHAIN& GetOutline() const
    {
        return m_triangulatedPoly->Outline( m_subpolyIndex );
    }

    VECTOR2I ClosestPoint( const VECTOR2I aPt )
    {
        VECTOR2I closest;

        m_triangulatedPoly->SquaredDistanceToPolygon( aPt, m_subpolyIndex, &closest );

        return closest;
    }

    bool Collide( SHAPE* aRefShape ) const
    {
        BOX2I bbox = aRefShape->BBox();
        int  min[2] = { bbox.GetX(), bbox.GetY() };
        int  max[2] = { bbox.GetRight(), bbox.GetBottom() };
        bool collision = false;

        auto visitor =
                [&]( const SHAPE* aShape ) -> bool
                {
                    if( aRefShape->Collide( aShape ) )
                    {
                        collision = true;
                        return false;
                    }

                    return true;
                };

        m_rTree.Search( min, max, visitor );

        return collision;
    }

private:
    int                                 m_subpolyIndex;
    PCB_LAYER_ID                        m_layer;
    std::shared_ptr<SHAPE_POLY_SET>     m_triangulatedPoly;
    RTree<const SHAPE*, int, 2, double> m_rTree;
};




class CN_LIST
{
protected:
    std::vector<CN_ITEM*> m_items;

    void addItemtoTree( CN_ITEM* item )
    {
        m_index.Insert( item );
    }

public:
    CN_LIST()
    {
        m_dirty = false;
        m_hasInvalid = false;
    }

    void Clear()
    {
        for( CN_ITEM* item : m_items )
            delete item;

        m_items.clear();
        m_index.RemoveAll();
    }

    using ITER       = decltype( m_items )::iterator;
    using CONST_ITER = decltype( m_items )::const_iterator;

    ITER begin() { return m_items.begin(); };
    ITER end() { return m_items.end(); };

    CONST_ITER begin() const { return m_items.begin(); }
    CONST_ITER end() const { return m_items.end(); }

    CN_ITEM* operator[] ( int aIndex ) { return m_items[aIndex]; }

    template <class T>
    void FindNearby( CN_ITEM* aItem, T aFunc )
    {
        m_index.Query( aItem->BBox(), aItem->Layers(), aFunc );
    }

    void SetHasInvalid( bool aInvalid = true ) { m_hasInvalid = aInvalid; }

    void SetDirty( bool aDirty = true ) { m_dirty = aDirty; }
    bool IsDirty() const { return m_dirty; }

    void RemoveInvalidItems( std::vector<CN_ITEM*>& aGarbage );

    void ClearDirtyFlags()
    {
        for( CN_ITEM* item : m_items )
            item->SetDirty( false );

        SetDirty( false );
    }

    int Size() const
    {
        return m_items.size();
    }

    CN_ITEM* Add( PAD* pad );

    CN_ITEM* Add( PCB_TRACK* track );

    CN_ITEM* Add( PCB_ARC* track );

    CN_ITEM* Add( PCB_VIA* via );

    CN_ITEM* Add( CN_ZONE_LAYER* zitem );

    const std::vector<CN_ITEM*> Add( ZONE* zone, PCB_LAYER_ID aLayer );

private:
    bool               m_dirty;
    bool               m_hasInvalid;
    CN_RTREE<CN_ITEM*> m_index;
};


class CN_CLUSTER
{
private:
    bool                         m_conflicting;
    int                          m_originNet;
    CN_ITEM*                     m_originPad;
    std::vector<CN_ITEM*>        m_items;
    std::unordered_map<int, int> m_netRanks;

public:
    CN_CLUSTER();
    ~CN_CLUSTER();

    bool HasValidNet() const { return m_originNet > 0; }
    int OriginNet() const { return m_originNet; }

    wxString OriginNetName() const;

    bool Contains( const CN_ITEM* aItem );
    bool Contains( const BOARD_CONNECTED_ITEM* aItem );
    void Dump();

    int Size() const { return m_items.size(); }

    bool IsOrphaned() const { return m_originPad == nullptr; }

    bool IsConflicting() const { return m_conflicting; }

    void Add( CN_ITEM* item );

    using ITER = decltype(m_items)::iterator;

    ITER begin() { return m_items.begin(); };
    ITER end() { return m_items.end(); };
};



#endif /* PCBNEW_CONNECTIVITY_ITEMS_H */
