/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <wx/stockitem.h>
#include <wx/richmsgdlg.h>
#include <wx/choicdlg.h>
#include <confirm.h>
#include <dialogs/html_message_box.h>
#include <functional>
#include <unordered_map>

// Set of dialogs that have been chosen not to be shown again
static std::unordered_map<unsigned long, int> doNotShowAgainDlgs;


KIDIALOG::KIDIALOG( wxWindow* aParent, const wxString& aMessage, const wxString& aCaption,
                    long aStyle )
    : wxRichMessageDialog( aParent, aMessage, aCaption, aStyle | wxCENTRE | wxSTAY_ON_TOP ),
      m_hash( 0 ),
      m_cancelMeansCancel( true )
{
}


KIDIALOG::KIDIALOG( wxWindow* aParent, const wxString& aMessage, KD_TYPE aType,
                    const wxString& aCaption )
    : wxRichMessageDialog( aParent, aMessage, getCaption( aType, aCaption ), getStyle( aType ) ),
      m_hash( 0 ),
      m_cancelMeansCancel( true )
{
}


void KIDIALOG::DoNotShowCheckbox( wxString aUniqueId, int line )
{
    ShowCheckBox( _( "Do not show again" ), false );

    m_hash = std::hash<wxString>{}( aUniqueId ) + line;
}


bool KIDIALOG::DoNotShowAgain() const
{
    return doNotShowAgainDlgs.count( m_hash ) > 0;
}


void KIDIALOG::ForceShowAgain()
{
    doNotShowAgainDlgs.erase( m_hash );
}


bool KIDIALOG::Show( bool aShow )
{
    // We should check the do-not-show-again setting only when the dialog is displayed
    if( aShow )
    {
        // Check if this dialog should be shown to the user
        auto it = doNotShowAgainDlgs.find( m_hash );

        if( it != doNotShowAgainDlgs.end() )
            return it->second;
    }

    int ret = wxRichMessageDialog::Show( aShow );

    // Has the user asked not to show the dialog again?
    // Note that we don't save a Cancel value unless the Cancel button is being used for some
    // other function (which is actually more common than it being used for Cancel).
    if( IsCheckBoxChecked() && (!m_cancelMeansCancel || ret != wxID_CANCEL ) )
        doNotShowAgainDlgs[m_hash] = ret;

    return ret;
}


int KIDIALOG::ShowModal()
{
    // Check if this dialog should be shown to the user
    auto it = doNotShowAgainDlgs.find( m_hash );

    if( it != doNotShowAgainDlgs.end() )
        return it->second;

    int ret = wxRichMessageDialog::ShowModal();

    // Has the user asked not to show the dialog again?
    // Note that we don't save a Cancel value unless the Cancel button is being used for some
    // other function (which is actually more common than it being used for Cancel).
    if( IsCheckBoxChecked() && (!m_cancelMeansCancel || ret != wxID_CANCEL ) )
        doNotShowAgainDlgs[m_hash] = ret;

    return ret;
}


wxString KIDIALOG::getCaption( KD_TYPE aType, const wxString& aCaption )
{
    if( !aCaption.IsEmpty() )
        return aCaption;

    switch( aType )
    {
    case KD_NONE:       /* fall through */
    case KD_INFO:       return _( "Message" );
    case KD_QUESTION:   return _( "Question" );
    case KD_WARNING:    return _( "Warning" );
    case KD_ERROR:      return _( "Error" );
    }

    return wxEmptyString;
}


long KIDIALOG::getStyle( KD_TYPE aType )
{
    long style = wxOK | wxCENTRE | wxSTAY_ON_TOP;

    switch( aType )
    {
    case KD_NONE:       break;
    case KD_INFO:       style |= wxICON_INFORMATION; break;
    case KD_QUESTION:   style |= wxICON_QUESTION; break;
    case KD_WARNING:    style |= wxICON_WARNING; break;
    case KD_ERROR:      style |= wxICON_ERROR; break;
    }

    return style;
}


bool OverrideLock( wxWindow* aParent, const wxString& aMessage )
{
#ifdef __APPLE__
    // wxMessageDialog gets the button spacing wrong on Mac so we have to use wxRichMessageDialog.
    // Note that its warning icon is more like wxMessageDialog's error icon, so we use it instead
    // of wxICON_ERROR.
    wxRichMessageDialog dlg( aParent, aMessage, _( "File Open Error" ),
                             wxYES_NO | wxICON_WARNING | wxCENTER );
    dlg.SetExtendedMessage( _( "Interleaved saves may produce very unexpected results." )
                                + wxS( "\n" ) );
    dlg.SetYesNoLabels( _( "OK" ), _( "Open Anyway" ) );
#else
    wxMessageDialog dlg( aParent, aMessage, _( "File Open Error" ),
                         wxYES_NO | wxICON_ERROR | wxCENTER );
    dlg.SetExtendedMessage( _( "Interleaved saves may produce very unexpected results." ) );
    dlg.SetYesNoLabels( _( "OK" ), _( "Open Anyway" ) );
#endif

    return dlg.ShowModal() == wxID_NO;
}


int UnsavedChangesDialog( wxWindow* parent, const wxString& aMessage, bool* aApplyToAll )
{
    static bool s_apply_to_all = false;

    wxRichMessageDialog dlg( parent, aMessage, _( "Save Changes?" ),
                             wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_WARNING | wxCENTER );
    dlg.SetExtendedMessage( _( "If you don't save, all your changes will be permanently lost." )
                                + wxS( "\n" ) );
    dlg.SetYesNoLabels( _( "Save" ), _( "Discard Changes" ) );

    if( aApplyToAll )
        dlg.ShowCheckBox( _( "Apply to all" ), s_apply_to_all );

    int ret = dlg.ShowModal();

    if( aApplyToAll )
    {
        *aApplyToAll = dlg.IsCheckBoxChecked();
        s_apply_to_all = dlg.IsCheckBoxChecked();
    }

    // Returns wxID_YES, wxID_NO, or wxID_CANCEL
    return ret;
}


int UnsavedChangesDialog( wxWindow* parent, const wxString& aMessage )
{
#ifdef __APPLE__
    // wxMessageDialog gets the button order (and spacing) wrong on Mac so we have to use
    // wxRichMessageDialog.
    return UnsavedChangesDialog( parent, aMessage, nullptr );
#else
    #ifdef _WIN32
    // wxMessageDialog on windows invokes TaskDialogIndirect which is a native function for a dialog
    // As a result it skips wxWidgets for modal management...and we don't parent frames properly
    // among other things for Windows to do the right thing by default
    // Disable all the windows manually to avoid being able to hit this dialog from the tool frame and kicad frame at the same time
    wxWindowDisabler disable( true );
    #endif

    wxMessageDialog dlg( parent, aMessage, _( "Save Changes?" ),
                         wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_WARNING | wxCENTER );
    dlg.SetExtendedMessage( _( "If you don't save, all your changes will be permanently lost." ) );
    dlg.SetYesNoLabels( _( "Save" ), _( "Discard Changes" ) );

    // Returns wxID_YES, wxID_NO, or wxID_CANCEL
    return dlg.ShowModal();
#endif
}


bool ConfirmRevertDialog( wxWindow* parent, const wxString& aMessage )
{
    wxMessageDialog dlg( parent, aMessage, wxEmptyString,
                         wxOK | wxCANCEL | wxOK_DEFAULT | wxICON_WARNING | wxCENTER );
    dlg.SetExtendedMessage( _( "Your current changes will be permanently lost." ) );
    dlg.SetOKCancelLabels( _( "Revert" ), _( "Cancel" ) );

    return dlg.ShowModal() == wxID_OK;
}


bool HandleUnsavedChanges( wxWindow* aParent, const wxString& aMessage,
                           const std::function<bool()>& aSaveFunction )
{
    switch( UnsavedChangesDialog( aParent, aMessage ) )
    {
    case wxID_YES:    return aSaveFunction();
    case wxID_NO:     return true;
    default:
    case wxID_CANCEL: return false;
    }
}


int OKOrCancelDialog( wxWindow* aParent, const wxString& aWarning, const wxString& aMessage,
                      const wxString& aDetailedMessage, const wxString& aOKLabel,
                      const wxString& aCancelLabel, bool* aApplyToAll )
{
    wxRichMessageDialog dlg( aParent, aMessage, aWarning,
                             wxOK | wxCANCEL | wxOK_DEFAULT | wxICON_WARNING | wxCENTER );

    dlg.SetOKCancelLabels( ( aOKLabel.IsEmpty() ) ? _( "OK" ) : aOKLabel,
                           ( aCancelLabel.IsEmpty() ) ?  _( "Cancel" ) : aCancelLabel );

    if( !aDetailedMessage.IsEmpty() )
        dlg.SetExtendedMessage( aDetailedMessage );

    if( aApplyToAll )
        dlg.ShowCheckBox( _( "Apply to all" ), true );

    int ret = dlg.ShowModal();

    if( aApplyToAll )
        *aApplyToAll = dlg.IsCheckBoxChecked();

    // Returns wxID_OK or wxID_CANCEL
    return ret;
}


// DisplayError should be deprecated, use DisplayErrorMessage instead
void DisplayError( wxWindow* aParent, const wxString& aText, int aDisplayTime )
{
    wxMessageDialog* dlg;
    int              icon = aDisplayTime > 0 ? wxICON_INFORMATION : wxICON_ERROR;

    dlg = new wxMessageDialog( aParent, aText, _( "Warning" ),
                               wxOK | wxCENTRE | wxRESIZE_BORDER | icon | wxSTAY_ON_TOP );

    dlg->ShowModal();
    dlg->Destroy();
}


void DisplayErrorMessage( wxWindow* aParent, const wxString& aText, const wxString& aExtraInfo )
{
    wxMessageDialog* dlg;

    dlg = new wxMessageDialog( aParent, aText, _( "Error" ),
                               wxOK | wxCENTRE | wxRESIZE_BORDER | wxICON_ERROR | wxSTAY_ON_TOP );

    if( !aExtraInfo.IsEmpty() )
        dlg->SetExtendedMessage( aExtraInfo );

    dlg->ShowModal();
    dlg->Destroy();
}


void DisplayInfoMessage( wxWindow* aParent, const wxString& aMessage, const wxString& aExtraInfo )
{
    wxMessageDialog* dlg;
    int              icon = wxICON_INFORMATION;

    dlg = new wxMessageDialog( aParent, aMessage, _( "Information" ),
                               wxOK | wxCENTRE | wxRESIZE_BORDER | icon | wxSTAY_ON_TOP );

    if( !aExtraInfo.IsEmpty() )
        dlg->SetExtendedMessage( aExtraInfo );

    dlg->ShowModal();
    dlg->Destroy();
}


bool IsOK( wxWindow* aParent, const wxString& aMessage )
{
    wxMessageDialog dlg( aParent, aMessage, _( "Confirmation" ),
                         wxYES_NO | wxCENTRE | wxICON_QUESTION | wxSTAY_ON_TOP );
    dlg.SetEscapeId( wxID_NO );

    return dlg.ShowModal() == wxID_YES;
}


int SelectSingleOption( wxWindow* aParent, const wxString& aTitle,
                        const wxString& aMessage, const wxArrayString& aOptions )
{
    wxSingleChoiceDialog dlg( aParent, aMessage, aTitle, aOptions );

    if( dlg.ShowModal() != wxID_OK )
        return -1;

    return dlg.GetSelection();
}

