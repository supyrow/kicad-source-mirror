/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 Jean-Pierre Charras, jp.charras@wanadoo.fr
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

/* Functions relative to the dialog creating the netlist for Pcbnew.  The dialog is a notebook
 * with 4 fixed netlist formats:
 *   Pcbnew
 *   ORCADPCB2
 *   CADSTAR
 *   SPICE
 * and up to CUSTOMPANEL_COUNTMAX user programmable formats.  These external converters are
 * referred to as plugins, but they are really just external binaries.
 */

#include <pgm_base.h>
#include <kiface_base.h>
#include <gestfich.h>
#include <widgets/wx_html_report_panel.h>
#include <sch_edit_frame.h>
#include <general.h>
#include <netlist.h>
#include <dialogs/dialog_netlist_base.h>
#include <wildcards_and_files_ext.h>
#include <invoke_sch_dialog.h>
#include <netlist_exporters/netlist_exporter_spice.h>
#include <eeschema_settings.h>
#include <schematic.h>
#include <paths.h>

#include <eeschema_id.h>
#include <wx/checkbox.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/regex.h>



#define CUSTOMPANEL_COUNTMAX 8  // Max number of netlist plugins


/* panel (notebook page) identifiers */
enum panel_netlist_index {
    PANELPCBNEW = 0,    /* Handle Netlist format Pcbnew */
    PANELORCADPCB2,     /* Handle Netlist format OracdPcb2 */
    PANELCADSTAR,       /* Handle Netlist format CadStar */
    PANELSPICE,         /* Handle Netlist format Spice */
    PANELSPICEMODEL,    /* Handle Netlist format Spice Model (subcircuit) */
    PANELCUSTOMBASE     /* First auxiliary panel (custom netlists).
                         * others use PANELCUSTOMBASE+1, PANELCUSTOMBASE+2.. */
};


/* wxPanels for creating the NoteBook pages for each netlist format: */
class NETLIST_PAGE_DIALOG : public wxPanel
{

public:
    /**
     * Create a setup page for one netlist format.
     *
     * Used in Netlist format dialog box creation.
     *
     * @param parent is the wxNotebook parent.
     * @param title is the title of the notebook page.
     * @param id_NetType is the netlist ID type.
     */
    NETLIST_PAGE_DIALOG( wxNotebook* aParent, const wxString& aTitle,
                         NETLIST_TYPE_ID aIdNetType, bool aCustom );
    ~NETLIST_PAGE_DIALOG() { };

    /**
     * @return the name of the netlist format for this page.
     */
    const wxString GetPageNetFmtName() { return m_pageNetFmtName; }

    NETLIST_TYPE_ID   m_IdNetType;
    // opt to reformat passive component values (e.g. 1M -> 1Meg):
    wxCheckBox*       m_CurSheetAsRoot;
    wxCheckBox*       m_SaveAllVoltages;
    wxCheckBox*       m_SaveAllCurrents;
    wxTextCtrl*       m_CommandStringCtrl;
    wxTextCtrl*       m_TitleStringCtrl;
    wxBoxSizer*       m_LeftBoxSizer;
    wxBoxSizer*       m_RightBoxSizer;
    wxBoxSizer*       m_RightOptionsBoxSizer;
    wxBoxSizer*       m_LowBoxSizer;

    bool IsCustom() const { return m_custom; }

private:
    wxString          m_pageNetFmtName;

    bool              m_custom;
};


/* Dialog frame for creating netlists */
class NETLIST_DIALOG : public NETLIST_DIALOG_BASE
{
public:
    NETLIST_DIALOG( SCH_EDIT_FRAME* parent );
    ~NETLIST_DIALOG() { };

private:
    friend class NETLIST_PAGE_DIALOG;

    void InstallCustomPages();
    NETLIST_PAGE_DIALOG* AddOneCustomPage( const wxString & aTitle,
                                           const wxString & aCommandString,
                                           NETLIST_TYPE_ID aNetTypeId );
    void InstallPageSpice();
    void InstallPageSpiceModel();

    bool TransferDataFromWindow() override;
    void NetlistUpdateOpt();

    void updateGeneratorButtons();

    // Called when changing the notebook page (and therefore the current netlist format)
    void OnNetlistTypeSelection( wxNotebookEvent& event ) override;

    /**
     * Add a new panel for a new netlist plugin.
     */
    void OnAddGenerator( wxCommandEvent& event ) override;

    /**
     * Remove a panel relative to a netlist plugin.
     */
    void OnDelGenerator( wxCommandEvent& event ) override;

    /**
     * Run the external spice simulator command.
     */
    void OnRunExternSpiceCommand( wxCommandEvent& event );

    /**
     * Enable (if the command line is not empty or disable the button to run spice command.
     */
    void OnRunSpiceButtUI( wxUpdateUIEvent& event );

    /**
     * Write the current netlist options setup in the configuration.
     */
    void WriteCurrentNetlistSetup();

    /**
     * Return the filename extension and the wildcard string for this page or a void name
     * if there is no default name.
     *
     * @param aType is the netlist type ( NET_TYPE_PCBNEW ... ).
     * @param aExt [in] is a holder for the netlist file extension.
     * @param aWildCard [in] is a holder for netlist file dialog wildcard.
     * @return true for known netlist type, false for custom formats.
     */
    bool FilenamePrms( NETLIST_TYPE_ID aType,  wxString* aExt, wxString* aWildCard );

    DECLARE_EVENT_TABLE()

public:
    SCH_EDIT_FRAME*      m_Parent;
    NETLIST_PAGE_DIALOG* m_PanelNetType[5 + CUSTOMPANEL_COUNTMAX];
};


class NETLIST_DIALOG_ADD_GENERATOR : public NETLIST_DIALOG_ADD_GENERATOR_BASE
{
public:
    NETLIST_DIALOG_ADD_GENERATOR( NETLIST_DIALOG* parent );

    const wxString GetGeneratorTitle()  { return m_textCtrlName->GetValue(); }
    const wxString GetGeneratorTCommandLine() { return m_textCtrlCommand->GetValue(); }

    bool TransferDataFromWindow() override;

private:
    /*
     * Browse plugin files, and set m_CommandStringCtrl field
     */
    void OnBrowseGenerators( wxCommandEvent& event ) override;

   NETLIST_DIALOG* m_Parent;
};


/* Event id for notebook page buttons: */
enum id_netlist {
    ID_CREATE_NETLIST = ID_END_EESCHEMA_ID_LIST + 1,
    ID_CUR_SHEET_AS_ROOT,
    ID_SAVE_ALL_VOLTAGES,
    ID_SAVE_ALL_CURRENTS,
    ID_RUN_SIMULATOR
};


BEGIN_EVENT_TABLE( NETLIST_DIALOG, NETLIST_DIALOG_BASE )
    EVT_BUTTON( ID_RUN_SIMULATOR, NETLIST_DIALOG::OnRunExternSpiceCommand )
    EVT_UPDATE_UI( ID_RUN_SIMULATOR, NETLIST_DIALOG::OnRunSpiceButtUI )
END_EVENT_TABLE()


NETLIST_PAGE_DIALOG::NETLIST_PAGE_DIALOG( wxNotebook* aParent, const wxString& aTitle,
                                          NETLIST_TYPE_ID aIdNetType, bool aCustom ) :
        wxPanel( aParent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL )
{
    m_IdNetType           = aIdNetType;
    m_pageNetFmtName      = aTitle;
    m_CommandStringCtrl   = nullptr;
    m_CurSheetAsRoot      = nullptr;
    m_TitleStringCtrl     = nullptr;
    m_SaveAllVoltages     = nullptr;
    m_SaveAllCurrents     = nullptr;
    m_custom              = aCustom;

    aParent->AddPage( this, aTitle, false );

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* UpperBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    m_LowBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( UpperBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( m_LowBoxSizer, 0, wxGROW | wxALL, 5 );

    m_LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    m_RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    m_RightOptionsBoxSizer = new wxBoxSizer( wxVERTICAL );
    UpperBoxSizer->Add( m_LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    UpperBoxSizer->Add( m_RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
    UpperBoxSizer->Add( m_RightOptionsBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
}


NETLIST_DIALOG::NETLIST_DIALOG( SCH_EDIT_FRAME* parent ) :
    NETLIST_DIALOG_BASE( parent )
{
    m_Parent = parent;

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();

    for( NETLIST_PAGE_DIALOG*& page : m_PanelNetType)
        page = nullptr;

    // Add notebook pages:
    m_PanelNetType[PANELPCBNEW] =
            new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "KiCad" ), NET_TYPE_PCBNEW, false );

    m_PanelNetType[PANELORCADPCB2] =
            new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "OrcadPCB2" ), NET_TYPE_ORCADPCB2, false );

    m_PanelNetType[PANELCADSTAR] =
            new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "CadStar" ), NET_TYPE_CADSTAR, false );

    InstallPageSpice();
    InstallPageSpiceModel();
    InstallCustomPages();

    SetupStandardButtons( { { wxID_OK,     _( "Export Netlist" ) },
                            { wxID_CANCEL, _( "Close" )          } } );

    for( int ii = 0; (ii < 4 + CUSTOMPANEL_COUNTMAX) && m_PanelNetType[ii]; ++ii )
    {
        if( m_PanelNetType[ii]->GetPageNetFmtName() == settings.m_NetFormatName )
        {
            m_NoteBook->ChangeSelection( ii );
            break;
        }
    }

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();

    updateGeneratorButtons();
}


void NETLIST_DIALOG::OnRunExternSpiceCommand( wxCommandEvent& event )
{
    // Run the external spice simulator command
    NetlistUpdateOpt();

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();
    wxString simulatorCommand = settings.m_SpiceCommandString;

    unsigned netlist_opt = 0;

    // Calculate the netlist filename and options
    wxFileName fn = m_Parent->Schematic().GetFileName();
    fn.SetExt( SpiceFileExtension );

    if( settings.m_SpiceSaveAllVoltages )
        netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES;

    if( settings.m_SpiceSaveAllCurrents )
        netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS;

    // Build the command line
    wxString commandLine = simulatorCommand;
    commandLine.Replace( "%I", fn.GetFullPath(), true );

    if( m_Parent->ReadyToNetlist( _( "Simulator requires a fully annotated schematic." ) ) )
    {
        m_Parent->WriteNetListFile( NET_TYPE_SPICE, fn.GetFullPath(), netlist_opt,
                                    &m_MessagesBox->Reporter() );

        commandLine.Trim( true ).Trim( false );

        if( !commandLine.IsEmpty() )
            wxExecute( commandLine, wxEXEC_ASYNC );
    }
}


void NETLIST_DIALOG::OnRunSpiceButtUI( wxUpdateUIEvent& aEvent )
{
    bool disable = m_PanelNetType[PANELSPICE]->m_CommandStringCtrl->IsEmpty();
    aEvent.Enable( !disable );
}


void NETLIST_DIALOG::InstallPageSpice()
{
    NETLIST_PAGE_DIALOG* page = m_PanelNetType[PANELSPICE] =
            new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "Spice" ), NET_TYPE_SPICE, false );

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();

    page->m_CurSheetAsRoot = new wxCheckBox( page, ID_CUR_SHEET_AS_ROOT,
                                               _( "Use current sheet as root" ) );
    page->m_CurSheetAsRoot->SetToolTip( _( "Export netlist only for the current sheet" ) );
    page->m_CurSheetAsRoot->SetValue( settings.m_SpiceCurSheetAsRoot );
    page->m_LeftBoxSizer->Add( page->m_CurSheetAsRoot, 0, wxGROW | wxBOTTOM | wxRIGHT, 5 );

    page->m_SaveAllVoltages = new wxCheckBox( page, ID_SAVE_ALL_VOLTAGES,
                                              _( "Save all voltages" ) );
    page->m_SaveAllVoltages->SetToolTip( _( "Write a directive to save all voltages (.save all)" ) );
    page->m_SaveAllVoltages->SetValue( settings.m_SpiceSaveAllVoltages );
    page->m_RightBoxSizer->Add( page->m_SaveAllVoltages, 0, wxBOTTOM | wxRIGHT, 5 );

    page->m_SaveAllCurrents = new wxCheckBox( page, ID_SAVE_ALL_CURRENTS,
                                              _( "Save all currents" ) );
    page->m_SaveAllCurrents->SetToolTip( _( "Write a directive to save all currents (.probe alli)" ) );
    page->m_SaveAllCurrents->SetValue( settings.m_SpiceSaveAllCurrents );
    page->m_RightBoxSizer->Add( page->m_SaveAllCurrents, 0, wxBOTTOM | wxRIGHT, 5 );


    wxString simulatorCommand = settings.m_SpiceCommandString;
    wxStaticText* spice_label = new wxStaticText( page, -1, _( "External simulator command:" ) );
    spice_label->SetToolTip( _( "Enter the command line to run spice\n"
                                "Usually <path to spice binary> %I\n"
                                "%I will be replaced by the actual spice netlist name" ) );
    page->m_LowBoxSizer->Add( spice_label, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    page->m_CommandStringCtrl = new wxTextCtrl( page, -1, simulatorCommand,
                                                wxDefaultPosition, wxDefaultSize );

    page->m_CommandStringCtrl->SetInsertionPoint( 1 );
    page->m_LowBoxSizer->Add( page->m_CommandStringCtrl, 0,
                              wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    // Add button to run spice command
    wxButton* button = new wxButton( page, ID_RUN_SIMULATOR,
                                     _( "Create Netlist and Run Simulator Command" ) );
    page->m_LowBoxSizer->Add( button, 0, wxGROW | wxBOTTOM | wxLEFT | wxRIGHT, 5 );
}


void NETLIST_DIALOG::InstallPageSpiceModel()
{
    NETLIST_PAGE_DIALOG* page = m_PanelNetType[PANELSPICEMODEL] =
            new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "Spice Model" ), NET_TYPE_SPICE_MODEL, false );

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();

    page->m_CurSheetAsRoot = new wxCheckBox( page, ID_CUR_SHEET_AS_ROOT,
                                               _( "Use current sheet as root" ) );
    page->m_CurSheetAsRoot->SetToolTip( _( "Export netlist only for the current sheet" ) );
    page->m_CurSheetAsRoot->SetValue( settings.m_SpiceModelCurSheetAsRoot );
    page->m_LeftBoxSizer->Add( page->m_CurSheetAsRoot, 0, wxGROW | wxBOTTOM | wxRIGHT, 5 );
}


void NETLIST_DIALOG::InstallCustomPages()
{
    NETLIST_PAGE_DIALOG* currPage;

    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    if( cfg )
    {
        for( size_t i = 0;
             i < CUSTOMPANEL_COUNTMAX && i < cfg->m_NetlistPanel.plugins.size();
             i++ )
        {
            // pairs of (title, command) are stored
            wxString title = cfg->m_NetlistPanel.plugins[i].name;

            if( i >= cfg->m_NetlistPanel.plugins.size() )
                break; // No more panel to install

            wxString command = cfg->m_NetlistPanel.plugins[i].command;

            currPage = AddOneCustomPage( title, command,
                    static_cast<NETLIST_TYPE_ID>( NET_TYPE_CUSTOM1 + i ) );

            m_PanelNetType[PANELCUSTOMBASE + i] = currPage;
        }
    }
}


NETLIST_PAGE_DIALOG* NETLIST_DIALOG::AddOneCustomPage( const wxString& aTitle,
                                                       const wxString& aCommandString,
                                                       NETLIST_TYPE_ID aNetTypeId )
{
    NETLIST_PAGE_DIALOG* currPage = new NETLIST_PAGE_DIALOG( m_NoteBook, aTitle, aNetTypeId, true );

    currPage->m_LowBoxSizer->Add( new wxStaticText( currPage, -1, _( "Title:" ) ), 0,
                                  wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    currPage->m_TitleStringCtrl = new wxTextCtrl( currPage, -1, aTitle,
                                                  wxDefaultPosition, wxDefaultSize );

    currPage->m_TitleStringCtrl->SetInsertionPoint( 1 );
    currPage->m_LowBoxSizer->Add( currPage->m_TitleStringCtrl, 0,
                                  wxGROW | wxTOP | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    currPage->m_LowBoxSizer->Add( new wxStaticText( currPage, -1, _( "Netlist command:" ) ), 0,
                                                    wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    currPage->m_CommandStringCtrl = new wxTextCtrl( currPage, -1, aCommandString,
                                                    wxDefaultPosition, wxDefaultSize );

    currPage->m_CommandStringCtrl->SetInsertionPoint( 1 );
    currPage->m_LowBoxSizer->Add( currPage->m_CommandStringCtrl, 0,
                                  wxGROW | wxTOP | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    return currPage;
}


void NETLIST_DIALOG::OnNetlistTypeSelection( wxNotebookEvent& event )
{
    updateGeneratorButtons();
}


void NETLIST_DIALOG::NetlistUpdateOpt()
{
    bool saveAllVoltages =  m_PanelNetType[ PANELSPICE ]->m_SaveAllVoltages->IsChecked();
    bool saveAllCurrents =  m_PanelNetType[ PANELSPICE ]->m_SaveAllCurrents->IsChecked();
    wxString spiceCmdString = m_PanelNetType[ PANELSPICE ]->m_CommandStringCtrl->GetValue();
    bool curSheetAsRoot = m_PanelNetType[ PANELSPICE ]->m_CurSheetAsRoot->GetValue();
    bool spiceModelCurSheetAsRoot = m_PanelNetType[ PANELSPICEMODEL ]->m_CurSheetAsRoot->GetValue();

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();

    settings.m_SpiceSaveAllVoltages  = saveAllVoltages;
    settings.m_SpiceSaveAllCurrents  = saveAllCurrents;
    settings.m_SpiceCommandString    = spiceCmdString;
    settings.m_SpiceCurSheetAsRoot = curSheetAsRoot;
    settings.m_SpiceModelCurSheetAsRoot = spiceModelCurSheetAsRoot;
    settings.m_NetFormatName         = m_PanelNetType[m_NoteBook->GetSelection()]->GetPageNetFmtName();
}


bool NETLIST_DIALOG::TransferDataFromWindow()
{
    wxFileName  fn;
    wxString    fileWildcard;
    wxString    fileExt;
    wxString    title = _( "Save Netlist File" );

    NetlistUpdateOpt();

    NETLIST_PAGE_DIALOG* currPage;
    currPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    unsigned netlist_opt = 0;

    // Calculate the netlist filename
    fn = m_Parent->Schematic().GetFileName();
    FilenamePrms( currPage->m_IdNetType, &fileExt, &fileWildcard );

    // Set some parameters
    switch( currPage->m_IdNetType )
    {
    case NET_TYPE_SPICE:
        // Set spice netlist options:
        if( currPage->m_SaveAllVoltages->GetValue() )
            netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES;
        if( currPage->m_SaveAllCurrents->GetValue() )
            netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS;
        if( currPage->m_CurSheetAsRoot->GetValue() )
            netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_CUR_SHEET_AS_ROOT;
        break;

    case NET_TYPE_SPICE_MODEL:
        if( currPage->m_CurSheetAsRoot->GetValue() )
            netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_CUR_SHEET_AS_ROOT;
        break;

    case NET_TYPE_CADSTAR:
        break;

    case NET_TYPE_PCBNEW:
        break;

    case NET_TYPE_ORCADPCB2:
        break;

    default:    // custom, NET_TYPE_CUSTOM1 and greater
    {
        title.Printf( _( "%s Export" ), currPage->m_TitleStringCtrl->GetValue() );
        break;
    }
    }

    fn.SetExt( fileExt );

    if( fn.GetPath().IsEmpty() )
       fn.SetPath( wxPathOnly( Prj().GetProjectFullName() ) );

    wxString fullpath = fn.GetFullPath();
    wxString fullname = fn.GetFullName();
    wxString path     = fn.GetPath();

    // full name does not and should not include the path, per wx docs.
    wxFileDialog dlg( this, title, path, fullname, fileWildcard, wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    fullpath = dlg.GetPath();   // directory + filename

    m_Parent->ClearMsgPanel();

    if( currPage->m_CommandStringCtrl )
        m_Parent->SetNetListerCommand( currPage->m_CommandStringCtrl->GetValue() );
    else
        m_Parent->SetNetListerCommand( wxEmptyString );

    if( m_Parent->ReadyToNetlist( _( "Exporting netlist requires a fully annotated schematic." ) ) )
    {
        m_Parent->WriteNetListFile( currPage->m_IdNetType, fullpath, netlist_opt,
                                    &m_MessagesBox->Reporter() );
    }

    WriteCurrentNetlistSetup();

    return true;
}


bool NETLIST_DIALOG::FilenamePrms( NETLIST_TYPE_ID aType, wxString * aExt, wxString * aWildCard )
{
    wxString fileExt;
    wxString fileWildcard;
    bool     ret = true;

    switch( aType )
    {
    case NET_TYPE_SPICE:
        fileExt = SpiceFileExtension;
        fileWildcard = SpiceNetlistFileWildcard();
        break;

    case NET_TYPE_CADSTAR:
        fileExt = CadstarNetlistFileExtension;
        fileWildcard = CadstarNetlistFileWildcard();
        break;

    case NET_TYPE_ORCADPCB2:
        fileExt = OrCadPcb2NetlistFileExtension;
        fileWildcard = OrCadPcb2NetlistFileWildcard();
        break;

    case NET_TYPE_PCBNEW:
        fileExt = NetlistFileExtension;
        fileWildcard = NetlistFileWildcard();
        break;

    default:    // custom, NET_TYPE_CUSTOM1 and greater
        fileWildcard = AllFilesWildcard();
        ret = false;
    }

    if( aExt )
        *aExt = fileExt;

    if( aWildCard )
        *aWildCard = fileWildcard;

    return ret;
}


void NETLIST_DIALOG::WriteCurrentNetlistSetup()
{
    NetlistUpdateOpt();

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    if( !cfg )
        return;

    cfg->m_NetlistPanel.plugins.clear();

    // Update existing custom pages
    for( int ii = 0; ii < CUSTOMPANEL_COUNTMAX; ii++ )
    {
        NETLIST_PAGE_DIALOG* currPage = m_PanelNetType[ii + PANELCUSTOMBASE];

        if( currPage == nullptr )
            break;

        wxString title = currPage->m_TitleStringCtrl->GetValue();
        wxString command = currPage->m_CommandStringCtrl->GetValue();

        if( title.IsEmpty() || command.IsEmpty() )
            continue;

        cfg->m_NetlistPanel.plugins.emplace_back( title, wxEmptyString );
        cfg->m_NetlistPanel.plugins.back().command = command;
    }
}


void NETLIST_DIALOG::OnDelGenerator( wxCommandEvent& event )
{
    NETLIST_PAGE_DIALOG* currPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    if( !currPage->IsCustom() )
        return;

    currPage->m_CommandStringCtrl->SetValue( wxEmptyString );
    currPage->m_TitleStringCtrl->SetValue( wxEmptyString );

    WriteCurrentNetlistSetup();

    if( IsQuasiModal() )
        EndQuasiModal( NET_PLUGIN_CHANGE );
    else
        EndDialog( NET_PLUGIN_CHANGE );
}


void NETLIST_DIALOG::OnAddGenerator( wxCommandEvent& event )
{
    NETLIST_DIALOG_ADD_GENERATOR dlg( this );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // Creates a new custom plugin page
    wxString title = dlg.GetGeneratorTitle();

    // Verify it does not exists
    int netTypeId = PANELCUSTOMBASE;    // the first not used type id
    NETLIST_PAGE_DIALOG* currPage;

    for( int ii = 0; ii < CUSTOMPANEL_COUNTMAX; ii++ )
    {
        netTypeId = PANELCUSTOMBASE + ii;
        currPage = m_PanelNetType[ii + PANELCUSTOMBASE];

        if( currPage == nullptr )
            break;

        if( currPage->GetPageNetFmtName() == title )
        {
            wxMessageBox( _("This plugin already exists.") );
            return;
        }
    }

    wxString cmd = dlg.GetGeneratorTCommandLine();
    currPage = AddOneCustomPage( title,cmd, (NETLIST_TYPE_ID)netTypeId );
    m_PanelNetType[netTypeId] = currPage;
    WriteCurrentNetlistSetup();

    if( IsQuasiModal() )
        EndQuasiModal( NET_PLUGIN_CHANGE );
    else
        EndDialog( NET_PLUGIN_CHANGE );
}


NETLIST_DIALOG_ADD_GENERATOR::NETLIST_DIALOG_ADD_GENERATOR( NETLIST_DIALOG* parent ) :
    NETLIST_DIALOG_ADD_GENERATOR_BASE( parent )
{
    m_Parent = parent;
    SetupStandardButtons();
    GetSizer()->SetSizeHints( this );
}


bool NETLIST_DIALOG_ADD_GENERATOR::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_textCtrlCommand->GetValue() == wxEmptyString )
    {
        wxMessageBox( _( "You must provide a netlist generator command string" ) );
        return false;
    }

    if( m_textCtrlName->GetValue() == wxEmptyString )
    {
        wxMessageBox( _( "You must provide a netlist generator title" ) );
        return false;
    }

    return true;
}


void NETLIST_DIALOG_ADD_GENERATOR::OnBrowseGenerators( wxCommandEvent& event )
{
    wxString FullFileName, Path;

#ifndef __WXMAC__
    Path = Pgm().GetExecutablePath();
#else
    Path = PATHS::GetOSXKicadDataDir() + wxT( "/plugins" );
#endif

    FullFileName = wxFileSelector( _( "Generator File" ), Path, FullFileName,
                                   wxEmptyString, wxFileSelectorDefaultWildcardStr,
                                   wxFD_OPEN, this );

    if( FullFileName.IsEmpty() )
        return;

    // Creates a default command line, suitable for external tool xslproc or python, based on
    // the plugin extension ("xsl" or "exe" or "py")
    wxString cmdLine;
    wxFileName fn( FullFileName );
    wxString ext = fn.GetExt();

    if( ext == wxT( "xsl" ) )
        cmdLine.Printf( wxT( "xsltproc -o \"%%O\" \"%s\" \"%%I\"" ), FullFileName );
    else if( ext == wxT( "exe" ) || ext.IsEmpty() )
        cmdLine.Printf( wxT( "\"%s\" > \"%%O\" < \"%%I\"" ), FullFileName );
    else if( ext == wxT( "py" ) || ext.IsEmpty() )
        cmdLine.Printf( wxT( "python \"%s\" \"%%I\" \"%%O\"" ), FullFileName );
    else
        cmdLine.Printf( wxT( "\"%s\"" ), FullFileName );

    m_textCtrlCommand->SetValue( cmdLine );

    // We need a title for this panel
    // Propose a default value if empty ( i.e. the short filename of the script)
    if( m_textCtrlName->GetValue().IsEmpty() )
        m_textCtrlName->SetValue( fn.GetName() );
}


void NETLIST_DIALOG::updateGeneratorButtons()
{
    NETLIST_PAGE_DIALOG* currPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    if( currPage == nullptr )
        return;

    m_buttonDelGenerator->Enable( currPage->IsCustom() );
}


int InvokeDialogNetList( SCH_EDIT_FRAME* aCaller )
{
    NETLIST_DIALOG dlg( aCaller );

    int ret = dlg.ShowModal();
    aCaller->SaveProjectSettings();

    return ret;
}
