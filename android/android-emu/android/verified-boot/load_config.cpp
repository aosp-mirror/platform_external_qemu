// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/verified-boot/load_config.h"

#include "android/base/Log.h"
#include "verified_boot_config.pb.h"

#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include <fcntl.h>
#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <unistd.h>
#endif

using ::android::verifiedboot::Status;
using ::android::verifiedboot::VerifiedBootConfig;
using ::google::protobuf::TextFormat;
using ::google::protobuf::io::ArrayInputStream;
using ::google::protobuf::io::ColumnNumber;
using ::google::protobuf::io::FileInputStream;
using ::google::protobuf::io::ZeroCopyInputStream;

static const int kMaxSupportedMajorVersion = 2;

namespace {

// Simple Proto parsing class that dumps parsing errors to the logs.
//
// This class is needed to satisfy the API requirements specified by the
// TextFormat proto parsing library.
class SimpleErrorCollector : public ::google::protobuf::io::ErrorCollector {
public:
    ~SimpleErrorCollector() override = default;
    void AddError(int line,
                  ColumnNumber column,
                  const std::string& message) override {
        if (mFirstError) {
            mFirstError = false;
            LOG(WARNING) << "Could not parse verified boot config: ";
        }
        LOG(WARNING) << "  " << (line + 1) << ":" << (column + 1) << ": "
                     << message.c_str();
    }

private:
    bool mFirstError = {true};
};

}  // namespace

// Parses |input| as a textproto, placing result in |config|.
//
// If |input| fails to parse, returns Status::ParseError
// and writes the specifc reason to the logs.
static Status parseMessage(ZeroCopyInputStream* input,
                           VerifiedBootConfig* config) {
    SimpleErrorCollector error_collector;
    TextFormat::Parser parser;
    parser.RecordErrorsTo(&error_collector);
    if (!parser.Parse(input, config)) {
        return Status::ParseError;
    }
    return Status::OK;
}

// Checks a given parameter for illegal characters.
//
// |param| is the parameter to check.
// |prefix| is used to log a better error message.  It should name the
//     repeated proto field.
// |param_idx| is used to log a better error message.  It should
//     identify the paramter number and be zero indexed.
static Status checkParam(const std::string& param,
                         const char* prefix,
                         int param_idx) {
    ++param_idx;  // Make the index 1-based
    if (param.empty()) {
        LOG(WARNING) << "Verified boot config " << prefix << " parameter "
                     << param_idx << " is empty.";
        return Status::EmptyParameter;
    }

    for (const char c : param) {
        const bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') || (c == '/') || (c == ',') ||
                        (c == '_') || (c == '-') || (c == '=') || (c == '.');

        if (!ok) {
            LOG(WARNING) << "Verified boot config " << prefix << " parameter "
                         << param_idx << " contains an illegal character: '"
                         << c << "'";
            return Status::IllegalCharacter;
        }
    }

    return Status::OK;
}

// Checks for the existance of max_version and asserts that
// it's value is within kMaxSupportedMajorVersion.
//
// |config| is the proto message that's expected to contain a
// major_version field.
static Status checkVersion(const VerifiedBootConfig& config) {
    if (!config.has_major_version()) {
        LOG(WARNING) << "Verified boot config is missing a required key: "
                     << "major_version";
        return Status::MissingVersion;
    }

    if (config.major_version() > kMaxSupportedMajorVersion) {
        LOG(WARNING) << "Verified boot config  version is too large to be "
                     << " supported. major_version = " << config.major_version()
                     << ", supported = " << kMaxSupportedMajorVersion;
        return Status::UnsupportedVersion;
    }

    return Status::OK;
}

// Builds a dm="..." string into the provided |dm_param| by parsing
// through config.dm_params.
//
// |config| is the configuration proto
// |dm_param| is the target string that will be populated with
//   dm="...".
static Status buildDMParam(const VerifiedBootConfig& config,
                           std::string* dm_param) {
    if (config.dm_param_size() < 1) {
        LOG(WARNING) << "Verified boot config  does not contain any dm_param "
                     << "options";
        return Status::MissingDMParam;
    }

    *dm_param = "dm=\"";
    for (int i = 0; i < config.dm_param_size(); ++i) {
        const std::string& param = config.dm_param(i);
        Status status = Status::OK;
        if ((status = checkParam(param, "dm_param", i)) != Status::OK) {
            return status;
        }
        if (i > 0) {
            *dm_param += ' ';
        }
        *dm_param += param;
    }
    *dm_param += '\"';

    return Status::OK;
}

// Converts |config| into a list of |params| that can be added to the kernel.
//
// |config| is the configuration proto
// |params| is the target vector that will be populated with
// resulting kernel parameters.
//
// If any parameter fails it's check, logs the error and
// returns an error status.  On an error |params| may be left
// in a partially built state.
static Status buildParams(const VerifiedBootConfig& config,
                          std::vector<std::string>* params) {
    std::string dm_param;
    Status status = Status::OK;
    if (config.major_version() == 1) {
        if ((status = buildDMParam(config, &dm_param)) != Status::OK) {
            return status;
        }
        params->push_back(dm_param);
    }

    for (int i = 0; i < config.param_size(); ++i) {
        const std::string& param = config.param(i);
        if ((status = checkParam(param, "param", i)) != Status::OK) {
            return status;
        }
        params->push_back(param);
    }

    return Status::OK;
}

// Common logic that is shared between getParameters()
// and getParametersFromFile().
//
// This interface is private because ZeroCopyInputStream is a proto-specific
// concept - The caller should not have to deal with it directly.
//
// |input| is the input stream that was prepared by the caller.
// |params| is populated with the resulting kernel parameters.
//
// Returns Status::OK if successful.  If there was a problem, |params|
// will be empty and false is returned.
static Status getParametersCommon(ZeroCopyInputStream* input,
                                  std::vector<std::string>* params) {
    VerifiedBootConfig config;
    Status status = Status::OK;

    if ((status = parseMessage(input, &config)) != Status::OK) {
        return status;
    }

    if ((status = checkVersion(config)) != Status::OK) {
        return status;
    }

    if ((status = buildParams(config, params)) != Status::OK) {
        params->clear();
        return status;
    }

    return Status::OK;
}

namespace android {
namespace verifiedboot {

// Public interface, see header for docs.
Status getParameters(const std::string& textproto,
                     std::vector<std::string>* params) {
    ArrayInputStream input(textproto.c_str(), textproto.length());
    return getParametersCommon(&input, params);
}

// Public interface, see header for docs.
Status getParametersFromFile(const char* pathname,
                             std::vector<std::string>* params) {
    if (!pathname) {
        LOG(VERBOSE) << "Verified boot params were not found.";
        return Status::CouldNotOpenFile;
    }

    const int fd = open(pathname, O_RDONLY);
    if (fd < 0) {
        LOG(INFO) << "Could not open " << pathname << ": " << strerror(errno);
        return Status::CouldNotOpenFile;
    }

    // fd is now an open file handle - don't return without closing it.

    FileInputStream input(fd);
    Status status = getParametersCommon(&input, params);

    close(fd);
    return status;
}

}  // namespace verifiedboot
}  // namespace android
