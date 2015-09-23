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

// Some free functions for manipulating Strings as URIs. Wherever possible,
// these functions take const references to StringView to avoid unnecessary
// copies.

#include "android/base/Uri.h"

#include "android/base/Limits.h"
#include "android/base/StringFormat.h"

#include <set>
#include <stdlib.h>

namespace android {
namespace base {

using std::set;

// Pre C++11 there isn't a nice initialization syntax for sets.
static set<unsigned char> populateReservedChars() {
    set<unsigned char> reservedChars;
    reservedChars.insert('!');
    reservedChars.insert('#');
    reservedChars.insert('$');
    reservedChars.insert('&');
    reservedChars.insert('\'');
    reservedChars.insert('(');
    reservedChars.insert(')');
    reservedChars.insert('*');
    reservedChars.insert('+');
    reservedChars.insert(',');
    reservedChars.insert('/');
    reservedChars.insert(':');
    reservedChars.insert(';');
    reservedChars.insert('=');
    reservedChars.insert('?');
    reservedChars.insert('@');
    reservedChars.insert('[');
    reservedChars.insert(']');
    reservedChars.insert(' ');
    reservedChars.insert('%');
    return reservedChars;
}

// static
String Uri::Encode(const StringView& uri) {
    static const set<unsigned char> reservedChars = populateReservedChars();
    static const set<unsigned char>::const_iterator notFoundIt =
            reservedChars.end();
    String encodedUri;
    for (StringView::const_iterator cit = uri.begin(); cit != uri.end();
         ++cit) {
        if (reservedChars.find(*cit) != notFoundIt) {
            encodedUri.append('%');
            encodedUri.append(StringFormat("%02X", *cit));
        } else {
            encodedUri.append(*cit);
        }
    }
    return encodedUri;
}

// static
String Uri::Decode(const StringView& uri) {
    String decodedUri;
    for (StringView::const_iterator cit = uri.begin(); cit < uri.end(); ++cit) {
        if (*cit == '%') {
            char hex[3];
            char decodedChar;

            ++cit;
            if (cit == uri.end()) {
                return "";
            }
            hex[0] = *cit;
            ++cit;
            if (cit == uri.end()) {
                return "";
            }
            hex[1] = *cit;

            hex[2] = '\0';
            decodedChar = static_cast<char>(strtoul(hex, NULL, 16));
            if (decodedChar == 0) {
                return "";
            }
            decodedUri.append(decodedChar);
        } else {
            decodedUri.append(*cit);
        }
    }
    return decodedUri;
}

}  // namespace base
}  // namespace android
