/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "dialog_lib_edit_pin_table.h"
#include "grid_tricks.h"
#include "lib_pin.h"
#include "pin_numbers.h"
#include <bitmaps.h>
#include <confirm.h>
#include <symbol_edit_frame.h>
#include <symbol_editor_settings.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/wx_grid.h>
#include <settings/settings_manager.h>
#include <string_utils.h>

class PIN_TABLE_DATA_MODEL : public wxGridTableBase
{
public:
    PIN_TABLE_DATA_MODEL( EDA_UNITS aUserUnits ) : m_userUnits( aUserUnits ), m_edited( false )
    {
    }

    int GetNumberRows() override { return (int) m_rows.size(); }
    int GetNumberCols() override { return COL_COUNT; }

    wxString GetColLabelValue( int aCol ) override
    {
        switch( aCol )
        {
        case COL_NUMBER:       return _( "Number" );
        case COL_NAME:         return _( "Name" );
        case COL_TYPE:         return _( "Electrical Type" );
        case COL_SHAPE:        return _( "Graphic Style" );
        case COL_ORIENTATION:  return _( "Orientation" );
        case COL_NUMBER_SIZE:  return _( "Number Text Size" );
        case COL_NAME_SIZE:    return _( "Name Text Size" );
        case COL_LENGTH:       return _( "Length" );
        case COL_POSX:         return _( "X Position" );
        case COL_POSY:         return _( "Y Position" );
        case COL_VISIBLE:      return _( "Visible" );
        default:               wxFAIL; return wxEmptyString;
        }
    }

    bool IsEmptyCell( int row, int col ) override
    {
        return false;   // don't allow adjacent cell overflow, even if we are actually empty
    }

    wxString GetValue( int aRow, int aCol ) override
    {
        return GetValue( m_rows[ aRow ], aCol, m_userUnits );
    }

    static wxString GetValue( const LIB_PINS& pins, int aCol, EDA_UNITS aUserUnits )
    {
        wxString fieldValue;

        if( pins.empty() )
            return fieldValue;

        for( LIB_PIN* pin : pins )
        {
            wxString val;

            switch( aCol )
            {
            case COL_NUMBER:
                val = pin->GetNumber();
                break;
            case COL_NAME:
                val = pin->GetName();
                break;
            case COL_TYPE:
                val = PinTypeNames()[static_cast<int>( pin->GetType() )];
                break;
            case COL_SHAPE:
                val = PinShapeNames()[static_cast<int>( pin->GetShape() )];
                break;
            case COL_ORIENTATION:
                if( PinOrientationIndex( pin->GetOrientation() ) >= 0 )
                    val = PinOrientationNames()[ PinOrientationIndex( pin->GetOrientation() ) ];
                break;
            case COL_NUMBER_SIZE:
                val = StringFromValue( aUserUnits, pin->GetNumberTextSize(), true );
                break;
            case COL_NAME_SIZE:
                val = StringFromValue( aUserUnits, pin->GetNameTextSize(), true );
                break;
            case COL_LENGTH:
                val = StringFromValue( aUserUnits, pin->GetLength() );
                break;
            case COL_POSX:
                val = StringFromValue( aUserUnits, pin->GetPosition().x );
                break;
            case COL_POSY:
                val = StringFromValue( aUserUnits, pin->GetPosition().y );
                break;
            case COL_VISIBLE:
                val = StringFromBool( pin->IsVisible() );
                break;
            default:
                wxFAIL;
                break;
            }

            if( aCol == COL_NUMBER )
            {
                if( fieldValue.length() )
                    fieldValue += wxT( ", " );
                fieldValue += val;
            }
            else
            {
                if( !fieldValue.Length() )
                    fieldValue = val;
                else if( val != fieldValue )
                    fieldValue = INDETERMINATE_STATE;
            }
        }

        return fieldValue;
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        if( aValue == INDETERMINATE_STATE )
            return;

        LIB_PINS pins = m_rows[ aRow ];

        for( LIB_PIN* pin : pins )
        {
            switch( aCol )
            {
            case COL_NUMBER:
                pin->SetNumber( aValue );
                break;

            case COL_NAME:
                pin->SetName( aValue );
                break;

            case COL_TYPE:
                if( PinTypeNames().Index( aValue ) != wxNOT_FOUND )
                    pin->SetType( (ELECTRICAL_PINTYPE) PinTypeNames().Index( aValue ) );

                break;

            case COL_SHAPE:
                if( PinShapeNames().Index( aValue ) != wxNOT_FOUND )
                    pin->SetShape( (GRAPHIC_PINSHAPE) PinShapeNames().Index( aValue ) );

                break;

            case COL_ORIENTATION:
                if( PinOrientationNames().Index( aValue ) != wxNOT_FOUND )
                    pin->SetOrientation( PinOrientationCode( PinOrientationNames().Index( aValue ) ) );
                break;

            case COL_NUMBER_SIZE:
                pin->SetNumberTextSize( ValueFromString( m_userUnits, aValue ) );
                break;

            case COL_NAME_SIZE:
                pin->SetNameTextSize( ValueFromString( m_userUnits, aValue ) );
                break;

            case COL_LENGTH:
                pin->SetLength( ValueFromString( m_userUnits, aValue ) );
                break;

            case COL_POSX:
                pin->SetPosition( wxPoint( ValueFromString( m_userUnits, aValue ),
                                           pin->GetPosition().y ) );
                break;

            case COL_POSY:
                pin->SetPosition( wxPoint( pin->GetPosition().x,
                                           ValueFromString( m_userUnits, aValue ) ) );
                break;

            case COL_VISIBLE:
                pin->SetVisible(BoolFromString( aValue ));
                break;
            default:
                wxFAIL;
                break;
            }
        }

        m_edited = true;
    }

    static int findRow( const std::vector<LIB_PINS>& aRowSet, const wxString& aName )
    {
        for( size_t i = 0; i < aRowSet.size(); ++i )
        {
            if( aRowSet[ i ][ 0 ] && aRowSet[ i ][ 0 ]->GetName() == aName )
                return i;
        }

        return -1;
    }

    static bool compare( const LIB_PINS& lhs, const LIB_PINS& rhs, int sortCol, bool ascending,
                         EDA_UNITS units )
    {
        wxString lhStr = GetValue( lhs, sortCol, units );
        wxString rhStr = GetValue( rhs, sortCol, units );

        if( lhStr == rhStr )
        {
            // Secondary sort key is always COL_NUMBER
            sortCol = COL_NUMBER;
            lhStr = GetValue( lhs, sortCol, units );
            rhStr = GetValue( rhs, sortCol, units );
        }

        bool res;

        // N.B. To meet the iterator sort conditions, we cannot simply invert the truth
        // to get the opposite sort.  i.e. ~(a<b) != (a>b)
        auto cmp = [ ascending ]( const auto a, const auto b )
                   {
                       if( ascending )
                           return a < b;
                       else
                           return b < a;
                   };

        switch( sortCol )
        {
        case COL_NUMBER:
        case COL_NAME:
            res = cmp( PIN_NUMBERS::Compare( lhStr, rhStr ), 0 );
            break;
        case COL_NUMBER_SIZE:
        case COL_NAME_SIZE:
            res = cmp( ValueFromString( units, lhStr ), ValueFromString( units, rhStr ) );
            break;
        case COL_LENGTH:
        case COL_POSX:
        case COL_POSY:
            res = cmp( ValueFromString( units, lhStr ), ValueFromString( units, rhStr ) );
            break;
        case COL_VISIBLE:
        default:
            res = cmp( StrNumCmp( lhStr, rhStr ), 0 );
            break;
        }

        return res;
    }

    void RebuildRows( const LIB_PINS& aPins, bool groupByName )
    {
        if ( GetView() )
        {
            // Commit any pending in-place edits before the row gets moved out from under
            // the editor.
            if( auto grid = dynamic_cast<WX_GRID*>( GetView() ) )
                grid->CommitPendingChanges( true );

            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, m_rows.size() );
            GetView()->ProcessTableMessage( msg );
        }

        m_rows.clear();

        for( LIB_PIN* pin : aPins )
        {
            int      rowIndex = -1;

            if( groupByName )
                rowIndex = findRow( m_rows, pin->GetName() );

            if( rowIndex < 0 )
            {
                m_rows.emplace_back( LIB_PINS() );
                rowIndex = m_rows.size() - 1;
            }

            m_rows[ rowIndex ].push_back( pin );
        }

        int sortCol = 0;
        bool ascending = true;

        if( GetView() && GetView()->GetSortingColumn() != wxNOT_FOUND )
        {
            sortCol = GetView()->GetSortingColumn();
            ascending = GetView()->IsSortOrderAscending();
        }

        for( LIB_PINS& row : m_rows )
            SortPins( row );

        SortRows( sortCol, ascending );

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_rows.size() );
            GetView()->ProcessTableMessage( msg );
        }
    }

    void SortRows( int aSortCol, bool ascending )
    {
        std::sort( m_rows.begin(), m_rows.end(),
                   [ aSortCol, ascending, this ]( const LIB_PINS& lhs, const LIB_PINS& rhs ) -> bool
                   {
                       return compare( lhs, rhs, aSortCol, ascending, m_userUnits );
                   } );
    }

    void SortPins( LIB_PINS& aRow )
    {
        std::sort( aRow.begin(), aRow.end(),
                   []( LIB_PIN* lhs, LIB_PIN* rhs ) -> bool
                   {
                       return PIN_NUMBERS::Compare( lhs->GetNumber(), rhs->GetNumber() ) < 0;
                   } );
    }

    void AppendRow( LIB_PIN* aPin )
    {
        LIB_PINS row;
        row.push_back( aPin );
        m_rows.push_back( row );

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
            GetView()->ProcessTableMessage( msg );
        }
    }

    LIB_PINS RemoveRow( int aRow )
    {
        LIB_PINS removedRow = m_rows[ aRow ];

        m_rows.erase( m_rows.begin() + aRow );

        if ( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aRow, 1 );
            GetView()->ProcessTableMessage( msg );
        }

        return removedRow;
    }

    bool IsEdited()
    {
        return m_edited;
    }

private:
    static wxString StringFromBool( bool aValue )
    {
        if( aValue )
            return wxT( "1" );
        else
            return wxT( "0" );
    }

    static bool BoolFromString( wxString aValue )
    {
        if( aValue == "1" )
        {
            return true;
        }
        else if( aValue == "0" )
        {
            return false;
        }
        else
        {
            wxFAIL_MSG( wxString::Format( "string \"%s\" can't be converted to boolean "
                                          "correctly, it will have been perceived as FALSE",
                                          aValue ) );
            return false;
        }
    }

    // Because the rows of the grid can either be a single pin or a group of pins, the
    // data model is a 2D vector.  If we're in the single pin case, each row's LIB_PINS
    // contains only a single pin.
    std::vector<LIB_PINS> m_rows;

    EDA_UNITS m_userUnits;
    bool      m_edited;
};


DIALOG_LIB_EDIT_PIN_TABLE::DIALOG_LIB_EDIT_PIN_TABLE( SYMBOL_EDIT_FRAME* parent,
                                                      LIB_SYMBOL* aSymbol ) :
        DIALOG_LIB_EDIT_PIN_TABLE_BASE( parent ),
        m_editFrame( parent ),
        m_part( aSymbol )
{
    m_dataModel = new PIN_TABLE_DATA_MODEL( GetUserUnits() );

    // Save original columns widths so we can do proportional sizing.
    for( int i = 0; i < COL_COUNT; ++i )
        m_originalColWidths[ i ] = m_grid->GetColSize( i );

    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    m_grid->SetTable( m_dataModel );
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    // Show/hide columns according to the user's preference
    auto cfg = parent->GetSettings();
    m_columnsShown = cfg->m_PinTableVisibleColumns;

    m_grid->ShowHideColumns( m_columnsShown );

    // Set special attributes
    wxGridCellAttr* attr;

    attr = new wxGridCellAttr;
    wxArrayString typeNames = PinTypeNames();
    typeNames.push_back( INDETERMINATE_STATE );
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinTypeIcons(), typeNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( PinTypeIcons(), typeNames ) );
    m_grid->SetColAttr( COL_TYPE, attr );

    attr = new wxGridCellAttr;
    wxArrayString shapeNames = PinShapeNames();
    shapeNames.push_back( INDETERMINATE_STATE );
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinShapeIcons(), shapeNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( PinShapeIcons(), shapeNames ) );
    m_grid->SetColAttr( COL_SHAPE, attr );

    attr = new wxGridCellAttr;
    wxArrayString orientationNames = PinOrientationNames();
    orientationNames.push_back( INDETERMINATE_STATE );
    attr->SetRenderer( new GRID_CELL_ICON_TEXT_RENDERER( PinOrientationIcons(),
                                                         orientationNames ) );
    attr->SetEditor( new GRID_CELL_ICON_TEXT_POPUP( PinOrientationIcons(), orientationNames ) );
    m_grid->SetColAttr( COL_ORIENTATION, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetEditor( new wxGridCellBoolEditor() );
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_grid->SetColAttr( COL_VISIBLE, attr );

    /* Right-aligned position values look much better, but only MSW and GTK2+
     * currently support right-aligned textEditCtrls, so the text jumps on all
     * the other platforms when you edit it.
    attr = new wxGridCellAttr;
    attr->SetAlignment( wxALIGN_RIGHT, wxALIGN_TOP );
    m_grid->SetColAttr( COL_POSX, attr );

    attr = new wxGridCellAttr;
    attr->SetAlignment( wxALIGN_RIGHT, wxALIGN_TOP );
    m_grid->SetColAttr( COL_POSY, attr );
    */

    m_addButton->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_deleteButton->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_refreshButton->SetBitmap( KiBitmap( BITMAPS::small_refresh ) );

    GetSizer()->SetSizeHints(this);
    Centre();

    if( !parent->IsSymbolEditable() || parent->IsSymbolAlias() )
    {
        m_ButtonsCancel->SetDefault();
        m_ButtonsOK->SetLabel( _( "Read Only" ) );
        m_ButtonsOK->Enable( false );
    }
    else
    {
        m_ButtonsOK->SetDefault();
    }

    m_ButtonsOK->SetDefault();
    m_initialized = true;
    m_modified = false;
    m_width = 0;

    // Connect Events
    m_grid->Connect( wxEVT_GRID_COL_SORT,
                     wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE::OnColSort ), nullptr, this );
}


DIALOG_LIB_EDIT_PIN_TABLE::~DIALOG_LIB_EDIT_PIN_TABLE()
{
    auto cfg = m_editFrame->GetSettings();
    cfg->m_PinTableVisibleColumns = m_grid->GetShownColumns().ToStdString();

    // Disconnect Events
    m_grid->Disconnect( wxEVT_GRID_COL_SORT,
                        wxGridEventHandler( DIALOG_LIB_EDIT_PIN_TABLE::OnColSort ), nullptr, this );

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_dataModel );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    // This is our copy of the pins.  If they were transferred to the part on an OK, then
    // m_pins will already be empty.
    for( auto pin : m_pins )
        delete pin;
}


bool DIALOG_LIB_EDIT_PIN_TABLE::TransferDataToWindow()
{
    // Make a copy of the pins for editing
    for( LIB_PIN* pin = m_part->GetNextPin( nullptr ); pin; pin = m_part->GetNextPin( pin ) )
        m_pins.push_back( new LIB_PIN( *pin ) );

    m_dataModel->RebuildRows( m_pins, m_cbGroup->GetValue() );

    updateSummary();

    return true;
}


bool DIALOG_LIB_EDIT_PIN_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    // Delete the part's pins
    while( LIB_PIN* pin = m_part->GetNextPin( nullptr ) )
        m_part->RemoveDrawItem( pin );

    // Transfer our pins to the part
    for( LIB_PIN* pin : m_pins )
    {
        pin->SetParent( m_part );
        m_part->AddDrawItem( pin );
    }

    m_pins.clear();

    return true;
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnColSort( wxGridEvent& aEvent )
{
    int sortCol = aEvent.GetCol();
    bool ascending;

    // This is bonkers, but wxWidgets doesn't tell us ascending/descending in the
    // event, and if we ask it will give us pre-event info.
    if( m_grid->IsSortingBy( sortCol ) )
        // same column; invert ascending
        ascending = !m_grid->IsSortOrderAscending();
    else
        // different column; start with ascending
        ascending = true;

    m_dataModel->SortRows( sortCol, ascending );
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnAddRow( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    LIB_PIN* newPin = new LIB_PIN( nullptr );

    if( m_pins.size() > 0 )
    {
        LIB_PIN* last = m_pins.back();

        newPin->SetOrientation( last->GetOrientation() );
        newPin->SetType( last->GetType() );
        newPin->SetShape( last->GetShape() );

        wxPoint pos = last->GetPosition();

        SYMBOL_EDITOR_SETTINGS* cfg = m_editFrame->GetSettings();

        if( last->GetOrientation() == PIN_LEFT || last->GetOrientation() == PIN_RIGHT )
            pos.y -= Mils2iu(cfg->m_Repeat.pin_step);
        else
            pos.x += Mils2iu(cfg->m_Repeat.pin_step);

        newPin->SetPosition( pos );
    }

    m_pins.push_back( newPin );

    m_dataModel->AppendRow( m_pins[ m_pins.size() - 1 ] );

    m_grid->MakeCellVisible( m_grid->GetNumberRows() - 1, 0 );
    m_grid->SetGridCursor( m_grid->GetNumberRows() - 1, 0 );

    m_grid->EnableCellEditControl( true );
    m_grid->ShowCellEditControl();

    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnDeleteRow( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    if( m_pins.size() == 0 )   // empty table
        return;

    int curRow = m_grid->GetGridCursorRow();

    if( curRow < 0 )
        return;

    LIB_PINS removedRow = m_dataModel->RemoveRow( curRow );

    for( auto pin : removedRow )
        m_pins.erase( std::find( m_pins.begin(), m_pins.end(), pin ) );

    curRow = std::min( curRow, m_grid->GetNumberRows() - 1 );
    m_grid->GoToCell( curRow, m_grid->GetGridCursorCol() );
    m_grid->SetGridCursor( curRow, m_grid->GetGridCursorCol() );
    m_grid->SelectRow( curRow );


    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnCellEdited( wxGridEvent& event )
{
    updateSummary();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnRebuildRows( wxCommandEvent&  )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    m_dataModel->RebuildRows( m_pins, m_cbGroup->GetValue() );

    adjustGridColumns( m_grid->GetRect().GetWidth() );
}


void DIALOG_LIB_EDIT_PIN_TABLE::adjustGridColumns( int aWidth )
{
    m_width = aWidth;

    // Account for scroll bars
    aWidth -= ( m_grid->GetSize().x - m_grid->GetClientSize().x );

    wxGridUpdateLocker deferRepaintsTillLeavingScope;

    // The Number and Name columns must be at least wide enough to hold their contents, but
    // no less wide than their original widths.

    m_grid->AutoSizeColumn( COL_NUMBER );

    if( m_grid->GetColSize( COL_NUMBER ) < m_originalColWidths[ COL_NUMBER ] )
        m_grid->SetColSize( COL_NUMBER, m_originalColWidths[ COL_NUMBER ] );

    m_grid->AutoSizeColumn( COL_NAME );

    if( m_grid->GetColSize( COL_NAME ) < m_originalColWidths[ COL_NAME ] )
        m_grid->SetColSize( COL_NAME, m_originalColWidths[ COL_NAME ] );

    // If the grid is still wider than the columns, then stretch the Number and Name columns
    // to fit.

    for( int i = 0; i < COL_COUNT; ++i )
        aWidth -= m_grid->GetColSize( i );

    if( aWidth > 0 )
    {
        m_grid->SetColSize( COL_NUMBER, m_grid->GetColSize( COL_NUMBER ) + aWidth / 2 );
        m_grid->SetColSize( COL_NAME, m_grid->GetColSize( COL_NAME ) + aWidth / 2 );
    }
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnSize( wxSizeEvent& event )
{
    auto new_size = event.GetSize().GetX();

    if( m_initialized && m_width != new_size )
    {
        adjustGridColumns( new_size );
    }

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnUpdateUI( wxUpdateUIEvent& event )
{
    wxString columnsShown = m_grid->GetShownColumns();

    if( columnsShown != m_columnsShown )
    {
        m_columnsShown = columnsShown;

        if( !m_grid->IsCellEditControlShown() )
            adjustGridColumns( m_grid->GetRect().GetWidth() );
    }
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnCancel( wxCommandEvent& event )
{
    Close();
}


void DIALOG_LIB_EDIT_PIN_TABLE::OnClose( wxCloseEvent& event )
{
    // This is a cancel, so commit quietly as we're going to throw the results away anyway.
    m_grid->CommitPendingChanges( true );

    int retval = wxID_CANCEL;

    if( m_dataModel->IsEdited() )
    {
        if( HandleUnsavedChanges( this, _( "Save changes?" ),
                                  [&]() -> bool
                                  {
                                      if( TransferDataFromWindow() )
                                      {
                                          retval = wxID_OK;
                                          return true;
                                      }

                                      return false;
                                  } ) )
        {
            if( IsQuasiModal() )
                EndQuasiModal( retval );
            else
                EndDialog( retval );

            return;
        }
        else
        {
            event.Veto();
            return;
        }
    }

    // No change in dialog: we can close it
    if( IsQuasiModal() )
        EndQuasiModal( retval );
    else
        EndDialog( retval );

    return;
}


void DIALOG_LIB_EDIT_PIN_TABLE::updateSummary()
{
    PIN_NUMBERS pinNumbers;

    for( LIB_PIN* pin : m_pins )
    {
        if( pin->GetNumber().Length() )
            pinNumbers.insert( pin->GetNumber() );
    }

    m_summary->SetLabel( pinNumbers.GetSummary() );
}
