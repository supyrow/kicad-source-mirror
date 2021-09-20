/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_symbol.h>
#include <id.h>
#include <kiway.h>
#include <confirm.h>
#include <tool/conditional_menu.h>
#include <tool/selection_conditions.h>
#include <tools/ee_actions.h>
#include <tools/ee_inspection_tool.h>
#include <tools/ee_selection_tool.h>
#include <tools/ee_selection.h>
#include <sim/sim_plot_frame.h>
#include <sch_edit_frame.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <eda_doc.h>
#include <sch_marker.h>
#include <project.h>
#include <dialogs/dialog_display_html_text_base.h>
#include <dialogs/dialog_erc.h>
#include <math/util.h>      // for KiROUND


class DIALOG_DISPLAY_HTML_TEXT : public DIALOG_DISPLAY_HTML_TEXT_BASE
{
public:
    DIALOG_DISPLAY_HTML_TEXT( wxWindow* aParent, wxWindowID aId, const wxString& aTitle,
                              const wxPoint& aPos, const wxSize& aSize, long aStyle = 0 ) :
        DIALOG_DISPLAY_HTML_TEXT_BASE( aParent, aId, aTitle, aPos, aSize, aStyle )
    { }

    ~DIALOG_DISPLAY_HTML_TEXT()
    { }

    void SetPage( const wxString& message )
    {
        // Handle light/dark mode colors...

        wxTextCtrl dummy( GetParent(), wxID_ANY );
        wxColour   foreground = dummy.GetForegroundColour();
        wxColour   background = dummy.GetBackgroundColour();

        m_htmlWindow->SetPage( wxString::Format( wxT( "<html>"
                                                      "  <body bgcolor='%s' text='%s'>"
                                                      "    %s"
                                                      "  </body>"
                                                      "</html>" ),
                                                 background.GetAsString( wxC2S_HTML_SYNTAX ),
                                                 foreground.GetAsString( wxC2S_HTML_SYNTAX ),
                                                 message ) );
    }
};


EE_INSPECTION_TOOL::EE_INSPECTION_TOOL() :
    EE_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.InspectionTool" ),
    m_ercDialog( nullptr )
{
}


bool EE_INSPECTION_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    auto singleMarkerCondition = SELECTION_CONDITIONS::OnlyType( SCH_MARKER_T )
                              && SELECTION_CONDITIONS::Count( 1 );

    // Add inspection actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( EE_ACTIONS::excludeMarker, singleMarkerCondition, 100 );

    selToolMenu.AddItem( EE_ACTIONS::showDatasheet,
                         EE_CONDITIONS::SingleSymbol && EE_CONDITIONS::Idle, 220 );

    return true;
}


void EE_INSPECTION_TOOL::Reset( RESET_REASON aReason )
{
    EE_TOOL_BASE::Reset( aReason );

    if( aReason == MODEL_RELOAD )
    {
        DestroyERCDialog();
    }
}


int EE_INSPECTION_TOOL::RunERC( const TOOL_EVENT& aEvent )
{
    ShowERCDialog();
    return 0;
}


void EE_INSPECTION_TOOL::ShowERCDialog()
{
    if( m_frame->IsType( FRAME_SCH ) )
    {
        if( m_ercDialog )
        {
            // Needed at least on Windows. Raise() is not enough
            m_ercDialog->Show( true );
            // Bring it to the top if already open.  Dual monitor users need this.
            m_ercDialog->Raise();
        }
        else
        {
            // This is a modeless dialog, so new it rather than instantiating on stack.
            m_ercDialog = new DIALOG_ERC( static_cast<SCH_EDIT_FRAME*>( m_frame ) );

            m_ercDialog->Show( true );
        }
    }
}


void EE_INSPECTION_TOOL::DestroyERCDialog()
{
    if( m_ercDialog )
        m_ercDialog->Destroy();

    m_ercDialog = nullptr;
}


int EE_INSPECTION_TOOL::PrevMarker( const TOOL_EVENT& aEvent )
{
    if( m_ercDialog )
    {
        m_ercDialog->Show( true );
        m_ercDialog->Raise();
        m_ercDialog->PrevMarker();
    }
    else
    {
        ShowERCDialog();
    }

    return 0;
}


int EE_INSPECTION_TOOL::NextMarker( const TOOL_EVENT& aEvent )
{
    if( m_ercDialog )
    {
        m_ercDialog->Show( true );
        m_ercDialog->Raise();
        m_ercDialog->NextMarker();
    }
    else
    {
        ShowERCDialog();
    }

    return 0;
}


int EE_INSPECTION_TOOL::ExcludeMarker( const TOOL_EVENT& aEvent )
{
    if( m_ercDialog )
    {
        // Let the ERC dialog handle it since it has more update hassles to worry about
        m_ercDialog->ExcludeMarker();
    }
    else
    {
        EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
        EE_SELECTION&      selection = selTool->GetSelection();

        if( selection.GetSize() == 1 && selection.Front()->Type() == SCH_MARKER_T )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( selection.Front() );

            marker->SetExcluded( true );
            m_frame->GetCanvas()->GetView()->Update( marker );
            m_frame->GetCanvas()->Refresh();
            m_frame->OnModify();
        }
    }

    return 0;
}


// helper function to sort pins by pin num
bool sort_by_pin_number( const LIB_PIN* ref, const LIB_PIN* tst )
{
    // Use number as primary key
    int test = ref->GetNumber().Cmp( tst->GetNumber() );

    // Use DeMorgan variant as secondary key
    if( test == 0 )
        test = ref->GetConvert() - tst->GetConvert();

    // Use unit as tertiary key
    if( test == 0 )
        test = ref->GetUnit() - tst->GetUnit();

    return test < 0;
}


int EE_INSPECTION_TOOL::CheckSymbol( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* symbol = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();
    EDA_UNITS units = m_frame->GetUserUnits();

    if( !symbol )
        return 0;

    LIB_PINS pinList;
    symbol->GetPins( pinList );

    // Test for duplicates:
    // Sort pins by pin num, so 2 duplicate pins
    // (pins with the same number) will be consecutive in list
    sort( pinList.begin(), pinList.end(), sort_by_pin_number );

    // The minimal grid size allowed to place a pin is 25 mils
    // the best grid size is 50 mils, but 25 mils is still usable
    // this is because all symbols are using a 50 mils grid to place pins, and therefore
    // the wires must be on the 50 mils grid
    // So raise an error if a pin is not on a 25 (or bigger :50 or 100) mils grid
    const int min_grid_size = Mils2iu( 25 );
    const int grid_size = KiROUND( getView()->GetGAL()->GetGridSize().x );
    const int clamped_grid_size = ( grid_size < min_grid_size ) ? min_grid_size : grid_size;

    std::vector<wxString> messages;
    wxString              msg;

    for( unsigned ii = 1; ii < pinList.size(); ii++ )
    {
        LIB_PIN* pin  = pinList[ii - 1];
        LIB_PIN* next = pinList[ii];

        if( pin->GetNumber() != next->GetNumber() || pin->GetConvert() != next->GetConvert() )
            continue;

        wxString pinName;
        wxString nextName;

        if( pin->GetName() != "~"  && !pin->GetName().IsEmpty() )
            pinName = " '" + pin->GetName() + "'";

        if( next->GetName() != "~"  && !next->GetName().IsEmpty() )
            nextName = " '" + next->GetName() + "'";

        if( symbol->HasConversion() && next->GetConvert() )
        {
            if( symbol->GetUnitCount() <= 1 )
            {
                msg.Printf( _( "<b>Duplicate pin %s</b> %s at location <b>(%.3f, %.3f)</b>"
                               " conflicts with pin %s%s at location <b>(%.3f, %.3f)</b>"
                               " of converted." ),
                            next->GetNumber(),
                            nextName,
                            MessageTextFromValue( units, next->GetPosition().x ),
                            MessageTextFromValue( units, -next->GetPosition().y ),
                            pin->GetNumber(),
                            pin->GetName(),
                            MessageTextFromValue( units, pin->GetPosition().x ),
                            MessageTextFromValue( units, -pin->GetPosition().y ) );
            }
            else
            {
                msg.Printf( _( "<b>Duplicate pin %s</b> %s at location <b>(%.3f, %.3f)</b>"
                               " conflicts with pin %s%s at location <b>(%.3f, %.3f)</b>"
                               " in units %c and %c of converted." ),
                            next->GetNumber(),
                            nextName,
                            MessageTextFromValue( units, next->GetPosition().x ),
                            MessageTextFromValue( units, -next->GetPosition().y ),
                            pin->GetNumber(),
                            pinName,
                            MessageTextFromValue( units, pin->GetPosition().x ),
                            MessageTextFromValue( units, -pin->GetPosition().y ),
                            'A' + next->GetUnit() - 1,
                            'A' + pin->GetUnit() - 1 );
            }
        }
        else
        {
            if( symbol->GetUnitCount() <= 1 )
            {
                msg.Printf( _( "<b>Duplicate pin %s</b> %s at location <b>(%s, %s)</b>"
                               " conflicts with pin %s%s at location <b>(%s, %s)</b>." ),
                            next->GetNumber(),
                            nextName,
                            MessageTextFromValue( units, next->GetPosition().x ),
                            MessageTextFromValue( units, -next->GetPosition().y ),
                            pin->GetNumber(),
                            pinName,
                            MessageTextFromValue( units, pin->GetPosition().x ),
                            MessageTextFromValue( units, -pin->GetPosition().y ) );
            }
            else
            {
                msg.Printf( _( "<b>Duplicate pin %s</b> %s at location <b>(%s, %s)</b>"
                               " conflicts with pin %s%s at location <b>(%s, %s)</b>"
                               " in units %c and %c." ),
                            next->GetNumber(),
                            nextName,
                            MessageTextFromValue( units, next->GetPosition().x ),
                            MessageTextFromValue( units, -next->GetPosition().y ),
                            pin->GetNumber(),
                            pinName,
                            MessageTextFromValue( units, pin->GetPosition().x ),
                            MessageTextFromValue( units, -pin->GetPosition().y ),
                            'A' + next->GetUnit() - 1,
                            'A' + pin->GetUnit() - 1 );
            }
        }

        msg += wxT( "<br><br>" );
        messages.push_back( msg );
    }

    for( LIB_PIN* pin : pinList )
    {
        wxString pinName = pin->GetName();

        if( pinName.IsEmpty() || pinName == "~" )
            pinName = "";
        else
            pinName = "'" + pinName + "'";

        if( !symbol->IsPower()
                && pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN
                && !pin->IsVisible() )
        {
            // hidden power pin
            if( symbol->HasConversion() && pin->GetConvert() )
            {
                if( symbol->GetUnitCount() <= 1 )
                {
                    msg.Printf( _( "Info: <b>Hidden power pin %s</b> %s at location <b>(%s, %s)</b>"
                                   " of converted." ),
                                pin->GetNumber(),
                                pinName,
                                MessageTextFromValue( units, pin->GetPosition().x ),
                                MessageTextFromValue( units, -pin->GetPosition().y ) );
                }
                else
                {
                    msg.Printf( _( "Info: <b>Hidden power pin %s</b> %s at location <b>(%s, %s)</b>"
                                   " in unit %c of converted." ),
                                pin->GetNumber(),
                                pinName,
                                MessageTextFromValue( units, pin->GetPosition().x ),
                                MessageTextFromValue( units, -pin->GetPosition().y ),
                                'A' + pin->GetUnit() - 1 );
                }
            }
            else
            {
                if( symbol->GetUnitCount() <= 1 )
                {
                    msg.Printf( _( "Info: <b>Hidden power pin %s</b> %s at location <b>(%s, %s)</b>." ),
                                pin->GetNumber(),
                                pinName,
                                MessageTextFromValue( units, pin->GetPosition().x ),
                                MessageTextFromValue( units, -pin->GetPosition().y ) );
                }
                else
                {
                    msg.Printf( _( "Info: <b>Hidden power pin %s</b> %s at location <b>(%s, %s)</b>"
                                   " in unit %c." ),
                                pin->GetNumber(),
                                pinName,
                                MessageTextFromValue( units, pin->GetPosition().x ),
                                MessageTextFromValue( units, -pin->GetPosition().y ),
                                'A' + pin->GetUnit() - 1 );
                }
            }

            msg += wxT( "<br>" );
            msg += _( "(Hidden power pins will drive their pin names on to any connected nets.)" );
            msg += wxT( "<br><br>" );
            messages.push_back( msg );
        }

        if( ( (pin->GetPosition().x % clamped_grid_size) != 0 )
                || ( (pin->GetPosition().y % clamped_grid_size) != 0 ) )
        {
            // pin is off grid
            if( symbol->HasConversion() && pin->GetConvert() )
            {
                if( symbol->GetUnitCount() <= 1 )
                {
                    msg.Printf( _( "<b>Off grid pin %s</b> %s at location <b>(%s, %s)</b>"
                                   " of converted." ),
                                pin->GetNumber(),
                                pinName,
                                MessageTextFromValue( units, pin->GetPosition().x ),
                                MessageTextFromValue( units, -pin->GetPosition().y ) );
                }
                else
                {
                    msg.Printf( _( "<b>Off grid pin %s</b> %s at location <b>(%.3s, %.3s)</b>"
                                   " in unit %c of converted." ),
                                pin->GetNumber(),
                                pinName,
                                MessageTextFromValue( units, pin->GetPosition().x ),
                                MessageTextFromValue( units, -pin->GetPosition().y ),
                                'A' + pin->GetUnit() - 1 );
                }
            }
            else
            {
                if( symbol->GetUnitCount() <= 1 )
                {
                    msg.Printf( _( "<b>Off grid pin %s</b> %s at location <b>(%s, %s)</b>." ),
                                pin->GetNumber(),
                                pinName,
                                MessageTextFromValue( units, pin->GetPosition().x ),
                                MessageTextFromValue( units, -pin->GetPosition().y ) );
                }
                else
                {
                    msg.Printf( _( "<b>Off grid pin %s</b> %s at location <b>(%s, %s)</b>"
                                   " in unit %c." ),
                                pin->GetNumber(),
                                pinName,
                                MessageTextFromValue( units, pin->GetPosition().x ),
                                MessageTextFromValue( units, -pin->GetPosition().y ),
                                'A' + pin->GetUnit() - 1 );
                }
            }

            msg += wxT( "<br><br>" );
            messages.push_back( msg );
        }
    }

    if( messages.empty() )
    {
        DisplayInfoMessage( m_frame, _( "No symbol issues found." ) );
    }
    else
    {
        wxColour bgcolor = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
        wxColour fgcolor = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );
        wxString outmsg = wxString::Format( "<html><body bgcolor='%s' text='%s'>",
                                            bgcolor.GetAsString( wxC2S_HTML_SYNTAX ),
                                            fgcolor.GetAsString( wxC2S_HTML_SYNTAX ) );

        for( const wxString& msg : messages )
            outmsg += msg;

        outmsg += "</body></html>";

        DIALOG_DISPLAY_HTML_TEXT error_display( m_frame, wxID_ANY, _( "Symbol Warnings" ),
                                                wxDefaultPosition, wxSize( 700, 350 ) );

        error_display.SetPage( outmsg );
        error_display.ShowModal();
    }

    return 0;
}


int EE_INSPECTION_TOOL::RunSimulation( const TOOL_EVENT& aEvent )
{
#ifdef KICAD_SPICE
    SIM_PLOT_FRAME* simFrame = (SIM_PLOT_FRAME*) m_frame->Kiway().Player( FRAME_SIMULATOR, true );

    if( !simFrame )
        return -1;

    simFrame->Show( true );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( simFrame->IsIconized() )
        simFrame->Iconize( false );

    simFrame->Raise();
#endif /* KICAD_SPICE */
    return 0;
}


int EE_INSPECTION_TOOL::ShowDatasheet( const TOOL_EVENT& aEvent )
{
    wxString datasheet;

    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        LIB_SYMBOL* symbol = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();

        if( !symbol )
            return 0;

        datasheet = symbol->GetDatasheetField().GetText();
    }
    else if( m_frame->IsType( FRAME_SCH_VIEWER ) || m_frame->IsType( FRAME_SCH_VIEWER_MODAL ) )
    {
        LIB_SYMBOL* entry = static_cast<SYMBOL_VIEWER_FRAME*>( m_frame )->GetSelectedSymbol();

        if( !entry )
            return 0;

        datasheet = entry->GetDatasheetField().GetText();
    }
    else if( m_frame->IsType( FRAME_SCH ) )
    {
        EE_SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::SymbolsOnly );

        if( selection.Empty() )
            return 0;

        SCH_SYMBOL* symbol = (SCH_SYMBOL*) selection.Front();

        datasheet = symbol->GetField( DATASHEET_FIELD )->GetText();
    }

    if( datasheet.IsEmpty() || datasheet == wxT( "~" ) )
        m_frame->ShowInfoBarError( _( "No datasheet defined." ) );
    else
        GetAssociatedDocument( m_frame, datasheet, &m_frame->Prj() );

    return 0;
}


int EE_INSPECTION_TOOL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    EE_SELECTION&      selection = selTool->GetSelection();

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM* item = (EDA_ITEM*) selection.Front();

        MSG_PANEL_ITEMS msgItems;
        item->GetMsgPanelInfo( m_frame, msgItems );
        m_frame->SetMsgPanel( msgItems );
    }
    else
    {
        m_frame->ClearMsgPanel();
    }

    if( SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
        editFrame->UpdateNetHighlightStatus();

    return 0;
}


void EE_INSPECTION_TOOL::setTransitions()
{
    Go( &EE_INSPECTION_TOOL::RunERC,              EE_ACTIONS::runERC.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::PrevMarker,          EE_ACTIONS::prevMarker.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::NextMarker,          EE_ACTIONS::nextMarker.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::ExcludeMarker,       EE_ACTIONS::excludeMarker.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::CheckSymbol,         EE_ACTIONS::checkSymbol.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::RunSimulation,       EE_ACTIONS::runSimulation.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::ShowDatasheet,       EE_ACTIONS::showDatasheet.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::SelectedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::UnselectedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::ClearedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::SelectedItemsModified );
}


