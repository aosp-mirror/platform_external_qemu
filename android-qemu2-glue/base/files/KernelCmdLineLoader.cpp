// Copyright 2024 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android-qemu2-glue/base/files/KernelCmdLineLoader.h"
#include <memory>
#include <fstream>
#include <sstream>

#include "absl/strings/strip.h"
#include "aemu/base/files/PathUtils.h"

namespace android {
namespace qemu {

std::string loadKernelCmdLine(const char* kernelCmdLinePath) {
    std::string systemImageKernelCommandLine;
    if (kernelCmdLinePath == nullptr) {
        return systemImageKernelCommandLine;
    }

    std::ifstream kernelCmdLineFile(
            base::PathUtils::asUnicodePath(kernelCmdLinePath).c_str());
    if (kernelCmdLineFile) {
        std::stringstream buffer;
        buffer << kernelCmdLineFile.rdbuf();
        systemImageKernelCommandLine = buffer.str();
        absl::StripTrailingAsciiWhitespace(&systemImageKernelCommandLine);
    }

    return systemImageKernelCommandLine;
}

std::string loadKernelCmdLineFromAvd(const AvdInfo* avd) {
    const std::unique_ptr<char, void (*)(void*)> kernelCmdLinePath(
            avdInfo_getKernelCmdLinePath(avd), ::free);
    return loadKernelCmdLine(kernelCmdLinePath.get());
}
}  // namespace qemu
}  // namespace android