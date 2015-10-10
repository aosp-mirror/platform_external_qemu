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

public:
    static const char* kCrashSubDir;
    static const char* kEmulatorBinary;
    static const char* kEmulatorCrashArg;
private:
    static const std::string kDumpSuffix;
    std::string mCaBundlePath;
    std::string mEmulatorPath;
    StringVector mCrashCmdLine;
    std::string mCrashDir;
#ifdef _WIN32
    std::wstring mWCrashDir;
#endif
};
