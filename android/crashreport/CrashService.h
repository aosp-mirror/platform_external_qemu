// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "google_breakpad/processor/process_state.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace android {
namespace crashreport {

// Class CrashService wraps Breakpad platform specific crash generation server
//
// Usage:
//    Instantiate with a version and build number to be used as default report
//    version
//
//    For manual processing:
//        setDumpFile
//    Otherwise:
//        startCrashServer
//        waitForDumpFile
//        stopCrashServer
//
//    Finally, to upload the crash:
//        uploadCrash
//
//    After uploading, the report id is populated:
//        getReportId
//
//    The dump can be processed locally with getCrashDetails
//
class CrashService {
public:
    // Context state used by breakpad crash server dump request callbacks
    struct DumpRequestContext {
        std::string file_path;
    };

    // Server state used by breakpad crash server connect / exit callbacks
    struct ServerState {
        bool waiting;
        int connected;
    };

    // version and build args will be used to construct breakpad version
    // identifier
    CrashService(const std::string& version, const std::string& build);

    // Desctructor
    virtual ~CrashService();

    // Save a dumpfile path
    void setDumpFile(const std::string& dumpfile);

    // Get saved dumpfile path
    std::string getDumpFile() const;

    // Check if saved dumpfile path is valid
    bool validDumpFile() const;

    // Wait for client to crashdump or exit or timeout in ms. |timeout| of -1
    // for no timeout.
    // Returns -1 if parent dies unexpectedly or timeout
    // Return ms time taken to receive crashdump otherwise
    int64_t waitForDumpFile(int clientpid, int timeout_ms = -1);

    // Platform specific start of the crash server
    // Returns false if already started or not successful
    virtual bool startCrashServer(const std::string& pipe) = 0;

    // Platform specific stop of the crash server
    // Returns false if not started
    virtual bool stopCrashServer() = 0;

    // Processes the saved dumpfile with breakpad processor libraries
    // Contains stack trace and loaded modules.
    // Is parsed for suggestions on debugging
    // (driver update etc)
    std::string getCrashDetails();

    // Collects system info, resulting in the "full" dump file.
    std::string collectSysInfo();

    // Uploads system-specific hardware information.
    virtual std::string getHWInfo() = 0;
    virtual void cleanupHWInfo() = 0;

    // Uploads the saved dumpfile to server defined by CrashSystem
    bool uploadCrash(const std::string& url);

    // Get the crash report ID returned by the crash servers
    std::string getReportId() const;

    // Factory method
    static CrashService* makeCrashService(const std::string& version,
                                          const std::string& build);

    // Get the stack dump (for parsing, and generating user suggestions)
    std::string getInitialDump() const;

    google_breakpad::ProcessState process_state;

protected:
    // Initialize serverstate
    void initCrashServer();

    // Platform specific set instance client
    // Returns false if invalid pid
    virtual bool setClient(int clientpid);

    // Platform specific check if instance client is still running
    // Returns false if client not set or not running
    virtual bool isClientAlive() = 0;

    DumpRequestContext mDumpRequestContext;
    ServerState mServerState;
    int mClientPID = 0;

    std::string mHWTmpFilePath;


private:
    CrashService();
    std::string mVersion;
    std::string mBuild;
    std::string mVersionId;
    std::string mDumpFile;
    std::string mReportId;
    std::string mDumpDetails; // Initial stack trace and live library dump from breakpad
    std::string mHWInfo; // System-specific information
};

// The Suggestion type represents possible suggestions
// presented to the user upon
// analyzing a crash report.
enum Suggestion {
    // TODO: Add more suggestion types
    // (Update OS, Get more RAM, etc)
    // as we find them
    UpdateGfxDrivers
};

// Class UserSuggestions parses and generates user suggestions.

class UserSuggestions {
public:
    std::set<Suggestion> suggestions;

    UserSuggestions(google_breakpad::ProcessState* process_state);
};

}  // namespace crashreport
}  // namespace android
