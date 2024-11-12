//   Copyright 2020-2024 PragmaTwice
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

#ifndef PROTOPUF_TEST_FIXTURE_H
#define PROTOPUF_TEST_FIXTURE_H

#include <gtest/gtest.h>

template<typename T>
struct test_fixture : testing::Test {
    static constexpr bool is_safe = T::value;
};

using is_safe_types = testing::Types<std::true_type, std::false_type>;

class test_name_generator {
public:
  template<typename T>
  static std::string GetName(int) {
    if constexpr (std::is_same_v<T, std::true_type>) return "safe";
    if constexpr (std::is_same_v<T, std::false_type>) return "unsafe";
  }
};

#endif //PROTOPUF_TEST_FIXTURE_H