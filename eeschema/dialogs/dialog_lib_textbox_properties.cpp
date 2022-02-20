/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <symbol_edit_frame.h>
#include <symbol_editor_settings.h>
#include <widgets/bitmap_button.h>
#include <widgets/font_choice.h>
#include <base_units.h>
#include <general.h>
#include <lib_textbox.h>
#include <confirm.h>
#include <dialogs/html_message_box.h>
#include <string_utils.h>
#include <scintilla_tricks.h>
#include <dialog_lib_textbox_properties.h>
#include <widgets/color_swatch.h>
#include "symbol_editor_drawing_tools.h"

DIALOG_LIB_TEXTBOX_PROPERTIES::DIALOG_LIB_TEXTBOX_PROPERTIES( SYMBOL_EDIT_FRAME* aParent,
                                                              LIB_TEXTBOX* aTextBox ) :
        DIALOG_LIB_TEXTBOX_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_currentText( aTextBox ),
        m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits ),
        m_borderWidth( aParent, m_borderWidthLabel, m_borderWidthCtrl, m_borderWidthUnits ),
        m_scintillaTricks( nullptr ),
        m_helpWindow( nullptr )
{
    m_borderColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    for( const std::pair<const PLOT_DASH_TYPE, lineTypeStruct>& typeEntry : lineTypeNames )
        m_borderStyleCombo->Append( typeEntry.second.name, KiBitmap( typeEntry.second.bitmap ) );

    m_borderStyleCombo->Append( DEFAULT_STYLE );
    m_fillColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

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
    m_spin1->SetBitmap( KiBitmap( BITMAPS::text_align_center ) );
    m_spin2->SetIsCheckButton();
    m_spin2->SetBitmap( KiBitmap( BITMAPS::text_align_right ) );
    m_spin3->SetIsCheckButton();
    m_spin3->SetBitmap( KiBitmap( BITMAPS::text_align_bottom ) );
    m_spin4->SetIsCheckButton();
    m_spin4->SetBitmap( KiBitmap( BITMAPS::text_align_middle ) );
    m_spin5->SetIsCheckButton();
    m_spin5->SetBitmap( KiBitmap( BITMAPS::text_align_top ) );

    m_separator3->SetIsSeparator();

    SetupStandardButtons();
    Layout();

    m_spin0->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXTBOX_PROPERTIES::onSpinButton, this );
    m_spin1->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXTBOX_PROPERTIES::onSpinButton, this );
    m_spin2->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXTBOX_PROPERTIES::onSpinButton, this );
    m_spin3->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXTBOX_PROPERTIES::onSpinButton, this );
    m_spin4->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXTBOX_PROPERTIES::onSpinButton, this );
    m_spin5->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXTBOX_PROPERTIES::onSpinButton, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_LIB_TEXTBOX_PROPERTIES::~DIALOG_LIB_TEXTBOX_PROPERTIES()
{
    delete m_scintillaTricks;

    if( m_helpWindow )
        m_helpWindow->Destroy();
}


bool DIALOG_LIB_TEXTBOX_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    m_textCtrl->SetValue( m_currentText->GetText() );

    m_fontCtrl->SetFontSelection( m_currentText->GetFont() );
    m_textSize.SetValue( m_currentText->GetTextWidth() );

    m_bold->Check( m_currentText->IsBold() );
    m_italic->Check( m_currentText->IsItalic() );

    m_borderCheckbox->SetValue( m_currentText->GetWidth() >= 0 );

    if( m_currentText->GetWidth() >= 0 )
        m_borderWidth.SetValue( m_currentText->GetWidth() );

    m_borderColorSwatch->SetSwatchColor( m_currentText->GetStroke().GetColor(), false );

    int style = static_cast<int>( m_currentText->GetStroke().GetPlotStyle() );

    if( style == -1 )
        m_borderStyleCombo->SetStringSelection( DEFAULT_STYLE );
    else if( style < (int) lineTypeNames.size() )
        m_borderStyleCombo->SetSelection( style );
    else
        wxFAIL_MSG( "Line type not found in the type lookup map" );

    m_borderWidth.Enable( m_currentText->GetWidth() >= 0 );
    m_borderColorLabel->Enable( m_currentText->GetWidth() >= 0 );
    m_borderColorSwatch->Enable( m_currentText->GetWidth() >= 0 );
    m_borderStyleLabel->Enable( m_currentText->GetWidth() >= 0 );
    m_borderStyleCombo->Enable( m_currentText->GetWidth() >= 0 );

    m_filledCtrl->SetValue( m_currentText->IsFilled() );
    m_fillColorSwatch->SetSwatchColor( m_currentText->GetFillColor(), false );

    m_fillColorLabel->Enable( m_currentText->IsFilled() );
    m_fillColorSwatch->Enable( m_currentText->IsFilled() );

    if( m_currentText->GetTextAngle() == ANGLE_VERTICAL )
    {
        switch( m_currentText->GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:   m_spin3->Check(); break;
        case GR_TEXT_H_ALIGN_CENTER: m_spin4->Check(); break;
        case GR_TEXT_H_ALIGN_RIGHT:  m_spin5->Check(); break;
        }
    }
    else
    {
        switch( m_currentText->GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:   m_spin0->Check(); break;
        case GR_TEXT_H_ALIGN_CENTER: m_spin1->Check(); break;
        case GR_TEXT_H_ALIGN_RIGHT:  m_spin2->Check(); break;
        }
    }

    m_privateCheckbox->SetValue( m_currentText->IsPrivate() );
    m_CommonUnit->SetValue( m_currentText->GetUnit() == 0 );
    m_CommonConvert->SetValue( m_currentText->GetConvert() == 0 );

    return true;
}


void DIALOG_LIB_TEXTBOX_PROPERTIES::onSpinButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_spin0, m_spin1, m_spin2, m_spin3, m_spin4, m_spin5 } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


bool DIALOG_LIB_TEXTBOX_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    // Don't allow text to disappear; it can be difficult to correct if you can't select it
    if( !m_textSize.Validate( 0.01, 1000.0, EDA_UNITS::MILLIMETRES ) )
        return false;

    wxString text = m_textCtrl->GetValue();

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

    if( m_fontCtrl->HaveFontSelection() )
    {
        m_currentText->SetFont( m_fontCtrl->GetFontSelection( m_bold->IsChecked(),
                                                              m_italic->IsChecked() ) );
    }

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

    if( m_spin0->IsChecked() )
    {
        m_currentText->SetTextAngle( ANGLE_HORIZONTAL );
        m_currentText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    }
    else if( m_spin1->IsChecked() )
    {
        m_currentText->SetTextAngle( ANGLE_HORIZONTAL );
        m_currentText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    }
    else if( m_spin2->IsChecked() )
    {
        m_currentText->SetTextAngle( ANGLE_HORIZONTAL );
        m_currentText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
    }
    else if( m_spin3->IsChecked() )
    {
        m_currentText->SetTextAngle( ANGLE_VERTICAL );
        m_currentText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    }
    else if( m_spin4->IsChecked() )
    {
        m_currentText->SetTextAngle( ANGLE_VERTICAL );
        m_currentText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    }
    else if( m_spin5->IsChecked() )
    {
        m_currentText->SetTextAngle( ANGLE_VERTICAL );
        m_currentText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
    }

    STROKE_PARAMS stroke = m_currentText->GetStroke();

    if( m_borderCheckbox->GetValue() )
    {
        if( !m_borderWidth.IsIndeterminate() )
            stroke.SetWidth( m_borderWidth.GetValue() );
    }
    else
    {
        stroke.SetWidth( -1 );
    }

    auto it = lineTypeNames.begin();
    std::advance( it, m_borderStyleCombo->GetSelection() );

    if( it == lineTypeNames.end() )
        stroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );
    else
        stroke.SetPlotStyle( it->first );

    stroke.SetColor( m_borderColorSwatch->GetSwatchColor() );

    m_currentText->SetStroke( stroke );

    m_currentText->SetFillMode( m_filledCtrl->GetValue() ? FILL_T::FILLED_WITH_COLOR
                                                         : FILL_T::NO_FILL );
    m_currentText->SetFillColor( m_fillColorSwatch->GetSwatchColor() );

    m_currentText->SetPrivate( m_privateCheckbox->GetValue() );

    if( !m_CommonUnit->GetValue() )
        m_currentText->SetUnit( m_frame->GetUnit() );
    else
        m_currentText->SetUnit( 0 );

    if( !m_CommonConvert->GetValue() )
        m_currentText->SetConvert( m_frame->GetConvert() );
    else
        m_currentText->SetConvert( 0 );

    // Record settings used for next time:
    auto* tools = m_frame->GetToolManager()->GetTool<SYMBOL_EDITOR_DRAWING_TOOLS>();
    tools->SetLastTextAngle( m_currentText->GetTextAngle() );
    tools->SetDrawSpecificConvert( !m_CommonConvert->GetValue() );
    tools->SetDrawSpecificUnit( !m_CommonUnit->GetValue() );

    m_frame->SetMsgPanel( m_currentText );

    return true;
}


void DIALOG_LIB_TEXTBOX_PROPERTIES::OnFormattingHelp( wxHyperlinkEvent& aEvent )
{
    m_helpWindow = SCH_TEXT::ShowSyntaxHelp( this );
}


void DIALOG_LIB_TEXTBOX_PROPERTIES::onMultiLineTCLostFocus( wxFocusEvent& event )
{
    if( m_scintillaTricks )
        m_scintillaTricks->CancelAutocomplete();

    event.Skip();
}


void DIALOG_LIB_TEXTBOX_PROPERTIES::onBorderChecked( wxCommandEvent& event )
{
    bool border = m_borderCheckbox->GetValue();

    if( border && m_borderWidth.GetValue() < 0 )
        m_borderWidth.SetValue( Mils2iu( m_frame->libeditconfig()->m_Defaults.line_width ) );

    m_borderWidth.Enable( border );
    m_borderColorLabel->Enable( border );
    m_borderColorSwatch->Enable( border );
    m_borderStyleLabel->Enable( border );
    m_borderStyleCombo->Enable( border );
}


void DIALOG_LIB_TEXTBOX_PROPERTIES::onFillChecked( wxCommandEvent& event )
{
    bool fill = m_filledCtrl->GetValue();

    m_fillColorLabel->Enable( fill );
    m_fillColorSwatch->Enable( fill );
}


