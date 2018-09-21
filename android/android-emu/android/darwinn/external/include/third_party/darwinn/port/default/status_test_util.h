/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef THIRD_PARTY_DARWINN_PORT_STATUS_TEST_UTIL_H_
#define THIRD_PARTY_DARWINN_PORT_STATUS_TEST_UTIL_H_

#include "testing/base/public/gunit.h"
#include "third_party/darwinn/port/default/status.h"

// Macros for testing the results of functions that return darwinn::Status.
#define EXPECT_OK(statement) \
  EXPECT_EQ(::platforms::darwinn::util::Status::OK(), (statement))
#define ASSERT_OK(statement) \
  ASSERT_EQ(::platforms::darwinn::util::Status::OK(), (statement))

// There are no EXPECT_NOT_OK/ASSERT_NOT_OK macros since they would not
// provide much value (when they fail, they would just print the OK status
// which conveys no more information than EXPECT_FALSE(status.ok());
// If you want to check for particular errors, a better alternative is:
// EXPECT_EQ(..expected darwinn::util::error::Code..., status.code());

// Point to existing implementation in status_macros.h.
#define ASSERT_OK_AND_ASSIGN(lhs, rexpr) ASSIGN_OR_ASSERT_OK(lhs, rexpr)

#endif  // THIRD_PARTY_DARWINN_PORT_STATUS_TEST_UTIL_H_
