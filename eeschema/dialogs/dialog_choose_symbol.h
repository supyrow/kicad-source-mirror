/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#ifndef DIALOG_CHOOSE_SYMBOL_H
#define DIALOG_CHOOSE_SYMBOL_H

#include "dialog_shim.h"
#include <symbol_tree_model_adapter.h>
#include <footprint_info.h>

class wxCheckBox;
class wxStaticBitmap;
class wxTextCtrl;
class wxStdDialogButtonSizer;
class wxDataViewCtrl;
class wxHtmlWindow;
class wxHtmlLinkEvent;
class wxPanel;
class wxChoice;
class wxButton;
class wxTimer;
class wxSplitterWindow;

class LIB_TREE;
class SYMBOL_PREVIEW_WIDGET;
class FOOTPRINT_PREVIEW_WIDGET;
class FOOTPRINT_SELECT_WIDGET;
class LIB_ALIAS;
class SCH_BASE_FRAME;
class SCH_DRAW_PANEL;


/**
 * Dialog class to select a symbol from the libraries. This is the master View class in a
 * Model-View-Adapter (mediated MVC) architecture. The other pieces are in:
 *
 * - Adapter: SYM_TREE_MODEL_ADAPTER in common/cmp_tree_model_adapter.h
 * - Model: SYM_TREE_NODE and descendants in common/cmp_tree_model.h
 *
 * Because everything is tied together in the adapter class, see that file
 * for thorough documentation. A simple example usage follows:
 *
 *     // Create the adapter class
 *     auto adapter( SYMBOL_TREE_MODEL_ADAPTER::Create( Prj().SchSymbolLibTable() ) );
 *
 *     // Perform any configuration of adapter properties here
 *     adapter->SetPreselectNode( "LIB_NICKNAME", "SYMBO_NAME", 2 );
 *
 *     // Initialize model from #SYMBOL_LIB_TABLE
 *     libNicknames = libs->GetLogicalLibs();
 *
 *     for( auto nickname : libNicknames )
 *         adapter->AddLibrary( nickname );
 *
 *     // Create and display dialog
 *     DIALOG_CHOOSE_SYMBOL dlg( this, title, adapter, 1 );
 *     bool selected = ( dlg.ShowModal() != wxID_CANCEL );
 *
 *     // Receive part
 *     if( selected )
 *     {
 *         int unit;
 *         #LIB_ID id = dlg.GetSelectedAlias( &unit );
 *         do_something( id, unit );
 *     }
 *
 */
class DIALOG_CHOOSE_SYMBOL : public DIALOG_SHIM
{
public:
    /**
     * Create dialog to choose symbol.
     *
     * @param aParent   a SCH_BASE_FRAME parent window.
     * @param aTitle    Dialog title.
     * @param aAdapter  SYMBOL_TREE_MODEL_ADAPTER::PTR. See SYM_TREE_MODEL_ADAPTER
     *                  for documentation.
     * @param aDeMorganConvert  preferred deMorgan conversion.
     *                          (TODO: should happen in dialog)
     * @param aAllowFieldEdits  if false, all functions that allow the user to edit fields
     *                          (currently just footprint selection) will not be available.
     * @param aShowFootprints   if false, all footprint preview and selection features are
     *                          disabled. This forces aAllowFieldEdits false too.
     * @param aAllowBrowser     show a Select with Browser button.
     */
    DIALOG_CHOOSE_SYMBOL( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                          wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>& aAdapter,
                          int aDeMorganConvert, bool aAllowFieldEdits, bool aShowFootprints,
                          bool aAllowBrowser );

    ~DIALOG_CHOOSE_SYMBOL();

    /**
     * To be called after this dialog returns from ShowModal().
     *
     * For multi-unit symbols, if the user selects the symbol itself rather than picking
     * an individual unit, 0 will be returned in aUnit.
     * Beware that this is an invalid unit number - this should be replaced with whatever
     * default is desired (usually 1).
     *
     * @param aUnit if not NULL, the selected unit is filled in here.
     * @return the #LIB_ID of the symbol that has been selected.
     */
    LIB_ID GetSelectedLibId( int* aUnit = nullptr ) const;

    /**
     * To be called after this dialog returns from ShowModal()
     *
     * In the case of multi-unit symbols, this preferences asks to iterate through
     * all units of the symbol, one per click.
     *
     * @return The value of the dialog preference checkbox.
     */
    bool GetUseAllUnits() const;

    /**
     * To be called after this dialog returns from ShowModal()
     *
     * Keeps a new copy of the symbol on the mouse cursor, allowing the user to rapidly
     * place multiple copies of the same symbol on their schematic.
     *
     * @return The value of the keep symbol preference checkbox.
     */
    bool GetKeepSymbol() const;

    /**
     * Get a list of fields edited by the user.
     *
     * @return vector of pairs; each.first = field ID, each.second = new value.
     */
    std::vector<std::pair<int, wxString>> GetFields() const
    {
        return m_field_edits;
    }

    /**
     * @return true if the user requested the symbol browser.
     */
    bool IsExternalBrowserSelected() const
    {
        return m_external_browser_requested;
    }

protected:
    static constexpr int DblClickDelay = 100; // milliseconds

    wxPanel* ConstructRightPanel( wxWindow* aParent );

    void OnInitDialog( wxInitDialogEvent& aEvent );
    void OnCharHook( wxKeyEvent& aEvt ) override;
    void OnCloseTimer( wxTimerEvent& aEvent );
    void OnUseBrowser( wxCommandEvent& aEvent );

    void OnFootprintSelected( wxCommandEvent& aEvent );
    void OnComponentPreselected( wxCommandEvent& aEvent );

    /**
     * Handle the selection of an item. This is called when either the search box or the tree
     * receive an Enter, or the tree receives a double click.
     * If the item selected is a category, it is expanded or collapsed; if it is a symbol, the
     * symbol is picked.
     */
    void OnComponentSelected( wxCommandEvent& aEvent );

    /**
     * Look up the footprint for a given symbol specified in the #LIB_ID and display it.
     */
    void ShowFootprintFor( LIB_ID const& aLibId );

    /**
     * Display the given footprint by name.
     */
    void ShowFootprint( wxString const& aFootprint );

    /**
     * Populate the footprint selector for a given alias.
     *
     * @param aLibId the #LIB_ID of the selection or invalid to clear.
     */
    void PopulateFootprintSelector( LIB_ID const& aLibId );

public:
    static std::mutex g_Mutex;

protected:
    wxTimer*                  m_dbl_click_timer;
    SYMBOL_PREVIEW_WIDGET*    m_symbol_preview;
    wxButton*                 m_browser_button;
    wxSplitterWindow*         m_hsplitter;
    wxSplitterWindow*         m_vsplitter;

    FOOTPRINT_SELECT_WIDGET*  m_fp_sel_ctrl;
    FOOTPRINT_PREVIEW_WIDGET* m_fp_preview;
    wxCheckBox*               m_keepSymbol;
    wxCheckBox*               m_useUnits;
    LIB_TREE*                 m_tree;
    wxHtmlWindow*             m_details;

    SCH_BASE_FRAME*           m_parent;
    int                       m_deMorganConvert;
    bool                      m_allow_field_edits;
    bool                      m_show_footprints;
    bool                      m_external_browser_requested;
    wxString                  m_fp_override;

    std::vector<std::pair<int, wxString>>  m_field_edits;
};

#endif /* DIALOG_CHOOSE_SYMBOL_H */
