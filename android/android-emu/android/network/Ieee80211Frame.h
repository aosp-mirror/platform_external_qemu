/*
 * Copyright 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "android/base/Compiler.h"
#include "android/base/IOVector.h"
#include "android/network/MacAddress.h"

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

typedef struct ieee80211_hdr ieee80211_hdr;

namespace android {
namespace network {

enum class CipherScheme {
    NONE = 0,
    WEP40,
    WEP104,
    TKIP,
    CCMP,
    AES_128_CMAC,
    GCMP,
    SMS4,
    GCMP_256,
    CCMP_256,
    BIP_GMAC_128,
    BIP_GMAC_256,
    BIP_CMAC_256,
};

enum class FrameType : uint8_t {
    Unknown,
    Ack,
    Data,
};

struct hwsim_tx_rate {
    int8_t idx;
    uint8_t count;
} __attribute((packed));

using Rates = std::array<hwsim_tx_rate, 4>;

struct FrameInfo {
    FrameInfo() = default;
    FrameInfo(MacAddress transmitter,
              uint64_t cookie,
              uint32_t flags,
              uint32_t channel,
              const hwsim_tx_rate* rates,
              size_t numRates);

    MacAddress mTransmitter;
    uint64_t mCookie = 0;
    uint32_t mFlags = 0;
    uint32_t mChannel = 0;
    Rates mTxRates;
};

class Ieee80211Frame {
public:
    Ieee80211Frame(const uint8_t* data, size_t size, FrameInfo info);
    Ieee80211Frame(const uint8_t* data, size_t size);
    Ieee80211Frame(size_t size);
    static std::unique_ptr<Ieee80211Frame>
    buildFromEthernet(const uint8_t* data, size_t size, MacAddress bssid);
    size_t size() const { return mData.size(); }
    size_t hdrLength() const;
    const uint8_t* data() const { return mData.data(); }
    uint8_t* data() { return mData.data(); }
    uint8_t* frameBody();
    const uint8_t* frameBody() const;
    const ieee80211_hdr& hdr() const;
    const FrameInfo& info() const { return mInfo; }
    FrameInfo& info() { return mInfo; }

    MacAddress addr1() const;
    MacAddress addr2() const;
    MacAddress addr3() const;
    MacAddress addr4() const;
    std::string toStr();
    bool isData() const;
    bool isMgmt() const;
    bool isRobustMgmt() const;
    bool isDataQoS() const;
    bool isDataNull() const;
    bool isBeacon() const;
    bool isToDS() const;
    bool isFromDS() const;
    bool isProtected() const;
    void setProtected(bool protect);
    bool isEAPoL() const;
    bool isAction() const;
    bool isDisassoc() const;
    bool isDeauth() const;
    bool uses4Addresses() const;
    uint16_t getQoSControl() const;
    void setPTKForTesting(std::vector<uint8_t> keyMaterial, int keyIdx);
    void setGTKForTesting(std::vector<uint8_t> keyMaterial, int keyIdx);
    // decrypt a protected frame in place.
    bool decrypt(const CipherScheme cs);
    // encrypt a frame in place.
    bool encrypt(const CipherScheme cs);
    const android::base::IOVector toEthernet();
    static constexpr uint32_t MAX_FRAME_LEN = 2352;
    static constexpr uint32_t TX_MAX_RATES = 4;
    static constexpr uint32_t WPA_GTK_MAX_LEN = 32;
    static constexpr uint32_t WPA_PTK_MAX_LEN = 32;
    static constexpr uint32_t IEEE80211_QOS_CTL_LEN = 2;
    static constexpr uint32_t IEEE80211_CCMP_PN_LEN = 6;
    static constexpr uint32_t IEEE80211_CCMP_HDR_LEN = 8;
    static constexpr uint32_t IEEE80211_CCMP_MIC_LEN = 8;
    static constexpr uint32_t IEEE80211_WEP_IV_LEN = 4;
    static constexpr uint32_t NUM_DEFAULT_KEYS = 4;

private:
    struct KeyData {
        std::vector<uint8_t> mKeyMaterial;
        int mKeyIdx;
    };

    struct ieee8023_hdr {
        uint8_t dest[ETH_ALEN]; /* destination eth addr */
        uint8_t src[ETH_ALEN];  /* source ether addr    */
        uint16_t ethertype;     /* packet type ID field */
    } __attribute__((__packed__));
    uint16_t getEtherType() const;
    std::vector<uint8_t> getPacketNumber(const CipherScheme cs) const;
    std::vector<uint8_t> getNonce(const std::vector<uint8_t>& pn) const;
    std::vector<uint8_t> getAad() const;
    KeyData getPTK();
    KeyData getGTK();
    uint8_t getQosTid() const;

    std::vector<uint8_t> mData;
    android::network::FrameInfo mInfo;
    KeyData mPTK;
    KeyData mGTK;
    ieee8023_hdr mEthHdr;

    static bool sForTesting;
    static std::atomic<int64_t> sTxPn;
};

}  // namespace network
}  // namespace android
