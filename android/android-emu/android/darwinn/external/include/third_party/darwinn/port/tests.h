#ifndef THIRD_PARTY_DARWINN_PORT_TESTS_H_
#define THIRD_PARTY_DARWINN_PORT_TESTS_H_

#if defined(DARWINN_PORT_GOOGLE3)
#include "testing/base/public/gmock.h"
#include "testing/base/public/gunit.h"
#endif

#if defined(DARWINN_PORT_DEFAULT) || \
    defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)
// TODO(jnjoseph): Change this to <gtest/gtest.h> and <gmock/gmock.h>
#include "testing/base/public/gmock.h"  // NOLINT
#include "testing/base/public/gunit.h"  // NOLINT
#include "third_party/darwinn/port/default/status_test_util.h"
#endif

#endif  // THIRD_PARTY_DARWINN_PORT_TESTS_H_
