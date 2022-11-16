/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SIM_MODEL_RAW_SPICE_H
#define SIM_MODEL_RAW_SPICE_H

#include <sim/sim_model_spice.h>
#include <sim/spice_generator.h>


class SPICE_GENERATOR_RAW_SPICE : public SPICE_GENERATOR
{
public:
    using SPICE_GENERATOR::SPICE_GENERATOR;

    std::string ModelLine( const SPICE_ITEM& aItem ) const override;

    std::string ItemName( const SPICE_ITEM& aItem ) const override;
    std::string ItemPins( const SPICE_ITEM& aItem ) const override;
    std::string ItemModelName( const SPICE_ITEM& aItem ) const override;
    std::string ItemParams() const override;

    std::string Preview( const SPICE_ITEM& aItem ) const override;
};


class SIM_MODEL_RAW_SPICE : public SIM_MODEL
{
public:
    friend class SPICE_GENERATOR_RAW_SPICE;

    DEFINE_ENUM_CLASS_WITH_ITERATOR( SPICE_PARAM,
        TYPE,
        MODEL,
        LIB
    )

    static constexpr auto LEGACY_TYPE_FIELD = "Spice_Primitive";
    static constexpr auto LEGACY_PINS_FIELD = "Spice_Node_Sequence";
    static constexpr auto LEGACY_MODEL_FIELD = "Spice_Model";
    static constexpr auto LEGACY_ENABLED_FIELD = "Spice_Netlist_Enabled";
    static constexpr auto LEGACY_LIB_FIELD = "Spice_Lib_File";


    SIM_MODEL_RAW_SPICE();

    //bool ReadSpiceCode( const std::string& aSpiceCode ) override;
    void ReadDataSchFields( unsigned aSymbolPinCount, const std::vector<SCH_FIELD>* aFields ) override;
    void ReadDataLibFields( unsigned aSymbolPinCount, const std::vector<LIB_FIELD>* aFields ) override;

    void WriteDataSchFields( std::vector<SCH_FIELD>& aFields ) const override;
    void WriteDataLibFields( std::vector<LIB_FIELD>& aFields ) const override;

protected:
    void CreatePins( unsigned aSymbolPinCount ) override;

private:
    static std::vector<PARAM::INFO> makeParamInfos();

    template <typename T>
    void readLegacyDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields );

    void parseLegacyPinsField( unsigned aSymbolPinCount, const std::string& aLegacyPinsField );

    bool requiresSpiceModelLine() const override { return false; }


    std::vector<std::unique_ptr<PARAM::INFO>> m_paramInfos;
};

#endif // SIM_MODEL_RAW_SPICE_H
