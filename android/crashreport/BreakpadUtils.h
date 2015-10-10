#include "android/base/String.h"
#include "android/base/misc/StringUtils.h"
#include <string>

using namespace android::base;

struct BreakpadUtils {
public:
    BreakpadUtils();
    const std::string& getCrashDirectory(void);
    const std::string& getEmulatorPath(void);
#ifdef _WIN32
    const std::wstring& getWCrashDirectory(void);
#endif
    const std::string& getCaBundlePath(void);
    StringVector& getCrashCmdLine(void);
    bool validatePaths(void);
    static BreakpadUtils* get(void);
    static bool isDump(const std::string &str);
    static const std::string getIniFilePath(const std::string& minidumpFilePath);
    static bool dumpIniFile(const std::string& minidumpFilePath);
    static bool uploadMiniDump(const std::string& minidumpFilePath);
    static void cleanupMiniDump(const std::string& minidumpFilePath);
private:
    static size_t WriteCallback(void *ptr, size_t size, size_t nmemb, void *userp);

public:
    static const char* kCrashSubDir;
    static const char* kEmulatorBinary;
    static const char* kEmulatorCrashArg;
    static const char* kCrashURL;
private:
    static const char* kNameKey;
    static const char* kVersionKey;
    static const char* kName;
    static const char* kVersion;

    static const std::string kDumpSuffix;
    std::string mCaBundlePath;
    std::string mEmulatorPath;
    StringVector mCrashCmdLine;
    std::string mCrashDir;
#ifdef _WIN32
    std::wstring mWCrashDir;
#endif
};
