// Copyright (C) 2018 The Android Open Source Project
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
#include "android/emulation/control/snapshot/TarStream.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_map>
#include "android/base/Log.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"


/* set to 1 for very verbose debugging */
#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("TarStream: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define DD_BUF(fd, buf, len)                            \
    do {                                                \
        printf("TarStream (%d):", fd);                 \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x]))                   \
                printf("%c", buf[x]);                   \
            else                                        \
                printf("[0x%02x]", 0xff & (int)buf[x]); \
        }                                               \
        printf("\n");                                   \
    } while (0)

#else
#define DD(...) (void)0
#define DD_BUF(...) (void)0
#endif

using android::base::System;
using PaxMap = std::unordered_map<std::string, std::string>;

namespace android {
namespace emulation {
namespace control {

// The chksum field represents the simple sum of all bytes in the header block.
// Each 8-bit byte in the header is added to an unsigned integer, initialized to
// zero, the precision of which shall be no less than seventeen bits. When
// calculating the checksum, the chksum field is treated as if it were all
// blanks.
unsigned tar_cksum(void* data) {
    uint32_t i, cksum = 8 * ' ';

    // Mind the gap at the chksum offset!
    for (i = 0; i < 500; i += (i == 147) ? 9 : 1)
        cksum += ((char*)data)[i];

    return cksum;
};

// Convert from octal to int.
static uint64_t OTOI(octalnr* in, uint8_t len) {
    std::string oct(in, len);
    return std::stoull(oct, nullptr, 8);
}
// convert from int to octal (or base-256)
static void itoo(char* str, uint8_t len, unsigned long long val) {
    // Do we need binary encoding?
    if (!(val >> (3 * (len - 1))))
        sprintf(str, "%0*llo", len - 1, val);
    else {
        *str = (char)128;
        while (--len)
            *++str = val >> (3 * len);
    }
}

#define ITOO(x, y) itoo(x, sizeof(x), y)
#define OTOI(x) OTOI(x, sizeof(x))

bool TarWriter::writeTarHeader(std::string name) {
    posix_header header = {0};
    if (name.size() > sizeof(header.name) - 1)
        return false;
    std::string fname = base::PathUtils::join(mCwd, name);
    struct stat sb;
    int err = android_stat(fname.c_str(), &sb);
    memcpy(header.name, name.data(), name.size());
    ITOO(header.mode, sb.st_mode);
    ITOO(header.mtime, sb.st_mtime);
    ITOO(header.size, sb.st_size);
    ITOO(header.uid, sb.st_uid);
    ITOO(header.gid, sb.st_gid);
    header.typeflag = System::get()->pathIsDir(fname) ? TarType::DIRTYPE
                                                      : TarType::REGTYPE;
    strncpy(header.magic, USTAR, sizeof(header.magic));

    // And finally we can create the checksum.
    ITOO(header.chksum, tar_cksum(&header));

    mDest.write(reinterpret_cast<char*>(&header), sizeof(header));
    return true;
}

bool TarWriter::addFileEntry(std::string name) {
    std::string fname = base::PathUtils::join(mCwd, name);
    if (!System::get()->pathIsFile(fname) || !writeTarHeader(name)) {
        return false;
    };

    std::ifstream ifs(fname, std::ios_base::in | std::ios_base::binary);
    char buf[TARBLOCK];
    char zero = 0;
    do {
        ifs.read(&buf[0], TARBLOCK);
        mDest.write(&buf[0], ifs.gcount());

        // A tar file conists of chunks of 512 bytes, so we need to
        // add some padding if we didn't write a full block..
        if (ifs.gcount() != 0 && ifs.gcount() < TARBLOCK) {
            int left = TARBLOCK - ifs.gcount();
            for (int i = 0; i < left; i++)
                mDest.write(&zero, sizeof(zero));
        }
    } while (ifs.gcount() > 0);
    return true;
}

bool TarWriter::addDirectoryEntry(std::string name) {
    std::string fname = base::PathUtils::join(mCwd, name);
    return System::get()->pathIsDir(fname) && writeTarHeader(name);
}

bool TarWriter::addDirectory(std::string path) {
    auto fullpath = base::PathUtils::join(mCwd, path);
    for (auto& name : System::get()->scanDirEntries(fullpath)) {
        auto fname = base::PathUtils::join(mCwd, name);
        if (System::get()->pathIsDir(fname)) {
            if (!addDirectoryEntry(name))
                return false;
        }
        if (System::get()->pathIsFile(fname)) {
            if (!addFileEntry(name))
                return false;
        }
    }

    return true;
}

bool TarWriter::close() {
    if (mClosed)
        return false;
    char empty[TARBLOCK] = {0};

    // At the end of the archive file there are two 512-byte blocks filled
    // with binary zeros as an end-of-file marker. A reasonable system should
    // write such end-of-file marker at the end of an archive.
    mDest.write(empty, sizeof(empty));
    mDest.write(empty, sizeof(empty));
    mDest.flush();
    return true;
}

static int roundUp(int numToRound, int multiple = TARBLOCK) {
    assert(multiple && ((multiple & (multiple - 1)) == 0));
    return (numToRound + multiple - 1) & -multiple;
}

// Parses out a pax header..
static PaxMap parsePaxHeader(std::istream& istream, int headerSize) {
    PaxMap paxHeaders;
    std::vector<char> data(headerSize);
    if (istream.read(data.data(), headerSize)) {
        static std::regex paxRegex("(\\d+) ([^=]+)=([^=]+)");
        std::stringstream ss(std::string(data.begin(), data.end()));
        std::string header;
        std::smatch m;
        std::string path;
        while (std::getline(ss, header, '\n')) {
            if (std::regex_match(header, m, paxRegex)) {
                paxHeaders[m[2]] = m[3];
            }
        }
    }

    // We need to skip to align with a tarblock..
    int skip = roundUp(headerSize) - headerSize;
    istream.ignore(skip);
    return paxHeaders;
}

TarInfo TarReader::next(TarInfo from) {
    posix_header header;
    TarInfo next;

    auto nextOffset = from.offset + roundUp(from.size);
    auto currOffset = mSrc.tellg();
    if (currOffset != nextOffset) {
        if (currOffset != -1) {
            mSrc.seekg(nextOffset);
            if (mSrc.fail()) {
                next.error = TAR_STREAM_NO_SEEK;
                return next;
            }
        }
    }
    if (!mSrc.read((char*)&header, sizeof(header))) {
        next.error = TAR_INVALID_HEADER;
        return next;
    }
    if (strncmp(header.magic, USTAR, 5) != 0 ||
        (tar_cksum(&header) != OTOI(header.chksum))) {
        next.error = TAR_INVALID_HEADER;
        return next;
    }

    next.size = OTOI(header.size);
    next.name = std::string(header.name, sizeof(header.name));

    next.mode = OTOI(header.mode);
    next.uid = OTOI(header.uid);
    next.gid = OTOI(header.gid);
    next.mtime = OTOI(header.mtime);
    next.type = static_cast<TarType>(header.typeflag);
    next.offset = mSrc.tellg();

    // Mainly here to appease those who are using a mac to
    // tar up their things..  (i.e. unittests.)
    if (header.typeflag == XHDTYPE) {
        // So for compatibility reasons these end up as "files"
        // we don't want that. So let's pull out the header..
        auto pax = parsePaxHeader(mSrc, next.size);
        next = TarReader::next(next);
        if (!next.valid) {
            return next;
        }

        if (pax.count("path")) {
            next.name = pax["path"];
        }
    }
    next.valid = true;
    return next;
}

bool TarReader::extract(TarInfo src) {
    if (!src.valid)
        return false;

    // Seek if the underlying stream supports it..
    auto currOffset = mSrc.tellg();
    if (currOffset != src.offset) {
        if (currOffset != -1) {
            mSrc.seekg(src.offset);
            if (mSrc.fail()) {
                return false;
            }
        }
    }

    path_mkdir_if_needed(mCwd.c_str(), 0700);
    std::string fname = base::PathUtils::join(mCwd, src.name);
    DD("Size: %lu, name:%s", src.size, fname.c_str());
    if (src.type == DIRTYPE) {
        return ::android_mkdir(fname.c_str(), src.mode) == 0;
    } else {
        std::ofstream ofs(fname, std::ios_base::out | std::ios_base::binary);

        char buf[TARBLOCK];
        int left = src.size, rd = 0;
        if (left > 0)
            // Note, we (technically) can always read a tar block.
            do {
                mSrc.read(buf, sizeof(buf));
                rd = mSrc.gcount();
                ofs.write(buf, std::min<int>(left, rd));
                left -= rd;
                DD("Left: %d, rd: %d", left, rd);
                if (rd == 0) {
                    LOG(WARNING) << "Unexpexted eof!";
                    return false;
                }
            } while (left > 0 && rd > 0);
    }

    // We currently don't care about the stats.. so we ignore it..
    // TODO(jansene): If you implement this keep in mind that this can
    // can have unexpected side effects for the snapshot api!
    // as someone could upload an unknown gid for example.
    // chown(...)
    // utime(...)
    // chmod(...)
    return true;
}

}  // namespace control
}  // namespace emulation
}  // namespace android