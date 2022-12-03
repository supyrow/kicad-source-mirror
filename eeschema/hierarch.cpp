/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <sch_edit_frame.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>

#include <hierarch.h>
#include <kiface_base.h>
#include <eeschema_settings.h>

#include <wx/object.h>
#include <wx/generic/textdlgg.h>
#include <wx/menu.h>

/**
 * Store an SCH_SHEET_PATH of each sheet in hierarchy.
 */
class TREE_ITEM_DATA : public wxTreeItemData
{
public:
    SCH_SHEET_PATH m_SheetPath;

    TREE_ITEM_DATA( SCH_SHEET_PATH& sheet ) :
            wxTreeItemData()
    {
        m_SheetPath = sheet;
    }
};


// Need to use wxRTTI macros in order for OnCompareItems to work properly
// See: https://docs.wxwidgets.org/3.1/classwx_tree_ctrl.html#ab90a465793c291ca7aa827a576b7d146
wxIMPLEMENT_ABSTRACT_CLASS( HIERARCHY_TREE, wxTreeCtrl );


int HIERARCHY_TREE::OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 )
{
    SCH_SHEET_PATH* item1Path = &static_cast<TREE_ITEM_DATA*>( GetItemData( item1 ) )->m_SheetPath;
    SCH_SHEET_PATH* item2Path = &static_cast<TREE_ITEM_DATA*>( GetItemData( item2 ) )->m_SheetPath;

    return item1Path->ComparePageNum( *item2Path );
}


HIERARCHY_NAVIG_PANEL::HIERARCHY_NAVIG_PANEL( SCH_EDIT_FRAME* aParent ) :
        WX_PANEL( aParent )
{
    wxASSERT( dynamic_cast<SCH_EDIT_FRAME*>( aParent ) );

    m_frame = aParent;

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( sizer );
    m_tree = new HIERARCHY_TREE( this );

    // Make an image list containing small icons
    // All icons are expected having the same size.
    wxBitmap tree_nosel_bm( KiBitmap( BITMAPS::tree_nosel ) );
    wxImageList* imageList = new wxImageList( tree_nosel_bm.GetWidth(), tree_nosel_bm.GetHeight(),
                                              true, 2 );

    imageList->Add( tree_nosel_bm );
    imageList->Add( KiBitmap( BITMAPS::tree_sel ) );

    m_tree->AssignImageList( imageList );

    sizer->Add( m_tree, 1, wxEXPAND, wxBORDER_NONE, 0 );

    m_events_bound = false;

    UpdateHierarchyTree();

    // Enable selection events
    Bind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_NAVIG_PANEL::onSelectSheetPath, this );
    Bind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_NAVIG_PANEL::onSelectSheetPath, this );
    Bind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_NAVIG_PANEL::onRightClick, this );

    m_events_bound = true;
}


HIERARCHY_NAVIG_PANEL::~HIERARCHY_NAVIG_PANEL()
{
    Unbind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_NAVIG_PANEL::onSelectSheetPath, this );
    Unbind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_NAVIG_PANEL::onSelectSheetPath, this );
    Unbind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_NAVIG_PANEL::onRightClick, this );
}


void HIERARCHY_NAVIG_PANEL::buildHierarchyTree( SCH_SHEET_PATH* aList, const wxTreeItemId& aParent )
{
    std::vector<SCH_ITEM*> sheetChildren;
    aList->LastScreen()->GetSheets( &sheetChildren );

    for( SCH_ITEM* aItem : sheetChildren )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( aItem );
        aList->push_back( sheet );

        wxString sheetName = formatPageString( sheet->GetFields()[SHEETNAME].GetShownText(),
                                               aList->GetPageNumber() );
        wxTreeItemId child = m_tree->AppendItem( aParent, sheetName, 0, 1 );
        m_tree->SetItemData( child, new TREE_ITEM_DATA( *aList ) );

        buildHierarchyTree( aList, child );
        aList->pop_back();
    }

    m_tree->SortChildren( aParent );
}


void HIERARCHY_NAVIG_PANEL::UpdateHierarchySelection()
{
    bool eventsWereBound = m_events_bound;

    if( eventsWereBound )
    {
        // Disable selection events
        Unbind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_NAVIG_PANEL::onSelectSheetPath, this );
        Unbind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_NAVIG_PANEL::onSelectSheetPath, this );
        Unbind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_NAVIG_PANEL::onRightClick, this );

        m_events_bound = false;
    }

    bool sheetSelected = false;

    std::function<void( const wxTreeItemId& )> recursiveDescent =
            [&]( const wxTreeItemId& id )
            {
                wxCHECK_RET( id.IsOk(), wxT( "Invalid tree item" ) );

                TREE_ITEM_DATA* itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( id ) );

                if( itemData->m_SheetPath == m_frame->GetCurrentSheet() )
                {
                    m_tree->EnsureVisible( id );
                    m_tree->SetItemBold( id, true );
                }
                else
                {
                    m_tree->SetItemBold( id, false );
                }

                if( itemData->m_SheetPath.Last()->IsSelected() )
                {
                    m_tree->EnsureVisible( id );
                    m_tree->SelectItem( id );
                    sheetSelected = true;
                }

                wxTreeItemIdValue cookie;
                wxTreeItemId      child = m_tree->GetFirstChild( id, cookie );

                while( child.IsOk() )
                {
                    recursiveDescent( child );
                    child = m_tree->GetNextChild( id, cookie );
                }
            };

    recursiveDescent( m_tree->GetRootItem() );

    if( !sheetSelected && m_tree->GetSelection() )
        m_tree->SelectItem( m_tree->GetSelection(), false );

    if( eventsWereBound )
    {
        // Enable selection events
        Bind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_NAVIG_PANEL::onSelectSheetPath, this );
        Bind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_NAVIG_PANEL::onSelectSheetPath, this );
        Bind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_NAVIG_PANEL::onRightClick, this );

        m_events_bound = true;
    }
}


void HIERARCHY_NAVIG_PANEL::UpdateHierarchyTree()
{
    Freeze();

    bool eventsWereBound = m_events_bound;

    if( eventsWereBound )
    {
        // Disable selection events
        Unbind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_NAVIG_PANEL::onSelectSheetPath, this );
        Unbind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_NAVIG_PANEL::onSelectSheetPath, this );
        Unbind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_NAVIG_PANEL::onRightClick, this );

        m_events_bound = false;
    }

    m_list.clear();
    m_list.push_back( &m_frame->Schematic().Root() );

    m_tree->DeleteAllItems();

    wxTreeItemId root = m_tree->AddRoot( getRootString(), 0, 1 );
    m_tree->SetItemData( root, new TREE_ITEM_DATA( m_list ) );

    buildHierarchyTree( &m_list, root );
    UpdateHierarchySelection();

    m_tree->ExpandAll();

    if( eventsWereBound )
    {
        // Enable selection events
        Bind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_NAVIG_PANEL::onSelectSheetPath, this );
        Bind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_NAVIG_PANEL::onSelectSheetPath, this );
        Bind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_NAVIG_PANEL::onRightClick, this );

        m_events_bound = true;
    }

    Thaw();
}


void HIERARCHY_NAVIG_PANEL::onSelectSheetPath( wxTreeEvent& aEvent )
{
    wxTreeItemId  itemSel = m_tree->GetSelection();
    TREE_ITEM_DATA* itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( itemSel ) );

    SetCursor( wxCURSOR_ARROWWAIT );
    m_frame->GetToolManager()->RunAction( EE_ACTIONS::changeSheet, true, &itemData->m_SheetPath );
    SetCursor( wxCURSOR_ARROW );
}


void HIERARCHY_NAVIG_PANEL::onRightClick( wxTreeEvent& aEvent )
{
    wxMenu ctxMenu;

    ctxMenu.Append( 1, _( "Edit Page Number" ) );

    if( GetPopupMenuSelectionFromUser( ctxMenu ) == 1 )
    {
        wxTreeItemId  itemSel = m_tree->GetSelection();
        TREE_ITEM_DATA* itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( itemSel ) );

        wxString msg;
        wxString sheetPath = itemData->m_SheetPath.PathHumanReadable( false );
        wxString pageNumber = itemData->m_SheetPath.GetPageNumber();

        msg.Printf( _( "Enter page number for sheet path%s" ),
                    ( sheetPath.Length() > 20 ) ? "\n" + sheetPath : " " + sheetPath );

        wxTextEntryDialog dlg( m_frame, msg, _( "Edit Sheet Page Number" ), pageNumber );

        dlg.SetTextValidator( wxFILTER_ALPHANUMERIC );  // No white space.

        if( dlg.ShowModal() == wxID_OK && dlg.GetValue() != itemData->m_SheetPath.GetPageNumber() )
        {
            m_frame->SaveCopyInUndoList( itemData->m_SheetPath.LastScreen(),
                                         itemData->m_SheetPath.Last(), UNDO_REDO::CHANGED, false );

            itemData->m_SheetPath.SetPageNumber( dlg.GetValue() );

            if( itemData->m_SheetPath == m_frame->GetCurrentSheet() )
            {
                m_frame->GetScreen()->SetPageNumber( dlg.GetValue() );
                m_frame->OnPageSettingsChange();
            }

            m_frame->OnModify();
        }
    }
}


wxString HIERARCHY_NAVIG_PANEL::getRootString()
{
    SCH_SHEET*     rootSheet = &m_frame->Schematic().Root();
    SCH_SHEET_PATH rootPath;
    rootPath.push_back( rootSheet );

    return formatPageString ( _( "Root" ), rootPath.GetPageNumber() );
}


wxString HIERARCHY_NAVIG_PANEL::formatPageString( const wxString& aName, const wxString& aPage )
{
    return aName + wxT( " " ) + wxString::Format( _( "(page %s)" ), aPage );
}
