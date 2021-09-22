/* Copyright 2020 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/network/Ieee80211Frame.h"
#include "android/base/Log.h"
#include "android/network/Endian.h"
#include "android/network/mac80211_hwsim.h"

#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#elif defined(__linux__)
#include <arpa/inet.h>
#endif

extern "C" {
#include "common/ieee802_11_defs.h"
#include "crypto/aes.h"
#include "crypto/aes_wrap.h"
#include "drivers/driver_virtio_wifi.h"
}

namespace {

static constexpr uint16_t ETH_P_ARP = 0x0806; /* Address Resolution packet */
static constexpr uint16_t ETH_P_IPV6 = 0x86dd;
static constexpr uint16_t ETH_P_NCSI = 0x88f8;

/* See IEEE 802.1H for LLC/SNAP encapsulation/decapsulation */
/* Ethernet-II snap header (RFC1042 for most EtherTypes) */
static constexpr uint8_t kRfc1042Header[] = {0xaa, 0xaa, 0x03,
                                             0x00, 0x00, 0x00};

static bool validEtherType(uint16_t ethertype) {
    switch (ethertype) {
        case ETH_P_IPV6:
        case ETH_P_IP:
        case ETH_P_ARP:
        case ETH_P_NCSI:
            return true;
        default:
            return false;
    }
}

}  // namespace

namespace android {
namespace network {

using namespace android::base;

FrameInfo::FrameInfo(MacAddress transmitter,
                     uint64_t cookie,
                     uint32_t flags,
                     uint32_t channel,
                     const hwsim_tx_rate* rates,
                     size_t numRates)
    : mTransmitter(transmitter),
      mCookie(cookie),
      mFlags(flags),
      mChannel(channel) {
    size_t i = 0;
    for (; i < numRates; ++i) {
        mTxRates[i].count = 0;
        mTxRates[i].idx = rates[i].idx;
    }
    for (; i < mTxRates.size(); ++i) {
        mTxRates[i].count = 0;
        mTxRates[i].idx = -1;
    }
}

// Frame Header layout as below
//       2         2           6        6        6       2       6
// Frame-Ctl | Duration ID | Addr1 | Addr2 | Addr3 | Seq-Ctl | Addr4 | Frame
// Body

std::atomic<int64_t> Ieee80211Frame::sTxPn = {1};
bool Ieee80211Frame::sForTesting = false;

Ieee80211Frame::Ieee80211Frame(const uint8_t* data, size_t size, FrameInfo info)
    : mData(data, data + size), mInfo(info) {}

Ieee80211Frame::Ieee80211Frame(const uint8_t* data, size_t size)
    : mData(data, data + size) {}

Ieee80211Frame::Ieee80211Frame(size_t size) : mData(size) {}

std::unique_ptr<Ieee80211Frame> Ieee80211Frame::buildFromEthernet(
        const uint8_t* data,
        size_t size,
        MacAddress bssid) {
    uint16_t ethertype = (data[12] << 8) | data[13];
    size_t skippedBytes = ETH_HLEN;
    size_t encapLen = 0;

    if (validEtherType(ethertype)) {
        encapLen = ETH_ALEN;
        skippedBytes -= sizeof(ethertype);
    } else {
        LOG(VERBOSE) << "Unexpected ether type: " << ethertype;
        return nullptr;
    }

    uint16_t fc = IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA);
    fc |= WLAN_FC_FROMDS;
    struct ieee80211_hdr hdr;
    memcpy(hdr.addr1, data, ETH_ALEN);
    memcpy(hdr.addr2, bssid.mAddr, ETH_ALEN);
    memcpy(hdr.addr3, data + ETH_ALEN, ETH_ALEN);
    hdr.frame_control = fc;
    hdr.duration_id = 0;
    hdr.seq_ctrl = 0;
    auto frame = std::make_unique<Ieee80211Frame>(0);

    frame->mData.insert(frame->mData.end(), (uint8_t*)&hdr,
                        (uint8_t*)&hdr + IEEE80211_HDRLEN);

    if (encapLen) {
        frame->mData.insert(frame->mData.end(), kRfc1042Header,
                            kRfc1042Header + encapLen);
    }
    frame->mData.insert(frame->mData.end(), data + skippedBytes, data + size);
    return frame;
}

MacAddress Ieee80211Frame::addr1() const {
    return MacAddress(MACARG(hdr().addr1));
}

MacAddress Ieee80211Frame::addr2() const {
    return MacAddress(MACARG(hdr().addr2));
}

MacAddress Ieee80211Frame::addr3() const {
    return MacAddress(MACARG(hdr().addr3));
}

MacAddress Ieee80211Frame::addr4() const {
    if (uses4Addresses()) {
        return MacAddress(MACARG(mData.data() + IEEE80211_HDRLEN));
    } else {
        return MacAddress();
    }
}

const ieee80211_hdr& Ieee80211Frame::hdr() const {
    const ieee80211_hdr* hdr = (ieee80211_hdr*)mData.data();
    return *hdr;
}

size_t Ieee80211Frame::hdrLength() const {
    size_t hdrlen = IEEE80211_HDRLEN;
    if (isData()) {
        if (uses4Addresses()) {
            hdrlen += ETH_ALEN;
        }
        if (isDataQoS()) {
            hdrlen += IEEE80211_QOS_CTL_LEN;
        }
    }
    return hdrlen;
}

uint8_t* Ieee80211Frame::frameBody() {
    return mData.data() +
           (IEEE80211_HDRLEN + (uses4Addresses() ? ETH_ALEN : 0));
}

const uint8_t* Ieee80211Frame::frameBody() const {
    return mData.data() +
           (IEEE80211_HDRLEN + (uses4Addresses() ? ETH_ALEN : 0));
}

std::string Ieee80211Frame::toStr() {
    std::stringstream ss;
    uint8_t type = WLAN_FC_GET_TYPE(hdr().frame_control);
    uint8_t subType = WLAN_FC_GET_STYPE(hdr().frame_control);
    switch (type) {
        case WLAN_FC_TYPE_MGMT:
            // Management
            ss << "Management (";
            switch (subType) {
                case WLAN_FC_STYPE_ASSOC_REQ:
                    ss << "Association Request";
                    break;
                case WLAN_FC_STYPE_ASSOC_RESP:
                    ss << "Association Response";
                    break;
                case WLAN_FC_STYPE_REASSOC_REQ:
                    ss << "Reassociation Request";
                    break;
                case WLAN_FC_STYPE_REASSOC_RESP:
                    ss << "Reassociation Response";
                    break;
                case WLAN_FC_STYPE_PROBE_REQ:
                    ss << "Probe Request";
                    break;
                case WLAN_FC_STYPE_PROBE_RESP:
                    ss << "Probe Response";
                    break;
                case 6:
                    ss << "Timing Advertisement";
                    break;
                case WLAN_FC_STYPE_BEACON:
                    ss << "Beacon";
                    break;
                case WLAN_FC_STYPE_ATIM:
                    ss << "ATIM";
                    break;
                case WLAN_FC_STYPE_DISASSOC:
                    ss << "Disassociation";
                    break;
                case WLAN_FC_STYPE_AUTH:
                    ss << "Authentication";
                    break;
                case WLAN_FC_STYPE_DEAUTH:
                    ss << "Deauthentication";
                    break;
                case WLAN_FC_STYPE_ACTION:
                    ss << "Action";
                    break;
                case 14:
                    ss << "Action No Ack";
                    break;
                default:
                    ss << subType;
                    break;
            }
            ss << ')';
            break;
        case WLAN_FC_TYPE_CTRL:
            // Control
            ss << "Control (";
            switch (subType) {
                case 7:
                    ss << "Control Wrapper";
                    break;
                case 8:
                    ss << "Block Ack Request";
                    break;
                case 9:
                    ss << "Block Ack";
                    break;
                case WLAN_FC_STYPE_PSPOLL:
                    ss << "PS-Poll";
                    break;
                case WLAN_FC_STYPE_RTS:
                    ss << "RTS";
                    break;
                case WLAN_FC_STYPE_CTS:
                    ss << "CTS";
                    break;
                case WLAN_FC_STYPE_ACK:
                    ss << "Ack";
                    break;
                case WLAN_FC_STYPE_CFEND:
                    ss << "CF-End";
                    break;
                case WLAN_FC_STYPE_CFENDACK:
                    ss << "CF-End+CF-Ack";
                    break;
                default:
                    ss << subType;
                    break;
            }
            ss << ')';
            break;
        case WLAN_FC_TYPE_DATA:
            // Data
            ss << "Data (";
            switch (subType) {
                case WLAN_FC_STYPE_DATA:
                    ss << "Data";
                    break;
                case WLAN_FC_STYPE_DATA_CFACK:
                    ss << "Data+CF-Ack";
                    break;
                case WLAN_FC_STYPE_DATA_CFPOLL:
                    ss << "Data+CF-Poll";
                    break;
                case WLAN_FC_STYPE_DATA_CFACKPOLL:
                    ss << "Data+CF-Ack+CF-Poll";
                    break;
                case WLAN_FC_STYPE_NULLFUNC:
                    ss << "Null";
                    break;
                case WLAN_FC_STYPE_CFACK:
                    ss << "CF-Ack";
                    break;
                case WLAN_FC_STYPE_CFPOLL:
                    ss << "CF-Poll";
                    break;
                case WLAN_FC_STYPE_CFACKPOLL:
                    ss << "CF-Ack+CF-Poll";
                    break;
                case WLAN_FC_STYPE_QOS_DATA:
                    ss << "QoS Data";
                    break;
                case WLAN_FC_STYPE_QOS_DATA_CFACK:
                    ss << "QoS Data+CF-Ack";
                    break;
                case WLAN_FC_STYPE_QOS_DATA_CFPOLL:
                    ss << "QoS Data+CF-Poll";
                    break;
                case WLAN_FC_STYPE_QOS_DATA_CFACKPOLL:
                    ss << "QoS Data+CF-Ack+CF-Poll";
                    break;
                case WLAN_FC_STYPE_QOS_NULL:
                    ss << "QoS Null";
                    break;
                case WLAN_FC_STYPE_QOS_CFPOLL:
                    ss << "QoS CF-Poll";
                    break;
                case WLAN_FC_STYPE_QOS_CFACKPOLL:
                    ss << "QoS CF-Poll+CF-Ack";
                    break;
                default:
                    ss << subType;
                    break;
            }
            ss << ')';
            break;
        case 3:
            // Extension
            ss << "Extension (";
            switch (subType) {
                case 0:
                    ss << "DMG Beacon";
                    break;
                default:
                    ss << subType;
                    break;
            }
            ss << ')';
            break;
        default:
            ss << type << " (" << subType << ')';
            break;
    }
    ss << std::endl;
    if (LOG_IS_ON(VERBOSE)) {
        for (int i = 0; i < size(); i++)
            ss << std::hex << mData[i] << std::setw(2) << ' ';
    }
    return ss.str();
}

bool Ieee80211Frame::isData() const {
    return WLAN_FC_GET_TYPE(hdr().frame_control) == WLAN_FC_TYPE_DATA;
}

bool Ieee80211Frame::isMgmt() const {
    return WLAN_FC_GET_TYPE(hdr().frame_control) == WLAN_FC_TYPE_MGMT;
}

// Reference linux kernel include/linux/ieee80211.h
bool Ieee80211Frame::isRobustMgmt() const {
    if (size() < 25)
        return false;
    if (isDisassoc() || isDeauth())
        return true;
    if (isAction()) {
        /*
         * Action frames, excluding Public Action frames, are Robust
         * Management Frames. However, if we are looking at a Protected
         * frame, skip the check since the data may be encrypted and
         * the frame has already been found to be a Robust Management
         * Frame (by the other end).
         */
        if (isProtected())
            return true;

        const uint8_t* category = reinterpret_cast<const uint8_t*>(&hdr()) + 24;
        switch (*category) {
            case WLAN_ACTION_PUBLIC:
            case WLAN_ACTION_HT:
            case WLAN_ACTION_SELF_PROTECTED:
            case WLAN_ACTION_VENDOR_SPECIFIC:
                return false;
            default:
                return true;
        }
    }
    return true;
}

bool Ieee80211Frame::isDataQoS() const {
    return isData() &&
           WLAN_FC_GET_STYPE(hdr().frame_control) == WLAN_FC_STYPE_QOS_DATA;
}

bool Ieee80211Frame::isDataNull() const {
    return isData() &&
           (WLAN_FC_GET_STYPE(hdr().frame_control) & WLAN_FC_STYPE_NULLFUNC);
}
bool Ieee80211Frame::isBeacon() const {
    return isMgmt() &&
           WLAN_FC_GET_STYPE(hdr().frame_control) == WLAN_FC_STYPE_BEACON;
}

bool Ieee80211Frame::isToDS() const {
    return isData() && (hdr().frame_control & WLAN_FC_TODS);
}

bool Ieee80211Frame::isFromDS() const {
    return isData() && (hdr().frame_control & WLAN_FC_FROMDS);
}

bool Ieee80211Frame::isProtected() const {
    return hdr().frame_control & WLAN_FC_ISWEP;
}

void Ieee80211Frame::setProtected(bool protect) {
    ieee80211_hdr* hdr = (ieee80211_hdr*)mData.data();
    if (protect)
        hdr->frame_control |= WLAN_FC_ISWEP;
    else
        hdr->frame_control &= ~WLAN_FC_ISWEP;
}

bool Ieee80211Frame::isEAPoL() const {
    return getEtherType() == ETH_P_EAPOL;
}

bool Ieee80211Frame::isDisassoc() const {
    return isMgmt() &&
           WLAN_FC_GET_STYPE(hdr().frame_control) == WLAN_FC_STYPE_DISASSOC;
}

bool Ieee80211Frame::isDeauth() const {
    return isMgmt() &&
           WLAN_FC_GET_STYPE(hdr().frame_control) == WLAN_FC_STYPE_DEAUTH;
}

bool Ieee80211Frame::isAction() const {
    return isMgmt() &&
           WLAN_FC_GET_STYPE(hdr().frame_control) == WLAN_FC_STYPE_ACTION;
}

bool Ieee80211Frame::uses4Addresses() const {
    return (mData[1] & WLAN_FC_TODS) && (mData[1] & WLAN_FC_FROMDS);
}

uint16_t Ieee80211Frame::getQoSControl() const {
    const uint8_t* addr = mData.data() + (IEEE80211_HDRLEN +
                                          (uses4Addresses() ? ETH_ALEN : 0));
    return *reinterpret_cast<const uint16_t*>(addr);
}

std::vector<uint8_t> Ieee80211Frame::getPacketNumber(
        const CipherScheme cs) const {
    if (cs == CipherScheme::CCMP) {
        std::vector<uint8_t> pn(IEEE80211_CCMP_PN_LEN, 0);
        const auto* ccmpHdr = frameBody();
        pn[0] = ccmpHdr[7];
        pn[1] = ccmpHdr[6];
        pn[2] = ccmpHdr[5];
        pn[3] = ccmpHdr[4];
        pn[4] = ccmpHdr[1];
        pn[5] = ccmpHdr[0];
        return pn;
    } else {
        return std::vector<uint8_t>();
    }
}

uint8_t Ieee80211Frame::getQosTid() const {
    uint8_t qos_tid;
    if (isDataQoS())
        qos_tid = (getQoSControl() >> 8) & 0xff;
    else
        qos_tid = 0;
    return qos_tid;
}

// Reference Linux kernel net/mac80211/wpa.c
std::vector<uint8_t> Ieee80211Frame::getNonce(
        const std::vector<uint8_t>& pn) const {
    /* Nonce: Nonce Flags | A2 | PN
     * Nonce Flags: Priority (b0..b3) | Management (b4) | Reserved (b5..b7)
     */
    std::vector<uint8_t> nonce(1 + ETH_ALEN + IEEE80211_CCMP_PN_LEN, 0);
    uint8_t qos_tid = getQosTid();
    int mgmt = isMgmt();
    nonce[0] = qos_tid | (mgmt << 4);

    memcpy(&nonce[1], hdr().addr2, ETH_ALEN);
    memcpy(&nonce[7], pn.data(), IEEE80211_CCMP_PN_LEN);
    return nonce;
}

// Reference Linux kernel net/mac80211/wpa.c
std::vector<uint8_t> Ieee80211Frame::getAad() const {
    uint8_t qos_tid = getQosTid();
    int a4_included = uses4Addresses();
    std::vector<uint8_t> aad(hdrLength() - 2, 0);
    uint16_t mask_fc = hdr().frame_control;
    mask_fc &= ~(WLAN_FC_RETRY | WLAN_FC_PWRMGT | WLAN_FC_MOREDATA);
    if (!isMgmt())
        mask_fc &= ~0x0070;
    mask_fc |= WLAN_FC_ISWEP;

    aad[0] = (mask_fc)&0xff;
    aad[1] = (mask_fc >> 8) & 0xff;
    // put_unaligned(mask_fc, (__le16 *)&aad[2]);
    memcpy(&aad[2], hdr().addr1, 3 * ETH_ALEN);

    /* Mask Seq#, leave Frag# */
    aad[20] = *((uint8_t*)(&hdr().seq_ctrl)) & 0x0f;
    aad[21] = 0;

    if (a4_included) {
        memcpy(&aad[22], data() + IEEE80211_HDRLEN, ETH_ALEN);
        if (isDataQoS())
            aad[28] = qos_tid;
    } else {
        if (isDataQoS())
            aad[22] = qos_tid;
    }
    return aad;
}

Ieee80211Frame::KeyData Ieee80211Frame::getPTK() {
    if (!sForTesting) {
        auto key = ::get_active_ptk();
        mPTK.mKeyMaterial = std::vector<uint8_t>(
                key.key_material, key.key_material + key.key_len);
        mPTK.mKeyIdx = key.key_idx;
    }
    return mPTK;
}

Ieee80211Frame::KeyData Ieee80211Frame::getGTK() {
    if (!sForTesting) {
        auto key = ::get_active_gtk();
        mGTK.mKeyMaterial = std::vector<uint8_t>(
                key.key_material, key.key_material + key.key_len);
        mGTK.mKeyIdx = key.key_idx;
    }
    return mGTK;
}

void Ieee80211Frame::setPTKForTesting(std::vector<uint8_t> keyMaterial,
                                      int keyIdx) {
    sForTesting = true;
    mPTK.mKeyMaterial = std::move(keyMaterial);
    mPTK.mKeyIdx = keyIdx;
}

void Ieee80211Frame::setGTKForTesting(std::vector<uint8_t> keyMaterial,
                                      int keyIdx) {
    sForTesting = true;
    mGTK.mKeyMaterial = std::move(keyMaterial);
    mGTK.mKeyIdx = keyIdx;
}

// Return true if decryption is successful
bool Ieee80211Frame::decrypt(const CipherScheme cs) {
    if (!isProtected())
        return false;
    if (!isData() && !isRobustMgmt())
        return false;
    if (cs == CipherScheme::NONE) {
        return true;
    } else if (cs != CipherScheme::CCMP) {
        LOG(ERROR) << "Currently only CCMP is supported.";
        return false;
    }
    const MacAddress dst = addr1();
    Ieee80211Frame::KeyData key;
    if (dst.isBroadcast() || dst.isMulticast()) {
        key = getGTK();
    } else {
        key = getPTK();
    }
    if (key.mKeyMaterial.size() == 0) {
        LOG(ERROR) << "Unable to obtain the key material.";
        return false;
    }
    // compute special ccmp block
    int dataLen = size() - hdrLength() - IEEE80211_CCMP_HDR_LEN -
                  IEEE80211_CCMP_MIC_LEN;
    if (dataLen < 0) {
        LOG(ERROR) << "Not enough data in the encrypted message";
        return false;
    }

    auto pn = getPacketNumber(cs);
    auto aad = getAad();
    auto nonce = getNonce(pn);

    std::vector<uint8_t> plain(dataLen, 0);

    if (::aes_ccm_ad(key.mKeyMaterial.data(), key.mKeyMaterial.size(),
                     nonce.data(), IEEE80211_CCMP_MIC_LEN,
                     frameBody() + IEEE80211_CCMP_HDR_LEN, dataLen, aad.data(),
                     aad.size(), mData.data() + size() - IEEE80211_CCMP_MIC_LEN,
                     plain.data())) {
        LOG(ERROR) << "Failed to decrypt the Ieee80211 Frame";
        return false;
    } else {
        // rebuild message in-place
        // Todo(wdu@) figure out a more efficient way of memcpy here
        memcpy(frameBody(), plain.data(), plain.size());
        setProtected(false);
        mData.resize(size() - IEEE80211_CCMP_MIC_LEN - IEEE80211_CCMP_HDR_LEN);
        return true;
    }
}

bool Ieee80211Frame::encrypt(const CipherScheme cs) {
    if (cs == CipherScheme::NONE) {
        return true;
    } else if (cs != CipherScheme::CCMP) {
        LOG(ERROR) << "Currently only CCMP is supported.";
        return false;
    }
    if (isProtected()) {
        return false;
    }
    if (!isData() && !isRobustMgmt())
        return false;
    size_t dataLen = size() - hdrLength();
    mData.resize(size() + IEEE80211_CCMP_MIC_LEN + IEEE80211_CCMP_HDR_LEN);
    memmove(frameBody() + IEEE80211_CCMP_HDR_LEN, frameBody(), dataLen);

    int64_t pn64 = sTxPn.fetch_add(1, std::memory_order_relaxed);
    std::vector<uint8_t> pn(IEEE80211_CCMP_PN_LEN, 0);
    pn[5] = pn64;
    pn[4] = pn64 >> 8;
    pn[3] = pn64 >> 16;
    pn[2] = pn64 >> 24;
    pn[1] = pn64 >> 32;
    pn[0] = pn64 >> 40;
    auto nonce = getNonce(pn);
    auto aad = getAad();
    uint8_t* ccmpHdr = frameBody();
    const MacAddress dst = addr1();
    Ieee80211Frame::KeyData key;
    if (dst.isBroadcast() || dst.isMulticast()) {
        key = getGTK();
    } else {
        key = getPTK();
    }
    ccmpHdr[0] = pn[5];
    ccmpHdr[1] = pn[4];
    ccmpHdr[2] = 0;
    ccmpHdr[3] = 0x20 | (key.mKeyIdx << 6);
    ccmpHdr[4] = pn[3];
    ccmpHdr[5] = pn[2];
    ccmpHdr[6] = pn[1];
    ccmpHdr[7] = pn[0];

    if (key.mKeyMaterial.size() == 0) {
        LOG(ERROR) << "Unable to obtain the key material.";
        return false;
    }

    std::vector<uint8_t> crypt(dataLen, 0);
    if (::aes_ccm_ae(key.mKeyMaterial.data(), key.mKeyMaterial.size(),
                     nonce.data(), IEEE80211_CCMP_MIC_LEN,
                     frameBody() + IEEE80211_CCMP_HDR_LEN, dataLen, aad.data(),
                     aad.size(), crypt.data(),
                     data() + size() - IEEE80211_CCMP_MIC_LEN)) {
        LOG(ERROR) << " Unable to encrypt the IEEE80211 frame.";
        return false;
    } else {
        // rebuild message in-place
        // Todo(wdu@) figure out a more efficient way of memcpy here
        memcpy(frameBody() + IEEE80211_CCMP_HDR_LEN, crypt.data(),
               crypt.size());
        setProtected(true);
        return true;
    }
}

const IOVector Ieee80211Frame::toEthernet() {
    IOVector ret;
    uint16_t ethertype = getEtherType();
    if (!validEtherType(ethertype)) {
        LOG(VERBOSE) << "Unexpected ether type. Dump frame: "
                     << toStr().c_str();
        return ret;
    }

    mEthHdr.ethertype = htons(ethertype);
    // Order of addresses: RA SA DA
    memcpy(mEthHdr.dest, &(addr3()[0]), ETH_ALEN);
    memcpy(mEthHdr.src, &(addr2()[0]), ETH_ALEN);

    ret.push_back({.iov_base = &mEthHdr, .iov_len = ETH_HLEN});
    size_t offset = hdrLength() + ETH_ALEN + sizeof(ethertype);
    IOVector inSg;
    inSg.push_back({.iov_base = data(), .iov_len = size()});
    inSg.appendEntriesTo(&ret, offset, size() - offset);
    return ret;
}

uint16_t Ieee80211Frame::getEtherType() const {
    if (!memcmp(frameBody(), kRfc1042Header, ETH_ALEN)) {
        return (frameBody()[ETH_ALEN] << 8) | frameBody()[ETH_ALEN + 1];
    } else {
        return (frameBody()[0] << 8) | frameBody()[1];
    }
}

}  // namespace network
}  // namespace android
