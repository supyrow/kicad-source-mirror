/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2021 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Christian Gagneraud <chgans@gna.org>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PNS_DEBUG_DECORATOR_H
#define __PNS_DEBUG_DECORATOR_H

#include <math/vector2d.h>
#include <math/box2.h>
#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>

#include <gal/color4d.h>

namespace PNS {

class DEBUG_DECORATOR
{
public:
    DEBUG_DECORATOR() : m_debugEnabled( false ) {}

    struct SRC_LOCATION_INFO
    {
        SRC_LOCATION_INFO( const std::string& aFileName = "", const std::string& aFuncName = "",
                           int aLine = 0 ) :
            fileName( aFileName ),
            funcName( aFuncName ),
            line( aLine )
        {
        }

        std::string fileName;
        std::string funcName;
        int         line;
    };

    virtual ~DEBUG_DECORATOR() {}

    void SetDebugEnabled( bool aEnabled ) { m_debugEnabled = aEnabled;}
    bool IsDebugEnabled() const { return m_debugEnabled; }

    virtual void SetIteration( int iter ){};
    virtual void Message( const wxString& msg,
                          const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ){};
    virtual void NewStage( const std::string& name, int iter,
                           const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ){};
    virtual void BeginGroup( const std::string& name,
                             const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ){};
    virtual void EndGroup( const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ){};
    virtual void AddPoint( const VECTOR2I& aP, const KIGFX::COLOR4D& aColor, int aSize,
                           const std::string& aName,
                           const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ){};
    virtual void AddLine( const SHAPE_LINE_CHAIN& aLine, const KIGFX::COLOR4D& aColor,
                          int aWidth, const std::string& aName,
                          const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ){};
    virtual void AddSegment( const SEG& aS, const KIGFX::COLOR4D& aColor,
                             const std::string& aName,
                             const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ){};
    virtual void AddBox( const BOX2I& aB, const KIGFX::COLOR4D& aColor,
                         const std::string& aName,
                         const SRC_LOCATION_INFO& aSrcLoc = SRC_LOCATION_INFO() ){};
    virtual void Clear(){};

private:

    bool m_debugEnabled;
};

/* WARNING! The marco below is a remarkably ugly hack, intended to log the
   call location of the debug calls without having to create the SRC_LOCATION_INFOs every time
   DEBUG_DECORATOR::Something() is called.

   Also checks if debug is enabled at all prior to calling decorator methods, thus saving some
   time wasted otherwise for string formatting and copying the geometry. */

#define PNS_DBG( dbg, method, ... )                                                                \
    if( dbg && dbg->IsDebugEnabled() )                                                             \
        dbg->method( __VA_ARGS__, PNS::DEBUG_DECORATOR::SRC_LOCATION_INFO( __FILE__, __FUNCTION__, \
                                                                           __LINE__ ) );

#define PNS_DBGN( dbg, method )                                                                    \
    if( dbg && dbg->IsDebugEnabled() )                                                             \
        dbg->method( PNS::DEBUG_DECORATOR::SRC_LOCATION_INFO( __FILE__, __FUNCTION__, __LINE__ ) );

} // namespace PNS

#endif
