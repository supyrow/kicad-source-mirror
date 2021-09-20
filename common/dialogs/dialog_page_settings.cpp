/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <base_screen.h>
#include <confirm.h>
#include <core/arraydim.h>
#include <dialogs/dialog_page_settings.h>
#include <eda_draw_frame.h>
#include <eda_item.h>
#include <gr_basic.h>
#include <kiface_base.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND, Clamp
#include <project.h>
#include <title_block.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <wildcards_and_files_ext.h>
#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/ds_painter.h>
#include <wx/valgen.h>
#include <wx/tokenzr.h>
#include <wx/filedlg.h>
#include <wx/dcmemory.h>

#define MAX_PAGE_EXAMPLE_SIZE 200


// List of page formats.
// they are prefixed by "_HKI" (already in use for hotkeys) instead of "_",
// because we need both the translated and the not translated version.
// when displayed in dialog we should explicitly call wxGetTranslation()
// to show the translated version.
// See hotkeys_basic.h for more info
#define _HKI( x ) wxT( x )
static const wxString pageFmts[] =
{
    _HKI("A5 148x210mm"),
    _HKI("A4 210x297mm"),
    _HKI("A3 297x420mm"),
    _HKI("A2 420x594mm"),
    _HKI("A1 594x841mm"),
    _HKI("A0 841x1189mm"),
    _HKI("A 8.5x11in"),
    _HKI("B 11x17in"),
    _HKI("C 17x22in"),
    _HKI("D 22x34in"),
    _HKI("E 34x44in"),
    _HKI("USLetter 8.5x11in"),      // USLetter without space is correct
    _HKI("USLegal 8.5x14in"),       // USLegal without space is correct
    _HKI("USLedger 11x17in"),       // USLedger without space is correct
    _HKI("User (Custom)"),          // size defined by user. The string must contain "Custom"
                                    // to be recognized in code
};

DIALOG_PAGES_SETTINGS::DIALOG_PAGES_SETTINGS( EDA_DRAW_FRAME* aParent, double aIuPerMils,
                                              const wxSize& aMaxUserSizeMils ) :
        DIALOG_PAGES_SETTINGS_BASE( aParent ),
        m_parent( aParent ),
        m_screen( m_parent->GetScreen() ),
        m_initialized( false ),
        m_pageBitmap( nullptr ),
        m_iuPerMils( aIuPerMils ),
        m_customSizeX( aParent, m_userSizeXLabel, m_userSizeXCtrl, m_userSizeXUnits ),
        m_customSizeY( aParent, m_userSizeYLabel, m_userSizeYCtrl, m_userSizeYUnits )
{
    m_projectPath = Prj().GetProjectPath();
    m_maxPageSizeMils = aMaxUserSizeMils;
    m_tb = m_parent->GetTitleBlock();
    m_customFmt = false;
    m_localPrjConfigChanged = false;

    m_drawingSheet = new DS_DATA_MODEL;
    wxString serialization;
    DS_DATA_MODEL::GetTheInstance().SaveInString( &serialization );
    m_drawingSheet->SetPageLayout( TO_UTF8( serialization ) );

    m_PickDate->SetValue( wxDateTime::Now() );

    if( m_parent->GetName() == PL_EDITOR_FRAME_NAME )
    {
        SetTitle( _( "Preview Settings" ) );
        m_staticTextPaper->SetLabel( _( "Preview Paper" ) );
        m_staticTextTitleBlock->SetLabel( _( "Preview Title Block Data" ) );
    }
    else
    {
        SetTitle( _( "Page Settings" ) );
        m_staticTextPaper->SetLabel( _( "Paper" ) );
        m_staticTextTitleBlock->SetLabel( _( "Title Block" ) );
    }

    Centre();
}


DIALOG_PAGES_SETTINGS::~DIALOG_PAGES_SETTINGS()
{
    delete m_pageBitmap;
    delete m_drawingSheet;
}


bool DIALOG_PAGES_SETTINGS::TransferDataToWindow()
{
    wxString      msg;

    // initialize page format choice box and page format list.
    // The first shows translated strings, the second contains not translated strings
    m_paperSizeComboBox->Clear();

    for( const wxString& pageFmt : pageFmts )
    {
        m_pageFmt.Add( pageFmt );
        m_paperSizeComboBox->Append( wxGetTranslation( pageFmt ) );
    }

    // initialize the drawing sheet filename
    SetWksFileName( BASE_SCREEN::m_DrawingSheetFileName );

    m_pageInfo = m_parent->GetPageSettings();
    SetCurrentPageSizeSelection( m_pageInfo.GetType() );
    m_orientationComboBox->SetSelection( m_pageInfo.IsPortrait() );

    // only a click fires the "selection changed" event, so have to fabricate this check
    wxCommandEvent dummy;
    OnPaperSizeChoice( dummy );

    if( m_customFmt )
    {
        m_customSizeX.SetValue( m_pageInfo.GetWidthMils() * m_iuPerMils );
        m_customSizeY.SetValue( m_pageInfo.GetHeightMils() * m_iuPerMils );
    }
    else
    {
        m_customSizeX.SetValue( m_pageInfo.GetCustomWidthMils() * m_iuPerMils );
        m_customSizeY.SetValue( m_pageInfo.GetCustomHeightMils() * m_iuPerMils );
    }

    m_TextRevision->SetValue( m_tb.GetRevision() );
    m_TextDate->SetValue( m_tb.GetDate() );
    m_TextTitle->SetValue( m_tb.GetTitle() );
    m_TextCompany->SetValue( m_tb.GetCompany() );
    m_TextComment1->SetValue( m_tb.GetComment( 0 ) );
    m_TextComment2->SetValue( m_tb.GetComment( 1 ) );
    m_TextComment3->SetValue( m_tb.GetComment( 2 ) );
    m_TextComment4->SetValue( m_tb.GetComment( 3 ) );
    m_TextComment5->SetValue( m_tb.GetComment( 4 ) );
    m_TextComment6->SetValue( m_tb.GetComment( 5 ) );
    m_TextComment7->SetValue( m_tb.GetComment( 6 ) );
    m_TextComment8->SetValue( m_tb.GetComment( 7 ) );
    m_TextComment9->SetValue( m_tb.GetComment( 8 ) );

    // The default is to disable aall these fields for the "generic" dialog
    m_TextSheetCount->Show( false );
    m_TextSheetNumber->Show( false );
    m_PaperExport->Show( false );
    m_RevisionExport->Show( false );
    m_DateExport->Show( false );
    m_TitleExport->Show( false );
    m_CompanyExport->Show( false );
    m_Comment1Export->Show( false );
    m_Comment2Export->Show( false );
    m_Comment3Export->Show( false );
    m_Comment4Export->Show( false );
    m_Comment5Export->Show( false );
    m_Comment6Export->Show( false );
    m_Comment7Export->Show( false );
    m_Comment8Export->Show( false );
    m_Comment9Export->Show( false );

    onTransferDataToWindow();

    GetPageLayoutInfoFromDialog();
    UpdateDrawingSheetExample();

    GetSizer()->SetSizeHints( this );

    // Make the OK button the default.
    m_sdbSizerOK->SetDefault();
    m_initialized = true;

    return true;
}


bool DIALOG_PAGES_SETTINGS::TransferDataFromWindow()
{
    int idx = std::max( m_paperSizeComboBox->GetSelection(), 0 );
    const wxString paperType = m_pageFmt[idx];

    if( paperType.Contains( PAGE_INFO::Custom ) )
    {
        if( !m_customSizeX.Validate( MIN_PAGE_SIZE_MILS, m_maxPageSizeMils.x, EDA_UNITS::MILS ) )
            return false;

        if( !m_customSizeY.Validate( MIN_PAGE_SIZE_MILS, m_maxPageSizeMils.y, EDA_UNITS::MILS ) )
            return false;
    }

    if( SavePageSettings() )
    {
        m_screen->SetContentModified();

        if( LocalPrjConfigChanged() )
            m_parent->SaveProjectSettings();

        // Call the post processing (if any) after changes
        m_parent->OnPageSettingsChange();
    }

    return true;
}


void DIALOG_PAGES_SETTINGS::OnPaperSizeChoice( wxCommandEvent& event )
{
    int idx = m_paperSizeComboBox->GetSelection();

    if( idx < 0 )
        idx = 0;

    const wxString paperType = m_pageFmt[idx];

    if( paperType.Contains( PAGE_INFO::Custom ) )
    {
        m_orientationComboBox->Enable( false );
        m_customSizeX.Enable( true );
        m_customSizeY.Enable( true );
        m_customFmt = true;
    }
    else
    {
        m_orientationComboBox->Enable( true );

#if 0
        // ForcePortrait() does not exist, but could be useful.
        // so I leave these lines, which could be seen as a todo feature
        if( paperType.ForcePortrait() )
        {
            m_orientationComboBox->SetStringSelection( _( "Portrait" ) );
            m_orientationComboBox->Enable( false );
        }
#endif
        m_customSizeX.Enable( false );
        m_customSizeY.Enable( false );
        m_customFmt = false;
    }

    GetPageLayoutInfoFromDialog();
    UpdateDrawingSheetExample();
}


void DIALOG_PAGES_SETTINGS::OnUserPageSizeXTextUpdated( wxCommandEvent& event )
{
    if( m_initialized )
    {
        GetPageLayoutInfoFromDialog();
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnUserPageSizeYTextUpdated( wxCommandEvent& event )
{
    if( m_initialized )
    {
        GetPageLayoutInfoFromDialog();
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnPageOrientationChoice( wxCommandEvent& event )
{
    if( m_initialized )
    {
        GetPageLayoutInfoFromDialog();
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnRevisionTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextRevision->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetRevision( m_TextRevision->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnDateTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextDate->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetDate( m_TextDate->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnTitleTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextTitle->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetTitle( m_TextTitle->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnCompanyTextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextCompany->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetCompany( m_TextCompany->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment1TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment1->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 0, m_TextComment1->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment2TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment2->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 1, m_TextComment2->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment3TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment3->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 2, m_TextComment3->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment4TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment4->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 3, m_TextComment4->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment5TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment5->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 4, m_TextComment5->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment6TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment6->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 5, m_TextComment6->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment7TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment7->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 6, m_TextComment7->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment8TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment8->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 7, m_TextComment8->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnComment9TextUpdated( wxCommandEvent& event )
{
    if( m_initialized && m_TextComment9->IsModified() )
    {
        GetPageLayoutInfoFromDialog();
        m_tb.SetComment( 8, m_TextComment9->GetValue() );
        UpdateDrawingSheetExample();
    }
}


void DIALOG_PAGES_SETTINGS::OnDateApplyClick( wxCommandEvent& event )
{
    wxDateTime datetime = m_PickDate->GetValue();
    wxString date =
    // We can choose different formats. Should probably be kept in sync with CURRENT_DATE
    // formatting in TITLE_BLOCK.
    //
    //  datetime.Format( wxLocale::GetInfo( wxLOCALE_SHORT_DATE_FMT ) );
    //  datetime.Format( wxLocale::GetInfo( wxLOCALE_LONG_DATE_FMT ) );
    //  datetime.Format( wxT("%Y-%b-%d") );
        datetime.FormatISODate();

    m_TextDate->SetValue( date );
}


bool DIALOG_PAGES_SETTINGS::SavePageSettings()
{
    bool success = false;

    wxString fileName = GetWksFileName();

    if( fileName != BASE_SCREEN::m_DrawingSheetFileName )
    {
        wxString fullFileName = DS_DATA_MODEL::MakeFullFileName( fileName, m_projectPath );

        if( !fullFileName.IsEmpty() && !wxFileExists( fullFileName ) )
        {
            wxString msg;
            msg.Printf( _( "Drawing sheet file '%s' not found." ), fullFileName );
            wxMessageBox( msg );
            return false;
        }

        BASE_SCREEN::m_DrawingSheetFileName = fileName;
        DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( fullFileName );
        m_localPrjConfigChanged = true;
    }

    int idx = std::max( m_paperSizeComboBox->GetSelection(), 0 );
    const wxString paperType = m_pageFmt[idx];

    if( paperType.Contains( PAGE_INFO::Custom ) )
    {
        GetCustomSizeMilsFromDialog();

        success = m_pageInfo.SetType( PAGE_INFO::Custom );

        if( success )
        {
            PAGE_INFO::SetCustomWidthMils( m_layout_size.x );
            PAGE_INFO::SetCustomHeightMils( m_layout_size.y );

            m_pageInfo.SetWidthMils( m_layout_size.x );
            m_pageInfo.SetHeightMils( m_layout_size.y );
        }
    }
    else
    {
        // search for longest common string first, e.g. A4 before A
        if( paperType.Contains( PAGE_INFO::USLetter ) )
            success = m_pageInfo.SetType( PAGE_INFO::USLetter );
        else if( paperType.Contains( PAGE_INFO::USLegal ) )
            success = m_pageInfo.SetType( PAGE_INFO::USLegal );
        else if( paperType.Contains( PAGE_INFO::USLedger ) )
            success = m_pageInfo.SetType( PAGE_INFO::USLedger );
        else if( paperType.Contains( PAGE_INFO::GERBER ) )
            success = m_pageInfo.SetType( PAGE_INFO::GERBER );
        else if( paperType.Contains( PAGE_INFO::A5 ) )
            success = m_pageInfo.SetType( PAGE_INFO::A5 );
        else if( paperType.Contains( PAGE_INFO::A4 ) )
            success = m_pageInfo.SetType( PAGE_INFO::A4 );
        else if( paperType.Contains( PAGE_INFO::A3 ) )
            success = m_pageInfo.SetType( PAGE_INFO::A3 );
        else if( paperType.Contains( PAGE_INFO::A2 ) )
            success = m_pageInfo.SetType( PAGE_INFO::A2 );
        else if( paperType.Contains( PAGE_INFO::A1 ) )
            success = m_pageInfo.SetType( PAGE_INFO::A1 );
        else if( paperType.Contains( PAGE_INFO::A0 ) )
            success = m_pageInfo.SetType( PAGE_INFO::A0 );
        else if( paperType.Contains( PAGE_INFO::A ) )
            success = m_pageInfo.SetType( PAGE_INFO::A );
        else if( paperType.Contains( PAGE_INFO::B ) )
            success = m_pageInfo.SetType( PAGE_INFO::B );
        else if( paperType.Contains( PAGE_INFO::C ) )
            success = m_pageInfo.SetType( PAGE_INFO::C );
        else if( paperType.Contains( PAGE_INFO::D ) )
            success = m_pageInfo.SetType( PAGE_INFO::D );
        else if( paperType.Contains( PAGE_INFO::E ) )
            success = m_pageInfo.SetType( PAGE_INFO::E );

        if( success )
        {
            int choice = m_orientationComboBox->GetSelection();
            m_pageInfo.SetPortrait( choice != 0 );
        }
    }

    if( !success )
    {
        wxASSERT_MSG( false,
                      _( "the translation for paper size must preserve original spellings" ) );
        m_pageInfo.SetType( PAGE_INFO::A4 );
    }

    m_parent->SetPageSettings( m_pageInfo );

    m_tb.SetRevision( m_TextRevision->GetValue() );
    m_tb.SetDate(     m_TextDate->GetValue() );
    m_tb.SetCompany(  m_TextCompany->GetValue() );
    m_tb.SetTitle(    m_TextTitle->GetValue() );
    m_tb.SetComment( 0, m_TextComment1->GetValue() );
    m_tb.SetComment( 1, m_TextComment2->GetValue() );
    m_tb.SetComment( 2, m_TextComment3->GetValue() );
    m_tb.SetComment( 3, m_TextComment4->GetValue() );
    m_tb.SetComment( 4, m_TextComment5->GetValue() );
    m_tb.SetComment( 5, m_TextComment6->GetValue() );
    m_tb.SetComment( 6, m_TextComment7->GetValue() );
    m_tb.SetComment( 7, m_TextComment8->GetValue() );
    m_tb.SetComment( 8, m_TextComment9->GetValue() );

    m_parent->SetTitleBlock( m_tb );

    return onSavePageSettings();
}


void DIALOG_PAGES_SETTINGS::SetCurrentPageSizeSelection( const wxString& aPaperSize )
{
    // search all the not translated label list containing our paper type
    for( unsigned i = 0; i < m_pageFmt.GetCount(); ++i )
    {
        // parse each label looking for aPaperSize within it
        wxStringTokenizer st( m_pageFmt[i] );

        while( st.HasMoreTokens() )
        {
            if( st.GetNextToken() == aPaperSize )
            {
                m_paperSizeComboBox->SetSelection( i );
                return;
            }
        }
    }
}


void DIALOG_PAGES_SETTINGS::UpdateDrawingSheetExample()
{
    int lyWidth, lyHeight;

    wxSize clamped_layout_size( Clamp( MIN_PAGE_SIZE_MILS, m_layout_size.x, m_maxPageSizeMils.x ),
                                Clamp( MIN_PAGE_SIZE_MILS, m_layout_size.y, m_maxPageSizeMils.y ) );

    double lyRatio = clamped_layout_size.x < clamped_layout_size.y ?
                        (double) clamped_layout_size.y / clamped_layout_size.x :
                        (double) clamped_layout_size.x / clamped_layout_size.y;

    if( clamped_layout_size.x < clamped_layout_size.y )
    {
        lyHeight = MAX_PAGE_EXAMPLE_SIZE;
        lyWidth = KiROUND( (double) lyHeight / lyRatio );
    }
    else
    {
        lyWidth = MAX_PAGE_EXAMPLE_SIZE;
        lyHeight = KiROUND( (double) lyWidth / lyRatio );
    }

    if( m_pageBitmap )
    {
        m_PageLayoutExampleBitmap->SetBitmap( wxNullBitmap );
        delete m_pageBitmap;
    }

    m_pageBitmap = new wxBitmap( lyWidth + 1, lyHeight + 1 );

    if( m_pageBitmap->IsOk() )
    {
        double scaleW = (double) lyWidth  / clamped_layout_size.x;
        double scaleH = (double) lyHeight / clamped_layout_size.y;
        double scale = std::min( scaleW, scaleH );

        // Prepare DC.
        wxSize example_size( lyWidth + 1, lyHeight + 1 );
        wxMemoryDC memDC;
        memDC.SelectObject( *m_pageBitmap );
        memDC.SetClippingRegion( wxPoint( 0, 0 ), example_size );
        memDC.Clear();
        memDC.SetUserScale( scale, scale );

        // Get logical page size and margins.
        PAGE_INFO pageDUMMY;

        // Get page type
        int idx = m_paperSizeComboBox->GetSelection();

        if( idx < 0 )
            idx = 0;

        wxString pageFmtName = m_pageFmt[idx].BeforeFirst( ' ' );
        bool portrait = clamped_layout_size.x < clamped_layout_size.y;
        pageDUMMY.SetType( pageFmtName, portrait );

        if( m_customFmt )
        {
            pageDUMMY.SetWidthMils( clamped_layout_size.x );
            pageDUMMY.SetHeightMils( clamped_layout_size.y );
        }

        // Draw layout preview.
        KIGFX::DS_RENDER_SETTINGS renderSettings;
        COLOR_SETTINGS*           colorSettings = m_parent->GetColorSettings();
        COLOR4D                   bgColor = m_parent->GetDrawBgColor();
        wxString                  emptyString;

        DS_DATA_MODEL::SetAltInstance( m_drawingSheet );
        {
            GRResetPenAndBrush( &memDC );
            renderSettings.SetDefaultPenWidth( 1 );
            renderSettings.LoadColors( colorSettings );
            renderSettings.SetPrintDC( &memDC );

            if( m_parent->IsType( FRAME_SCH )
                || m_parent->IsType( FRAME_SCH_SYMBOL_EDITOR )
                || m_parent->IsType( FRAME_SCH_VIEWER )
                || m_parent->IsType( FRAME_SCH_VIEWER_MODAL ) )
            {
                COLOR4D color = renderSettings.GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET );
                renderSettings.SetLayerColor( LAYER_DRAWINGSHEET, color );
            }

            GRFilledRect( nullptr, &memDC, 0, 0, m_layout_size.x, m_layout_size.y, bgColor,
                          bgColor );

            PrintDrawingSheet( &renderSettings, pageDUMMY, emptyString, emptyString, m_tb,
                               m_screen->GetPageCount(), m_screen->GetPageNumber(), 1, &Prj(),
                               wxEmptyString, m_screen->GetVirtualPageNumber() == 1 );

            memDC.SelectObject( wxNullBitmap );
            m_PageLayoutExampleBitmap->SetBitmap( *m_pageBitmap );
        }

        DS_DATA_MODEL::SetAltInstance( nullptr );

        // Refresh the dialog.
        Layout();
        Refresh();
    }
}


void DIALOG_PAGES_SETTINGS::GetPageLayoutInfoFromDialog()
{
    int idx = std::max( m_paperSizeComboBox->GetSelection(), 0 );
    const wxString paperType = m_pageFmt[idx];

    // here we assume translators will keep original paper size spellings
    if( paperType.Contains( PAGE_INFO::Custom ) )
    {
        GetCustomSizeMilsFromDialog();

        if( m_layout_size.x && m_layout_size.y )
        {
            if( m_layout_size.x < m_layout_size.y )
                m_orientationComboBox->SetStringSelection( _( "Portrait" ) );
            else
                m_orientationComboBox->SetStringSelection( _( "Landscape" ) );
        }
    }
    else
    {
        PAGE_INFO       pageInfo;   // SetType() later to lookup size

        static const wxChar* papers[] = {
            // longest common string first, since sequential search below
            PAGE_INFO::A5,
            PAGE_INFO::A4,
            PAGE_INFO::A3,
            PAGE_INFO::A2,
            PAGE_INFO::A1,
            PAGE_INFO::A0,
            PAGE_INFO::A,
            PAGE_INFO::B,
            PAGE_INFO::C,
            PAGE_INFO::D,
            PAGE_INFO::E,
            PAGE_INFO::USLetter,
            PAGE_INFO::USLegal,
            PAGE_INFO::USLedger,
        };

        unsigned i;

        for( i=0;  i < arrayDim( papers );  ++i )
        {
            if( paperType.Contains( papers[i] ) )
            {
                pageInfo.SetType( papers[i] );
                break;
            }
        }

        wxASSERT( i != arrayDim(papers) );   // dialog UI match the above list?

        m_layout_size = pageInfo.GetSizeMils();

        // swap sizes to match orientation
        bool isPortrait = (bool) m_orientationComboBox->GetSelection();

        if( ( isPortrait  && m_layout_size.x >= m_layout_size.y ) ||
            ( !isPortrait && m_layout_size.x <  m_layout_size.y ) )
        {
            m_layout_size.Set( m_layout_size.y, m_layout_size.x );
        }
    }
}


void DIALOG_PAGES_SETTINGS::GetCustomSizeMilsFromDialog()
{
    double customSizeX = (double) m_customSizeX.GetValue() / m_iuPerMils;
    double customSizeY = (double) m_customSizeY.GetValue() / m_iuPerMils;

    // Prepare to painless double -> int conversion.
    customSizeX = Clamp( double( INT_MIN ), customSizeX, double( INT_MAX ) );
    customSizeY = Clamp( double( INT_MIN ), customSizeY, double( INT_MAX ) );
    m_layout_size = wxSize( KiROUND( customSizeX ), KiROUND( customSizeY ) );
}


void DIALOG_PAGES_SETTINGS::OnWksFileSelection( wxCommandEvent& event )
{
    wxFileName fn = GetWksFileName();
    wxString name = GetWksFileName();
    wxString path;

    if( fn.IsAbsolute() )
    {
        path = fn.GetPath();
        name = fn.GetFullName();
    }
    else
    {
        path = m_projectPath;
    }

    // Display a file picker dialog
    wxFileDialog fileDialog( this, _( "Select Drawing Sheet File" ),
                             path, name, DrawingSheetFileWildcard(),
                             wxFD_DEFAULT_STYLE | wxFD_FILE_MUST_EXIST );

    if( fileDialog.ShowModal() != wxID_OK )
        return;

    wxString fileName = fileDialog.GetPath();

    // Try to remove the path, if the path is the current working dir,
    // or the dir of kicad.pro (template), and use a relative path
    wxString shortFileName = DS_DATA_MODEL::MakeShortFileName( fileName, m_projectPath );

    // For Win/Linux/macOS compatibility, a relative path is a good idea
    if( shortFileName != GetWksFileName() && shortFileName != fileName )
    {
        wxString msg = wxString::Format( _( "The drawing sheet file name has changed.\n"
                                            "Do you want to use the relative path:\n"
                                            "\"%s\"\n"
                                            "instead of\n"
                                            "\"%s\"?" ),
                                         shortFileName,
                                         fileName );

        if( !IsOK( this, msg ) )
            shortFileName = fileName;
    }

    std::unique_ptr<DS_DATA_MODEL> ws = std::make_unique<DS_DATA_MODEL>();

    if( ws->LoadDrawingSheet( fileName ) )
    {
        if( m_drawingSheet != nullptr )
        {
            delete m_drawingSheet;
        }

        m_drawingSheet = ws.release();

        SetWksFileName( shortFileName );

        GetPageLayoutInfoFromDialog();
        UpdateDrawingSheetExample();
    }
}
