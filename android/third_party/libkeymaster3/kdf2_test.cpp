/*
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "kdf2.h"
#include <gtest/gtest.h>
#include <string.h>

#include "android_keymaster_test_utils.h"

using std::string;

namespace keymaster {

namespace test {

struct Kdf2Test {
    const char* key_hex;
    const char* info_hex;
    const char* expected_output_hex;
    keymaster_digest_t digest_type;
};

static const Kdf2Test kKdf2Tests[] = {
    {"032e45326fa859a72ec235acff929b15d1372e30b207255f0611b8f785d764374152e0ac009e509e7ba30cd2f1778"
     "e113b64e135cf4e2292c75efe5288edfda4",
     "",
     "10a2403db42a8743cb989de86e668d168cbe6046e23ff26f741e87949a3bba1311ac179f819a3d18412e9eb45668f"
     "2923c087c1299005f8d5fd42ca257bc93e8fee0c5a0d2a8aa70185401fbbd99379ec76c663e9a29d0b70f3fe261a5"
     "9cdc24875a60b4aacb1319fa11c3365a8b79a44669f26fba933d012db213d7e3b16349",
     KM_DIGEST_SHA_2_256},
    {"032e45326fa859a72ec235acff929b15d1372e30b207255f0611b8f785d764374152e0ac009e509e7ba30cd2f1778"
     "e113b64e135cf4e2292c75efe5288edfda4",
     "",
     "0e6a26eb7b956ccb8b3bdc1ca975bc57c3989e8fbad31a224655d800c46954840ff32052cdf0d640562bdfadfa263"
     "cfccf3c52b29f2af4a1869959bc77f854cf15bd7a25192985a842dbff8e13efee5b7e7e55bbe4d389647c686a9a9a"
     "b3fb889b2d7767d3837eea4e0a2f04b53ca8f50fb31225c1be2d0126c8c7a4753b0807",
     KM_DIGEST_SHA1},
    {"CA7C0F8C3FFA87A96E1B74AC8E6AF594347BB40A", "", "744AB703F5BC082E59185F6D049D2D367DB245C2",
     KM_DIGEST_SHA1},
    {"0499B502FC8B5BAFB0F4047E731D1F9FD8CD0D8881", "",
     "03C62280C894E103C680B13CD4B4AE740A5EF0C72547292F82DC6B1777F47D63BA9D1EA73"
     "2DBF386",
     KM_DIGEST_SHA1},
    {"5E10B967A95606853E528F04262AD18A4767C761163971391E17CB05A21668D4CE2B9F151617408042CE091958382"
     "3FD346D1751FBE2341AF2EE0461B62F100FFAD4F723F70C18B38238ED183E9398C8CA517EE0CBBEFFF9C59471FE27"
     "8093924089480DBC5A38E9A1A97D23038106847D0D22ECF85F49A861821199BAFCB0D74E6ACFFD7D142765EBF4C71"
     "2414FE4B6AB957F4CB466B46601289BB82060428272842EE28F113CD11F39431CBFFD823254CE472E2105E49B3D7F"
     "113B825076E6264585807BC46454665F27C5E4E1A4BD03470486322981FDC894CCA1E2930987C92C15A38BC42EB38"
     "810E867C4432F07259EC00CDBBB0FB99E1727C706DA58DD",
     "484D4143204B6579", "BC98EB018CB00EE26D1F97A15AE166912A7AC4C5", KM_DIGEST_SHA1},

};

TEST(Kdf2Test, Kdf2) {
    for (auto& test : kKdf2Tests) {
        const string key = hex2str(test.key_hex);
        const string info = hex2str(test.info_hex);
        const string expected_output = hex2str(test.expected_output_hex);
        size_t output_len = expected_output.size();
        uint8_t output[output_len];

        Kdf2 kdf2;
        ASSERT_TRUE(
            kdf2.Init(test.digest_type, reinterpret_cast<const uint8_t*>(key.data()), key.size()));
        ASSERT_TRUE(kdf2.GenerateKey(reinterpret_cast<const uint8_t*>(info.data()), info.size(),
                                     output, output_len));
        EXPECT_EQ(0, memcmp(output, expected_output.data(), output_len));
    }
}

}  // namespace test

}  // namespace keymaster
