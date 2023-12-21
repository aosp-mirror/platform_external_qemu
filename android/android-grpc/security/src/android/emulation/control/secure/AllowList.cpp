// Copyright (C) 2022 The Android Open Source Project
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
#include "android/emulation/control/secure/AllowList.h"

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "android/utils/debug.h"
#include "nlohmann/json.hpp"
#include "re2/re2.h"

namespace android {
namespace emulation {
namespace control {

enum class AllowGroup {
    None,    // No auth needed
    Green,   // Ok, if token present
    Yellow,  // Ok, if token present and proper aud claim.
};

using regex = re2::RE2;
using iss = std::string;

using AccessList = std::vector<std::unique_ptr<regex>>;
using AllowMap = std::unordered_map<iss, AccessList>;

using AllowSet = std::unordered_set<std::string>;
using AllowCache = std::unordered_map<iss, AllowSet>;

using SecurityMap = std::unordered_map<AllowGroup, AllowMap>;
using RejectionCache = std::unordered_map<AllowGroup, AllowCache>;
using AcceptCache = std::unordered_map<AllowGroup, AllowCache>;

using json = nlohmann::json;

// An allowlist that will cache responses, making sure we can validate
// subjects quickly once they have tried to access an endpoint.
class CachingAllowList : public AllowList {
public:
    static constexpr char const* UNPROTECTED = "__everyone__";

    CachingAllowList(SecurityMap securityMap)
        : mSecurityMap(std::move(securityMap)) {}

    bool requiresAuthentication(std::string_view path) override {
        return !isAllowed(AllowGroup::None, UNPROTECTED, path);
    };

    bool isAllowed(std::string_view sub, std::string_view path) override {
        return isAllowed(AllowGroup::Green, sub, path);
    }

    bool isProtected(std::string_view sub, std::string_view path) override {
        return isAllowed(AllowGroup::Yellow, sub, path);
    }

private:
    // Inserts an entry into the cache, expunging a random element
    // in case of overflow.
    void insert(std::unordered_map<AllowGroup, AllowCache>& cache,
                AllowGroup color,
                std::string iss,
                std::string path) {
        constexpr int MAX_URI_CACHE = 256;
        // Prevent cache overflow..

        if (cache[color][iss].size() >= MAX_URI_CACHE) {
            // Randomly remove an element
            auto& set = cache[color][iss];
            auto it = set.begin();
            std::advance(it, rand() % set.size());

            VERBOSE_PRINT(grpc, "Cache overflow, removing %s", it->c_str());
            set.erase(it);
        }

        cache[color][iss].insert(path);
    }

    bool isAllowed(AllowGroup color,
                   std::string_view subView,
                   std::string_view pathView) {
        std::lock_guard<std::mutex> lock(mAllowCheck);

        std::string sub = std::string(subView.data(), subView.length());
        if (!mSecurityMap[color].count(sub)) {
            dwarning("Unknown subject %s is requesting access.", sub.c_str());
            return false;
        }

        std::string path = std::string(pathView.data(), pathView.length());
        if (mRejectionCache[color][sub].count(path)) {
            return false;
        }

        if (mAcceptCache[color][sub].count(path)) {
            return true;
        }

        for (const auto& regex : mSecurityMap[color][sub]) {
            if (regex::FullMatch(path, *regex)) {
                VERBOSE_PRINT(grpc,
                              "%s is in %s group, access to %s is allowed, "
                              "caching uri",
                              sub.c_str(), colorStr(color), path.c_str());
                insert(mAcceptCache, color, sub, path);
                return true;
            }
        }

        VERBOSE_PRINT(grpc,
                      "%s is in %s group, access to %s is denied, "
                      "caching uri",
                      sub.c_str(), colorStr(color), path.c_str());
        insert(mRejectionCache, color, sub, path);
        return false;
    }

    const char* colorStr(AllowGroup group) {
        switch (group) {
            case AllowGroup::None:
                return "unprotected";
            case AllowGroup::Green:
                return "allowed";
            case AllowGroup::Yellow:
                return "protected";
        }
    }

    std::mutex mAllowCheck;
    SecurityMap mSecurityMap;
    RejectionCache mRejectionCache;
    AcceptCache mAcceptCache;
};

AccessList parseAccessList(json regexList) {
    AccessList access;
    for (const auto& entry : regexList) {
        auto expr = entry.get<std::string>();
        auto re = std::make_unique<regex>(expr);
        if (!re->ok()) {
            dwarning("Ignoring invalid regex: %s, error: %s", expr.c_str(),
                     re->error().c_str());
        } else {
            VERBOSE_PRINT(grpc, "   %s", re->pattern());
            access.push_back(std::move(re));
        }
    }
    return access;
}

std::unique_ptr<AllowList> parseJsonObject(const json& jsonObject) {
    if (jsonObject.is_discarded()) {
        derror("The json is invalid, access disabled!");
        return std::make_unique<DisableAccess>();
    }

    SecurityMap secure;
    std::vector<std::unique_ptr<regex>> unprotected;
    if (jsonObject.count("unprotected")) {
        VERBOSE_PRINT(grpc, "Open calls: ");
        secure[AllowGroup::None][CachingAllowList::UNPROTECTED] =
                parseAccessList(jsonObject["unprotected"]);
    }

    if (jsonObject.count("allowlist")) {
        for (const auto& entry : jsonObject["allowlist"]) {
            if (!entry.count("iss")) {
                dwarning(
                        "Invalid allow list. Missing \"iss\" claim, "
                        "skipping "
                        "entry: %s",
                        entry.dump(2).c_str());
                continue;
            }

            if (!entry.count("allowed") && !entry.count("protected")) {
                dwarning(
                        "Invalid allow list. Missing \"allowed\" and \"protected\" "
                        "list, "
                        "skipping "
                        "entry: %s",
                        entry.dump(2).c_str());
                continue;
            }

            auto iss = entry["iss"].get<std::string>();

            if (iss == CachingAllowList::UNPROTECTED) {
                dwarning("Skipping %s, this is a reserved issuer.",
                         CachingAllowList::UNPROTECTED);
                continue;
            }

            if (entry.count("allowed")) {
                VERBOSE_PRINT(grpc, "Green list for iss: %s", iss)
                secure[AllowGroup::Green][iss] =
                        parseAccessList(entry["allowed"]);
            }

            if (entry.count("protected")) {
                VERBOSE_PRINT(grpc, "Yellow list for iss: %s", iss);
                secure[AllowGroup::Yellow][iss] =
                        parseAccessList(entry["protected"]);
            }
        }
    }

    return std::make_unique<CachingAllowList>(std::move(secure));
}

std::unique_ptr<AllowList> AllowList::fromJson(
        std::string_view jsonWithComments) {
    return parseJsonObject(json::parse(jsonWithComments, nullptr, false, true));
}

std::unique_ptr<AllowList> AllowList::fromStream(
        std::istream& jsonWithComments) {
    if (!jsonWithComments.good()) {
        dwarning("Unable to access file!");
    }
    return parseJsonObject(json::parse(jsonWithComments, nullptr, false, true));
}

}  // namespace control
}  // namespace emulation
}  // namespace android
