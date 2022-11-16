/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#include <kiway.h>
#include <pgm_base.h>
#include <sch_painter.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/symbol_editor_control.h>
#include <symbol_edit_frame.h>
#include <lib_symbol_library_manager.h>
#include <symbol_viewer_frame.h>
#include <symbol_tree_model_adapter.h>
#include <wildcards_and_files_ext.h>
#include <bitmaps/bitmap_types.h>
#include <confirm.h>
#include <wx/filedlg.h>
#include "wx/generic/textdlgg.h"
#include "string_utils.h"

bool SYMBOL_EDITOR_CONTROL::Init()
{
    m_frame = getEditFrame<SCH_BASE_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    m_isSymbolEditor = m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR );

    if( m_isSymbolEditor )
    {
        CONDITIONAL_MENU& ctxMenu = m_menu.GetMenu();
        SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();

        wxCHECK( editFrame, false );

        auto libSelectedCondition =
                [ editFrame ]( const SELECTION& aSel )
                {
                    LIB_ID sel = editFrame->GetTreeLIBID();
                    return !sel.GetLibNickname().empty() && sel.GetLibItemName().empty();
                };
        // The libInferredCondition allows you to do things like New Symbol and Paste with a
        // symbol selected (in other words, when we know the library context even if the library
        // itself isn't selected.
        auto libInferredCondition =
                [ editFrame ]( const SELECTION& aSel )
                {
                    LIB_ID sel = editFrame->GetTreeLIBID();
                    return !sel.GetLibNickname().empty();
                };
        auto pinnedLibSelectedCondition =
                [ editFrame ]( const SELECTION& aSel )
                {
                    LIB_TREE_NODE* current = editFrame->GetCurrentTreeNode();
                    return current && current->m_Type == LIB_TREE_NODE::LIB && current->m_Pinned;
                };
        auto unpinnedLibSelectedCondition =
                [ editFrame ](const SELECTION& aSel )
                {
                    LIB_TREE_NODE* current = editFrame->GetCurrentTreeNode();
                    return current && current->m_Type == LIB_TREE_NODE::LIB && !current->m_Pinned;
                };
        auto symbolSelectedCondition =
                [ editFrame ]( const SELECTION& aSel )
                {
                    LIB_ID sel = editFrame->GetTargetLibId();
                    return !sel.GetLibNickname().empty() && !sel.GetLibItemName().empty();
                };
        auto saveSymbolAsCondition =
                [ editFrame ]( const SELECTION& aSel )
                {
                    LIB_ID sel = editFrame->GetTargetLibId();
                    return !sel.GetLibNickname().empty() && !sel.GetLibItemName().empty();
                };
        auto multiSelectedCondition =
                [ editFrame ]( const SELECTION& aSel )
                {
                    return editFrame->GetTreeSelectionCount() > 1;
                };

        ctxMenu.AddItem( ACTIONS::pinLibrary,            unpinnedLibSelectedCondition );
        ctxMenu.AddItem( ACTIONS::unpinLibrary,          pinnedLibSelectedCondition );

        ctxMenu.AddSeparator();
        ctxMenu.AddItem( EE_ACTIONS::newSymbol,          libInferredCondition );

        ctxMenu.AddSeparator();
        ctxMenu.AddItem( ACTIONS::save,                  symbolSelectedCondition || libInferredCondition );
        ctxMenu.AddItem( EE_ACTIONS::saveLibraryAs,      libSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::saveSymbolAs,       saveSymbolAsCondition );
        ctxMenu.AddItem( ACTIONS::revert,                symbolSelectedCondition || libInferredCondition );

        ctxMenu.AddSeparator();
        ctxMenu.AddItem( EE_ACTIONS::cutSymbol,          symbolSelectedCondition || multiSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::copySymbol,         symbolSelectedCondition || multiSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::pasteSymbol,        libInferredCondition );
        ctxMenu.AddItem( EE_ACTIONS::duplicateSymbol,    symbolSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::renameSymbol,       symbolSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::deleteSymbol,       symbolSelectedCondition || multiSelectedCondition );

        ctxMenu.AddSeparator();
        ctxMenu.AddItem( EE_ACTIONS::importSymbol,       libInferredCondition );
        ctxMenu.AddItem( EE_ACTIONS::exportSymbol,       symbolSelectedCondition );

        // If we've got nothing else to show, at least show a hide tree option
        ctxMenu.AddItem( EE_ACTIONS::hideSymbolTree,    !libInferredCondition );
    }

    return true;
}


int SYMBOL_EDITOR_CONTROL::AddLibrary( const TOOL_EVENT& aEvent )
{
    bool createNew = aEvent.IsAction( &ACTIONS::newLibrary );

    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->AddLibraryFile( createNew );

    return 0;
}


int SYMBOL_EDITOR_CONTROL::DdAddLibrary( const TOOL_EVENT& aEvent )
{
    wxString libFile = *aEvent.Parameter<wxString*>();
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->DdAddLibrary( libFile );

    return 0;
}


int SYMBOL_EDITOR_CONTROL::EditSymbol( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        SYMBOL_EDIT_FRAME* editFrame = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );
        int                unit = 0;
        LIB_ID             partId = editFrame->GetTreeLIBID( &unit );

        editFrame->LoadSymbol( partId.GetLibItemName(), partId.GetLibNickname(), unit );
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::AddSymbol( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        SYMBOL_EDIT_FRAME* editFrame = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );

        LIB_ID          sel = editFrame->GetTreeLIBID();
        const wxString& libName = sel.GetLibNickname();
        wxString        msg;

        if( libName.IsEmpty() )
        {
            msg.Printf( _( "No symbol library selected." ) );
            m_frame->ShowInfoBarError( msg );
            return 0;
        }

        if( editFrame->GetLibManager().IsLibraryReadOnly( libName ) )
        {
            msg.Printf( _( "Symbol library '%s' is not writable." ), libName );
            m_frame->ShowInfoBarError( msg );
            return 0;
        }

        if( aEvent.IsAction( &EE_ACTIONS::newSymbol ) )
            editFrame->CreateNewSymbol();
        else if( aEvent.IsAction( &EE_ACTIONS::importSymbol ) )
            editFrame->ImportSymbol();
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::Save( const TOOL_EVENT& aEvt )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        SYMBOL_EDIT_FRAME* editFrame = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );

        if( aEvt.IsAction( &EE_ACTIONS::save ) )
            editFrame->Save();
        else if( aEvt.IsAction( &EE_ACTIONS::saveLibraryAs ) )
            editFrame->SaveLibraryAs();
        else if( aEvt.IsAction( &EE_ACTIONS::saveSymbolAs ) )
            editFrame->SaveSymbolAs();
        else if( aEvt.IsAction( &EE_ACTIONS::saveAll ) )
            editFrame->SaveAll();
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::Revert( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->Revert();

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ExportSymbol( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->ExportSymbol();

    return 0;
}


int SYMBOL_EDITOR_CONTROL::CutCopyDelete( const TOOL_EVENT& aEvt )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        SYMBOL_EDIT_FRAME* editFrame = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );

        if( aEvt.IsAction( &EE_ACTIONS::cutSymbol ) || aEvt.IsAction( &EE_ACTIONS::copySymbol ) )
            editFrame->CopySymbolToClipboard();

        if( aEvt.IsAction( &EE_ACTIONS::cutSymbol ) || aEvt.IsAction( &EE_ACTIONS::deleteSymbol ) )
        {
            bool hasWritableLibs = false;
            wxString msg;

            for( LIB_ID& sel : editFrame->GetSelectedLibIds() )
            {
                const wxString& libName = sel.GetLibNickname();

                if( editFrame->GetLibManager().IsLibraryReadOnly( libName ) )
                    msg.Printf( _( "Symbol library '%s' is not writable." ), libName );
                else
                    hasWritableLibs = true;
            }

            if( !msg.IsEmpty() )
                m_frame->ShowInfoBarError( msg );

            if( !hasWritableLibs )
                return 0;

            editFrame->DeleteSymbolFromLibrary();
        }
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::DuplicateSymbol( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        SYMBOL_EDIT_FRAME* editFrame = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );
        LIB_ID             sel = editFrame->GetTargetLibId();
        // DuplicateSymbol() is called to duplicate a symbol, or to paste a previously
        // saved symbol in clipboard
        bool               isPasteAction = aEvent.IsAction( &EE_ACTIONS::pasteSymbol );
        wxString           msg;

        if( !sel.IsValid() && !isPasteAction )
        {
            // When duplicating a symbol, a source symbol must exists.
            msg.Printf( _( "No symbol selected" ) );
            m_frame->ShowInfoBarError( msg );
            return 0;
        }

        const wxString& libName = sel.GetLibNickname();

        if( editFrame->GetLibManager().IsLibraryReadOnly( libName ) )
        {
            msg.Printf( _( "Symbol library '%s' is not writable." ), libName );
            m_frame->ShowInfoBarError( msg );
            return 0;
        }

        editFrame->DuplicateSymbol( isPasteAction );
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::RenameSymbol( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        SYMBOL_EDIT_FRAME*          editFrame = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );
        LIB_SYMBOL_LIBRARY_MANAGER& libMgr    = editFrame->GetLibManager();

        LIB_ID   libId = editFrame->GetTreeLIBID();
        wxString libName = libId.GetLibNickname();
        wxString symbolName = libId.GetLibItemName();
        wxString newName = symbolName;
        bool     done = false;

        if( !libMgr.LibraryExists( libName ) )
            return 0;

        while( !done )
        {
            wxTextEntryDialog dlg( m_frame, _( "New name:" ), _( "Change Symbol Name" ), newName );

            if( dlg.ShowModal() != wxID_OK )
                return 0;   // canceled by user

            newName = EscapeString( dlg.GetValue(), CTX_LIBID );
            newName.Trim( true ).Trim( false );

            if( newName.IsEmpty() )
            {
                DisplayErrorMessage( editFrame, _( "Symbol name cannot be empty." ) );
            }
            else if( libMgr.SymbolExists( newName, libName ) )
            {
                DisplayErrorMessage( editFrame, wxString::Format( _( "Symbol name '%s' already "
                                                                     "in use in library '%s'." ),
                                                                  UnescapeString( newName ),
                                                                  libName ) );
                newName = symbolName;
            }
            else
            {
                done = true;
            }
        }

        wxString    oldName = symbolName;
        LIB_SYMBOL* libSymbol = nullptr;

        if( editFrame->IsCurrentSymbol( libId ) )
        {
            // Update buffered copy
            libSymbol = libMgr.GetBufferedSymbol( oldName, libName );

            libSymbol->SetName( newName );
            libSymbol->GetFieldById( VALUE_FIELD )->SetText( newName );

            libMgr.UpdateSymbolAfterRename( libSymbol, newName, libName );

            // Now update canvasy copy
            libSymbol = editFrame->GetCurSymbol();

            libSymbol->SetName( newName );
            libSymbol->GetFieldById( VALUE_FIELD )->SetText( newName );

            editFrame->RebuildView();
            editFrame->OnModify();

            // N.B. The view needs to be rebuilt first as the Symbol Properties change may
            // invalidate the view pointers by rebuilting the field table
            editFrame->UpdateMsgPanel();
        }
        else
        {
            libSymbol = libMgr.GetBufferedSymbol( oldName, libName );

            libSymbol->SetName( newName );
            libSymbol->GetFieldById( VALUE_FIELD )->SetText( newName );

            libMgr.UpdateSymbolAfterRename( libSymbol, newName, libName );
            libMgr.SetSymbolModified( newName, libName );
        }

        wxDataViewItem treeItem = libMgr.GetAdapter()->FindItem( libId );
        editFrame->UpdateLibraryTree( treeItem, libSymbol );
        editFrame->FocusOnLibId( LIB_ID( libName, newName ) );
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::OnDeMorgan( const TOOL_EVENT& aEvent )
{
    int convert = aEvent.IsAction( &EE_ACTIONS::showDeMorganStandard ) ?
            LIB_ITEM::LIB_CONVERT::BASE : LIB_ITEM::LIB_CONVERT::DEMORGAN;

    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        m_toolMgr->RunAction( ACTIONS::cancelInteractive, true );
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        SYMBOL_EDIT_FRAME* symbolEditor = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );
        symbolEditor->SetConvert( convert );

        m_toolMgr->ResetTools( TOOL_BASE::MODEL_RELOAD );
        symbolEditor->RebuildView();
    }
    else if( m_frame->IsType( FRAME_SCH_VIEWER ) || m_frame->IsType( FRAME_SCH_VIEWER_MODAL ) )
    {
        SYMBOL_VIEWER_FRAME* symbolViewer = static_cast<SYMBOL_VIEWER_FRAME*>( m_frame );
        symbolViewer->SetUnitAndConvert( symbolViewer->GetUnit(), convert );
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::PinLibrary( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        SYMBOL_EDIT_FRAME* editFrame = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );
        LIB_TREE_NODE*  currentNode = editFrame->GetCurrentTreeNode();

        if( currentNode && !currentNode->m_Pinned )
        {
            m_frame->Prj().PinLibrary( currentNode->m_LibId.GetLibNickname(), true );

            currentNode->m_Pinned = true;
            editFrame->RegenerateLibraryTree();
        }
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::UnpinLibrary( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        SYMBOL_EDIT_FRAME* editFrame = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );
        LIB_TREE_NODE*  currentNode = editFrame->GetCurrentTreeNode();

        if( currentNode && currentNode->m_Pinned )
        {
            m_frame->Prj().UnpinLibrary( currentNode->m_LibId.GetLibNickname(), true );

            currentNode->m_Pinned = false;
            editFrame->RegenerateLibraryTree();
        }
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ToggleSymbolTree( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        wxCommandEvent dummy;
        static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->OnToggleSymbolTree( dummy );
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ShowElectricalTypes( const TOOL_EVENT& aEvent )
{
    KIGFX::SCH_RENDER_SETTINGS* renderSettings = m_frame->GetRenderSettings();
    renderSettings->m_ShowPinsElectricalType = !renderSettings->m_ShowPinsElectricalType;

    // Update canvas
    m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ShowPinNumbers( const TOOL_EVENT& aEvent )
{
    KIGFX::SCH_RENDER_SETTINGS* renderSettings = m_frame->GetRenderSettings();
    renderSettings->m_ShowPinNumbers = !renderSettings->m_ShowPinNumbers;

    // Update canvas
    m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ToggleSyncedPinsMode( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    editFrame->m_SyncPinEdit = !editFrame->m_SyncPinEdit;

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ExportView( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    LIB_SYMBOL*        symbol = editFrame->GetCurSymbol();

    if( !symbol )
    {
        wxMessageBox( _( "No symbol to export" ) );
        return 0;
    }

    wxFileName fn( symbol->GetName() );
    fn.SetExt( "png" );

    wxString projectPath = wxPathOnly( m_frame->Prj().GetProjectFullName() );

    wxFileDialog dlg( editFrame, _( "Image File Name" ), projectPath, fn.GetFullName(),
                      PngFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_OK && !dlg.GetPath().IsEmpty() )
    {
        // calling wxYield is mandatory under Linux, after closing the file selector dialog
        // to refresh the screen before creating the PNG or JPEG image from screen
        wxYield();

        if( !SaveCanvasImageToFile( editFrame, dlg.GetPath(), BITMAP_TYPE::PNG ) )
        {
            wxMessageBox( wxString::Format( _( "Can't save file '%s'." ), dlg.GetPath() ) );
        }
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ExportSymbolAsSVG( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    LIB_SYMBOL*        symbol = editFrame->GetCurSymbol();

    if( !symbol )
    {
        wxMessageBox( _( "No symbol to export" ) );
        return 0;
    }

    wxFileName fn( symbol->GetName() );
    fn.SetExt( SVGFileExtension );

    wxString pro_dir = wxPathOnly( m_frame->Prj().GetProjectFullName() );

    wxString fullFileName = wxFileSelector( _( "SVG File Name" ), pro_dir, fn.GetFullName(),
                                            SVGFileExtension, SVGFileWildcard(), wxFD_SAVE,
                                            m_frame );

    if( !fullFileName.IsEmpty() )
    {
        PAGE_INFO pageSave = editFrame->GetScreen()->GetPageSettings();
        PAGE_INFO pageTemp = pageSave;

        VECTOR2I symbolSize = symbol->GetUnitBoundingBox( editFrame->GetUnit(),
                                                          editFrame->GetConvert() ).GetSize();

        // Add a small margin to the plot bounding box
        pageTemp.SetWidthMils(  int( symbolSize.x * 1.2 ) );
        pageTemp.SetHeightMils( int( symbolSize.y * 1.2 ) );

        editFrame->GetScreen()->SetPageSettings( pageTemp );
        editFrame->SVGPlotSymbol( fullFileName );
        editFrame->GetScreen()->SetPageSettings( pageSave );
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::AddSymbolToSchematic( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* libSymbol = nullptr;
    LIB_ID      libId;
    int         unit, convert;

    if( m_isSymbolEditor )
    {
        SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();

        libSymbol = editFrame->GetCurSymbol();
        unit = editFrame->GetUnit();
        convert = editFrame->GetConvert();

        if( libSymbol )
            libId = libSymbol->GetLibId();
    }
    else
    {
        SYMBOL_VIEWER_FRAME* viewerFrame = getEditFrame<SYMBOL_VIEWER_FRAME>();

        if( viewerFrame->IsModal() )
        {
            // if we're modal then we just need to return the symbol selection; the caller is
            // already in a EE_ACTIONS::placeSymbol coroutine.
            viewerFrame->FinishModal();
            return 0;
        }
        else
        {
            libSymbol = viewerFrame->GetSelectedSymbol();
            unit      = viewerFrame->GetUnit();
            convert   = viewerFrame->GetConvert();

            if( libSymbol )
                libId = libSymbol->GetLibId();
        }
    }

    if( libSymbol )
    {
        SCH_EDIT_FRAME* schframe = (SCH_EDIT_FRAME*) m_frame->Kiway().Player( FRAME_SCH, false );

        if( !schframe )      // happens when the schematic editor is not active (or closed)
        {
            DisplayErrorMessage( m_frame, _( "No schematic currently open." ) );
            return 0;
        }

        wxWindow* blocking_dialog = schframe->Kiway().GetBlockingDialog();

        if( blocking_dialog )
            blocking_dialog->Close( true );

        wxCHECK( libSymbol->GetLibId().IsValid(), 0 );

        SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &schframe->GetCurrentSheet(),
                                             unit, convert );

        symbol->SetParent( schframe->GetScreen() );

        if( schframe->eeconfig()->m_AutoplaceFields.enable )
            symbol->AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );

        schframe->Raise();
        schframe->GetToolManager()->RunAction( EE_ACTIONS::placeSymbol, false, symbol );
    }

    return 0;
}


void SYMBOL_EDITOR_CONTROL::setTransitions()
{
    Go( &SYMBOL_EDITOR_CONTROL::AddLibrary,            ACTIONS::newLibrary.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddLibrary,            ACTIONS::addLibrary.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddSymbol,             EE_ACTIONS::newSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddSymbol,             EE_ACTIONS::importSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::EditSymbol,            EE_ACTIONS::editSymbol.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::DdAddLibrary,          ACTIONS::ddAddLibrary.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::Save,                  ACTIONS::save.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Save,                  EE_ACTIONS::saveLibraryAs.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Save,                  EE_ACTIONS::saveSymbolAs.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Save,                  ACTIONS::saveAll.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Revert,                ACTIONS::revert.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::DuplicateSymbol,       EE_ACTIONS::duplicateSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::RenameSymbol,          EE_ACTIONS::renameSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::CutCopyDelete,         EE_ACTIONS::deleteSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::CutCopyDelete,         EE_ACTIONS::cutSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::CutCopyDelete,         EE_ACTIONS::copySymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::DuplicateSymbol,       EE_ACTIONS::pasteSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ExportSymbol,          EE_ACTIONS::exportSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ExportView,            EE_ACTIONS::exportSymbolView.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ExportSymbolAsSVG,     EE_ACTIONS::exportSymbolAsSVG.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddSymbolToSchematic,  EE_ACTIONS::addSymbolToSchematic.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::OnDeMorgan,            EE_ACTIONS::showDeMorganStandard.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::OnDeMorgan,            EE_ACTIONS::showDeMorganAlternate.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::ShowElectricalTypes,   EE_ACTIONS::showElectricalTypes.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ShowPinNumbers,        EE_ACTIONS::showPinNumbers.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::PinLibrary,            ACTIONS::pinLibrary.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::UnpinLibrary,          ACTIONS::unpinLibrary.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ToggleSymbolTree,      EE_ACTIONS::showSymbolTree.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ToggleSymbolTree,      EE_ACTIONS::hideSymbolTree.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ToggleSyncedPinsMode,  EE_ACTIONS::toggleSyncedPinsMode.MakeEvent() );
}
