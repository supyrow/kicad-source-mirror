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

#ifndef SIM_SERDE_H
#define SIM_SERDE_H

#include <sim/sim_model.h>


namespace SIM_SERDE_GRAMMAR
{
    using namespace SIM_VALUE_GRAMMAR;

    struct sep : plus<space> {};


    struct legacyPinNumber : digits {};
    struct legacyPinSequence : list<legacyPinNumber, sep> {};

    struct legacyPinSequenceGrammar : must<legacyPinSequence,
                                           tao::pegtl::eof> {};


    struct pinNumber : plus<not_at<sep>, any> {};
    struct pinSequence : list<pinNumber, sep> {};
    struct pinSequenceGrammar : must<opt<sep>,
                                     opt<pinSequence>,
                                     opt<sep>,
                                     tao::pegtl::eof> {};

    struct param : plus<alnum> {};

    struct unquotedString : plus<not_at<sep>, any> {};
    struct quotedStringContent : star<not_at<one<'"'>>, any> {}; // TODO: Allow escaping '"'.
    struct quotedString : seq<one<'"'>,
                              quotedStringContent,
                              one<'"'>> {};

    struct fieldParamValuePair : if_must<param,
                                         opt<sep>,
                                         one<'='>,
                                         opt<sep>,
                                         sor<quotedString,
                                             unquotedString>> {};
    struct fieldParamValuePairs : list<fieldParamValuePair, sep> {};
    struct fieldParamValuePairsGrammar : must<opt<sep>,
                                              opt<fieldParamValuePairs>,
                                              opt<sep>,
                                              tao::pegtl::eof> {};

    struct fieldInferValueType : plus<upper> {};
    struct fieldInferValuePrimaryValue : seq<// HACK: Because `number` matches empty string,
                                             // ensure it is not empty.
                                             at<sor<tao::pegtl::digit,
                                                    seq<one<'.'>>,
                                                        tao::pegtl::digit>>,
                                             // END HACK.
                                             number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>,
                                             // Hackish: match anything until param-value pairs.
                                             // Because the user may want to write something like
                                             // "10k 30% 30mW w=0.4", but we care only about the
                                             // "10k" and "w=0.4".
                                             star<not_at<sep,
                                                         try_catch<fieldParamValuePairs>>,
                                                  any>> {};
    struct fieldInferValue : sor<seq<fieldInferValueType,
                                     opt<sep,
                                         fieldParamValuePairs>>,
                                 seq<opt<fieldInferValuePrimaryValue>,
                                     opt<sep>,
                                     opt<fieldParamValuePairs>>> {};
    struct fieldInferValueGrammar : must<opt<sep>,
                                         fieldInferValue,
                                         opt<sep>,
                                         tao::pegtl::eof> {};


    template <typename> inline constexpr const char* errorMessage = nullptr;
    template <> inline constexpr auto errorMessage<opt<sep>> = "";
    template <> inline constexpr auto errorMessage<opt<pinSequence>> = "";
    template <> inline constexpr auto errorMessage<opt<sor<fieldInferValueType,
                                                           fieldInferValuePrimaryValue>>> = "";
    template <> inline constexpr auto errorMessage<one<'='>> =
        "expected '='";
    template <> inline constexpr auto errorMessage<sor<quotedString,
                                                       unquotedString>> =
        "expected quoted or unquoted string";
    template <> inline constexpr auto errorMessage<fieldParamValuePairs> =
        "expected parameter=value pairs";
    template <> inline constexpr auto errorMessage<opt<fieldParamValuePairs>> = "";
    template <> inline constexpr auto errorMessage<fieldInferValue> =
        "expected parameter=value pairs, together possibly preceded by a type or primary value";
    template <> inline constexpr auto errorMessage<tao::pegtl::eof> =
        "expected end of string";

    struct error
    {
        template <typename Rule> static constexpr bool raise_on_failure = false;
        template <typename Rule> static constexpr auto message = errorMessage<Rule>;
    };

    template <typename Rule> using control = must_if<error>::control<Rule>;
}


/**
 * SerDe = Serializer-Deserializer
 */
class SIM_SERDE
{
public:
    static constexpr auto REFERENCE_FIELD = "Reference";
    static constexpr auto VALUE_FIELD = "Value";

    static constexpr auto DEVICE_TYPE_FIELD = "Sim_Device";
    static constexpr auto TYPE_FIELD = "Sim_Type";
    static constexpr auto PINS_FIELD = "Sim_Pins";
    static constexpr auto PARAMS_FIELD = "Sim_Params";
    static constexpr auto ENABLE_FIELD = "Sim_Enable";

    SIM_SERDE( SIM_MODEL& aModel ) : m_model( aModel ) {}

    std::string GenerateDevice() const;
    std::string GenerateType() const;
    std::string GenerateValue() const;
    std::string GenerateParams() const;
    std::string GeneratePins() const;
    std::string GenerateEnable() const;

    SIM_MODEL::TYPE ParseDeviceAndType( const std::string& aDevice,
                                        const std::string& aType );
    void ParseValue( const std::string& aValue );
    void ParseParams( const std::string& aParams );
    void ParsePins( const std::string& aPins );
    void ParseEnable( const std::string& aEnable );

    static SIM_MODEL::TYPE InferTypeFromRefAndValue( const std::string& aRef,
                                                     const std::string& aValue,
                                                     int aSymbolPinCount );

protected:
    virtual std::string GenerateParamValuePair( const SIM_MODEL::PARAM& aParam ) const;

private:
    SIM_MODEL& m_model;
};

#endif // SIM_SERDE_H
