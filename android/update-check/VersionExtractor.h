#pragma once

#include "android/update-check/UpdateChecker.h"

namespace android {
namespace update_check {

class VersionExtractor : public IVersionExtractor {
public:
    static const char* const kXmlNamespace;

    virtual android::base::Version extractVersion(const std::string &data);
    virtual android::base::Version getCurrentVersion() const;
};

}  // namespace update_check
}  // namespace android
