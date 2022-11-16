/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2010-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_schematic_find.h>
#include <tool/actions.h>
#include <sch_edit_frame.h>
#include <tools/sch_editor_control.h>


DIALOG_SCH_FIND::DIALOG_SCH_FIND( SCH_EDIT_FRAME* aParent, SCH_SEARCH_DATA* aData,
                                  const wxPoint& aPosition, const wxSize& aSize, int aStyle ) :
    DIALOG_SCH_FIND_BASE( aParent, wxID_ANY, _( "Find" ), aPosition, aSize,
                          wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | aStyle ),
    m_frame( aParent ),
    m_editorControl( m_frame->GetToolManager()->GetTool<SCH_EDITOR_CONTROL>() ),
    m_findReplaceData( aData ),
    m_findDirty( true )
{
    wxASSERT_MSG( m_findReplaceData, wxT( "can't create find dialog without data" ) );

    if( aStyle & wxFR_REPLACEDIALOG )
    {
        SetTitle( _( "Find and Replace" ) );
        m_buttonReplace->Show( true );
        m_buttonReplaceAll->Show( true );
        m_staticReplace->Show( true );
        m_comboReplace->Show( true );
        m_checkReplaceReferences->Show( true );
        m_checkWildcardMatch->Show( false );  // Wildcard replace is not implemented.
    }

    m_checkMatchCase->SetValue( m_findReplaceData->matchCase );
    m_checkWholeWord->SetValue( m_findReplaceData->matchMode == EDA_SEARCH_MATCH_MODE::WHOLEWORD );
    m_checkWildcardMatch->SetValue( m_findReplaceData->matchMode
                                    == EDA_SEARCH_MATCH_MODE::WILDCARD );

    m_checkAllFields->SetValue( m_findReplaceData->searchAllFields );
    m_checkReplaceReferences->SetValue( m_findReplaceData->replaceReferences );
    m_checkAllPins->SetValue( m_findReplaceData->searchAllPins );
    m_checkCurrentSheetOnly->SetValue( m_findReplaceData->searchCurrentSheetOnly );

    m_buttonFind->SetDefault();
    SetInitialFocus( m_comboFind );

    // Adjust the height of the dialog to prevent controls from being hidden when
    // switching between the find and find/replace modes of the dialog.  This ignores
    // the users preferred height if any of the controls would be hidden.
    GetSizer()->SetSizeHints( this );
    wxSize size = aSize;

    if( aSize != wxDefaultSize )
    {
        wxSize bestSize = GetBestSize();

        if( size.GetHeight() != bestSize.GetHeight() )
            size.SetHeight( bestSize.GetHeight() );
    }

    SetSize( size );

    GetSizer()->Fit( this ); // Needed on Ubuntu/Unity to display the dialog

    Connect( wxEVT_CHAR, wxKeyEventHandler( DIALOG_SCH_FIND::OnChar ), nullptr, this );
}


void DIALOG_SCH_FIND::OnClose( wxCloseEvent& aEvent )
{
    // Notify the SCH_EDIT_FRAME
    m_frame->OnFindDialogClose();
}


void DIALOG_SCH_FIND::OnIdle( wxIdleEvent& aEvent )
{
    if( m_findDirty )
    {
        m_editorControl->UpdateFind( ACTIONS::updateFind.MakeEvent() );
        m_findDirty = false;
    }
}


void DIALOG_SCH_FIND::OnCancel( wxCommandEvent& aEvent )
{
    wxCloseEvent dummy;
    OnClose( dummy );
}


void DIALOG_SCH_FIND::OnUpdateReplaceUI( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( HasFlag( wxFR_REPLACEDIALOG ) && !m_comboFind->GetValue().empty() &&
                    m_editorControl->HasMatch() );
}


void DIALOG_SCH_FIND::OnUpdateReplaceAllUI( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( HasFlag( wxFR_REPLACEDIALOG ) && !m_comboFind->GetValue().empty() );
}


void DIALOG_SCH_FIND::OnChar( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_RETURN )
    {
        wxCommandEvent dummyCommand;
        OnFind( dummyCommand );
    }
}


void DIALOG_SCH_FIND::OnSearchForText( wxCommandEvent& aEvent )
{
    m_findReplaceData->findString = m_comboFind->GetValue();
    m_findDirty = true;
}


void DIALOG_SCH_FIND::OnSearchForSelect( wxCommandEvent& aEvent )
{
    m_findReplaceData->findString = m_comboFind->GetValue();

    // Move the search string to the top of the list if it isn't already there.
    if( aEvent.GetSelection() != 0 )
    {
        wxString tmp = m_comboFind->GetValue();
        m_comboFind->Delete( aEvent.GetSelection() );
        m_comboFind->Insert( tmp, 0 );
        m_comboFind->SetSelection( 0 );
    }

    m_editorControl->UpdateFind( ACTIONS::updateFind.MakeEvent() );
}


void DIALOG_SCH_FIND::OnReplaceWithText( wxCommandEvent& aEvent )
{
    m_findReplaceData->replaceString = m_comboReplace->GetValue();
}


void DIALOG_SCH_FIND::OnReplaceWithSelect( wxCommandEvent& aEvent )
{
    m_findReplaceData->replaceString = m_comboReplace->GetValue();

    // Move the replace string to the top of the list if it isn't already there.
    if( aEvent.GetSelection() != 0 )
    {
        wxString tmp = m_comboReplace->GetValue();
        m_comboReplace->Delete( aEvent.GetSelection() );
        m_comboReplace->Insert( tmp, 0 );
        m_comboReplace->SetSelection( 0 );
    }
}


void DIALOG_SCH_FIND::OnSearchForEnter( wxCommandEvent& aEvent )
{
    OnFind( aEvent );
}


void DIALOG_SCH_FIND::OnReplaceWithEnter( wxCommandEvent& aEvent )
{
    OnFind( aEvent );
}


void DIALOG_SCH_FIND::OnOptions( wxCommandEvent& aEvent )
{
    updateFlags();
    m_findDirty = true;
}

void DIALOG_SCH_FIND::updateFlags()
{
    // Rebuild the search flags in m_findReplaceData from dialog settings

    if( m_checkMatchCase->GetValue() )
        m_findReplaceData->matchCase = true;
    else
        m_findReplaceData->matchCase = false;

    if( m_checkWholeWord->GetValue() )
        m_findReplaceData->matchMode = EDA_SEARCH_MATCH_MODE::WHOLEWORD;
    else if( m_checkWildcardMatch->IsShown() && m_checkWildcardMatch->GetValue() )
        m_findReplaceData->matchMode = EDA_SEARCH_MATCH_MODE::WILDCARD;
    else
        m_findReplaceData->matchMode = EDA_SEARCH_MATCH_MODE::PLAIN;

    if( m_checkAllFields->GetValue() )
        m_findReplaceData->searchAllFields = true;
    else
        m_findReplaceData->searchAllFields = false;

    if( m_checkAllPins->GetValue() )
        m_findReplaceData->searchAllPins = true;
    else
        m_findReplaceData->searchAllPins = false;

    if( m_checkCurrentSheetOnly->GetValue() )
        m_findReplaceData->searchCurrentSheetOnly = true;
    else
        m_findReplaceData->searchCurrentSheetOnly = false;

    if( m_checkReplaceReferences->GetValue() )
        m_findReplaceData->replaceReferences = true;
    else
        m_findReplaceData->replaceReferences = false;
}


void DIALOG_SCH_FIND::OnFind( wxCommandEvent& aEvent )
{
    updateFlags();      // Ensure search flags are up to date

    int index = m_comboFind->FindString( m_comboFind->GetValue(), true );

    if( index == wxNOT_FOUND )
    {
        m_comboFind->Insert( m_comboFind->GetValue(), 0 );
    }
    else if( index != 0 )
    {
        /* Move the search string to the top of the list if it isn't already there. */
        wxString tmp = m_comboFind->GetValue();
        m_comboFind->Delete( index );
        m_comboFind->Insert( tmp, 0 );
        m_comboFind->SetSelection( 0 );
    }

    m_editorControl->FindNext( ACTIONS::findNext.MakeEvent() );
}


void DIALOG_SCH_FIND::OnReplace( wxCommandEvent& aEvent )
{
    updateFlags();      // Ensure search flags are up to date

    int index = m_comboReplace->FindString( m_comboReplace->GetValue(), true );

    if( index == wxNOT_FOUND )
    {
        m_comboReplace->Insert( m_comboReplace->GetValue(), 0 );
    }
    else if( index != 0 )
    {
        /* Move the search string to the top of the list if it isn't already there. */
        wxString tmp = m_comboReplace->GetValue();
        m_comboReplace->Delete( index );
        m_comboReplace->Insert( tmp, 0 );
        m_comboReplace->SetSelection( 0 );
    }

    if( aEvent.GetId() == wxID_REPLACE )
        m_editorControl->ReplaceAndFindNext( ACTIONS::replaceAndFindNext.MakeEvent() );
    else if( aEvent.GetId() == wxID_REPLACE_ALL )
        m_editorControl->ReplaceAll( ACTIONS::replaceAll.MakeEvent() );
}


wxArrayString DIALOG_SCH_FIND::GetFindEntries() const
{
    int index = m_comboFind->FindString( m_comboFind->GetValue(), true );

    if( index == wxNOT_FOUND )
    {
        m_comboFind->Insert( m_comboFind->GetValue(), 0 );
    }
    else if( index != 0 )
    {
        /* Move the search string to the top of the list if it isn't already there. */
        wxString tmp = m_comboFind->GetValue();
        m_comboFind->Delete( index );
        m_comboFind->Insert( tmp, 0 );
    }

    return m_comboFind->GetStrings();
}


void DIALOG_SCH_FIND::SetFindEntries( const wxArrayString& aEntries, const wxString& aFindString )
{
    m_comboFind->Append( aEntries );

    while( m_comboFind->GetCount() > 10 )
    {
        m_frame->GetFindHistoryList().pop_back();
        m_comboFind->Delete( 9 );
    }

    if( !aFindString.IsEmpty() )
    {
        m_comboFind->SetValue( aFindString );
        m_comboFind->SelectAll();
    }
    else if( m_comboFind->GetCount() )
    {
        m_comboFind->SetSelection( 0 );
        m_comboFind->SelectAll();
    }
}


void DIALOG_SCH_FIND::SetReplaceEntries( const wxArrayString& aEntries )
{
    m_comboReplace->Append( aEntries );

    while( m_comboReplace->GetCount() > 10 )
    {
        m_frame->GetFindHistoryList().pop_back();
        m_comboReplace->Delete( 9 );
    }

    if( m_comboReplace->GetCount() )
    {
        m_comboReplace->SetSelection( 0 );
        m_comboFind->SelectAll();
    }
}
