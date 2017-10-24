#include <string>

#include "android/base/memory/ScopedPtr.h"
#include "android/base/testing/TestTempDir.h"
#include "android/cmdline-option.h"
#include "android/telephony/proto/sim_access_rules.pb.h"
#include "android/telephony/SimAccessRules.h"

#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"

#include <gtest/gtest.h>
#include <fstream>

using ::google::protobuf::TextFormat;
using ::google::protobuf::io::OstreamOutputStream;

namespace android {
namespace base {

static const char kAraAid[] = "A00000015141434C00";

TEST(SimAccessRulesTest, defaultAccessRules) {
    AndroidOptions options = {};
    android_cmdLineOptions = &options;
    auto undoCommandLine = makeCustomScopedPtr(
            &android_cmdLineOptions,
            [](const AndroidOptions** opts) { *opts = nullptr; });

    SimAccessRules rules;
    ASSERT_STREQ(
            rules.getRule(kAraAid),
            "FF403eE23cE12eC11461ed377e85d386a8dfee6b864bd85b0bfaa5af81CA16616e"
            "64726f69642e636172726965726170692e637473E30aDB080000000000000000");
}

TEST(SimAccessRulesTest, customAccessRules) {
    TestTempDir testDir("simaccessrulestest");
    auto config_file_path =
            testDir.makeSubPath("custom_access_rules.textproto");

    android_emulator::AllRefArDo ref_ar_dos;
    auto ref_ar_do = ref_ar_dos.add_ref_ar_dos();
    ref_ar_do->mutable_ref_do()->set_device_app_id_ref_do(
            "61ed377e85d386a8dfee6b864bd85b0bfaa5af81");
    ref_ar_do->mutable_ar_do()->set_perm_ar_do("0000000000000000");

    android_emulator::SimAccessRules access_rules;
    (*access_rules.mutable_sim_access_rules())[std::string(kAraAid)] =
            ref_ar_dos;

    {
        std::ofstream file(config_file_path);
        OstreamOutputStream ostream(&file);
        ASSERT_TRUE(TextFormat::Print(access_rules, &ostream));
    }

    AndroidOptions options = {};
    options.sim_access_rules_file = const_cast<char*>(config_file_path.c_str());
    android_cmdLineOptions = &options;
    auto undoCommandLine = makeCustomScopedPtr(
            &android_cmdLineOptions,
            [](const AndroidOptions** opts) { *opts = nullptr; });

    SimAccessRules rules;
    ASSERT_STREQ(
            rules.getRule(kAraAid),
            "FF4026E224E116C11461ed377e85d386a8dfee6b864bd85b0bfaa5af81E30aDB08"
            "0000000000000000");
}

}  // namespace base
}  // namespace android
