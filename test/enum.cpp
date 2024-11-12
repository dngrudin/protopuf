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

#include <gtest/gtest.h>

#include <protopuf/byte.h>
#include <protopuf/enum.h>
#include <array>

#include "test_fixture.h"

using namespace pp;
using namespace std;

template<typename T>
struct test_enum_coder : test_fixture<T> {};
TYPED_TEST_SUITE(test_enum_coder, is_safe_types, test_name_generator);

TYPED_TEST(test_enum_coder, encode) {
    array<byte, 10> a{};
    bytes b;

    enum E1 { red, green, blue = 128 };

    ASSERT_TRUE(get_value_from_result<TestFixture::is_safe>(
        enum_coder<E1>::template encode<TestFixture::is_safe>(green, a), b));
    EXPECT_EQ(begin_diff(b, a), 1);
    EXPECT_EQ(a[0], 1_b);

    ASSERT_TRUE(get_value_from_result<TestFixture::is_safe>(
        enum_coder<E1>::template encode<TestFixture::is_safe>(blue, a), b));
    EXPECT_EQ(begin_diff(b, a), 2);
    EXPECT_EQ(a[0], 0x80_b);
    EXPECT_EQ(a[1], 0x1_b);

    enum class E2 { red, green, blue };

    ASSERT_TRUE(get_value_from_result<TestFixture::is_safe>(
        enum_coder<E2>::template encode<TestFixture::is_safe>(E2::green, a), b));
    EXPECT_EQ(begin_diff(b, a), 1);
    EXPECT_EQ(a[0], 1_b);

    enum E3 : pp::uint<8> { x, y, z };

    ASSERT_TRUE(get_value_from_result<TestFixture::is_safe>(
        enum_coder<E3>::template encode<TestFixture::is_safe>(z, a), b));
    EXPECT_EQ(begin_diff(b, a), 1);
    EXPECT_EQ(a[0], 2_b);
}

TYPED_TEST(test_enum_coder, decode) {
    array<byte, 10> a{};

    enum E1 { red, green, blue = 128 };

    {
        a[0] = 1_b;
        decode_value<E1> value;
        ASSERT_TRUE(get_value_from_result<TestFixture::is_safe>(
            enum_coder<E1>::template decode<TestFixture::is_safe>(a), value));
        auto [v, n] = value;
        EXPECT_EQ(begin_diff(n, a), 1);
        EXPECT_EQ(v, green);
    }

    {
        a[0] = 0x80_b; a[1] = 0x1_b;
        decode_value<E1> value;
        ASSERT_TRUE(get_value_from_result<TestFixture::is_safe>(
            enum_coder<E1>::template decode<TestFixture::is_safe>(a), value));
        auto [v, n] = value;
        EXPECT_EQ(begin_diff(n, a), 2);
        EXPECT_EQ(v, blue);
    }

    enum class E2 { red, green, blue };

    {
        a[0] = 0x1_b;
        decode_value<E2> value;
        ASSERT_TRUE(get_value_from_result<TestFixture::is_safe>(
            enum_coder<E2>::template decode<TestFixture::is_safe>(a), value));
        auto [v, n] = value;
        EXPECT_EQ(begin_diff(n, a), 1);
        EXPECT_EQ(v, E2::green);
    }

    enum E3 : pp::uint<8> { x, y, z };

    {
        a[0] = 0x2_b;
        decode_value<E3> value;
        ASSERT_TRUE(get_value_from_result<TestFixture::is_safe>(
            enum_coder<E3>::template decode<TestFixture::is_safe>(a), value));
        auto [v, n] = value;
        EXPECT_EQ(begin_diff(n, a), 1);
        EXPECT_EQ(v, z);
    }
}

GTEST_TEST(enum_coder, encode_with_insufficient_buffer_size) {
    enum E1 { red, green, blue = 128 };
    run_safe_encode_tests_with_insufficient_buffer_size<enum_coder<E1>, 2>(blue);
}

GTEST_TEST(enum_coder, decode_with_insufficient_buffer_size) {
    enum E1 { red, green, blue = 128 };
    array<byte, 2> a{0x80_b, 0x1_b};
    run_safe_decode_tests_with_insufficient_buffer_size<enum_coder<E1>>(a);
}