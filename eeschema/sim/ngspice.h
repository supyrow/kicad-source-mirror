/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef NGSPICE_H
#define NGSPICE_H

#include "spice_simulator.h"

#include <wx/dynlib.h>
#include <ngspice/sharedspice.h>

// We have an issue here where NGSPICE incorrectly used bool for years
// and defined it to be int when in C-mode.  We cannot adjust the function
// signatures without re-writing sharedspice.h for KiCad.
// Instead, we maintain status-quo for older NGSPICE versions (<=34) and
// use the new signatures for newer versions
#ifndef NGSPICE_PACKAGE_VERSION
    typedef bool NG_BOOL;
#endif

class wxDynamicLibrary;

class NGSPICE : public SPICE_SIMULATOR
{
public:
    NGSPICE();
    virtual ~NGSPICE();

    ///< @copydoc SPICE_SIMULATOR::Init()
    void Init( const SPICE_SIMULATOR_SETTINGS* aSettings = nullptr ) override;

    ///< @copydoc SPICE_SIMULATOR::LoadNetlist()
    bool LoadNetlist( const std::string& aNetlist ) override;

    ///< @copydoc SPICE_SIMULATOR::Run()
    bool Run() override;

    ///< @copydoc SPICE_SIMULATOR::Stop()
    bool Stop() override;

    ///< @copydoc SPICE_SIMULATOR::IsRunning()
    bool IsRunning() override;

    ///< @copydoc SPICE_SIMULATOR::Command()
    bool Command( const std::string& aCmd ) override;

    ///< @copydoc SPICE_SIMULATOR::GetXAxis()
    std::string GetXAxis( SIM_TYPE aType ) const override;

    ///< @copydoc SPICE_SIMULATOR::AllPlots()
    std::vector<std::string> AllPlots() const override;

    ///< @copydoc SPICE_SIMULATOR::GetPlot()
    std::vector<COMPLEX> GetPlot( const std::string& aName, int aMaxLen = -1 ) override;

    ///< @copydoc SPICE_SIMULATOR::GetRealPlot()
    std::vector<double> GetRealPlot( const std::string& aName, int aMaxLen = -1 ) override;

    ///< @copydoc SPICE_SIMULATOR::GetImagPlot()
    std::vector<double> GetImagPlot( const std::string& aName, int aMaxLen = -1 ) override;

    ///< @copydoc SPICE_SIMULATOR::GetMagPlot()
    std::vector<double> GetMagPlot( const std::string& aName, int aMaxLen = -1 ) override;

    ///< @copydoc SPICE_SIMULATOR::GetPhasePlot()
    std::vector<double> GetPhasePlot( const std::string& aName, int aMaxLen = -1 ) override;

    std::vector<std::string> GetSettingCommands() const override;

    ///< @copydoc SPICE_SIMULATOR::GetNetlist()
    virtual const std::string GetNetlist() const override;

private:
    void init();

    // Performs DLL initialization, obtains function pointers
    void init_dll();

    // ngspice library functions
    typedef void ( *ngSpice_Init )( SendChar*, SendStat*, ControlledExit*, SendData*, SendInitData*,
                                    BGThreadRunning*, void* );
    typedef int ( *ngSpice_Circ )( char** circarray );
    typedef int ( *ngSpice_Command )( char* command );
    typedef pvector_info ( *ngGet_Vec_Info )( char* vecname );
    typedef char* ( *ngSpice_CurPlot )( void );
    typedef char** ( *ngSpice_AllPlots )( void );
    typedef char** ( *ngSpice_AllVecs )( char* plotname );
    typedef bool ( *ngSpice_Running )( void );

    ///< Handle to DLL functions
    ngSpice_Init m_ngSpice_Init;
    ngSpice_Circ m_ngSpice_Circ;
    ngSpice_Command m_ngSpice_Command;
    ngGet_Vec_Info m_ngGet_Vec_Info;
    ngSpice_CurPlot  m_ngSpice_CurPlot;
    ngSpice_AllPlots m_ngSpice_AllPlots;
    ngSpice_AllVecs m_ngSpice_AllVecs;
    ngSpice_Running m_ngSpice_Running;

    wxDynamicLibrary m_dll;


    ///< Execute commands from a file
    bool loadSpinit( const std::string& aFileName );

    ///< Check a few different locations for codemodel files and returns one if it exists.
    std::string findCmPath() const;

    ///< Load codemodel files from a directory.
    bool loadCodemodels( const std::string& aPath );

    // Callback functions
    static int cbSendChar( char* what, int aId, void* aUser );
    static int cbSendStat( char* what, int aId, void* aUser );
    static int cbBGThreadRunning( NG_BOOL aFinished, int aId, void* aUser );
    static int cbControlledExit( int aStatus, NG_BOOL aImmediate, NG_BOOL aExitOnQuit, int aId,
                                 void* aUser );

    // Assure ngspice is in a valid state and reinitializes it if need be.
    void validate();

    ///< Error flag indicating that ngspice needs to be reloaded.
    bool m_error;

    ///< Ngspice should be initialized only once.
    static bool m_initialized;

    ///< Current netlist
    std::string m_netlist;
};

#endif /* NGSPICE_H */
