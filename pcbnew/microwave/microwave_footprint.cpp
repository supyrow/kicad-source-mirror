/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2022 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <confirm.h>
#include <dialogs/dialog_text_entry.h>
#include <microwave/microwave_tool.h>
#include <trigo.h>


FOOTPRINT* MICROWAVE_TOOL::createFootprint( MICROWAVE_FOOTPRINT_SHAPE aFootprintShape )
{
    int        oX;
    PAD*       pad;
    FOOTPRINT* footprint;
    wxString   msg;
    wxString   cmp_name;
    int        pad_count = 2;
    EDA_ANGLE  angle     = ANGLE_0;

    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();

    // Ref and value text size (O = use board default value.
    // will be set to a value depending on the footprint size, if possible
    int text_size = 0;

    // Enter the size of the gap or stub
    int gap_size = editFrame->GetDesignSettings().GetCurrentTrackWidth();

    switch( aFootprintShape )
    {
    case MICROWAVE_FOOTPRINT_SHAPE::GAP:
        msg       = _( "Gap Size:" );
        cmp_name  = wxT( "muwave_gap" );
        text_size = gap_size;
        break;

    case MICROWAVE_FOOTPRINT_SHAPE::STUB:
        msg       = _( "Stub Size:" );
        cmp_name  = wxT( "muwave_stub" );
        text_size = gap_size;
        pad_count = 2;
        break;

    case MICROWAVE_FOOTPRINT_SHAPE::STUB_ARC:
        msg       = _( "Arc Stub Radius Value:" );
        cmp_name  = wxT( "muwave_arcstub" );
        pad_count = 1;
        break;

    default:
        msg = wxT( "???" );
        break;
    }

    wxString             value = editFrame->StringFromValue( gap_size );
    WX_TEXT_ENTRY_DIALOG dlg( editFrame, msg, _( "Create Microwave Footprint" ), value );

    if( dlg.ShowQuasiModal() != wxID_OK )
        return nullptr; // cancelled by user

    value    = dlg.GetValue();
    gap_size = editFrame->ValueFromString( value );

    bool abort = false;

    if( aFootprintShape == MICROWAVE_FOOTPRINT_SHAPE::STUB_ARC )
    {
        msg = wxT( "0.0" );
        WX_TEXT_ENTRY_DIALOG angledlg( editFrame, _( "Angle in degrees:" ),
                                       _( "Create Microwave Footprint" ), msg );

        if( angledlg.ShowQuasiModal() != wxID_OK )
            return nullptr; // cancelled by user

        msg = angledlg.GetValue();

        double fval;

        if( !msg.ToDouble( &fval ) )
        {
            DisplayError( editFrame, _( "Incorrect number, abort" ) );
            abort = true;
        }

        angle = EDA_ANGLE( fval, DEGREES_T );

        if( angle < ANGLE_0 )
            angle = -angle;

        if( angle > ANGLE_180 )
            angle = ANGLE_180;
    }

    if( abort )
        return nullptr;

    footprint = createBaseFootprint( cmp_name, text_size, pad_count );
    auto it = footprint->Pads().begin();
    pad = *it;

    switch( aFootprintShape )
    {
    case MICROWAVE_FOOTPRINT_SHAPE::GAP:     //Gap :
        oX = -( gap_size + pad->GetSize().x ) / 2;
        pad->SetX0( oX );

        pad->SetX( pad->GetPos0().x + pad->GetPosition().x );

        pad = *( it + 1 );

        pad->SetX0( oX + gap_size + pad->GetSize().x );
        pad->SetX( pad->GetPos0().x + pad->GetPosition().x );
        break;

    case MICROWAVE_FOOTPRINT_SHAPE::STUB:     //Stub :
        pad->SetNumber( wxT( "1" ) );
        pad = *( it + 1 );
        pad->SetY0( -( gap_size + pad->GetSize().y ) / 2 );
        pad->SetSize( wxSize( pad->GetSize().x, gap_size ) );
        pad->SetY( pad->GetPos0().y + pad->GetPosition().y );
        break;

    case MICROWAVE_FOOTPRINT_SHAPE::STUB_ARC:     // Arc Stub created by a polygonal approach:
    {
        pad->SetShape( PAD_SHAPE::CUSTOM );
        pad->SetAnchorPadShape( PAD_SHAPE::RECT );

        int numPoints = ( angle.AsDegrees() / 5.0 ) + 3;
        std::vector<VECTOR2I> polyPoints;
        polyPoints.reserve( numPoints );

        polyPoints.emplace_back( wxPoint( 0, 0 ) );

        EDA_ANGLE theta = -angle / 2;

        for( int ii = 1; ii < numPoints - 1; ii++ )
        {
            wxPoint pt( 0, -gap_size );
            RotatePoint( &pt.x, &pt.y, theta );
            polyPoints.push_back( pt );

            theta += EDA_ANGLE( 5.0, DEGREES_T );

            if( theta > angle / 2 )
                theta = angle / 2;
        }

        // Close the polygon:
        polyPoints.push_back( polyPoints[0] );

        pad->AddPrimitivePoly( polyPoints, 0, true ); // add a polygonal basic shape
        break;
    }

    default:
        break;
    }

    // Update the footprint and board
    editFrame->OnModify();

    return footprint;
}


FOOTPRINT* MICROWAVE_TOOL::createBaseFootprint( const wxString& aValue,
                                                int aTextSize, int aPadCount )
{
    PCB_EDIT_FRAME& editFrame = *getEditFrame<PCB_EDIT_FRAME>();

    FOOTPRINT* footprint = editFrame.CreateNewFootprint( aValue, true );

    footprint->SetAttributes( FP_EXCLUDE_FROM_POS_FILES | FP_EXCLUDE_FROM_BOM );

    if( aTextSize > 0 )
    {
        footprint->Reference().SetTextSize( wxSize( aTextSize, aTextSize ) );
        footprint->Reference().SetTextThickness( aTextSize / 5 );
        footprint->Value().SetTextSize( wxSize( aTextSize, aTextSize ) );
        footprint->Value().SetTextThickness( aTextSize / 5 );
    }

    // Create 2 pads used in gaps and stubs.  The gap is between these 2 pads
    // the stub is the pad 2
    int pad_num = 1;

    while( aPadCount-- )
    {
        PAD* pad = new PAD( footprint );

        footprint->Add( pad, ADD_MODE::INSERT );

        int tw = editFrame.GetDesignSettings().GetCurrentTrackWidth();
        pad->SetSize( wxSize( tw, tw ) );

        pad->SetPosition( footprint->GetPosition() );
        pad->SetShape( PAD_SHAPE::RECT );
        pad->SetAttribute( PAD_ATTRIB::SMD );
        pad->SetLayerSet( F_Cu );

        pad->SetNumber( wxString::Format( wxT( "%d" ), pad_num ) );
        pad_num++;
    }

    return footprint;
}
