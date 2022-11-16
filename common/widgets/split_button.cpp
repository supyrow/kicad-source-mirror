/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 Anil8735(https://stackoverflow.com/users/3659387/anil8753)
 *                    from https://stackoverflow.com/a/37274011
 * Copyright (C) 2020-2022 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/split_button.h>
#include <wx/button.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/menu.h>
#include <wx/renderer.h>
#include <wx/settings.h>
#include <kiplatform/ui.h>

SPLIT_BUTTON::SPLIT_BUTTON( wxWindow* aParent, wxWindowID aId, const wxString& aLabel,
                            const wxPoint& aPos, const wxSize& aSize ) :
        wxPanel( aParent, aId, aPos, aSize, wxBORDER_NONE | wxTAB_TRAVERSAL, "DropDownButton" ),
        m_label( aLabel )
{
    if( aSize == wxDefaultSize )
    {
        wxSize defaultSize = wxButton::GetDefaultSize();

        wxSize textSize = GetTextExtent( m_label );
        SetMinSize( wxSize( textSize.GetWidth(), defaultSize.GetHeight() + 1 ) );
    }

    Bind( wxEVT_PAINT, &SPLIT_BUTTON::OnPaint, this );
    Bind( wxEVT_LEFT_UP, &SPLIT_BUTTON::OnLeftButtonUp, this );
    Bind( wxEVT_LEFT_DOWN, &SPLIT_BUTTON::OnLeftButtonDown, this );
    Bind( wxEVT_KILL_FOCUS, &SPLIT_BUTTON::OnKillFocus, this );
    Bind( wxEVT_LEAVE_WINDOW, &SPLIT_BUTTON::OnMouseLeave, this );
    Bind( wxEVT_ENTER_WINDOW, &SPLIT_BUTTON::OnMouseEnter, this );

    Bind( wxEVT_SYS_COLOUR_CHANGED, wxSysColourChangedEventHandler( SPLIT_BUTTON::onThemeChanged ),
          this );

    m_pMenu = new wxMenu();
}


SPLIT_BUTTON::~SPLIT_BUTTON()
{
    delete m_pMenu;
    m_pMenu = nullptr;
}


void SPLIT_BUTTON::onThemeChanged( wxSysColourChangedEvent &aEvent )
{
    Refresh();
}


void SPLIT_BUTTON::SetMinSize( const wxSize& aSize )
{
    m_unadjustedMinSize = aSize;
    wxPanel::SetMinSize( wxSize( aSize.GetWidth() + m_arrowButtonWidth + m_widthPadding,
                                 aSize.GetHeight() ) );
}


void SPLIT_BUTTON::SetWidthPadding( int aPadding )
{
    m_widthPadding = aPadding;
    SetMinSize( m_unadjustedMinSize );
}


void SPLIT_BUTTON::SetBitmap( const wxBitmap& aBmp )
{
    m_bitmap = aBmp;

    SetMinSize( wxSize( m_bitmap.GetWidth(), m_bitmap.GetHeight() ) );
}


void SPLIT_BUTTON::SetLabel( const wxString& aLabel )
{
    m_label = aLabel;
    Refresh();
}


wxMenu* SPLIT_BUTTON::GetSplitButtonMenu()
{
    return m_pMenu;
}


void SPLIT_BUTTON::OnKillFocus( wxFocusEvent& aEvent )
{
    m_stateButton = wxCONTROL_CURRENT;
    m_stateMenu   = wxCONTROL_CURRENT;
    Refresh();

    aEvent.Skip();
}


void SPLIT_BUTTON::OnMouseLeave( wxMouseEvent& aEvent )
{
    m_stateButton = 0;
    m_stateMenu   = 0;
    Refresh();

    aEvent.Skip();
}


void SPLIT_BUTTON::OnMouseEnter( wxMouseEvent& aEvent )
{
    m_stateButton = wxCONTROL_CURRENT;
    m_stateMenu   = wxCONTROL_CURRENT;
    Refresh();

    aEvent.Skip();
}


void SPLIT_BUTTON::OnLeftButtonUp( wxMouseEvent& aEvent )
{
    m_stateButton = 0;
    m_stateMenu   = 0;

    Refresh();

    int x = -1;
    int y = -1;
    aEvent.GetPosition( &x, &y );

    if( x < ( GetSize().GetWidth() - m_arrowButtonWidth ) )
    {
        wxEvtHandler* pEventHandler = GetEventHandler();
        wxASSERT( pEventHandler );

        pEventHandler->CallAfter(
                [=]()
                {
                    wxCommandEvent evt( wxEVT_BUTTON, GetId() );
                    evt.SetEventObject( this );
                    GetEventHandler()->ProcessEvent( evt );
                } );
    }

    m_bLButtonDown = false;

    aEvent.Skip();
}


void SPLIT_BUTTON::OnLeftButtonDown( wxMouseEvent& aEvent )
{
    m_bLButtonDown = true;

    int x = -1;
    int y = -1;
    aEvent.GetPosition( &x, &y );

    if( x >= ( GetSize().GetWidth() - m_arrowButtonWidth ) )
    {
        m_stateButton = 0;
        m_stateMenu   = wxCONTROL_PRESSED;
        Refresh();

        wxSize  size = GetSize();
        wxPoint position;
        position.x = 0;
        position.y = size.GetHeight();
        PopupMenu( m_pMenu, position );

        m_stateMenu = 0;
        Refresh();
    }
    else
    {
        m_stateButton = wxCONTROL_PRESSED;
        m_stateMenu   = wxCONTROL_PRESSED;
        Refresh();
    }

    aEvent.Skip();
}


void SPLIT_BUTTON::OnPaint( wxPaintEvent& WXUNUSED( aEvent ) )
{
    wxPaintDC dc( this );
    wxSize    size  = GetSize();
    const int width = size.GetWidth() - m_arrowButtonWidth;

#ifdef __WXMAC__
    auto drawBackground =
            [&]( wxRect aRect )
            {
                // wxWidgets doesn't have much support for dark mode on OSX; none of the
                // system colours return the right values, nor does wxRendererNative draw
                // the borders correctly.  So we add some empirically chosen hacks here.

                wxColor fg = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT );
                wxColor bg = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );

                aRect.width += 1;
                aRect.height += 1;

                if( KIPLATFORM::UI::IsDarkTheme() )
                {
                    bg = bg.ChangeLightness( m_bIsEnable ? 130 : 120 );
                    dc.SetBrush( bg );
                    dc.SetPen( bg );
                }
                else
                {
                    bg = bg.ChangeLightness( m_bIsEnable ? 200 : 160 );
                    dc.SetBrush( bg );
                    fg = fg.ChangeLightness( 180 );
                    dc.SetPen( fg );
                }

                dc.DrawRoundedRectangle( aRect, aRect.height / 4 );
            };
#endif

    // Draw first part of button
    wxRect r1;
    r1.x      = 0;
    r1.y      = 0;
    r1.width  = width;
    r1.height = size.GetHeight();

#ifdef __WXMAC__
    // wxRendereNative doesn't handle dark mode on OSX.
    drawBackground( r1 );
#else
    r1.width += 2;
    wxRendererNative::Get().DrawPushButton( this, dc, r1, m_stateButton );
#endif

    SetForegroundColour( m_bIsEnable ? wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT )
                                     : wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    if( m_bitmap.IsOk() )
    {
        wxMemoryDC mdc( m_bitmap );

        r1.x = ( width - m_bitmap.GetWidth() ) / 2;

        if( r1.x < 0 )
            r1.x = 0;

        r1.y += ( size.GetHeight() - m_bitmap.GetHeight() ) / 2;

        dc.Blit( wxPoint( r1.x, r1.y ), m_bitmap.GetSize(), &mdc, wxPoint( 0, 0 ), wxCOPY, true );
    }
    else
    {
        r1.y += ( ( size.GetHeight() - GetCharHeight() ) / 2 ) - 1;
        dc.DrawLabel( m_label, r1, wxALIGN_CENTER_HORIZONTAL );
    }

    // Draw second part of button
    wxRect r2;
    r2.x      = width;
    r2.y      = 0;
    r2.width  = m_arrowButtonWidth;
    r2.height = size.GetHeight();

#ifdef __WXMAC__
    // wxRendereNative doesn't handle dark mode on OSX.
    drawBackground( r2 );
#else
    r2.x -= 2;
    wxRendererNative::Get().DrawPushButton( this, dc, r2, m_stateMenu );
#endif

    wxRendererNative::Get().DrawDropArrow( this, dc, r2, m_stateMenu );
}


bool SPLIT_BUTTON::Enable( bool aEnable )
{
    m_bIsEnable = aEnable;
    wxPanel::Enable( m_bIsEnable );

    if( m_bIsEnable )
    {
        m_stateButton = 0;
        m_stateMenu   = 0;
    }
    else
    {
        m_stateButton = wxCONTROL_DISABLED;
        m_stateMenu   = wxCONTROL_DISABLED;
    }

    Refresh();

    return aEnable;
}
