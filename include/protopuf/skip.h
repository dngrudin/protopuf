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

#ifndef PROTOPUF_SKIP_H
#define PROTOPUF_SKIP_H

#include "coder.h"
#include "int.h"
#include "varint.h"
#include "zigzag.h"
#include "float.h"
#include "bool.h"
#include "enum.h"

namespace pp {

    /// @brief A type which `decode_skipper`'s `decode_skip` returns.
    /// @param is_safe the decode skipping mode (safe or without buffer overflow control)
    template<bool is_safe>
    using decode_skip_result = std::conditional_t<is_safe, std::optional<bytes>, bytes>;

    /// @brief Constructs the decode skipping result depending on the decoding mode.
    ///
    /// Template parameters:
    /// @param is_safe the decoding mode (safe or without buffer overflow control)
    /// @param Args	the types of the arguments list with which the result will be constructed
    ///
    /// Function parameters:
    /// @param args the arguments list with which the result will be constructed
    /// @returns the @ref decode_skip_result which depends on the decoding mode
    template<bool is_safe, typename... Args>
    inline constexpr decode_skip_result<is_safe> make_decode_skip_result(Args&&... args) {
        if constexpr (is_safe) {
            return decode_skip_result<is_safe>{std::in_place, std::forward<Args>(args)...};
        } else {
            return decode_skip_result<is_safe>{std::forward<Args>(args)...};
        }
    }

    /// @brief A concept statisfied while `T::coder` is a @ref coder and 
    /// type `T` has static member function `encode_skip`, which try to encode an object 
    /// but actually not decode it into any byte sequence; it just skip the bytes which can be encoded from the object.
    ///
    /// Static member function `encode_skip`:
    /// @param v the object to be encoded from
    /// @returns the length of the encoded object (byte form)
    /// 
    /// Corresponding to @ref encoder, except it just return the length of the encoded object 
    /// and does not really encode it into bytes. 
    template <typename T>
    concept encode_skipper = coder<typename T::coder> && requires(typename T::value_type v) {
        { T::encode_skip(v) } -> std::same_as<std::size_t>;
    };

    /// @brief A concept statisfied while `T::coder` is a @ref coder and 
    /// type `T` has static member function `decode_skip`, which try to decode a byte sequence 
    /// but actually not decode it to an object; it just skip the bytes which can be decoded to an object.
    ///
    /// Decode skiping can be performed in two modes: with buffer 'v' overflow control (safe mode) and without.
    /// Static member function `decode_skip`:
    /// @param v the bytes to be decoded from
    /// @returns the bytes from `begin(v) + decoded_object_length` to `end(v)`
    /// 
    /// Corresponding to @ref decoder, except it just skip the length of the decoded object from the bytes 
    /// and does not really generate the object. 
    template <typename T>
    concept decode_skipper = coder<typename T::coder> && requires(bytes v) {
        { T::template decode_skip<true>(v) } -> std::same_as<decode_skip_result<true>>;
        { T::template decode_skip<false>(v) } -> std::same_as<decode_skip_result<false>>;
    };

    /// @brief A concept statisfied while `T` is both @ref encode_skipper and @ref decode_skipper
    template <typename T>
    concept skipper_c = encode_skipper<T> && decode_skipper<T>;

    /// @brief The implementations of @ref skipper_c
    /// @param C the corresponding @ref coder to the skipper
    template <coder C>
    struct skipper;

    template <typename T>
    struct skipper<integer_coder<T>> {
        using coder = integer_coder<T>;
        using value_type = T;

        static constexpr std::size_t encode_skip(T) {
            return sizeof(T);
        }

        template<bool is_safe = false>
        static constexpr decode_skip_result<is_safe> decode_skip(bytes b) {
            if constexpr (is_safe) {
                if (b.size() < sizeof(T)) {
                    return {};
                }
            }
            return make_decode_skip_result<is_safe>(b.subspan<sizeof(T)>());
        }
    };

    template <typename T>
    struct skipper<float_coder<T>> {
        using coder = float_coder<T>;
        using value_type = T;

        static constexpr std::size_t encode_skip(T) {
            return sizeof(T);
        }

        template<bool is_safe = false>
        static constexpr decode_skip_result<is_safe> decode_skip(bytes b) {
            if constexpr (is_safe) {
                if (b.size() < sizeof(T)) {
                    return {};
                }
            }
            return make_decode_skip_result<is_safe>(b.subspan<sizeof(T)>());
        }
    };

    template <std::unsigned_integral T>
    struct skipper<varint_coder<T>> {
        using coder = varint_coder<T>;
        using value_type = T;

        static constexpr std::size_t encode_skip(T v) {
            std::size_t n = 0;

            do {
                v >>= 7, ++n;
            } while(v != 0);

            return n;
        }

        template<bool is_safe = false>
        static constexpr decode_skip_result<is_safe> decode_skip(bytes b) {
            auto iter = b.begin();
            [[maybe_unused]] const auto end = b.end();

            if constexpr (is_safe) {
                if (iter == end) {
                    return {};
                }
            }

            while((*iter++ >> 7) == 1_b) {
                if constexpr (is_safe) {
                    if (iter == end) {
                        return {};
                    }
                }
            }

            return make_decode_skip_result<is_safe>(iter, b.end());
        }
    };

    template <std::signed_integral T>
    struct skipper<varint_coder<T>> {
        using coder = varint_coder<T>;
        using value_type = T;

        static constexpr std::size_t encode_skip(T v) {
            return skipper<varint_coder<std::make_unsigned_t<T>>>::encode_skip(static_cast<std::make_unsigned_t<T>>(v));
        }

        template<bool is_safe = false>
        static constexpr decode_skip_result<is_safe> decode_skip(bytes b) {
            return skipper<varint_coder<std::make_unsigned_t<T>>>::template decode_skip<is_safe>(b);
        }
    };

    template <std::size_t N>
    struct skipper<varint_coder<sint_zigzag<N>>> {
    private:
        using T = sint_zigzag<N>;

    public:
        using coder = varint_coder<T>;
        using value_type = T;

        static constexpr std::size_t encode_skip(T v) {
            return skipper<varint_coder<uint<N>>>::encode_skip(v.get_underlying());
        }

        template<bool is_safe = false>
        static constexpr decode_skip_result<is_safe> decode_skip(bytes b) {
            return skipper<varint_coder<uint<N>>>::template decode_skip<is_safe>(b);
        }
    };

    template <>
    struct skipper<bool_coder> {
    public:
        using coder = bool_coder;
        using value_type = bool;

        static constexpr std::size_t encode_skip(bool v) {
            return skipper<integer_coder<uint<1>>>::encode_skip(v);
        }

        template<bool is_safe = false>
        static constexpr decode_skip_result<is_safe> decode_skip(bytes b) {
            return skipper<integer_coder<uint<1>>>::template decode_skip<is_safe>(b);
        }
    };

    template <typename T>
    struct skipper<enum_coder<T>> {
    public:
        using coder = enum_coder<T>;
        using value_type = T;

        static constexpr std::size_t encode_skip(T v) {
            return skipper<varint_coder<std::underlying_type_t<T>>>::encode_skip(static_cast<std::underlying_type_t<T>>(v));
        }

        template<bool is_safe = false>
        static constexpr decode_skip_result<is_safe> decode_skip(bytes b) {
            return skipper<integer_coder<std::underlying_type_t<T>>>::template decode_skip<is_safe>(b);
        }
    };

}

#endif //PROTOPUF_SKIP_H
