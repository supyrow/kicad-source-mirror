/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <sch_edit_frame.h>
#include <widgets/bitmap_button.h>
#include <base_units.h>
#include <tool/tool_manager.h>
#include <general.h>
#include <gr_text.h>
#include <confirm.h>
#include <schematic.h>
#include <dialogs/html_message_box.h>
#include <string_utils.h>
#include <scintilla_tricks.h>
#include <dialog_text_properties.h>

class SCH_EDIT_FRAME;
class SCH_TEXT;


DIALOG_TEXT_PROPERTIES::DIALOG_TEXT_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_TEXT* aTextItem ) :
        DIALOG_TEXT_PROPERTIES_BASE( aParent ),
        m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, false ),
        m_scintillaTricks( nullptr ),
        m_helpWindow( nullptr )
{
    m_Parent = aParent;
    m_currentText = aTextItem;

    m_textCtrl->SetEOLMode( wxSTC_EOL_LF );

    m_scintillaTricks = new SCINTILLA_TRICKS( m_textCtrl, wxT( "{}" ), false,
            [this]()
            {
                wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            } );

    m_textEntrySizer->AddGrowableRow( 0 );

    SetInitialFocus( m_textCtrl );

    m_separator1->SetIsSeparator();

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmap( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmap( BITMAPS::text_italic ) );

    m_separator2->SetIsSeparator();

    m_spin0->SetIsCheckButton();
    m_spin0->SetBitmap( KiBitmap( BITMAPS::text_align_left ) );
    m_spin1->SetIsCheckButton();
    m_spin1->SetBitmap( KiBitmap( BITMAPS::text_align_right ) );
    m_spin2->SetIsCheckButton();
    m_spin2->SetBitmap( KiBitmap( BITMAPS::text_align_bottom ) );
    m_spin3->SetIsCheckButton();
    m_spin3->SetBitmap( KiBitmap( BITMAPS::text_align_top ) );

    m_separator3->SetIsSeparator();

    SetupStandardButtons();
    Layout();

    m_textCtrl->Bind( wxEVT_STC_CHARADDED, &DIALOG_TEXT_PROPERTIES::onScintillaCharAdded, this );
    m_spin0->Bind( wxEVT_BUTTON, &DIALOG_TEXT_PROPERTIES::onSpinButton, this );
    m_spin1->Bind( wxEVT_BUTTON, &DIALOG_TEXT_PROPERTIES::onSpinButton, this );
    m_spin2->Bind( wxEVT_BUTTON, &DIALOG_TEXT_PROPERTIES::onSpinButton, this );
    m_spin3->Bind( wxEVT_BUTTON, &DIALOG_TEXT_PROPERTIES::onSpinButton, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_TEXT_PROPERTIES::~DIALOG_TEXT_PROPERTIES()
{
    delete m_scintillaTricks;

    if( m_helpWindow )
        m_helpWindow->Destroy();
}


bool DIALOG_TEXT_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    SCHEMATIC& schematic = m_Parent->Schematic();

    // show text variable cross-references in a human-readable format
    m_textCtrl->SetValue( schematic.ConvertKIIDsToRefs( m_currentText->GetText() ) );

    m_textSize.SetValue( m_currentText->GetTextWidth() );

    m_bold->Check( m_currentText->IsBold() );
    m_italic->Check( m_currentText->IsItalic() );

    switch( m_currentText->GetLabelSpinStyle() )
    {
    case LABEL_SPIN_STYLE::RIGHT:  m_spin0->Check( true ); break;
    case LABEL_SPIN_STYLE::LEFT:   m_spin1->Check( true ); break;
    case LABEL_SPIN_STYLE::UP:     m_spin2->Check( true ); break;
    case LABEL_SPIN_STYLE::BOTTOM: m_spin3->Check( true ); break;
    }

    return true;
}


void DIALOG_TEXT_PROPERTIES::onScintillaCharAdded( wxStyledTextEvent &aEvent )
{
    wxStyledTextCtrl* te = m_textCtrl;
    wxArrayString     autocompleteTokens;
    int               text_pos = te->GetCurrentPos();
    int               start = te->WordStartPosition( text_pos, true );
    wxString          partial;

    auto textVarRef =
            [&]( int pos )
            {
                return pos >= 2 && te->GetCharAt( pos-2 ) == '$' && te->GetCharAt( pos-1 ) == '{';
            };

    // Check for cross-reference
    if( start > 1 && te->GetCharAt( start-1 ) == ':' )
    {
        int refStart = te->WordStartPosition( start-1, true );

        if( textVarRef( refStart ) )
        {
            partial = te->GetRange( start+1, text_pos );

            wxString           ref = te->GetRange( refStart, start-1 );
            SCH_SHEET_LIST     sheets = m_Parent->Schematic().GetSheets();
            SCH_REFERENCE_LIST refs;
            SCH_SYMBOL*        refSymbol = nullptr;

            sheets.GetSymbols( refs );

            for( size_t jj = 0; jj < refs.GetCount(); jj++ )
            {
                if( refs[ jj ].GetSymbol()->GetRef( &refs[ jj ].GetSheetPath(), true ) == ref )
                {
                    refSymbol = refs[ jj ].GetSymbol();
                    break;
                }
            }

            if( refSymbol )
                refSymbol->GetContextualTextVars( &autocompleteTokens );
        }
    }
    else if( textVarRef( start ) )
    {
        partial = te->GetTextRange( start, text_pos );

        SCHEMATIC* schematic = m_currentText->Schematic();

        if( schematic && schematic->CurrentSheet().Last() )
            schematic->CurrentSheet().Last()->GetContextualTextVars( &autocompleteTokens );

        for( std::pair<wxString, wxString> entry : Prj().GetTextVars() )
            autocompleteTokens.push_back( entry.first );
    }

    m_scintillaTricks->DoAutocomplete( partial, autocompleteTokens );
    m_textCtrl->SetFocus();
}


void DIALOG_TEXT_PROPERTIES::onSpinButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_spin0, m_spin1, m_spin2, m_spin3 } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


bool DIALOG_TEXT_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    // Don't allow text to disappear; it can be difficult to correct if you can't select it
    if( !m_textSize.Validate( 0.01, 1000.0, EDA_UNITS::MILLIMETRES ) )
        return false;

    wxString text;

    /* save old text in undo list if not already in edit */
    if( m_currentText->GetEditFlags() == 0 )
    {
        m_Parent->SaveCopyInUndoList( m_Parent->GetScreen(), m_currentText, UNDO_REDO::CHANGED,
                                      false );
    }

    m_Parent->GetCanvas()->Refresh();

    // convert any text variable cross-references to their UUIDs
    text = m_Parent->Schematic().ConvertRefsToKIIDs( m_textCtrl->GetValue() );

    if( !text.IsEmpty() )
    {
#ifdef __WXMAC__
        // On macOS CTRL+Enter produces '\r' instead of '\n' regardless of EOL setting
        text.Replace( "\r", "\n" );
#endif

        m_currentText->SetText( text );
    }
    else if( !m_currentText->IsNew() )
    {
        DisplayError( this, _( "Text can not be empty." ) );
        return false;
    }

    if( m_currentText->GetTextWidth() != m_textSize.GetValue() )
        m_currentText->SetTextSize( wxSize( m_textSize.GetValue(), m_textSize.GetValue() ) );

    if( m_bold->IsChecked() != m_currentText->IsBold() )
    {
        if( m_bold->IsChecked() )
        {
            m_currentText->SetBold( true );
            m_currentText->SetTextThickness( GetPenSizeForBold( m_currentText->GetTextWidth() ) );
        }
        else
        {
            m_currentText->SetBold( false );
            m_currentText->SetTextThickness( 0 ); // Use default pen width
        }
    }

    m_currentText->SetItalic( m_italic->IsChecked() );

    LABEL_SPIN_STYLE selectedSpinStyle= LABEL_SPIN_STYLE::LEFT;

    if( m_spin0->IsChecked() )
        selectedSpinStyle = LABEL_SPIN_STYLE::RIGHT;
    else if( m_spin1->IsChecked() )
        selectedSpinStyle = LABEL_SPIN_STYLE::LEFT;
    else if( m_spin2->IsChecked() )
        selectedSpinStyle = LABEL_SPIN_STYLE::UP;
    else if( m_spin3->IsChecked() )
        selectedSpinStyle = LABEL_SPIN_STYLE::BOTTOM;

    if( m_currentText->GetLabelSpinStyle() != selectedSpinStyle )
        m_currentText->SetLabelSpinStyle( selectedSpinStyle );

    m_Parent->UpdateItem( m_currentText, false, true );
    m_Parent->GetCanvas()->Refresh();
    m_Parent->OnModify();

    return true;
}


void DIALOG_TEXT_PROPERTIES::OnFormattingHelp( wxHyperlinkEvent& aEvent )
{
    m_helpWindow = SCH_TEXT::ShowSyntaxHelp( this );
}


void DIALOG_TEXT_PROPERTIES::onMultiLineTCLostFocus( wxFocusEvent& event )
{
    if( m_scintillaTricks )
        m_scintillaTricks->CancelAutocomplete();

    event.Skip();
}
