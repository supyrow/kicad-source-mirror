/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_preview_3d_model.h"
#include <3d_canvas/eda_3d_canvas.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/eda_3d_actions.h>
#include <tools/eda_3d_controller.h>
#include <base_units.h>
#include <bitmaps.h>
#include <board.h>
#include <common_ogl/ogl_attr_list.h>
#include <gal/dpi_scaling.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <widgets/infobar.h>
#include <eda_3d_viewer_settings.h>

PANEL_PREVIEW_3D_MODEL::PANEL_PREVIEW_3D_MODEL( wxWindow* aParent, PCB_BASE_FRAME* aFrame,
                                                FOOTPRINT* aFootprint,
                                                std::vector<FP_3DMODEL>* aParentModelList ) :
        PANEL_PREVIEW_3D_MODEL_BASE( aParent, wxID_ANY ),
        m_previewPane( nullptr ),
        m_infobar( nullptr ),
        m_boardAdapter(),
        m_currentCamera( m_trackBallCamera ),
        m_trackBallCamera( RANGE_SCALE_3D, 0.85f )
{
    m_userUnits = aFrame->GetUserUnits();

    m_dummyBoard = new BOARD();

    // This board will only be used to hold a footprint for viewing
    m_dummyBoard->SetBoardUse( BOARD_USE::FPHOLDER );

    m_selected = -1;

    // Set the bitmap of 3D view buttons:
    m_bpvTop->SetBitmap( KiBitmap( BITMAPS::axis3d_top ) );
    m_bpvFront->SetBitmap( KiBitmap( BITMAPS::axis3d_front ) );
    m_bpvBack->SetBitmap( KiBitmap( BITMAPS::axis3d_back ) );
    m_bpvLeft->SetBitmap( KiBitmap( BITMAPS::axis3d_left ) );
    m_bpvRight->SetBitmap( KiBitmap( BITMAPS::axis3d_right ) );
    m_bpvBottom->SetBitmap( KiBitmap( BITMAPS::axis3d_bottom ) );
    m_bpvISO->SetBitmap( KiBitmap( BITMAPS::ortho ) );
    m_bpUpdate->SetBitmap( KiBitmap( BITMAPS::reload ) );

    // Set the min and max values of spin buttons (mandatory on Linux)
    // They are not used, so they are set to min and max 32 bits int values
    // (the min and max values supported by a wxSpinButton)
    // It avoids blocking the up or down arrows when reaching this limit after
    // a few clicks.
    wxSpinButton* spinButtonList[] =
    {
        m_spinXscale, m_spinYscale, m_spinZscale,
        m_spinXrot, m_spinYrot, m_spinZrot,
        m_spinXoffset,m_spinYoffset, m_spinZoffset
    };

    for( wxSpinButton* button : spinButtonList )
        button->SetRange(INT_MIN, INT_MAX );

    m_parentModelList = aParentModelList;

    m_dummyFootprint = new FOOTPRINT( *aFootprint );
    m_dummyBoard->Add( m_dummyFootprint );

    // Create the 3D canvas
    m_previewPane = new EDA_3D_CANVAS( this,
                                       OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE::AA_8X ),
                                       m_boardAdapter, m_currentCamera,
                                       aFrame->Prj().Get3DCacheManager() );

    m_boardAdapter.SetBoard( m_dummyBoard );
    loadSettings();

    m_boardAdapter.SetFlag( FL_USE_SELECTION, false );
    m_boardAdapter.SetFlag( FL_HIGHLIGHT_ROLLOVER_ITEM, false );

    // Create the manager
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( m_dummyBoard, nullptr, nullptr, nullptr, this );

    m_actions = new EDA_3D_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );
    m_previewPane->SetEventDispatcher( m_toolDispatcher );

    // Register tools
    m_toolManager->RegisterTool( new EDA_3D_CONTROLLER );
    m_toolManager->InitTools();

    // Run the viewer control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "3DViewer.Control" );

    m_infobar = new WX_INFOBAR( this );
    m_previewPane->SetInfoBar( m_infobar );

    m_SizerPanelView->Add( m_infobar, 0, wxEXPAND, 0 );
    m_SizerPanelView->Add( m_previewPane, 1, wxEXPAND, 5 );

    for( wxEventType eventType : { wxEVT_MENU_OPEN, wxEVT_MENU_CLOSE, wxEVT_MENU_HIGHLIGHT } )
        Connect( eventType, wxMenuEventHandler( PANEL_PREVIEW_3D_MODEL::OnMenuEvent ), nullptr,
                 this );

#ifdef __WXOSX__
    // Call layout once to get the proper button sizes after the bitmaps have been set
    Layout();

    // The rounded-button style used has a small border on the left/right sides.
    // This is automatically fixed in wx for buttons with a bitmap < 20, but not
    // when the bitmap is set to be 26x26.
    wxSize borderFix = wxSize( 4, 4 );

    m_bpvTop->SetMinSize( m_bpvTop->GetSize() + borderFix );
    m_bpvFront->SetMinSize( m_bpvFront->GetSize() + borderFix );
    m_bpvBack->SetMinSize( m_bpvBack->GetSize() + borderFix );
    m_bpvLeft->SetMinSize( m_bpvLeft->GetSize() + borderFix );
    m_bpvRight->SetMinSize( m_bpvRight->GetSize() + borderFix );
    m_bpvBottom->SetMinSize( m_bpvBottom->GetSize() + borderFix );
    m_bpvISO->SetMinSize( m_bpvISO->GetSize() + borderFix );
    m_bpUpdate->SetMinSize( m_bpUpdate->GetSize() + borderFix );
#endif
}


PANEL_PREVIEW_3D_MODEL::~PANEL_PREVIEW_3D_MODEL()
{
    delete m_dummyBoard;
    delete m_previewPane;
}


void PANEL_PREVIEW_3D_MODEL::OnMenuEvent( wxMenuEvent& aEvent )
{
    if( !m_toolDispatcher )
        aEvent.Skip();
    else
        m_toolDispatcher->DispatchWxEvent( aEvent );
}


void PANEL_PREVIEW_3D_MODEL::loadSettings()
{
    wxCHECK_RET( m_previewPane, "Cannot load settings to null canvas" );

    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    const DPI_SCALING dpi{ settings, this };
    m_previewPane->SetScaleFactor( dpi.GetScaleFactor() );

    // TODO(JE) use all control options
    m_boardAdapter.SetFlag( FL_MOUSEWHEEL_PANNING, settings->m_Input.scroll_modifier_zoom != 0  );

    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();

    if( colors )
    {
        auto set =
                [] ( const COLOR4D& aColor, SFVEC4F& aTarget )
                {
                    aTarget.r = aColor.r;
                    aTarget.g = aColor.g;
                    aTarget.b = aColor.b;
                    aTarget.a = aColor.a;
                };

        set( colors->GetColor( LAYER_3D_BACKGROUND_BOTTOM ), m_boardAdapter.m_BgColorBot );
        set( colors->GetColor( LAYER_3D_BACKGROUND_TOP ), m_boardAdapter.m_BgColorTop );
        set( colors->GetColor( LAYER_3D_BOARD ), m_boardAdapter.m_BoardBodyColor );
        set( colors->GetColor( LAYER_3D_COPPER ), m_boardAdapter.m_CopperColor );
        set( colors->GetColor( LAYER_3D_SILKSCREEN_BOTTOM ), m_boardAdapter.m_SilkScreenColorBot );
        set( colors->GetColor( LAYER_3D_SILKSCREEN_TOP ), m_boardAdapter.m_SilkScreenColorTop );
        set( colors->GetColor( LAYER_3D_SOLDERMASK_BOTTOM ), m_boardAdapter.m_SolderMaskColorBot );
        set( colors->GetColor( LAYER_3D_SOLDERMASK_TOP ), m_boardAdapter.m_SolderMaskColorTop );
        set( colors->GetColor( LAYER_3D_SOLDERPASTE ), m_boardAdapter.m_SolderPasteColor );
    }

    EDA_3D_VIEWER_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EDA_3D_VIEWER_SETTINGS>();

    if( cfg )
    {
        m_boardAdapter.SetRenderEngine( RENDER_ENGINE::OPENGL_LEGACY );
        m_boardAdapter.SetFlag( FL_USE_REALISTIC_MODE, cfg->m_Render.realistic );
        m_boardAdapter.SetMaterialMode( static_cast<MATERIAL_MODE>( cfg->m_Render.material_mode ) );

        m_previewPane->SetAnimationEnabled( cfg->m_Camera.animation_enabled );
        m_previewPane->SetMovingSpeedMultiplier( cfg->m_Camera.moving_speed_multiplier );
        m_previewPane->SetProjectionMode( cfg->m_Camera.projection_mode );
    }
}


/**
 * Ensure -MAX_ROTATION <= rotation <= MAX_ROTATION.
 *
 * @param \a aRotation will be normalized between -MAX_ROTATION and MAX_ROTATION.
 */
static double rotationFromString( const wxString& aValue )
{
    double rotation = DoubleValueFromString( EDA_UNITS::DEGREES, aValue ) / 10.0;

    if( rotation > MAX_ROTATION )
    {
        int n = KiROUND( rotation / MAX_ROTATION );
        rotation -= MAX_ROTATION * n;
    }
    else if( rotation < -MAX_ROTATION )
    {
        int n = KiROUND( -rotation / MAX_ROTATION );
        rotation += MAX_ROTATION * n;
    }

    return rotation;
}


wxString PANEL_PREVIEW_3D_MODEL::formatScaleValue( double aValue )
{
    return wxString::Format( "%.4f", aValue );
}


wxString PANEL_PREVIEW_3D_MODEL::formatRotationValue( double aValue )
{
    return wxString::Format( "%.2f %s", aValue, GetAbbreviatedUnitsLabel( EDA_UNITS::DEGREES ) );
}


wxString PANEL_PREVIEW_3D_MODEL::formatOffsetValue( double aValue )
{
    // Convert from internal units (mm) to user units
    if( m_userUnits == EDA_UNITS::INCHES )
        aValue /= 25.4f;

    return wxString::Format( "%.4f %s", aValue, GetAbbreviatedUnitsLabel( m_userUnits ) );
}


void PANEL_PREVIEW_3D_MODEL::SetSelectedModel( int idx )
{
    if( m_parentModelList && idx >= 0 && idx < (int) m_parentModelList->size() )
    {
        m_selected = idx;
        const FP_3DMODEL& modelInfo = m_parentModelList->at( (unsigned) m_selected );

        // Use ChangeValue() instead of SetValue().  It's not the user making the change, so we
        // don't want to generate wxEVT_GRID_CELL_CHANGED events.
        xscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.x ) );
        yscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.y ) );
        zscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.z ) );

        xrot->ChangeValue( formatRotationValue( modelInfo.m_Rotation.x ) );
        yrot->ChangeValue( formatRotationValue( modelInfo.m_Rotation.y ) );
        zrot->ChangeValue( formatRotationValue( modelInfo.m_Rotation.z ) );

        xoff->ChangeValue( formatOffsetValue( modelInfo.m_Offset.x ) );
        yoff->ChangeValue( formatOffsetValue( modelInfo.m_Offset.y ) );
        zoff->ChangeValue( formatOffsetValue( modelInfo.m_Offset.z ) );

        m_opacity->SetValue( modelInfo.m_Opacity * 100.0 );
    }
    else
    {
        m_selected = -1;

        xscale->ChangeValue( wxEmptyString );
        yscale->ChangeValue( wxEmptyString );
        zscale->ChangeValue( wxEmptyString );

        xrot->ChangeValue( wxEmptyString );
        yrot->ChangeValue( wxEmptyString );
        zrot->ChangeValue( wxEmptyString );

        xoff->ChangeValue( wxEmptyString );
        yoff->ChangeValue( wxEmptyString );
        zoff->ChangeValue( wxEmptyString );

        m_opacity->SetValue( 100 );
    }
}


void PANEL_PREVIEW_3D_MODEL::updateOrientation( wxCommandEvent &event )
{
    if( m_parentModelList && m_selected >= 0 && m_selected < (int) m_parentModelList->size() )
    {
        // Write settings back to the parent
        FP_3DMODEL* modelInfo = &m_parentModelList->at( (unsigned) m_selected );

        modelInfo->m_Scale.x = DoubleValueFromString( EDA_UNITS::UNSCALED, xscale->GetValue() );
        modelInfo->m_Scale.y = DoubleValueFromString( EDA_UNITS::UNSCALED, yscale->GetValue() );
        modelInfo->m_Scale.z = DoubleValueFromString( EDA_UNITS::UNSCALED, zscale->GetValue() );

        modelInfo->m_Rotation.x = rotationFromString( xrot->GetValue() );
        modelInfo->m_Rotation.y = rotationFromString( yrot->GetValue() );
        modelInfo->m_Rotation.z = rotationFromString( zrot->GetValue() );

        modelInfo->m_Offset.x = DoubleValueFromString( m_userUnits, xoff->GetValue() ) / IU_PER_MM;
        modelInfo->m_Offset.y = DoubleValueFromString( m_userUnits, yoff->GetValue() ) / IU_PER_MM;
        modelInfo->m_Offset.z = DoubleValueFromString( m_userUnits, zoff->GetValue() ) / IU_PER_MM;

        // Update the dummy footprint for the preview
        UpdateDummyFootprint( false );
    }
}


void PANEL_PREVIEW_3D_MODEL::onOpacitySlider( wxCommandEvent& event )
{
    if( m_parentModelList && m_selected >= 0 && m_selected < (int) m_parentModelList->size() )
    {
        // Write settings back to the parent
        FP_3DMODEL* modelInfo = &m_parentModelList->at( (unsigned) m_selected );

        modelInfo->m_Opacity = m_opacity->GetValue() / 100.0;

        // Update the dummy footprint for the preview
        UpdateDummyFootprint( false );
    }
}


void PANEL_PREVIEW_3D_MODEL::doIncrementScale( wxSpinEvent& event, double aSign )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) event.GetEventObject();

    wxTextCtrl * textCtrl = xscale;

    if( spinCtrl == m_spinYscale )
        textCtrl = yscale;
    else if( spinCtrl == m_spinZscale )
        textCtrl = zscale;

    double curr_value = DoubleValueFromString( EDA_UNITS::UNSCALED, textCtrl->GetValue() );

    curr_value += ( SCALE_INCREMENT * aSign );
    curr_value = std::max( 1/MAX_SCALE, curr_value );
    curr_value = std::min( curr_value, MAX_SCALE );

    textCtrl->SetValue( formatScaleValue( curr_value ) );
}


void PANEL_PREVIEW_3D_MODEL::doIncrementRotation( wxSpinEvent& aEvent, double aSign )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) aEvent.GetEventObject();
    wxTextCtrl* textCtrl = xrot;

    if( spinCtrl == m_spinYrot )
        textCtrl = yrot;
    else if( spinCtrl == m_spinZrot )
        textCtrl = zrot;

    double curr_value = DoubleValueFromString( EDA_UNITS::DEGREES, textCtrl->GetValue() ) / 10.0;

    curr_value += ( ROTATION_INCREMENT * aSign );
    curr_value = std::max( -MAX_ROTATION, curr_value );
    curr_value = std::min( curr_value, MAX_ROTATION );

    textCtrl->SetValue( formatRotationValue( curr_value ) );
}


void PANEL_PREVIEW_3D_MODEL::doIncrementOffset( wxSpinEvent& event, double aSign )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) event.GetEventObject();

    wxTextCtrl * textCtrl = xoff;

    if( spinCtrl == m_spinYoffset )
        textCtrl = yoff;
    else if( spinCtrl == m_spinZoffset )
        textCtrl = zoff;

    double step = OFFSET_INCREMENT_MM;

    if( m_userUnits == EDA_UNITS::INCHES )
        step = OFFSET_INCREMENT_MIL/1000.0;

    double curr_value = DoubleValueFromString( m_userUnits, textCtrl->GetValue() ) / IU_PER_MM;

    curr_value += ( step * aSign );
    curr_value = std::max( -MAX_OFFSET, curr_value );
    curr_value = std::min( curr_value, MAX_OFFSET );

    textCtrl->SetValue( formatOffsetValue( curr_value ) );
}


void PANEL_PREVIEW_3D_MODEL::onMouseWheelScale( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = (wxTextCtrl*) event.GetEventObject();

    double step = SCALE_INCREMENT;

    if( event.ShiftDown( ) )
        step = SCALE_INCREMENT_FINE;

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    double curr_value = DoubleValueFromString( EDA_UNITS::UNSCALED, textCtrl->GetValue() );

    curr_value += step;
    curr_value = std::max( 1/MAX_SCALE, curr_value );
    curr_value = std::min( curr_value, MAX_SCALE );

    textCtrl->SetValue( formatScaleValue( curr_value ) );
}


void PANEL_PREVIEW_3D_MODEL::onMouseWheelRot( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = (wxTextCtrl*) event.GetEventObject();

    double step = ROTATION_INCREMENT_WHEEL;

    if( event.ShiftDown( ) )
        step = ROTATION_INCREMENT_WHEEL_FINE;

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    double curr_value = DoubleValueFromString( EDA_UNITS::DEGREES, textCtrl->GetValue() ) / 10.0;

    curr_value += step;
    curr_value = std::max( -MAX_ROTATION, curr_value );
    curr_value = std::min( curr_value, MAX_ROTATION );

    textCtrl->SetValue( formatRotationValue( curr_value ) );
}


void PANEL_PREVIEW_3D_MODEL::onMouseWheelOffset( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = (wxTextCtrl*) event.GetEventObject();

    double step = OFFSET_INCREMENT_MM;

    if( event.ShiftDown( ) )
        step = OFFSET_INCREMENT_MM_FINE;

    if( m_userUnits == EDA_UNITS::INCHES )
    {
        step = OFFSET_INCREMENT_MIL/1000.0;

        if( event.ShiftDown( ) )
            step = OFFSET_INCREMENT_MIL_FINE/1000.0;
    }

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    double curr_value = DoubleValueFromString( m_userUnits, textCtrl->GetValue() ) / IU_PER_MM;

    curr_value += step;
    curr_value = std::max( -MAX_OFFSET, curr_value );
    curr_value = std::min( curr_value, MAX_OFFSET );

    textCtrl->SetValue( formatOffsetValue( curr_value ) );
}


void PANEL_PREVIEW_3D_MODEL::UpdateDummyFootprint( bool aReloadRequired )
{
    m_dummyFootprint->Models().clear();

    for( FP_3DMODEL& model : *m_parentModelList )
    {
        if( model.m_Show )
            m_dummyFootprint->Models().push_back( model );
    }

    if( aReloadRequired )
        m_previewPane->ReloadRequest();

    m_previewPane->Request_refresh();
}
