// Copyright 2020 The Android Open Source Project
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

#include "android/base/Log.h"

namespace android {
namespace network {
using android::base::IOVector;

#define NLMSG_ALIGNTO	4U
#define NLMSG_ALIGN(len) ( ((len)+NLMSG_ALIGNTO-1) & ~(NLMSG_ALIGNTO-1) )
#define NLMSG_HDRLEN	 ((int) NLMSG_ALIGN(sizeof(struct nlmsghdr)))
#define GENL_HDRLEN	NLMSG_ALIGN(sizeof(struct genlmsghdr))

static int nlmsg_size(int payload)
{
	return NLMSG_HDRLEN + payload;
}

static int nlmsg_msg_size(int payload)
{
	return nlmsg_size(payload);
}

static int nlmsg_valid_hdr(const struct nlmsghdr *nlh, int hdrlen)
{
	if (nlh->nlmsg_len < nlmsg_msg_size(hdrlen))
		return 0;

	return 1;
}

#define NLA_F_NESTED		(1 << 15)
#define NLA_F_NET_BYTEORDER	(1 << 14)
#define NLA_TYPE_MASK		~(NLA_F_NESTED | NLA_F_NET_BYTEORDER)
#define NLA_ALIGNTO		4
#define NLA_ALIGN(len)		(((len) + NLA_ALIGNTO - 1) & ~(NLA_ALIGNTO - 1))
#define NLA_HDRLEN		((int) NLA_ALIGN(sizeof(struct nlattr)))

struct nlattr {
	uint16_t           nla_len;
	uint16_t           nla_type;
};

/**
 * @name Attribute Size Calculation
 * @{
 */

/**
 * Return size of attribute whithout padding.
 * @arg payload		Payload length of attribute.
 *
 * @code
 *    <-------- nla_attr_size(payload) --------->
 *   +------------------+- - -+- - - - - - - - - +- - -+
 *   | Attribute Header | Pad |     Payload      | Pad |
 *   +------------------+- - -+- - - - - - - - - +- - -+
 * @endcode
 *
 * @return Size of attribute in bytes without padding.
 */
int nla_attr_size(int payload)
{
	return NLA_HDRLEN + payload;
}

/**
 * Return size of attribute including padding.
 * @arg payload		Payload length of attribute.
 *
 * @code
 *    <----------- nla_total_size(payload) ----------->
 *   +------------------+- - -+- - - - - - - - - +- - -+
 *   | Attribute Header | Pad |     Payload      | Pad |
 *   +------------------+- - -+- - - - - - - - - +- - -+
 * @endcode
 *
 * @return Size of attribute in bytes.
 */
int nla_total_size(int payload)
{
	return NLA_ALIGN(nla_attr_size(payload));
}

#define nla_for_each_attr(pos, head, len, rem) \
	for (pos = head, rem = len; \
	     nla_ok(pos, rem); \
	     pos = nla_next(pos, &(rem)))

static int nla_ok(const struct nlattr *nla, int remaining)
{
	return remaining >= (int) sizeof(*nla) &&
	       nla->nla_len >= sizeof(*nla) &&
	       nla->nla_len <= remaining;
}

static struct nlattr *nla_next(const struct nlattr *nla, int *remaining)
{
	int totlen = NLA_ALIGN(nla->nla_len);

	*remaining -= totlen;
	return (struct nlattr *) ((char *) nla + totlen);
}

static int nla_type(const struct nlattr *nla) {
	return nla->nla_type & NLA_TYPE_MASK;
}

static struct nlattr *nla_find(const struct nlattr *head, int len, int attrtype)
{
	const struct nlattr *nla;
	int rem;

	nla_for_each_attr(nla, head, len, rem)
		if (nla_type(nla) == attrtype)
			return (struct nlattr*)nla;

	return NULL;
}

static void *nla_data(const struct nlattr *nla)
{
	return (uint8_t *) nla + NLA_HDRLEN;
}

static int nla_len(const struct nlattr *nla)
{
	return nla->nla_len - NLA_HDRLEN;
}

static int nla_memcpy(void *dest, const struct nlattr *src, int count)
{
	int minlen;

	if (!src) {
		LOG(VERBOSE) << "src is null";
		return 0;
	}
	
	minlen = std::min(count, nla_len(src));
	memcpy(dest, nla_data(src), minlen);
	
	return minlen;
}

GenericNetlinkMessage::GenericNetlinkMessage(uint32_t port, uint32_t seq, int family,
		  int hdrlen, int flags, uint8_t cmd, uint8_t version) : 
					mData(NLMSG_HDRLEN + GENL_HDRLEN, 0), 
					mUserHeaderLen(hdrlen) {
	LOG(VERBOSE) << "Added netlink header type=" << family << ", flags=" << flags << ", pid=" 
		<< port << ", seq=" << seq;
	putHeader(port, seq, family, flags);
	LOG(VERBOSE) << "Added generic netlink header cmd =" << (int)cmd << ", version =" << (int)version;
	struct genlmsghdr* hdr = genericNetlinkHeader();
	hdr->cmd = cmd;
	hdr->version = version;
}

GenericNetlinkMessage::GenericNetlinkMessage(const uint8_t* data, size_t size, int hdrlen) : 
				mData(data, data+size), mUserHeaderLen(hdrlen) {
	struct nlmsghdr* hdr = netlinkHeader();
	// resize according to nlmsghdr.
	if (isValid()) {
		if (hdr->nlmsg_len < size) {
			mData.resize(hdr->nlmsg_len);
		}
	}
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
	return mData.data() + (NLMSG_HDRLEN + GENL_HDRLEN + NLMSG_ALIGN(mUserHeaderLen));
}

const uint8_t* GenericNetlinkMessage::userData() const {
	return data() + (NLMSG_HDRLEN + GENL_HDRLEN + NLMSG_ALIGN(mUserHeaderLen));
}

size_t GenericNetlinkMessage::userDataLen() const {
	return dataLen() - (NLMSG_HDRLEN + GENL_HDRLEN + NLMSG_ALIGN(mUserHeaderLen));
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
	const struct nlmsghdr* nlh = netlinkHeader();
	if (!nlmsg_valid_hdr(nlh, mUserHeaderLen))	return false;
	return (nlh->nlmsg_len - GENL_HDRLEN - NLMSG_HDRLEN) >= NLMSG_ALIGN(mUserHeaderLen);
}

void GenericNetlinkMessage::putHeader(uint32_t pid, uint32_t seq,
			   int type, int flags) {
	// Resize std::vector to be big enough for nlmsg header, genlmsg header and user header.
	mData.resize(NLMSG_HDRLEN + GENL_HDRLEN + NLMSG_ALIGN(mUserHeaderLen), 0);

	struct nlmsghdr* nlh = netlinkHeader();
	nlh->nlmsg_type = type;
	nlh->nlmsg_flags = flags;
	nlh->nlmsg_pid = pid;
	nlh->nlmsg_seq = seq;
	nlh->nlmsg_len = dataLen();
}


bool GenericNetlinkMessage::getAttribute(int attributeId, void* dst, size_t size) const {
	const struct nlattr* head = reinterpret_cast<const struct nlattr*>(userData());
	struct nlattr* src = nla_find(head, userDataLen(), attributeId);
	if (src == NULL) {
		LOG(VERBOSE) << "NLA attribute " << attributeId << "is not found";
	}
	return src != NULL && nla_memcpy(dst, src, size) > 0;
}

IOVector GenericNetlinkMessage::getAttribute(int attributeId) const {
	IOVector iov;
	const struct nlattr* head = reinterpret_cast<const struct nlattr*>(userData());
	struct nlattr* src = nla_find(head, userDataLen(), attributeId);
	if (src == NULL) {
		LOG(VERBOSE) << "NLA attribute " << attributeId << "is not found";
	} else {
            iov.push_back({.iov_base = nla_data(src),
                           .iov_len = static_cast<size_t>(nla_len(src))});
        }
	return iov;

}

bool GenericNetlinkMessage::putAttribute(int attributeId, const void* src, size_t size) {
	IOVector iov = getAttribute(attributeId);
	// already exists
	if (iov.size() == 1) {
		std::memcpy(iov[0].iov_base, src, std::min(size, iov[0].iov_len));
	} else {
		size_t origLen = mData.size();
		size_t totalLen = origLen + nla_total_size(size);
		mData.resize(totalLen, 0);
		struct nlmsghdr* nlh = netlinkHeader();
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
