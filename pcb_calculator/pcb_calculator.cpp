/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2015 jean-pierre.charras
 * Copyright (C) 2015-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pgm_base.h>
#include <kiface_i.h>
#include <settings/settings_manager.h>

#include "pcb_calculator_frame.h"
#include "pcb_calculator_settings.h"

// Pcb_calculator data file extension:
const wxString PcbCalcDataFileExt( wxT( "pcbcalc" ) );


namespace PCBCALC {

static struct IFACE : public KIFACE_I
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
        KIFACE_I( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) override;

    void OnKifaceEnd() override;

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway,
                            int aCtlBits = 0 ) override
    {
        return new PCB_CALCULATOR_FRAME( aKiway, aParent );
    }

    /**
     * Return a pointer to the requested object.
     *
     * The safest way to use this is to retrieve a pointer to a static instance of an interface,
     * similar to how the KIFACE interface is exported.  But if you know what you are doing
     * use it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     * @return the requested object and must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        return nullptr;
    }

} kiface( "pcb_calculator", KIWAY::FACE_PCB_CALCULATOR );


} // namespace


using namespace PCBCALC;

static PGM_BASE* process;


KIFACE_I& Kiface() { return kiface; }


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
MY_API( KIFACE* ) KIFACE_GETTER(  int* aKIFACEversion, int aKiwayVersion, PGM_BASE* aProgram )
{
    process = (PGM_BASE*) aProgram;
    return &kiface;
}


PGM_BASE& Pgm()
{
    wxASSERT( process );    // KIFACE_GETTER has already been called.
    return *process;
}


// Similar to PGM_BASE& Pgm(), but return nullptr when a *.ki_face is run from a python script.
PGM_BASE* PgmOrNull()
{
    return process;
}

bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits )
{
    InitSettings( new PCB_CALCULATOR_SETTINGS );
    aProgram->GetSettingsManager().RegisterSettings( KifaceSettings() );
    start_common( aCtlBits );

    return true;
}


void IFACE::OnKifaceEnd()
{
    end_common();
}
