// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/ProcessControl.h"

#include <stddef.h>  // for size_t

#include <algorithm>   // for count_if
#include <fstream>     // for operator<<, basic_ostream
#include <functional>  // for function, __base
#include <sstream>

#include "android/base/FunctionView.h"         // for FunctionView
#include "android/base/Optional.h"             // for Optional, kNullopt
#include "android/base/files/PathUtils.h"      // for PathUtils
#include "android/base/memory/LazyInstance.h"  // for LazyInstance, LAZY_INS...
#include "android/base/process-control.h"      // for handle_emulator_restart
#include "android/base/system/System.h"        // for System

using android::base::PathUtils;

namespace android {
namespace base {

bool ProcessLaunchParameters::operator==(
        const ProcessLaunchParameters& other) const {
    if (workingDirectory != other.workingDirectory)
        return false;
    if (programPath != other.programPath)
        return false;
    if (argv.size() != other.argv.size())
        return false;

    for (int i = 0; i < argv.size(); ++i) {
        if (argv[i] != other.argv[i])
            return false;
    }

    return true;
}

std::string ProcessLaunchParameters::str() const {
    std::stringstream ss;

    ss << "Working directory: [" << workingDirectory << "]" << std::endl;
    ss << "Program path: [" << programPath << "]" << std::endl;
    ss << "Arg count: " << argv.size() << std::endl;

    for (int i = 0; i < argv.size(); i++) {
        ss << "Argv[ " << i << "] : [" << argv[i] << "]" << std::endl;
    }

    return ss.str();
}

inline static std::string escapeCommandLine(const std::string& line) {
    size_t quoteCnt = std::count_if(line.begin(), line.end(), [](char x) {
        return x == '"' || x == '\\';
    });
    if (quoteCnt == 0)
        return '"' + line + '"';

    std::string escaped;
    escaped.reserve(quoteCnt + line.size() + 2);
    escaped += '"';
    for (int i = 0; i < line.size(); i++) {
        if (line[i] == '"' || line[i] == '\\') {
            escaped += '\\';
        }
        escaped += line[i];
    }
    escaped += '"';

    return escaped;
}

std::vector<std::string> makeArgvStrings(int argc, const char** argv) {
    std::vector<std::string> res;

    for (int i = 0; i < argc; ++i) {
        res.push_back(argv[i]);
    }

    return res;
}

std::string createEscapedLaunchString(int argc, const char* const* argv) {
    std::string result;
    auto mParams = makeArgvStrings(argc, (const char**)argv);
    for (size_t n = 0; n < mParams.size(); ++n) {
        if (n > 0) {
            result += ' ';
        }
        result += escapeCommandLine(mParams[n]);
    }
    return result;
}

/**
 * Parses a command line string into individual arguments. The arguments are
 * separated by whitespace characters. Each argument is optionally enclosed into
 * double quotes. Double quotes inside quoted arguments and whitespaces inside a
 * non-quoted ones are escaped by backslashes. Backslashes inside arguments are
 * doubled. A character preceded by a single backslash is taken literally and
 * doesn't have any special meaning.
 */
std::vector<std::string> parseEscapedLaunchString(std::string launch) {
    constexpr char kQuote = '\'', kDoubleQuote = '"', kEscape = '\\', kNone = '\000';
    std::vector<std::string> args;
    std::string argBuilder;
    bool insideArgument = false, escaped = false;
    char quoted = kNone;
    for (char c : launch) {
        if (insideArgument) {
            if (escaped) {
                argBuilder.push_back(c);
                escaped = false;
            } else {
                if ( (quoted == c) || (quoted == kNone && isspace(c))) {
                    args.push_back(argBuilder);
                    argBuilder.clear();
                    insideArgument = false;
                    quoted = kNone;
                } else if (c == kEscape)
                    escaped = true;
                else
                    argBuilder.push_back(c);
            }
        } else {
            if (!isspace(c)) {
                insideArgument = true;
                switch (c) {
                    case kQuote:
                        quoted = kQuote;
                        break;
                    case kDoubleQuote:
                        quoted = kDoubleQuote;
                        break;
                    case kEscape:
                        escaped = true;
                        break;
                    default:
                        argBuilder.push_back(c);
                }
            }
        }
    }
    if (!argBuilder.empty()) {
        args.push_back(argBuilder);
    }
    return args;
}

ProcessLaunchParameters createLaunchParametersForCurrentProcess(int argc,
                                                                char** argv) {
    ProcessLaunchParameters currentLaunchParams = {
            System::get()->getCurrentDirectory(),
            PathUtils::join(System::get()->getProgramDirectory(),
                            PathUtils::decompose(argv[0]).back()),
            makeArgvStrings(argc, (const char**)argv),
    };
    return currentLaunchParams;
}

void saveLaunchParameters(const ProcessLaunchParameters& launchParams,
                          StringView filename) {
    std::ofstream file(PathUtils::asUnicodePath(filename).c_str());

    file << launchParams.workingDirectory << std::endl;
    file << launchParams.programPath << std::endl;
    file << launchParams.argv.size() << std::endl;

    for (const auto& arg : launchParams.argv) {
        // argv[0] is still saved, but can be skipped when launching
        file << arg << std::endl;
    }
}

ProcessLaunchParameters loadLaunchParameters(StringView filename) {
    ProcessLaunchParameters res;

    std::ifstream file(PathUtils::asUnicodePath(filename).c_str());

    std::string line;

    std::getline(file, line);
    res.workingDirectory = line;

    std::getline(file, line);
    res.programPath = line;

    std::getline(file, line);
    std::istringstream iss(line);

    int argc = 0;
    iss >> argc;
    res.argv.resize(argc);

    for (int i = 0; i < argc; i++) {
        std::getline(file, line);
        res.argv[i] = line;
    }

    return res;
}

void launchProcessFromParameters(const ProcessLaunchParameters& launchParams,
                                 bool useArgv0) {
    std::vector<std::string> cmdArgs;
    cmdArgs.push_back(launchParams.programPath);

    for (int i = 1; i < launchParams.argv.size(); ++i) {
        cmdArgs.push_back(launchParams.argv[i]);
    }

    System::get()->runCommand(cmdArgs);
}

class RestartGlobals {
public:
    RestartGlobals() = default;
    bool disabled = false;
    bool doRestartOnExit = false;
    Optional<ProcessLaunchParameters> restartParams = kNullopt;
};

static LazyInstance<RestartGlobals> sRestartGlobals = LAZY_INSTANCE_INIT;

void disableRestart() {
    sRestartGlobals->disabled = true;
}

bool isRestartDisabled() {
    return sRestartGlobals->disabled;
}

static constexpr char kRestartParam[] = "-is-restart";

void initializeEmulatorRestartParameters(int argc,
                                         char** argv,
                                         const char* outPath) {
    auto params = createLaunchParametersForCurrentProcess(argc, argv);
    android::base::saveLaunchParameters(
            createLaunchParametersForCurrentProcess(argc, argv),
            PathUtils::join(outPath, kLaunchParamsFileName));
    sRestartGlobals->restartParams.emplace(params);
}

void finalizeEmulatorRestartParameters(const char* dir) {
    auto path = PathUtils::join(dir, kLaunchParamsFileName);

    if (!System::get()->pathExists(path))
        return;

    auto params = loadLaunchParameters(path);

    // Add or replace the "-is-restart" option
    // to make the launch wait for process exit next time
    bool hasExistingRestartOpt = false;
    int restartArgIndex = 0;
    for (const auto& arg : params.argv) {
        if (arg == kRestartParam) {
            hasExistingRestartOpt = true;
            break;
        }
        ++restartArgIndex;
    }

    std::stringstream ss;
    ss << System::get()->getCurrentProcessId();

    if (hasExistingRestartOpt) {
        params.argv[restartArgIndex + 1] = ss.str();
    } else {
        params.argv.push_back(kRestartParam);
        params.argv.push_back(ss.str());
    }

    sRestartGlobals->restartParams.emplace(params);
}

void setEmulatorRestartOnExit() {
    if (sRestartGlobals->disabled)
        return;
    sRestartGlobals->doRestartOnExit = true;
}

void handleEmulatorRestart() {
    auto params = sRestartGlobals->restartParams;
    if (sRestartGlobals->doRestartOnExit && params) {
        launchProcessFromParameters(*params);
    }
}

class EmulatorRestarter {
public:
    EmulatorRestarter() = default;

    void registerQuit(FunctionView<void()> func) { mQuitFunc = func; }

    void quit() {
        if (mQuitFunc)
            mQuitFunc();
    }

    void restart() {
        if (!mQuitFunc)
            return;
        android::base::setEmulatorRestartOnExit();
        quit();
    }

private:
    std::function<void()> mQuitFunc;
};

static android::base::LazyInstance<EmulatorRestarter> sRestarter =
        LAZY_INSTANCE_INIT;

void registerEmulatorQuitCallback(FunctionView<void()> func) {
    sRestarter->registerQuit(func);
}

void restartEmulator() {
    sRestarter->restart();
}

}  // namespace base
}  // namespace android

extern "C" {

void handle_emulator_restart() {
    android::base::handleEmulatorRestart();
}
}
