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

#include "android/network/GenericNetlinkMessage.h"

#include "aemu/base/Log.h"

#include <algorithm>
#include <cstring>

namespace {

using android::network::genlmsghdr;
using android::network::nlmsghdr;

static constexpr uint32_t NLMSG_ALIGNTO = 4U;

static constexpr unsigned int nlmsg_align(unsigned int len) {
    return ((len) + NLMSG_ALIGNTO - 1) & ~(NLMSG_ALIGNTO - 1);
}

static constexpr int NLMSG_HDRLEN = nlmsg_align(sizeof(nlmsghdr));

static constexpr int GENL_HDRLEN = nlmsg_align(sizeof(genlmsghdr));

static constexpr int NLA_F_NESTED = (1 << 15);

static constexpr int NLA_F_NET_BYTEORDER = (1 << 14);

static constexpr int NLA_TYPE_MASK = ~(NLA_F_NESTED | NLA_F_NET_BYTEORDER);

static constexpr uint32_t NLA_ALIGNTO = 4U;

struct nlattr {
    uint16_t nla_len;
    uint16_t nla_type;
};

static int nlmsg_size(int payload) {
    return NLMSG_HDRLEN + payload;
}

static int nlmsg_msg_size(int payload) {
    return nlmsg_size(payload);
}

static int nlmsg_valid_hdr(const struct nlmsghdr* nlh, int hdrlen) {
    if (nlh->nlmsg_len < nlmsg_msg_size(hdrlen))
        return 0;

    return 1;
}

static constexpr unsigned int nla_align(unsigned int len) {
    return ((len) + NLA_ALIGNTO - 1) & ~(NLA_ALIGNTO - 1);
}

static constexpr int NLA_HDRLEN = nla_align(sizeof(struct nlattr));

/**
 * Return size of attribute whithout padding.
 */
int nla_attr_size(int payload) {
    return NLA_HDRLEN + payload;
}

/**
 * Return size of attribute including padding.
 */
static int nla_total_size(int payload) {
    return nla_align(nla_attr_size(payload));
}

static int nla_ok(const struct nlattr* nla, int remaining) {
    return remaining >= (int)sizeof(*nla) && nla->nla_len >= sizeof(*nla) &&
           nla->nla_len <= remaining;
}

static struct nlattr* nla_next(const struct nlattr* nla, int* remaining) {
    int totlen = nla_align(nla->nla_len);

    *remaining -= totlen;
    return (struct nlattr*)((char*)nla + totlen);
}

static int nla_type(const struct nlattr* nla) {
    return nla->nla_type & NLA_TYPE_MASK;
}

static struct nlattr* nla_find(const struct nlattr* head,
                               int len,
                               int attrtype) {
    const struct nlattr* nla;
    int rem;
    for (nla = head, rem = len; nla_ok(nla, rem); nla = nla_next(nla, &(rem))) {
        if (nla_type(nla) == attrtype)
            return (struct nlattr*)nla;
    }

    return NULL;
}

static void* nla_data(const struct nlattr* nla) {
    return (uint8_t*)nla + NLA_HDRLEN;
}

static int nla_len(const struct nlattr* nla) {
    return nla->nla_len - NLA_HDRLEN;
}

static int nla_memcpy(void* dest, const struct nlattr* src, int count) {
    int minlen = std::min(count, nla_len(src));
    std::memcpy(dest, nla_data(src), minlen);
    return minlen;
}

}  // namespace

namespace android {
namespace network {
using android::base::IOVector;

GenericNetlinkMessage::GenericNetlinkMessage(uint32_t port,
                                             uint32_t seq,
                                             int family,
                                             int hdrlen,
                                             int flags,
                                             uint8_t cmd,
                                             uint8_t version)
    : mData(NLMSG_HDRLEN + GENL_HDRLEN, 0), mUserHeaderLen(hdrlen) {
    LOG(VERBOSE) << "Generic netlink header type=" << family
                 << ", flags=" << flags << ", pid=" << port << ", seq=" << seq
                 << ", cmd =" << (int)cmd << ", version =" << (int)version;
    putHeader(port, seq, family, flags);
    auto* hdr = genericNetlinkHeader();
    hdr->cmd = cmd;
    hdr->version = version;
}

GenericNetlinkMessage::GenericNetlinkMessage(const uint8_t* data,
                                             size_t size,
                                             int hdrlen)
    : mData(data, data + size), mUserHeaderLen(hdrlen) {
    resizeByHeaderLength(size);
}

GenericNetlinkMessage::GenericNetlinkMessage(IOVector iovec, int hdrlen)
    : mData(iovec.summedLength()), mUserHeaderLen(hdrlen) {
    iovec.copyTo(mData.data(), 0, mData.size());
    resizeByHeaderLength(mData.size());
}

struct nlmsghdr* GenericNetlinkMessage::netlinkHeader() {
    return reinterpret_cast<struct nlmsghdr*>(mData.data());
}
const struct nlmsghdr* GenericNetlinkMessage::netlinkHeader() const {
    return reinterpret_cast<const struct nlmsghdr*>(data());
}

struct genlmsghdr* GenericNetlinkMessage::genericNetlinkHeader() {
    return reinterpret_cast<struct genlmsghdr*>(mData.data() + NLMSG_HDRLEN);
}

const struct genlmsghdr* GenericNetlinkMessage::genericNetlinkHeader() const {
    return reinterpret_cast<const struct genlmsghdr*>(data() + NLMSG_HDRLEN);
}

uint8_t* GenericNetlinkMessage::userHeader() {
    return mData.data() + (NLMSG_HDRLEN + GENL_HDRLEN);
}
const uint8_t* GenericNetlinkMessage::userHeader() const {
    return data() + (NLMSG_HDRLEN + GENL_HDRLEN);
}

uint8_t* GenericNetlinkMessage::userData() {
    return mData.data() +
           (NLMSG_HDRLEN + GENL_HDRLEN + nlmsg_align(mUserHeaderLen));
}

const uint8_t* GenericNetlinkMessage::userData() const {
    return data() + (NLMSG_HDRLEN + GENL_HDRLEN + nlmsg_align(mUserHeaderLen));
}

size_t GenericNetlinkMessage::userDataLen() const {
    return dataLen() -
           (NLMSG_HDRLEN + GENL_HDRLEN + nlmsg_align(mUserHeaderLen));
}

uint8_t* GenericNetlinkMessage::data() {
    return mData.data();
}

const uint8_t* GenericNetlinkMessage::data() const {
    return mData.data();
}

size_t GenericNetlinkMessage::dataLen() const {
    return mData.size();
}

// only validate netlink header and generic netlink header
bool GenericNetlinkMessage::isValid() const {
    const auto* nlh = netlinkHeader();
    if (!nlmsg_valid_hdr(nlh, mUserHeaderLen))
        return false;
    return (nlh->nlmsg_len - GENL_HDRLEN - NLMSG_HDRLEN) >=
           nlmsg_align(mUserHeaderLen);
}

void GenericNetlinkMessage::putHeader(uint32_t pid,
                                      uint32_t seq,
                                      int type,
                                      int flags) {
    // Resize std::vector to be big enough for nlmsg header, genlmsg header and
    // user header.
    mData.resize(NLMSG_HDRLEN + GENL_HDRLEN + nlmsg_align(mUserHeaderLen), 0);

    auto* nlh = netlinkHeader();
    nlh->nlmsg_type = type;
    nlh->nlmsg_flags = flags;
    nlh->nlmsg_pid = pid;
    nlh->nlmsg_seq = seq;
    nlh->nlmsg_len = dataLen();
}

void GenericNetlinkMessage::resizeByHeaderLength(size_t currentSize) {
    // resize according to nlmsghdr.
    if (isValid()) {
        auto* hdr = netlinkHeader();
        if (hdr->nlmsg_len < currentSize) {
            mData.resize(hdr->nlmsg_len);
        }
    }
}

bool GenericNetlinkMessage::getAttribute(int attributeId,
                                         void* dst,
                                         size_t size) const {
    const struct nlattr* head =
            reinterpret_cast<const struct nlattr*>(userData());
    struct nlattr* src = nla_find(head, userDataLen(), attributeId);
    if (!src) {
        LOG(VERBOSE) << "NLA attribute " << attributeId << "is not found";
    }
    return src && dst && nla_memcpy(dst, src, size) > 0;
}

struct iovec GenericNetlinkMessage::getAttribute(int attributeId) const {
    struct iovec iov = {.iov_base = nullptr, .iov_len = 0};
    const struct nlattr* head =
            reinterpret_cast<const struct nlattr*>(userData());
    struct nlattr* src = nla_find(head, userDataLen(), attributeId);
    if (!src) {
        LOG(VERBOSE) << "NLA attribute " << attributeId << "is not found";
    } else {
        iov.iov_base = nla_data(src),
        iov.iov_len = static_cast<size_t>(nla_len(src));
    }
    return iov;
}

bool GenericNetlinkMessage::putAttribute(int attributeId,
                                         const void* src,
                                         size_t size) {
    if (!src) { return false; }
    struct iovec iov = getAttribute(attributeId);
    // Copy contents if attribute already exists
    if (iov.iov_len != 0) {
        std::memcpy(iov.iov_base, src, std::min(size, iov.iov_len));
    } else {
        size_t origLen = mData.size();
        size_t totalLen = origLen + nla_total_size(size);
        mData.resize(totalLen, 0);
        auto* nlh = netlinkHeader();
        nlh->nlmsg_len = totalLen;
        struct nlattr* nla = reinterpret_cast<struct nlattr*>(data() + origLen);
        nla->nla_type = attributeId;
        nla->nla_len = nla_attr_size(size);
        std::memcpy(nla_data(nla), src, size);
    }
    return true;
}

}  // namespace network
}  // namespace android
