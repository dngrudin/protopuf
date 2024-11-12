//   Copyright 2020-2021 PragmaTwice
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#ifndef PROTOPUF_CODER_H
#define PROTOPUF_CODER_H

#include <optional>
#include <utility>
#include "byte.h"

namespace pp {

    /// @brief A type which `encoder`'s `encode` returns.
    /// @param is_safe the encoding mode (safe or without buffer overflow control)
    template<bool is_safe>
    using encode_result = std::conditional_t<is_safe, std::optional<bytes>, bytes>;

    /// @brief Constructs the encoding result depending on the decoding mode.
    ///
    /// Template parameters:
    /// @param is_safe the decoding mode (safe or without buffer overflow control)
    /// @param Args	the types of the arguments list with which the result will be constructed
    ///
    /// Function parameters:
    /// @param args the arguments list with which the result will be constructed
    /// @returns the @ref encode_result which depends on the decoding mode
    template<bool is_safe, typename... Args>
    inline constexpr encode_result<is_safe> make_encode_result(Args&&... args) {
        if constexpr (is_safe) {
            return encode_result<is_safe>{std::in_place, std::forward<Args>(args)...};
        } else {
            return encode_result<is_safe>{std::forward<Args>(args)...};
        }
    }

    /// @brief A pair type which `decoder`'s `decode` returns.
    /// - Left type of pair `T`: the type of decoded object.
    /// - Right type of pair `bytes`: the `bytes` which remains not decoded after finishing `decode`.
    template<typename T>
    using decode_value = std::pair<T, bytes>;

    /// @brief A type which `decoder`'s `decode` returns.
    /// @param T the type of decoded object
    /// @param is_safe the decoding mode (safe or without buffer overflow control)
    template<typename T, bool is_safe>
    using decode_result = std::conditional_t<is_safe, std::optional<decode_value<T>>, decode_value<T>>;

    /// @brief Constructs the decoding result depending on the decoding mode.
    ///
    /// Template parameters:
    /// @param T the type of decoded object
    /// @param is_safe the decoding mode (safe or without buffer overflow control)
    ///
    /// Function parameters:
    /// @param value the decoded object
    /// @param b the `bytes` which remains not decoded after finishing `decode`
    /// @returns the @ref decode_result which depends on the decoding mode
    template<typename T, bool is_safe>
    inline constexpr decode_result<T, is_safe> make_decode_result(T&& value, bytes b) {
        if constexpr (is_safe) {
            return decode_result<T, is_safe>{std::in_place, std::forward<T>(value), b};
        } else {
            return decode_result<T, is_safe>{std::forward<T>(value), b};
        }
    }

    /// @brief Helper function to extract value from encoding/decoding result depending on encoding/decoding mode.
    ///
    /// Template parameters:
    /// @param is_safe the encoding/decoding mode (safe or without buffer overflow control)
    /// @param T the type of result
    ///
    /// Function parameters:
    /// @param result the result of encoding/decoding
    /// @param value the extracted value
    /// @returns true if the value is extracted, otherwise false
    template<bool is_safe, typename T>
    inline constexpr bool get_value_from_result(T&& result, auto& value) {
        if constexpr (is_safe) {
            if (result.has_value()) {
                value = std::forward<decltype(*result)>(*result);
            } else {
                return false;
            }
        } else {
            value = std::forward<T>(result);
        }
        return true;
    }

    /// @brief Describes a type with static member function `encode`, which serializes an object to `bytes` (no ownership).
    ///
    /// Encoding can be performed in two modes: with buffer 's' overflow control (safe mode) and without.
    /// Type alias `value_type` describes type of the object to be encoded.
    /// Static member function `encode`:
    /// @param v the object to be encoded (source object).
    /// @param s the bytes which the object `v` is encoded into (target bytes).
    /// @returns the @ref encode_result which depends on the encoding mode, in safe mode the result is wrapped in std::optional.
    /// The result contains a bytes from `begin(s) + encoding_length(v)` to `end(s)`, where `encoding_length` is the length of
    /// encoded object (bytes form), representing the left bytes which remains not used yet.
    template<typename T>
    concept encoder = requires(typename T::value_type v, bytes s) {
        typename T::value_type;
        { T::template encode<true>(v, s) } -> std::same_as<encode_result<true>>;
        { T::template encode<false>(v, s) } -> std::same_as<encode_result<false>>;
    };

    /// @brief Describes a type with static member function `decode`, which deserializes some `bytes` to an object.
    ///
    /// Decoding can be performed in two modes: with buffer 's' overflow control (safe mode) and without.
    /// Type alias `value_type` describes type of the object to be decoded.
    /// Static member function `decode`:
    /// @param s the bytes which the object is decoded from (source bytes).
    /// @returns the @ref decode_result which depends on the encoding mode, in safe mode the result is wrapped in std::optional.
    /// The result contains a pair including:
    /// - the decoded object `v`;
    /// - the bytes from `begin(s) + decoding_length(v)` to `end(s)`, where `decoding_length` is the length of
    /// decoded object (bytes form), representing the left bytes which remains not used yet.
    template<typename T>
    concept decoder = requires(bytes s) {
        typename T::value_type;
        { T::template decode<true>(s) } -> std::same_as<decode_result<typename T::value_type, true>>;
        { T::template decode<false>(s) } -> std::same_as<decode_result<typename T::value_type, false>>;
    };

    /// @brief Describes a type which is both @ref encoder and @ref decoder.
    template<typename T>
    concept coder = encoder<T> && decoder<T>;

}

#endif //PROTOPUF_CODER_H
