/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright (C) 2004-2021 KiCad Developers, see AITHORS.txt for contributors.
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
#include <kiway.h>
#include <confirm.h>
#include <string_utils.h>
#include <sch_base_frame.h>
#include <sch_edit_frame.h>
#include <ee_collectors.h>
#include <sch_symbol.h>
#include <lib_field.h>
#include <template_fieldnames.h>
#include <symbol_library.h>
#include <sch_validators.h>
#include <schematic.h>
#include <dialog_field_properties.h>
#include <sch_text.h>
#include <scintilla_tricks.h>
#include <wildcards_and_files_ext.h>


DIALOG_FIELD_PROPERTIES::DIALOG_FIELD_PROPERTIES( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                                                  const EDA_TEXT* aTextItem ) :
    DIALOG_LIB_TEXT_PROPERTIES_BASE( aParent ),
    m_posX( aParent, m_xPosLabel, m_xPosCtrl, m_xPosUnits, true ),
    m_posY( aParent, m_yPosLabel, m_yPosCtrl, m_yPosUnits, true ),
    m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, true ),
    m_firstFocus( true ),
    m_scintillaTricks( nullptr )
{
    wxASSERT( aTextItem );

    SetTitle( aTitle );

    m_note->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_note->Show( false );

    // The field ID and power status are Initialized in the derived object's ctor.
    m_fieldId = VALUE_FIELD;
    m_isPower = false;

    m_scintillaTricks = new SCINTILLA_TRICKS( m_StyledTextCtrl, wxT( "{}" ), true,
            [this]()
            {
                wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            } );
    m_StyledTextCtrl->SetEOLMode( wxSTC_EOL_LF );   // Normalize EOL across platforms

    m_text = aTextItem->GetText();
    m_isItalic = aTextItem->IsItalic();
    m_isBold = aTextItem->IsBold();
    m_position = aTextItem->GetTextPos();
    m_size = aTextItem->GetTextWidth();
    m_isVertical = ( aTextItem->GetTextAngle() == TEXT_ANGLE_VERT );
    m_verticalJustification = aTextItem->GetVertJustify() + 1;
    m_horizontalJustification = aTextItem->GetHorizJustify() + 1;
    m_isVisible = aTextItem->IsVisible();
}


DIALOG_FIELD_PROPERTIES::~DIALOG_FIELD_PROPERTIES()
{
    delete m_scintillaTricks;
}


void DIALOG_FIELD_PROPERTIES::init()
{
    SCH_BASE_FRAME* parent = GetParent();
    bool isSymbolEditor = parent && parent->IsType( FRAME_SCH_SYMBOL_EDITOR );

    // Disable options for graphic text editing which are not needed for fields.
    m_CommonConvert->Show( false );
    m_CommonUnit->Show( false );

    // Predefined fields cannot contain some chars, or cannot be empty,
    // and need a SCH_FIELD_VALIDATOR (m_StyledTextCtrl cannot use a SCH_FIELD_VALIDATOR).
    bool use_validator = m_fieldId == REFERENCE_FIELD
                         || m_fieldId == VALUE_FIELD
                         || m_fieldId == FOOTPRINT_FIELD
                         || m_fieldId == DATASHEET_FIELD
                         || m_fieldId == SHEETNAME_V
                         || m_fieldId == SHEETFILENAME_V;

    if( use_validator )
    {
        m_TextCtrl->SetValidator( SCH_FIELD_VALIDATOR( isSymbolEditor, m_fieldId, &m_text ) );
        SetInitialFocus( m_TextCtrl );

        m_StyledTextCtrl->Show( false );
    }
    else
    {
        SetInitialFocus( m_StyledTextCtrl );

        m_TextCtrl->Show( false );
    }

    // Show the footprint selection dialog if this is the footprint field.
    m_TextValueSelectButton->SetBitmap( KiBitmap( BITMAPS::small_library ) );
    m_TextValueSelectButton->Show( m_fieldId == FOOTPRINT_FIELD );

    // Value fields of power symbols cannot be modified. This will grey out
    // the text box and display an explanation.
    if( m_fieldId == VALUE_FIELD && m_isPower )
    {
        m_note->SetLabel( wxString::Format( m_note->GetLabel(),
                                            _( "Power symbol value field text cannot be changed." ) ) );
        m_note->Show( true );
        m_TextCtrl->Enable( false );
    }
    else
    {
        m_TextCtrl->Enable( true );
    }

    m_sdbSizerButtonsOK->SetDefault();

    GetSizer()->SetSizeHints( this );

    // Adjust the height of the scintilla text editor after the first layout
    // To show only one line
    // (multiline text are is supported in fields and will be removed)
    if( m_StyledTextCtrl->IsShown() )
    {
        wxSize maxSize = m_StyledTextCtrl->GetSize();
        maxSize.x = -1;     // Do not fix the max width
        maxSize.y = m_xPosCtrl->GetSize().y;
        m_StyledTextCtrl->SetMaxSize( maxSize );
        m_StyledTextCtrl->SetUseVerticalScrollBar( false );
        m_StyledTextCtrl->SetUseHorizontalScrollBar( false );
    }

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_FIELD_PROPERTIES::OnTextValueSelectButtonClick( wxCommandEvent& aEvent )
{
    // pick a footprint using the footprint picker.
    wxString fpid;

    if( m_StyledTextCtrl->IsShown() )
        fpid = m_StyledTextCtrl->GetValue();
    else
        fpid = m_TextCtrl->GetValue();

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_FOOTPRINT_VIEWER_MODAL, true );

    if( frame->ShowModal( &fpid, this ) )
    {
        if( m_StyledTextCtrl->IsShown() )
            m_StyledTextCtrl->SetValue( fpid );
        else
            m_TextCtrl->SetValue( fpid );
    }

    frame->Destroy();
}


void DIALOG_FIELD_PROPERTIES::OnSetFocusText( wxFocusEvent& event )
{
    if( m_firstFocus )
    {
#ifdef __WXGTK__
        // Force an update of the text control before setting the text selection
        // This is needed because GTK seems to ignore the selection on first update
        //
        // Note that we can't do this on OSX as it tends to provoke Apple's
        // "[NSAlert runModal] may not be invoked inside of transaction begin/commit pair"
        // bug.  See: https://bugs.launchpad.net/kicad/+bug/1837225
        if( m_fieldId == REFERENCE_FIELD || m_fieldId == VALUE_FIELD || m_fieldId == SHEETNAME_V )
            m_TextCtrl->Update();
#endif

        if( m_fieldId == REFERENCE_FIELD )
            KIUI::SelectReferenceNumber( static_cast<wxTextEntry*>( m_TextCtrl ) );
        else if( m_fieldId == VALUE_FIELD || m_fieldId == SHEETNAME_V )
            m_TextCtrl->SetSelection( -1, -1 );

        m_firstFocus = false;
    }

    event.Skip();
}


bool DIALOG_FIELD_PROPERTIES::TransferDataToWindow()
{
    if( m_TextCtrl->IsShown() )
        m_TextCtrl->SetValue( m_text );
    else if( m_StyledTextCtrl->IsShown() )
        m_StyledTextCtrl->SetValue( m_text );

    m_posX.SetValue( m_position.x );
    m_posY.SetValue( m_position.y );
    m_textSize.SetValue( m_size );
    m_orientChoice->SetSelection( m_isVertical ? 1 : 0 );
    m_hAlignChoice->SetSelection( m_horizontalJustification );
    m_vAlignChoice->SetSelection( m_verticalJustification );
    m_visible->SetValue( m_isVisible );
    m_italic->SetValue( m_isItalic );
    m_bold->SetValue( m_isBold );

    return true;
}


bool DIALOG_FIELD_PROPERTIES::TransferDataFromWindow()
{
    if( m_TextCtrl->IsShown() )
        m_text = m_TextCtrl->GetValue();
    else if( m_StyledTextCtrl->IsShown() )
        m_text = m_StyledTextCtrl->GetValue();

    if( m_fieldId == REFERENCE_FIELD )
    {
        // Test if the reference string is valid:
        if( !SCH_SYMBOL::IsReferenceStringValid( m_text ) )
        {
            DisplayError( this, _( "Illegal reference designator value!" ) );
            return false;
        }
    }
    else if( m_fieldId == VALUE_FIELD )
    {
        if( m_text.IsEmpty() )
        {
            DisplayError( this, _( "Value may not be empty." ) );
            return false;
        }
    }
    else if( m_fieldId == SHEETFILENAME_V )
    {
        wxFileName fn( m_text );

        // It's annoying to throw up nag dialogs when the extension isn't right.  Just
        // fix it.
        if( fn.GetExt().CmpNoCase( KiCadSchematicFileExtension ) != 0 )
        {
            fn.SetExt( KiCadSchematicFileExtension );
            m_text = fn.GetFullPath();
        }
    }

    m_isVertical = m_orientChoice->GetSelection() == 1;
    m_position = wxPoint( m_posX.GetValue(), m_posY.GetValue() );
    m_size = m_textSize.GetValue();
    m_horizontalJustification = m_hAlignChoice->GetSelection();
    m_verticalJustification = m_vAlignChoice->GetSelection();
    m_isVisible = m_visible->GetValue();
    m_isItalic = m_italic->GetValue();
    m_isBold = m_bold->GetValue();

    return true;
}


void DIALOG_FIELD_PROPERTIES::updateText( EDA_TEXT* aText )
{
    aText->SetTextPos( m_position );

    if( aText->GetTextWidth() != m_size )
        aText->SetTextSize( wxSize( m_size, m_size ) );

    aText->SetVisible( m_isVisible );
    aText->SetTextAngle( m_isVertical ? TEXT_ANGLE_VERT : TEXT_ANGLE_HORIZ );
    aText->SetItalic( m_isItalic );
    aText->SetBold( m_isBold );
    aText->SetHorizJustify( EDA_TEXT::MapHorizJustify( m_horizontalJustification - 1 ) );
    aText->SetVertJustify( EDA_TEXT::MapVertJustify( m_verticalJustification - 1 ) );
}


DIALOG_LIB_FIELD_PROPERTIES::DIALOG_LIB_FIELD_PROPERTIES( SCH_BASE_FRAME* aParent,
                                                          const wxString& aTitle,
                                                          const LIB_FIELD* aField ) :
        DIALOG_FIELD_PROPERTIES( aParent, aTitle, aField )
{
    m_fieldId = aField->GetId();

    if( m_fieldId == VALUE_FIELD )
        m_text = UnescapeString( aField->GetText() );

    // When in the library editor, power symbols can be renamed.
    m_isPower = false;
    init();
}


DIALOG_SCH_FIELD_PROPERTIES::DIALOG_SCH_FIELD_PROPERTIES( SCH_BASE_FRAME* aParent,
                                                          const wxString& aTitle,
                                                          const SCH_FIELD* aField ) :
        DIALOG_FIELD_PROPERTIES( aParent, aTitle, aField ),
        m_field( aField )
{
    m_isSheetFilename = false;

    if( aField->GetParent() && aField->GetParent()->Type() == SCH_SYMBOL_T )
    {
        m_fieldId = aField->GetId();
    }
    else if( aField->GetParent() && aField->GetParent()->Type() == SCH_SHEET_T )
    {
        switch( aField->GetId() )
        {
        case SHEETNAME:
            m_fieldId = SHEETNAME_V;
            break;

        case SHEETFILENAME:
            m_isSheetFilename = true;
            m_fieldId = SHEETFILENAME_V;
            m_note->SetLabel( wxString::Format( m_note->GetLabel(),
                              _( "Sheet filename can only be modified in Sheet Properties dialog." ) ) );
            m_note->Show( true );
            break;

        default:
            m_fieldId = SHEETUSERFIELD_V;
            break;
        }
    }

    // show text variable cross-references in a human-readable format
    m_text = aField->Schematic()->ConvertKIIDsToRefs( aField->GetText() );

    m_isPower = false;

    m_textLabel->SetLabel( m_field->GetName() + ":" );

    // The library symbol may have been removed so using SCH_SYMBOL::GetLibSymbolRef() here
    // could result in a segfault.  If the library symbol is no longer available, the
    // schematic fields can still edit so set the power symbol flag to false.  This may not
    // be entirely accurate if the power library is missing but it's better then a segfault.
    if( aField->GetParent() && aField->GetParent()->Type() == SCH_SYMBOL_T )
    {
        const SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( aField->GetParent() );
        const LIB_SYMBOL* libSymbol = GetParent()->GetLibSymbol( symbol->GetLibId(), true );

        if( libSymbol && libSymbol->IsPower() )
            m_isPower = true;
    }

    m_StyledTextCtrl->Bind( wxEVT_STC_CHARADDED,
                            &DIALOG_SCH_FIELD_PROPERTIES::onScintillaCharAdded, this );

    init();

    if( m_isSheetFilename )
    {
        m_StyledTextCtrl->Enable( false );
        m_TextCtrl->Enable( false );
    }
}


void DIALOG_SCH_FIELD_PROPERTIES::onScintillaCharAdded( wxStyledTextEvent &aEvent )
{
    int key = aEvent.GetKey();

    SCH_EDIT_FRAME* editFrame = static_cast<SCH_EDIT_FRAME*>( GetParent() );
    wxArrayString   autocompleteTokens;
    int             pos = m_StyledTextCtrl->GetCurrentPos();
    int             start = m_StyledTextCtrl->WordStartPosition( pos, true );
    wxString        partial;

    // Currently, '\n' is not allowed in fields. So remove it when entered
    // TODO: see if we must close the dialog. However this is not obvious, as
    // if a \n is typed (and removed) when a text is selected, this text is deleted
    // (in fact replaced by \n, that is removed by the filter)
    if( key == '\n' )
    {
        wxString text = m_StyledTextCtrl->GetText();
        int currpos = m_StyledTextCtrl->GetCurrentPos();
        text.Replace( "\n", "" );
        m_StyledTextCtrl->SetText( text );
        m_StyledTextCtrl->GotoPos( currpos-1 );
        return;
    }

    auto textVarRef =
            [&]( int pt )
            {
                return pt >= 2
                        && m_StyledTextCtrl->GetCharAt( pt - 2 ) == '$'
                        && m_StyledTextCtrl->GetCharAt( pt - 1 ) == '{';
            };

    // Check for cross-reference
    if( start > 1 && m_StyledTextCtrl->GetCharAt( start - 1 ) == ':' )
    {
        int refStart = m_StyledTextCtrl->WordStartPosition( start - 1, true );

        if( textVarRef( refStart ) )
        {
            partial = m_StyledTextCtrl->GetRange( start, pos );

            wxString           ref = m_StyledTextCtrl->GetRange( refStart, start - 1 );
            SCH_SHEET_LIST     sheets = editFrame->Schematic().GetSheets();
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
        partial = m_StyledTextCtrl->GetTextRange( start, pos );

        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( m_field->GetParent() );
        SCH_SHEET*  sheet = dynamic_cast<SCH_SHEET*>( m_field->GetParent() );

        if( symbol )
        {
            symbol->GetContextualTextVars( &autocompleteTokens );

            SCHEMATIC* schematic = symbol->Schematic();

            if( schematic && schematic->CurrentSheet().Last() )
                schematic->CurrentSheet().Last()->GetContextualTextVars( &autocompleteTokens );
        }

        if( sheet )
            sheet->GetContextualTextVars( &autocompleteTokens );

        for( std::pair<wxString, wxString> entry : Prj().GetTextVars() )
            autocompleteTokens.push_back( entry.first );
    }

    m_scintillaTricks->DoAutocomplete( partial, autocompleteTokens );
    m_StyledTextCtrl->SetFocus();
}


void DIALOG_SCH_FIELD_PROPERTIES::UpdateField( SCH_FIELD* aField, SCH_SHEET_PATH* aSheetPath )
{
    SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );
    SCH_ITEM*       parent = dynamic_cast<SCH_ITEM*>( aField->GetParent() );
    int             fieldType = aField->GetId();

    if( parent && parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( parent );

        if( fieldType == REFERENCE_FIELD )
            symbol->SetRef( aSheetPath, m_text );
        else if( fieldType == VALUE_FIELD )
            symbol->SetValue( m_text );
        else if( fieldType == FOOTPRINT_FIELD )
            symbol->SetFootprint( m_text );
    }

    bool positioningModified = false;

    if( aField->GetTextPos() != m_position )
        positioningModified = true;

    if( ( aField->GetTextAngle() == TEXT_ANGLE_VERT ) != m_isVertical )
        positioningModified = true;

    if( aField->GetHorizJustify() != EDA_TEXT::MapHorizJustify( m_horizontalJustification - 1 ) )
        positioningModified = true;

    if( aField->GetVertJustify() != EDA_TEXT::MapVertJustify( m_verticalJustification - 1 ) )
        positioningModified = true;

    // convert any text variable cross-references to their UUIDs
    m_text = aField->Schematic()->ConvertRefsToKIIDs( m_text );

    aField->SetText( m_text );
    updateText( aField );

    // The value, footprint and datasheet fields should be kept in sync in multi-unit parts.
    // Of course the symbol must be annotated to collect other units.
    if( editFrame && parent && parent->Type() == SCH_SYMBOL_T )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( parent );

        if( symbol->IsAnnotated( aSheetPath ) && ( fieldType == VALUE_FIELD
                                                || fieldType == FOOTPRINT_FIELD
                                                || fieldType == DATASHEET_FIELD ) )
        {
            wxString ref = symbol->GetRef( aSheetPath );
            int      unit = symbol->GetUnit();
            LIB_ID   libId = symbol->GetLibId();

            for( SCH_SHEET_PATH& sheet : editFrame->Schematic().GetSheets() )
            {
                SCH_SCREEN*              screen = sheet.LastScreen();
                std::vector<SCH_SYMBOL*> otherUnits;
                constexpr bool           appendUndo = true;

                CollectOtherUnits( ref, unit, libId, sheet, &otherUnits );

                for( SCH_SYMBOL* otherUnit : otherUnits )
                {
                    editFrame->SaveCopyInUndoList( screen, otherUnit, UNDO_REDO::CHANGED,
                                                   appendUndo );

                    if( fieldType == VALUE_FIELD )
                        otherUnit->SetValue( m_text );
                    else if( fieldType == FOOTPRINT_FIELD )
                        otherUnit->SetFootprint( m_text );
                    else
                        otherUnit->GetField( DATASHEET_FIELD )->SetText( m_text );

                    editFrame->UpdateItem( otherUnit );
                }
            }
        }
    }

    if( positioningModified && parent )
        parent->ClearFieldsAutoplaced();
}
