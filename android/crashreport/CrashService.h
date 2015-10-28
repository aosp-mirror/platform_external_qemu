/* Copyright (C) 2011 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#pragma once

#include <string>
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
//    The dump can be processed by locally by getCrashDetails
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
    std::string getDumpFile(void);

    // Check if saved dumpfile path is valid
    bool validDumpFile(void);

    // Wait for client to crashdump or exit or timeout in ms
    // Returns -1 if parent dies unexpectedly or timeout
    // Return ms time taken to receive crashdump otherwise
    int64_t waitForDumpFile(int clientpid, int timeout = -1);

    // Platform specific start of the crash server
    // Returns false if already started or not successful
    virtual bool startCrashServer(const std::string pipe) = 0;

    // Platform specific stop of the crash server
    // Returns false if not started
    virtual bool stopCrashServer(void) = 0;

    // Processes the saved dumpfile with breakpad processor libraries
    const std::string getCrashDetails(void);

    // Uploads the saved dumpfile to server defined by CrashSystem
    bool uploadCrash(const std::string& url);

    // Get the crash report ID returned by the crash servers
    std::string getReportId(void);

    // Factory method
    static CrashService* makeCrashService(const std::string& version,
                                          const std::string& build);

protected:
    // Initialize serverstate
    void initCrashServer(void);

    // Platform specific check if parent process still alive
    virtual bool isClientAlive(const int pid) = 0;

    DumpRequestContext mDumpRequestContext;
    ServerState mServerState;

private:
    CrashService();
    std::string mVersion;
    std::string mBuild;
    std::string mVersionId;
    std::string mDumpFile;
    std::string mReportId;
};

}  // namespace crashreport
}  // namespace android
