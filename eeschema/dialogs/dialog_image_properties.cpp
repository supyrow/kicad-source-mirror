/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_image_properties.h>
#include <dialogs/panel_image_editor.h>

#include <sch_edit_frame.h>
#include <sch_bitmap.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>


DIALOG_IMAGE_PROPERTIES::DIALOG_IMAGE_PROPERTIES( SCH_EDIT_FRAME* aParent, SCH_BITMAP* aBitmap ) :
        DIALOG_IMAGE_PROPERTIES_BASE( aParent ), m_frame( aParent ), m_bitmap( aBitmap ),
        m_posX( aParent, m_XPosLabel, m_ModPositionX, m_XPosUnit ),
        m_posY( aParent, m_YPosLabel, m_ModPositionY, m_YPosUnit )
{
    // Create the image editor page
    m_imageEditor = new PANEL_IMAGE_EDITOR( m_Notebook, aBitmap->GetImage() );
    m_Notebook->AddPage( m_imageEditor, _( "Image" ), false );

    m_posX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_posY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    SetupStandardButtons();

    finishDialogSettings();
}


bool DIALOG_IMAGE_PROPERTIES::TransferDataToWindow()
{
    m_posX.SetValue( m_bitmap->GetPosition().x );
    m_posY.SetValue( m_bitmap->GetPosition().y );

    return true;
}


bool DIALOG_IMAGE_PROPERTIES::TransferDataFromWindow()
{
    if( m_imageEditor->TransferDataFromWindow() )
    {
        // Save old image in undo list if not already in edit
        if( m_bitmap->GetEditFlags() == 0 )
            m_frame->SaveCopyInUndoList( m_frame->GetScreen(), m_bitmap, UNDO_REDO::CHANGED, false,
                                         false );

        // Update our bitmap from the editor
        m_imageEditor->TransferToImage( m_bitmap->GetImage() );

        m_bitmap->SetPosition( VECTOR2I( m_posX.GetValue(), m_posY.GetValue() ) );

        return true;
    }

    return false;
}
