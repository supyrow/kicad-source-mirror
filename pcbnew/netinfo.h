/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/*
 *  Classes to handle info on nets
 */

#ifndef CLASS_NETINFO_
#define CLASS_NETINFO_

#include <macros_swig.h>
#include <gr_basic.h>
#include <netclass.h>
#include <board_item.h>



class wxDC;
class wxPoint;
class LINE_READER;
class EDA_DRAW_FRAME;
class PAD;
class BOARD;
class BOARD_ITEM;
class MSG_PANEL_ITEM;
class PCB_BASE_FRAME;


/*****************************/
/* flags for a RATSNEST_ITEM */
/*****************************/
#define CH_VISIBLE          1        /* Visible */
#define CH_UNROUTABLE       2        /* Don't use autorouter. */
#define CH_ROUTE_REQ        4        /* Must be routed by the autorouter. */
#define CH_ACTIF            8        /* Not routed. */
#define LOCAL_RATSNEST_ITEM 0x8000   /* Line between two pads of a single footprint. */

DECL_VEC_FOR_SWIG( PADS_VEC, PAD* )

/**
 * Handle the data for a net.
 */
class NETINFO_ITEM : public BOARD_ITEM
{
public:

    NETINFO_ITEM( BOARD* aParent, const wxString& aNetName = wxEmptyString, int aNetCode = -1 );
    ~NETINFO_ITEM();

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_NETINFO_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "NETINFO_ITEM" );
    }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override
    {
    }
#endif

    wxPoint GetPosition() const override
    {
        static wxPoint dummy(0, 0);
        return dummy;
    }

    void SetPosition( const wxPoint& aPos ) override
    {
    }

    void SetNetClass( const NETCLASSPTR& aNetClass );

    /**
     * @note Do **not** return a std::shared_ptr from this.  It is used heavily in DRC, and the
     *       std::shared_ptr stuff shows up large in performance profiling.
     */
    NETCLASS* GetNetClass()
    {
        return m_netClass.get();
    }

    wxString GetNetClassName() const
    {
        return m_netClass ? m_netClass->GetName() : NETCLASS::Default;
    }

    int GetNetCode() const { return m_netCode; }
    void SetNetCode( int aNetCode ) { m_netCode = aNetCode; }

    /**
     * @return the full netname.
     */
    const wxString& GetNetname() const { return m_netname; }

    /**
     * @return the short netname.
     */
    const wxString& GetShortNetname() const { return m_shortNetname; }

    /**
     * Set the long netname to \a aNetName, and the short netname to the last token in
     * the long netname's path.
     */
    void SetNetname( const wxString& aNewName )
    {
        m_netname = aNewName;

        if( aNewName.Contains( "/" ) )
            m_shortNetname = aNewName.AfterLast( '/' );
        else
            m_shortNetname = aNewName;
    }

    bool IsCurrent() const { return m_isCurrent; }
    void SetIsCurrent( bool isCurrent ) { m_isCurrent = isCurrent; }

    /**
     * Return the information about the #NETINFO_ITEM in \a aList to display in the
     * message panel.
     *
     * @param aList is the list in which to place the  status information.
     */
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /**
     * Set all fields to their default values.
     */
    void Clear()
    {
        SetNetClass( NETCLASSPTR());
    }

    BOARD* GetParent() const
    {
        return m_parent;
    }

private:
    friend class NETINFO_LIST;

    int         m_netCode;         ///< A number equivalent to the net name.
    wxString    m_netname;         ///< Full net name like /sheet/subsheet/vout used by Eeschema.
    wxString    m_shortNetname;    ///< short net name, like vout from /sheet/subsheet/vout.

    NETCLASSPTR m_netClass;

    bool        m_isCurrent;       ///< Indicates the net is currently in use.  We still store
                                   ///< those that are not during a session for undo/redo and to
                                   ///< keep netclass membership information.

    BOARD*      m_parent;          ///< The parent board the net belongs to.
};


class NETINFO_MAPPING
{
public:
    NETINFO_MAPPING()
    {
        m_board = nullptr;
    }


    /**
     * Set a BOARD object that is used to prepare the net code map.
     */
    void SetBoard( const BOARD* aBoard )
    {
        m_board = aBoard;
        Update();
    }

    /**
     * Prepare a mapping for net codes so they can be saved as consecutive numbers.
     *
     * To retrieve a mapped net code, use translateNet() function after calling this.
     */
    void Update();

    /**
     * Translate net number according to the map prepared by Update() function.
     *
     * It allows one to have items stored with consecutive net codes.
     *
     * @param aNetCode is an old net code.
     * @return Net code that follows the mapping.
     */
    int Translate( int aNetCode ) const;

    ///< Wrapper class, so you can iterate through NETINFO_ITEM*s, not
    ///< std::pair<int/wxString, NETINFO_ITEM*>
    class iterator
    {
    public:
        iterator( std::map<int, int>::const_iterator aIter, const NETINFO_MAPPING* aMapping ) :
            m_iterator( aIter ), m_mapping( aMapping )
        {
        }

        /// pre-increment operator
        const iterator& operator++()
        {
            ++m_iterator;

            return *this;
        }

        /// post-increment operator
        iterator operator++( int )
        {
            iterator ret = *this;
            ++m_iterator;

            return ret;
        }

        NETINFO_ITEM* operator*() const;

        NETINFO_ITEM* operator->() const;

        bool operator!=( const iterator& aOther ) const
        {
            return m_iterator != aOther.m_iterator;
        }

        bool operator==( const iterator& aOther ) const
        {
            return m_iterator == aOther.m_iterator;
        }

    private:
        std::map<int, int>::const_iterator m_iterator;
        const NETINFO_MAPPING*             m_mapping;
    };

    /**
     * Return iterator to the first entry in the mapping.
     *
     * @note The entry is a pointer to the original NETINFO_ITEM object, this it contains
     *       not mapped net code.
     */
    iterator begin() const
    {
        return iterator( m_netMapping.begin(), this );
    }

    /**
     * Return iterator to the last entry in the mapping.
     *
     * @note The entry is a pointer to the original NETINFO_ITEM object, this it contains
     *       not mapped net code.
     */
    iterator end() const
    {
        return iterator( m_netMapping.end(), this );
    }

    /**
     * @return Number of mapped nets (i.e. not empty nets for a given BOARD object).
     */
    int GetSize() const
    {
        return m_netMapping.size();
    }

private:
    const BOARD*       m_board;         ///< Board for which mapping is prepared
    std::map<int, int> m_netMapping;    ///< Map that allows saving net codes with consecutive
                                        ///< numbers (for compatibility reasons)
};


#if 0
// waiting for swig to support std::unordered_map, see
// http://www.swig.org/Doc3.0/CPlusPlus11.html
// section 7.3.3
#include <hashtables.h>
DECL_HASH_FOR_SWIG( NETNAMES_MAP, wxString,  NETINFO_ITEM* )
DECL_HASH_FOR_SWIG( NETCODES_MAP, int,       NETINFO_ITEM* )
#else
// use std::map for now
DECL_MAP_FOR_SWIG( NETNAMES_MAP, wxString,  NETINFO_ITEM* )
DECL_MAP_FOR_SWIG( NETCODES_MAP, int,       NETINFO_ITEM* )
#endif

/**
 * Container for #NETINFO_ITEM elements, which are the nets.
 */
class NETINFO_LIST
{
    friend class BOARD;

public:
    NETINFO_LIST( BOARD* aParent );
    ~NETINFO_LIST();

    /**
     * @param aNetCode netcode to identify a given #NETINFO_ITEM.
     * @return net item by \a aNetCode, or NULL if not found.
     */
    NETINFO_ITEM* GetNetItem( int aNetCode ) const;

    /**
     * @param aNetName net name to identify a given #NETINFO_ITEM.
     * @return net item by \a aNetName, or NULL if not found.
     */
    NETINFO_ITEM* GetNetItem( const wxString& aNetName ) const;

    /**
     * @return the number of nets ( always >= 1 ) because the first net is the "not connected"
     *         net and always exists
     */
    unsigned GetNetCount() const { return m_netNames.size(); }

    /**
     * Add \a aNewElement to the end of the net list. Negative net code means it is going to be
     * auto-assigned.
     */
    void AppendNet( NETINFO_ITEM* aNewElement );

    /**
     * Remove a net from the net list.
     */
    void RemoveNet( NETINFO_ITEM* aNet );
    void RemoveUnusedNets();

    /**
     * @return the number of pads in board.
     */

    /// Return the name map, at least for python.
    const NETNAMES_MAP& NetsByName() const      { return  m_netNames; }

    /// Return the netcode map, at least for python.
    const NETCODES_MAP& NetsByNetcode() const   { return m_netCodes; }

    ///< Constant that holds the "unconnected net" number (typically 0)
    ///< all items "connected" to this net are actually not connected items
    static const int UNCONNECTED;

    ///< Constant that forces initialization of a netinfo item to the NETINFO_ITEM ORPHANED
    ///< (typically -1) when calling SetNetCode on board connected items.
    static const int ORPHANED;

    ///< NETINFO_ITEM meaning that there was no net assigned for an item, as there was no
    ///< board storing net list available.
    static NETINFO_ITEM* OrphanedItem()
    {
        static NETINFO_ITEM* g_orphanedItem;

        if( !g_orphanedItem )
            g_orphanedItem = new NETINFO_ITEM( nullptr, wxEmptyString, NETINFO_LIST::UNCONNECTED );

        return g_orphanedItem;
    }

#if defined(DEBUG)
    void Show() const;
#endif

#ifndef SWIG
    ///< Wrapper class, so you can iterate through NETINFO_ITEM*s, not
    ///< std::pair<int/wxString, NETINFO_ITEM*>
    class iterator
    {
    public:
        iterator( NETNAMES_MAP::const_iterator aIter ) : m_iterator( aIter )
        {
        }

        /// pre-increment operator
        const iterator& operator++()
        {
            ++m_iterator;
            return *this;
        }

        /// post-increment operator
        iterator operator++( int )
        {
            iterator ret = *this;
            ++m_iterator;
            return ret;
        }

        NETINFO_ITEM* operator*() const
        {
            return m_iterator->second;
        }

        NETINFO_ITEM* operator->() const
        {
            return m_iterator->second;
        }

        bool operator!=( const iterator& aOther ) const
        {
            return m_iterator != aOther.m_iterator;
        }

        bool operator==( const iterator& aOther ) const
        {
            return m_iterator == aOther.m_iterator;
        }

    private:
        NETNAMES_MAP::const_iterator m_iterator;
    };

    iterator begin() const
    {
        return iterator( m_netNames.begin() );
    }

    iterator end() const
    {
        return iterator( m_netNames.end() );
    }
#endif

    BOARD* GetParent() const
    {
        return m_parent;
    }

private:
    /**
     * Delete the list of nets (and free memory).
     */
    void clear();

    /**
     * Rebuild the list of NETINFO_ITEMs
     *
     * The list is sorted by names.
     */
    void buildListOfNets();

    /**
     * Return the first available net code that is not used by any other net.
     */
    int getFreeNetCode();

    BOARD*       m_parent;

    NETNAMES_MAP m_netNames;        ///< map of <wxString, NETINFO_ITEM*>, is NETINFO_ITEM owner
    NETCODES_MAP m_netCodes;        ///< map of <int, NETINFO_ITEM*> is NOT owner

    int          m_newNetCode;      ///< possible value for new net code assignment
};

#endif  // CLASS_NETINFO_
