/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2020 KiCad Developers, see change_log.txt for contributors.
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

#ifndef EDA_ITEM_H
#define EDA_ITEM_H

#include <deque>

#include <core/typeinfo.h>
#include <eda_item_flags.h>
#include <view/view_item.h>
#include <kiid.h>


enum class BITMAPS : unsigned int;


enum class SEARCH_RESULT
{
    QUIT,
    CONTINUE
};


/**
 * Additional flag values wxFindReplaceData::m_Flags
 */
enum FIND_REPLACE_FLAGS
{
    // The last wxFindReplaceFlag enum is wxFR_MATCHCASE = 0x4.
    FR_CURRENT_SHEET_ONLY = 0x4 << 1,   // Search the current sheet only.
    FR_SEARCH_ALL_FIELDS  = 0x4 << 2,   // Search hidden fields too.
    FR_SEARCH_ALL_PINS    = 0x4 << 3,   // Search pin name and number.
    FR_MATCH_WILDCARD     = 0x4 << 4,   // Use simple wild card matching (* & ?).
    FR_SEARCH_WRAP        = 0x4 << 5,   // Wrap around the start or end of search.
    FR_SEARCH_REPLACE     = 0x4 << 7,   // Search for a item that has replaceable text.
    FR_REPLACE_ITEM_FOUND = 0x4 << 8,   // Indicates an item with replaceable text has been found.
    FR_REPLACE_REFERENCES = 0x4 << 9    // Don't replace in references.
};


class wxFindReplaceData;
class EDA_DRAW_FRAME;
class EDA_RECT;
class MSG_PANEL_ITEM;


/**
 * Used to inspect and possibly collect the (search) results of iterating over a list or
 * tree of #KICAD_T objects.
 *
 * Provide an implementation as needed to inspect EDA_ITEMs visited via #EDA_ITEM::Visit()
 * and #EDA_ITEM::IterateForward().
 *
 * FYI the std::function may hold a lambda, std::bind, pointer to func, or ptr to member
 * function, per modern C++. It is used primarily for searching, but not limited to that.
 * It can also collect or modify the scanned objects.  'Capturing' lambdas are particularly
 * convenient because they can use context and this often means @a aTestData is not used.
 *
 * @param aItem An #EDA_ITEM to examine.
 * @param aTestData is arbitrary data needed by the inspector to determine
 *  if the EDA_ITEM under test meets its match criteria, and is often NULL
 *  with the advent of capturing lambdas.
 * @return A #SEARCH_RESULT type #SEARCH_QUIT if the iterator function is to
 *          stop the scan, else #SEARCH_CONTINUE;
 */
typedef std::function< SEARCH_RESULT ( EDA_ITEM* aItem, void* aTestData ) > INSPECTOR_FUNC;

///< std::function passed to nested users by ref, avoids copying std::function.
typedef const INSPECTOR_FUNC& INSPECTOR;


/**
 * A base class for most all the KiCad significant classes used in schematics and boards.
 */
class EDA_ITEM : public KIGFX::VIEW_ITEM
{
public:
    virtual ~EDA_ITEM() { };

    /**
     * Returns the type of object.
     *
     * This attribute should never be changed after a ctor sets it, so there is no public
     * "setter" method.
     *
     * @return the type of object.
     */
    inline KICAD_T Type() const { return m_structType; }

    EDA_ITEM* GetParent() const { return m_parent; }
    virtual void SetParent( EDA_ITEM* aParent )   { m_parent = aParent; }

    inline bool IsModified() const { return m_flags & IS_CHANGED; }
    inline bool IsNew() const { return m_flags & IS_NEW; }
    inline bool IsMoving() const { return m_flags & IS_MOVING; }
    inline bool IsDragging() const { return m_flags & IS_DRAGGING; }
    inline bool IsWireImage() const { return m_flags & IS_WIRE_IMAGE; }
    inline bool IsSelected() const { return m_flags & SELECTED; }
    inline bool IsEntered() const { return m_flags & ENTERED; }
    inline bool IsResized() const { return m_flags & IS_RESIZING; }
    inline bool IsBrightened() const { return m_flags & BRIGHTENED; }

    inline void SetWireImage() { SetFlags( IS_WIRE_IMAGE ); }
    inline void SetSelected() { SetFlags( SELECTED ); }
    inline void SetBrightened() { SetFlags( BRIGHTENED ); }

    inline void ClearSelected() { ClearFlags( SELECTED ); }
    inline void ClearBrightened() { ClearFlags( BRIGHTENED ); }

    void SetModified();

    int GetState( EDA_ITEM_FLAGS type ) const
    {
        return m_status & type;
    }

    void SetState( EDA_ITEM_FLAGS type, bool state )
    {
        if( state )
            m_status |= type; // state = ON or OFF
        else
            m_status &= ~type;
    }

    EDA_ITEM_FLAGS GetStatus() const { return m_status; }
    void           SetStatus( EDA_ITEM_FLAGS aStatus ) { m_status = aStatus; }

    void           SetFlags( EDA_ITEM_FLAGS aMask ) { m_flags |= aMask; }
    void           ClearFlags( EDA_ITEM_FLAGS aMask = EDA_ITEM_ALL_FLAGS ) { m_flags &= ~aMask; }
    EDA_ITEM_FLAGS GetFlags() const { return m_flags; }
    bool           HasFlag( EDA_ITEM_FLAGS aFlag ) const { return ( m_flags & aFlag ) == aFlag; }

    EDA_ITEM_FLAGS GetEditFlags() const
    {
        constexpr int mask = ( IS_NEW | IS_PASTED | IS_MOVING | IS_RESIZING | IS_DRAGGING
                               | IS_WIRE_IMAGE | STRUCT_DELETED );

        return m_flags & mask;
    }

    void ClearTempFlags()
    {
        ClearFlags( STARTPOINT | ENDPOINT | CANDIDATE | TEMP_SELECTED | IS_LINKED | SKIP_STRUCT |
                    DO_NOT_DRAW );
    }

    void ClearEditFlags()
    {
        ClearFlags( GetEditFlags() );
    }

    /**
     * Check whether the item is one of the listed types.
     *
     * @param aScanTypes List of item types
     * @return true if the item type is contained in the list aScanTypes
     */
    virtual bool IsType( const KICAD_T aScanTypes[] ) const
    {
        if( aScanTypes[0] == SCH_LOCATE_ANY_T )
            return true;

        for( const KICAD_T* p = aScanTypes; *p != EOT; ++p )
        {
            if( m_structType == *p )
                return true;
        }

        return false;
    }

    /**
     * Set and clear force visible flag used to force the item to be drawn even if it's draw
     * attribute is set to not visible.
     *
     * @param aEnable True forces the item to be drawn.  False uses the item's visibility
     *                setting to determine if the item is to be drawn.
     */
    void SetForceVisible( bool aEnable ) { m_forceVisible = aEnable; }

    bool IsForceVisible() const { return m_forceVisible; }

    /**
     * Populate \a aList of #MSG_PANEL_ITEM objects with it's internal state for display
     * purposes.
     *
     * @param aList is the list to populate.
     */
    virtual void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
    {
    }

    /**
     * Test if \a aPosition is inside or on the boundary of this item.
     *
     * @param aPosition A reference to a wxPoint object containing the coordinates to test.
     * @param aAccuracy Increase the item bounding box by this amount.
     * @return True if \a aPosition is within the item bounding box.
     */
    virtual bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const
    {
        return false;   // derived classes should override this function
    }

    /**
     * Test if \a aRect intersects this item.
     *
     * @param aRect A reference to a #EDA_RECT object containing the rectangle to test.
     * @param aContained Set to true to test for containment instead of an intersection.
     * @param aAccuracy Increase \a aRect by this amount.
     * @return True if \a aRect contains or intersects the item bounding box.
     */
    virtual bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const
    {
        return false;   // derived classes should override this function
    }

    /**
     * Return the orthogonal bounding box of this object for display purposes.
     *
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate
     * system.  It is OK to overestimate the size by a few counts.
     */
    virtual const EDA_RECT GetBoundingBox() const;

    virtual wxPoint GetPosition() const { return wxPoint(); }
    virtual void SetPosition( const wxPoint& aPos ) {};

    /**
     * Similar to GetPosition, but allows items to return their visual center rather
     * than their anchor.
     */
    virtual const wxPoint GetFocusPosition() const { return GetPosition(); }

    /**
     * Create a duplicate of this item with linked list members set to NULL.
     *
     * The default version will return NULL in release builds and likely crash the
     * program.  In debug builds, a warning message indicating the derived class
     * has not implemented cloning.  This really should be a pure virtual function.
     * Due to the fact that there are so many objects derived from EDA_ITEM, the
     * decision was made to return NULL until all the objects derived from EDA_ITEM
     * implement cloning.  Once that happens, this function should be made pure.
     *
     * @return A clone of the item.
     */
    virtual EDA_ITEM* Clone() const; // should not be inline, to save the ~ 6 bytes per call site.

    /**
     * May be re-implemented for each derived class in order to handle all the types given
     * by its member data.
     *
     * Implementations should call inspector->Inspect() on types in scanTypes[], and may use
     * #IterateForward() to do so on lists of such data.
     *
     * @param inspector An #INSPECTOR instance to use in the inspection.
     * @param testData Arbitrary data used by the inspector.
     * @param scanTypes Which# KICAD_T types are of interest and the order
     *                  is significant too, terminated by EOT.
     * @return #SEARCH_RESULT SEARCH_QUIT if the Iterator is to stop the scan,
     *         else #SCAN_CONTINUE, and determined by the inspector.
     */
    virtual SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] );

    /**
     * This changes first parameter to avoid the DList and use the main queue instead.
     */
    template< class T >
    static SEARCH_RESULT IterateForward( std::deque<T>&  aList,
                                         INSPECTOR       inspector,
                                         void*           testData,
                                         const KICAD_T   scanTypes[] )
    {
        for( auto it : aList )
        {
            if( static_cast<EDA_ITEM*>( it )->Visit( inspector, testData, scanTypes )
                    == SEARCH_RESULT::QUIT )
                return SEARCH_RESULT::QUIT;
        }

        return SEARCH_RESULT::CONTINUE;
    }

    /**
     * Change first parameter to avoid the DList and use std::vector instead.
     */
    template <class T>
    static SEARCH_RESULT IterateForward(
            std::vector<T>& aList, INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] )
    {
        for( auto it : aList )
        {
            if( static_cast<EDA_ITEM*>( it )->Visit( inspector, testData, scanTypes )
                    == SEARCH_RESULT::QUIT )
                return SEARCH_RESULT::QUIT;
        }

        return SEARCH_RESULT::CONTINUE;
    }

    /**
     * Return the class name.
     */
    virtual wxString GetClass() const = 0;

    /**
     * Return the text to display to be used in the selection clarification context menu
     * when multiple items are found at the current cursor position.
     *
     * The default version of this function raises an assertion in the debug mode and
     * returns a string to indicate that it was not overridden to provide the object
     * specific text.
     *
     * @return The menu text string.
     */
    virtual wxString GetSelectMenuText( EDA_UNITS aUnits ) const;

    /**
     * Return a pointer to an image to be used in menus.
     *
     * The default version returns the right arrow image.  Override this function to provide
     * object specific menu images.
     *
     * @return The menu image associated with the item.
     */
    virtual BITMAPS GetMenuImage() const;

    /**
     * Compare the item against the search criteria in \a aSearchData.
     *
     * The base class returns false since many of the objects derived from EDA_ITEM
     * do not have any text to search.
     *
     * @param aSearchData A reference to a wxFindReplaceData object containing the
     *                    search criteria.
     * @param aAuxData A pointer to optional data required for the search or NULL if not used.
     * @return True if the item's text matches the search criteria in \a aSearchData.
     */
    virtual bool Matches( const wxFindReplaceData& aSearchData, void* aAuxData ) const
    {
        return false;
    }

    /**
     * Perform a text replace on \a aText using the find and replace criteria in
     * \a aSearchData on items that support text find and replace.
     *
     * @param aSearchData A reference to a wxFindReplaceData object containing the
     *                    search and replace criteria.
     * @param aText A reference to a wxString object containing the text to be replaced.
     * @return True if \a aText was modified, otherwise false.
     */
    static bool Replace( const wxFindReplaceData& aSearchData, wxString& aText );

    /**
     * Perform a text replace using the find and replace criteria in \a aSearchData
     * on items that support text find and replace.
     *
     * This function must be overridden for items that support text replace.
     *
     * @param aSearchData A reference to a wxFindReplaceData object containing the search and
     *                    replace criteria.
     * @param aAuxData A pointer to optional data required for the search or NULL if not used.
     * @return True if the item text was modified, otherwise false.
     */
    virtual bool Replace( const wxFindReplaceData& aSearchData, void* aAuxData = nullptr )
    {
        return false;
    }

    /**
     * Override this method in any derived object that supports test find and replace.
     *
     * @return True if the item has replaceable text that can be modified using
     *         the find and replace dialog.
     */
    virtual bool IsReplaceable() const { return false; }

    /**
     * Test if another item is less than this object.
     *
     * @param aItem - Item to compare against.
     * @return - True if \a aItem is less than the item.
     */
    bool operator<( const EDA_ITEM& aItem ) const;

    /**
     * Helper function to be used by the C++ STL sort algorithm for sorting a STL
     * container of #EDA_ITEM pointers.
     *
     * @param aLeft The left hand item to compare.
     * @param aRight The right hand item to compare.
     * @return True if \a aLeft is less than \a aRight.
     */
    static bool Sort( const EDA_ITEM* aLeft, const EDA_ITEM* aRight ) { return *aLeft < *aRight; }

    /**
     * Assign the members of \a aItem to another object.
     */
    EDA_ITEM& operator=( const EDA_ITEM& aItem );

    virtual const BOX2I ViewBBox() const override;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

#if defined(DEBUG)

    /**
     * Output the object tree, currently for debugging only.
     *
     * This is pure virtual so compiler warns if somebody mucks up a derived declaration.
     *
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *                  of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os ) const = 0;

    void ShowDummy( std::ostream& os ) const;  ///< call this if you are a lazy developer

    /**
     * Output nested space for pretty indenting.
     *
     * @param nestLevel The nest count.
     * @param os The ostream&, where to output
     * @return The std::ostream& for continuation.
     **/
    static std::ostream& NestedSpace( int nestLevel, std::ostream& os );

#endif

protected:
    EDA_ITEM( EDA_ITEM* parent, KICAD_T idType );
    EDA_ITEM( KICAD_T idType );
    EDA_ITEM( const EDA_ITEM& base );

    /**
     * Compare \a aText against search criteria in \a aSearchData.
     *
     * This is a helper function for simplify derived class logic.
     *
     * @param aText A reference to a wxString object containing the string to test.
     * @param aSearchData The criteria to search against.
     * @return True if \a aText matches the search criteria in \a aSearchData.
     */
    bool Matches( const wxString& aText, const wxFindReplaceData& aSearchData ) const;

public:
    const KIID  m_Uuid;

protected:
    EDA_ITEM_FLAGS m_status;
    EDA_ITEM*      m_parent; ///< Linked list: Link (parent struct)
    bool           m_forceVisible;
    EDA_ITEM_FLAGS m_flags;

private:
    /**
     * Run time identification, _keep private_ so it can never be changed after a ctor
     * sets it.  See comment near SetType() regarding virtual functions.
     */
    KICAD_T       m_structType;
};


/**
 * Provide cloning capabilities for all Boost pointer containers of #EDA_ITEM pointers.
 *
 * @param aItem EDA_ITEM to clone.
 * @return Clone of \a aItem.
 */
inline EDA_ITEM* new_clone( const EDA_ITEM& aItem ) { return aItem.Clone(); }


/**
 * Define list of drawing items for screens.
 *
 * The standard C++ container was chosen so the pointer can be removed  from a list without
 * it being destroyed.
 */
typedef std::vector< EDA_ITEM* > EDA_ITEMS;

#endif // EDA_ITEM_H
