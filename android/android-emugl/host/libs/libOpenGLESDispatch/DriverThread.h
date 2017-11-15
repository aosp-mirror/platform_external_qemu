#pragma once

#include <functional>

class DriverThreadWorker;

class DriverThread {
public:
    struct ContextInfo {
        uintptr_t read;
        uintptr_t draw;
        uintptr_t context;
    };

    using ContextQueryCallback = std::function<ContextInfo()>;
    using ContextChangeCallback = std::function<bool(const ContextInfo& info)>;

    DriverThread();

    static void registerContextQueryCallback(const ContextQueryCallback& c);
    static void registerContextChangeCallback(const ContextChangeCallback& c);

    static void initWorker();
    static DriverThreadWorker* getWorker();
    static void setWorker(DriverThreadWorker*);

    static bool useDriverThread();

    template <class T>
    static T callOnDriverThread(std::function<T()> call, bool changeContext = true);

    template <class T>
    static T callOnDriverThreadAsync(std::function<T()> call, bool changeContext = true);

    static void callOnDriverThreadSync(std::function<void()> call, bool changeContext = true);

    static bool makeCurrent(const ContextInfo& context);

    static bool makeCurrentToCallingThread(const ContextInfo& context);
    static ContextInfo getCallingThreadContextInfo();
};
