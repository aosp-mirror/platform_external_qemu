// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "VersionExtractor.h"

#include "android/base/Compiler.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/utils/debug.h"
#include "android/version.h"
#include "config-host.h"

#include <libxml/tree.h>

#include <errno.h>
#include <stdlib.h>

#include <memory>

namespace android {
namespace update_check {

using android::base::Version;

const char* const VersionExtractor::kXmlNamespace =
        "http://schemas.android.com/sdk/android/repository/10";

namespace {

struct DeleteXmlObject {
    template <class T>
    void operator()(T t) const {
        xmlFree(t);
    }

    void operator()(xmlDocPtr doc) const {
        xmlFreeDoc(doc);
    }
};

}

template <class T>
using xmlAutoPtr = android::base::ScopedPtr<T, DeleteXmlObject>;

static int parseInt(const xmlChar* str) {
    char* end;
    errno = 0;
    const int value = strtol(reinterpret_cast<const char*>(str), &end, 10);
    if (errno || *end != 0) {
        dwarning("UpdateCheck: invalid value of a version attribute");
        return -1;
    }
    return value;
}

static Version parseVersion(xmlNodePtr node, const xmlChar* ns) {
    xmlNodePtr revNode = node->children;
    for (; revNode; revNode = revNode->next) {
        if (xmlStrcmp(revNode->name, BAD_CAST "revision") == 0) {
            break;
        }
    }
    if (!revNode) {
        return Version::Invalid();
    }

    int major = -1, minor = -1, micro = -1;

    for (xmlNodePtr verPart = revNode->children; verPart;
         verPart = verPart->next) {
        // libxml2 has the following structure of the node content:
        // <a>aaa</a> ->
        // xmlNode(name = "a", type=element_node)
        //      -> first child xmlNode(name = "text", type=element_text)->content
        if (verPart->type != XML_ELEMENT_NODE || !verPart->children ||
            verPart->children->type != XML_TEXT_NODE || !verPart->children->content) {
            continue;
        }

        const int val = parseInt(verPart->children->content);
        if (xmlStrcmp(verPart->name, BAD_CAST "major") == 0) {
            major = val;
        } else if (xmlStrcmp(verPart->name, BAD_CAST "minor") == 0) {
            minor = val;
        } else if (xmlStrcmp(verPart->name, BAD_CAST "micro") == 0) {
            micro = val;
        }
    }

    if (major < 0 || minor < 0 || micro < 0) {
        return Version::Invalid();
    }

    return Version(major, minor, micro);
}

Version VersionExtractor::extractVersion(const std::string& data) const {
    // make sure libxml2 is initialized and is of proper version
    LIBXML_TEST_VERSION

    const xmlAutoPtr<xmlDoc> doc(
            xmlReadMemory(data.c_str(), data.size(), "none.xml", nullptr, 0));
    if (!doc) {
        return Version::Invalid();
    }

    const xmlNodePtr root = xmlDocGetRootElement(doc.get());
    if (!root->ns ||
        xmlStrcmp(root->ns->href, BAD_CAST kXmlNamespace) != 0) {
        return Version::Invalid();
    }

    Version ver = Version::Invalid();

    // iterate over all nodes in the document and find all <tool> ones
    for (xmlNodePtr node = root->children; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) {
            continue;
        }
        if (xmlStrcmp(node->name, BAD_CAST "tool") != 0) {
            continue;
        }

        const Version nodeVer = parseVersion(node, root->ns->href);
        if (nodeVer.isValid() && (!ver.isValid() || ver < nodeVer)) {
            ver = nodeVer;
        }
    }

    return ver;
}

Version VersionExtractor::getCurrentVersion() const {
    static const Version currentVersion = Version(VERSION_STRING_SHORT);
    return currentVersion;
}

}  // namespace update_check
}  // namespace android
