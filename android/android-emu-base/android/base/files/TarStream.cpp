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
#include "android/base/files/TarStream.h"

#include <assert.h>  // for assert
#include <stdio.h>   // for sprintf
#include <string.h>  // for memcpy, strncmp, strncpy

#include <algorithm>      // for min
#include <fstream>        // for basic_istream, getline
#include <regex>          // for regex_match, match_results
#include <sstream>        // for stringstream
#include <string>         // for char_traits<>::pos_type
#include <unordered_map>  // for unordered_map, unordered_m...
#include <vector>         // for vector<>::iterator, vector

#include "android/base/Log.h"              // for LOG, LogMessage
#include "android/base/files/PathUtils.h"  // for PathUtils
#include "android/base/system/System.h"    // for System
#include "android/utils/file_io.h"         // for android_mkdir, android_stat
#include "android/utils/path.h"            // for path_mkdir_if_needed

/* set to 1 for very verbose debugging */
#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("TarStream: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define DD_BUF(fd, buf, len)                            \
    do {                                                \
        printf("TarStream (%d):", fd);                  \
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
namespace base {

// The chksum field represents the simple sum of all bytes in the header block.
// Each 8-bit byte in the header is added to an unsigned integer, initialized to
// zero, the precision of which shall be no less than seventeen bits. When
// calculating the checksum, the chksum field is treated as if it were all
// blanks.
static unsigned tar_cksum(void* data) {
    uint32_t i, cksum = 8 * ' ';

    // Mind the gap at the chksum offset!
    for (i = 0; i < 500; i += (i == 147) ? 9 : 1)
        cksum += ((char*)data)[i];

    return cksum;
};

static bool is_eof(void* data) {
    for (int i = 0; i < TARBLOCK; i++) {
        if (((char*)data)[i] != 0)
            return false;
    }
    return true;
}

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

bool TarWriter::error(std::string msg) {
    setstate(std::ios_base::failbit);
    mErrMsg = msg;
    DD("msg: %s", msg.c_str());
    assert(fail());
    return false;
}

bool TarWriter::writeTarHeader(std::string name, bool isDir, struct stat sb) {
    if (eof()) {
        return error("Refusing to add to closed archive.");
    }
    posix_header header = {0};
    if (name.size() > sizeof(header.name) - 1)
        return false;

    memcpy(header.name, name.data(), name.size());
    ITOO(header.mode, sb.st_mode);
    ITOO(header.mtime, sb.st_mtime);
    ITOO(header.uid, sb.st_uid);
    ITOO(header.gid, sb.st_gid);

    if (isDir) {
        header.typeflag = TarType::DIRTYPE;
        ITOO(header.size, 0);
    } else {
        header.typeflag = TarType::REGTYPE;
        ITOO(header.size, sb.st_size);
    }
    strncpy(header.magic, USTAR, sizeof(header.magic));

    // And finally we can create the checksum.
    ITOO(header.chksum, tar_cksum(&header));

    mDest.write(reinterpret_cast<char*>(&header), sizeof(header));
    if (mDest.bad()) {
        return error("Failed to write header for " + name +
                     ", internal stream error..");
    }
    return true;
}

bool TarWriter::writeTarHeader(std::string name) {
    posix_header header = {0};
    if (name.size() > sizeof(header.name) - 1)
        return false;
    std::string fname = base::PathUtils::join(mCwd, name);
    struct stat sb;
    if (android_stat(fname.c_str(), &sb) != 0) {
        return error("Unable to stat " + fname);
    }

    return writeTarHeader(name, System::get()->pathIsDir(fname), sb);
}

TarWriter::TarWriter(std::string cwd, std::ostream& dest, size_t bufferSize)
    : mDest(dest), mCwd(cwd), mBufferSize(bufferSize) {
    init(dest.rdbuf());
    assert(good());
}

TarWriter::~TarWriter() {
    if (!mClosed) {
        close();
    }
}

bool TarWriter::addFileEntryFromStream(std::istream& ifs,
                                       std::string name,
                                       struct stat sb) {
    if (!writeTarHeader(name, false, sb)) {
        return false;
    }

    char buf[TARBLOCK];
    do {
        ifs.read(&buf[0], TARBLOCK);
        mDest.write(&buf[0], ifs.gcount());
        if (mDest.bad()) {
            return error("Failed to write " + name + " to stream");
        }
    } while (ifs.gcount() > 0);

    size_t bytes_to_write = sb.st_size;
    int padding = TARBLOCK - (bytes_to_write % TARBLOCK);
    if (padding != TARBLOCK) {
        char empty[TARBLOCK] = {0};
        mDest.write(empty, padding);
        if (mDest.bad()) {
            return error("Failed to write " + std::to_string(padding) +
                         " padding bytes for entry " + name + " to stream");
        }
    }
    return true;
}

bool TarWriter::addFileEntry(std::string name) {
    std::string fname = base::PathUtils::join(mCwd, name);
    if (!System::get()->pathIsFile(fname)) {
        return error("Refusing to add: " + fname + ", it is not a file.");
    };

    struct stat sb;
    if (android_stat(fname.c_str(), &sb) != 0) {
        return error("Unable to stat " + fname);
    }

    std::ifstream ifs(fname, std::ios_base::in | std::ios_base::binary);
    char readBuffer[mBufferSize];
    if (mBufferSize != 0) {
        ifs.rdbuf()->pubsetbuf(readBuffer, mBufferSize);
    }

    return addFileEntryFromStream(ifs, name, sb);
}

bool TarWriter::addDirectoryEntry(std::string name) {
    std::string fname = base::PathUtils::join(mCwd, name);
    if (!System::get()->pathIsDir(fname)) {
        return error("Refusing to add " + fname + " is not a directory.");
    }

    return writeTarHeader(name);
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
    if (eof())
        return true;
    char empty[TARBLOCK] = {0};

    // At the end of the archive file there are two 512-byte blocks filled
    // with binary zeros as an end-of-file marker. A reasonable system should
    // write such end-of-file marker at the end of an archive.
    mDest.write(empty, sizeof(empty));
    mDest.write(empty, sizeof(empty));
    mDest.flush();
    if (mDest.bad()) {
        return error("Failed to write closing headers to destination");
    }
    setstate(std::ios_base::eofbit);
    return true;
}

static int64_t roundUp(int64_t numToRound, int64_t multiple = TARBLOCK) {
    assert(multiple && ((multiple & (multiple - 1)) == 0));
    return (numToRound + multiple - 1) & -multiple;
}

// Parses out a pax header..
static PaxMap parsePaxHeader(std::istream& istream, int headerSize) {
    PaxMap paxHeaders;
    std::vector<char> data(headerSize);
    istream.read(data.data(), headerSize);
    // Errors will be caught later on..
    if (!istream.good() || istream.gcount() != headerSize) {
        return paxHeaders;
    }

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

    // We need to skip to align with a tarblock..
    int skip = roundUp(headerSize) - headerSize;
    istream.ignore(skip);
    return paxHeaders;
}

TarReader::TarReader(std::string cwd, std::istream& src, bool has_seek)
    : mSrc(src), mCwd(cwd), mSeek(has_seek) {
    init(src.rdbuf());
    setstate(std::ios_base::goodbit);
    if (path_mkdir_if_needed(mCwd.c_str(), 0700) != 0) {
        error("Failed to create: " + mCwd);
    }
    assert(good());
}

bool TarReader::error(std::string msg) {
    setstate(std::ios_base::failbit);
    mErrMsg = msg;
    DD("msg: %s", msg.c_str());
    assert(fail());
    return false;
}

// seekg to the position, if the underlying stream supports it..
void TarReader::seekg(std::streampos pos) {
    if (!mSeek)
        return;
    auto currOffset = mSrc.tellg();
    if (currOffset != pos) {
        mSrc.seekg(pos);
        if (!mSrc.good()) {
            error("Failed to seek to " + std::to_string(pos));
        }
    }
}

TarInfo TarReader::next(TarInfo from) {
    posix_header header;
    TarInfo next;

    auto nextOffset = from.offset + roundUp(from.size);
    seekg(nextOffset);
    mSrc.read((char*)&header, sizeof(header));

    if (mSrc.good() && mSrc.gcount() != sizeof(header)) {
        error("Failed to read the next tar header, only read: " +
              std::to_string(mSrc.gcount()) + " bytes");
        return next;
    }

    // At the end of the archive file there are two 512-byte blocks filled with
    // binary zeros as an end-of-file marker. A reasonable system should write
    // such end-of-file marker at the end of an archive, but must not assume
    // that such a block exists when reading an archive
    if (mSrc.eof()) {
        setstate(std::ios_base::eofbit);
        DD("eof");
        return next;
    }

    if (is_eof(&header)) {
        setstate(std::ios_base::eofbit);
        DD("eof due to empty header");
        return next;
    }

    // Do some sanity checking..
    if (strncmp(header.magic, USTAR, 5)) {
        error("Incorrect tar header. Expected ustar format not: " +
              std::string(header.magic, sizeof(header.magic)));
        return next;
    }

    if (tar_cksum(&header) != OTOI(header.chksum)) {
        error("Incorrect tar header. Invalid checksum: " +
              std::to_string(OTOI(header.chksum)) +
              " != " + std::to_string(tar_cksum(&header)));
        return next;
    }

    // Guess we are looking good..
    next.size = OTOI(header.size);
    next.name = std::string(header.name);

    next.mode = OTOI(header.mode);
    next.uid = OTOI(header.uid);
    next.gid = OTOI(header.gid);
    next.mtime = OTOI(header.mtime);
    next.type = static_cast<TarType>(header.typeflag);
    next.offset = mSrc.tellg();

    // Mainly here to appease those who are using a mac to
    // tar up their things..  (i.e. unittests on mac.)
    if (header.typeflag == XHDTYPE) {
        // So for compatibility reasons these end up as "files"
        // we don't want that. So let's pull out the pax header..
        auto pax = parsePaxHeader(mSrc, next.size);
        next = TarReader::next(next);
        if (!next.valid) {
            error("Failed to extract extended pax header, due to: " + mErrMsg);
            return next;
        }

        // We only use the "path" extension.
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
    seekg(src.offset);
    std::string fname = base::PathUtils::join(mCwd, src.name);
    DD("Size: %lu, name:%s", src.size, fname.c_str());
    if (src.type == DIRTYPE) {
        if (android_mkdir(fname.c_str(), src.mode) != 0) {
            return error("Failed to create: " + src.name + " in " + mCwd);
        };
    }

    int64_t left = src.size, rd = 0;

    // Okay, let's create and extract a regular file..
    std::ofstream ofs(fname, std::ios_base::out | std::ios_base::binary |
                                     std::ios_base::trunc);

    // File of 0 length.. we are done!
    if (left == 0) {
        return true;
    }

    char buf[TARBLOCK];

    // Note, we (technically) can always read a tar block.
    // And it is expected that a file will always end at a
    // 512 bit boundary.. A file of 10 bytes for example would
    // be padded by 502 bytes of zeroes.
    do {
        mSrc.read(buf, sizeof(buf));
        rd = mSrc.gcount();
        if (rd == 0) {
            return error("Unexpected EOF during extraction of " + src.name +
                         " at offset " +
                         std::to_string(src.offset + (src.size - left)));
        }

        // Make sure we don't write out the padding 0 bytes.
        ofs.write(buf, std::min<int64_t>(left, rd));
        left -= rd;
    } while (left > 0 && rd > 0);

    // We currently don't care about the stats.. so we ignore it..
    // TODO(jansene): If you implement this keep in mind that this can
    // can have unexpected side effects for the snapshot api!
    // as someone could upload an unknown gid for example, or a gid/ mode
    // we are not allowed to read, etc, etc..
    // chown(...)
    // utime(...)
    // chmod(...)
    return true;
}

}  // namespace base
}  // namespace android
