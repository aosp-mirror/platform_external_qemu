// Copyright 2021 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <stdint.h>
#include <vector>

#include "aemu/base/Compiler.h"
#include "aemu/base/IOVector.h"

namespace android {
namespace network {

struct nlmsghdr {
    uint32_t nlmsg_len;   /* Length of message including header */
    uint16_t nlmsg_type;  /* Message content */
    uint16_t nlmsg_flags; /* Additional flags */
    uint32_t nlmsg_seq;   /* Sequence number */
    uint32_t nlmsg_pid;   /* Sending process port ID */
} __attribute__((packed));

struct genlmsghdr {
    uint8_t cmd;
    uint8_t version;
    uint16_t reserved;
} __attribute__((packed));

/*  The generic netlink message format and payload is encoded as attributes.
 *
 *  +--------+---+----------+---+--------+-----+--------+---------+---+--------+---------+---+
 *  |nlmsghdr|pad|genlmsghdr|pad|userhdr | pad | nlattr | payload |pad| nlattr | payload |pad|
 *  +--------+---+----------+---+--------+-----+--------+---------+---+--------+---------+---+
 *                                             ^
 *               userData-----------------------
 *
 */
class GenericNetlinkMessage {
public:
    class Builder;
    GenericNetlinkMessage(uint32_t port,
                          uint32_t seq,
                          int family,
                          int hdrlen,
                          int flags,
                          uint8_t cmd,
                          uint8_t version);

    GenericNetlinkMessage(const uint8_t* data, size_t size, int hdrlen = 0);
    GenericNetlinkMessage(android::base::IOVector iovec, int hdrlen = 0);

    // will not search in nested attribute
    struct iovec getAttribute(int attributeId) const;
    bool getAttribute(int attributeId, void* dst, size_t size) const;
    // If attributeId already exists, this method will override the original
    // value
    bool putAttribute(int attributeId, const void* src, size_t size);
    struct nlmsghdr* netlinkHeader();
    const struct nlmsghdr* netlinkHeader() const;
    struct genlmsghdr* genericNetlinkHeader();
    const struct genlmsghdr* genericNetlinkHeader() const;
    uint8_t* userHeader();
    const uint8_t* userHeader() const;
    uint8_t* userData();
    const uint8_t* userData() const;
    size_t userDataLen() const;
    uint8_t* data();
    const uint8_t* data() const;
    size_t dataLen() const;
    bool isValid() const;

    static constexpr int NL_AUTO_SEQ = 0;
    static constexpr int NL_AUTO_PORT = 0;
    static constexpr int NLMSG_MIN_TYPE = 0x10;

private:
    void putHeader(uint32_t pid, uint32_t seq, int type, int flags);
    void resizeByHeaderLength(size_t size);

    std::vector<uint8_t> mData;
    size_t mUserHeaderLen;
    DISALLOW_COPY_AND_ASSIGN(GenericNetlinkMessage);
};

}  // namespace network
}  // namespace android
