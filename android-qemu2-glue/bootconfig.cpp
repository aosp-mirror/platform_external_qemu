#include "android-qemu2-glue/bootconfig.h"

#include <numeric>
#include <memory>
#include <string_view>
#include <stdio.h>

namespace {
using namespace std::literals;

constexpr std::string_view kBootconfigMagic = "#BOOTCONFIG\n"sv;
constexpr uint32_t kBootconfigAlign = 4;

std::pair<int, size_t> copyFile(FILE* src, FILE* dst) {
    size_t sz = 0;
    std::vector<char> buf(64 * 1024);

    while (true) {
        const size_t szR = ::fread(buf.data(), 1, buf.size(), src);
        if (!szR) {
            return {::ferror(src), sz};
        }

        const size_t szW = ::fwrite(buf.data(), 1, szR, dst);
        if (szR != szW) {
            return {::ferror(dst), sz};
        }

        sz += szR;
    }
}

void host2le32(const uint32_t v32, void* dst) {
    auto m8 = static_cast<uint8_t*>(dst);
    m8[0] = v32;
    m8[1] = v32 >> 8;
    m8[2] = v32 >> 16;
    m8[3] = v32 >> 24;
}

bool needsQuotes(const std::string_view s) {
    return (s.size() > 10) || !std::all_of(s.begin(), s.end(), [](char c){
        return (c >= '0') && (c <= '9');
    });
}

std::vector<char> flattenBootconfig(const std::vector<std::pair<std::string, std::string>>& bootconfig) {
    std::vector<char> bits;

    for (const auto& kv: bootconfig) {
        bits.insert(bits.end(), kv.first.begin(), kv.first.end());
        bits.push_back('=');
        if (needsQuotes(kv.second)) {
            bits.push_back('\"');
            bits.insert(bits.end(), kv.second.begin(), kv.second.end());
            bits.push_back('\"');
        } else {
            bits.insert(bits.end(), kv.second.begin(), kv.second.end());
        }
        bits.push_back('\n');
    }
    bits.push_back(0);  // it is ASCIIZ

    return bits;
}

int appendBootconfig(const size_t srcSize,
                     const std::vector<std::pair<std::string, std::string>>& bootconfig,
                     FILE* dst) {
    std::vector<char> bootconfigBits = flattenBootconfig(bootconfig);

    const size_t unaligend = (srcSize + bootconfigBits.size()) % kBootconfigAlign;
    if (unaligend) {
        bootconfigBits.insert(bootconfigBits.end(), unaligend, '+');
    }

    if (bootconfigBits.size() != ::fwrite(bootconfigBits.data(),
                                          1, bootconfigBits.size(),
                                          dst)) {
        return ::ferror(dst);
    }

    const uint32_t csum =
        std::accumulate(bootconfigBits.begin(), bootconfigBits.end(), 0,
        [](const uint32_t z, const char c){
            return z + static_cast<uint8_t>(c);
        });


    uint32_t sizecsumLe32[2];
    host2le32(bootconfigBits.size(), &sizecsumLe32[0]);
    host2le32(csum, &sizecsumLe32[1]);

    if (sizeof(sizecsumLe32) != ::fwrite(sizecsumLe32, 1, sizeof(sizecsumLe32), dst)) {
        return ::ferror(dst);
    }

    if (kBootconfigMagic.size() != ::fwrite(kBootconfigMagic.data(),
                                            1,
                                            kBootconfigMagic.size(),
                                            dst)) {
        return ::ferror(dst);
    }

    return 0;
}
}  // namespace

// [src ramdisk][bootconfig][padding][size(le32)][csum(le32)][#BOOTCONFIG\n]
//                                   ^ 4 byte aligned
int updateRamdiskBootconfig(const char* srcRamdiskPath,
                            const char* dstRamdiskPath,
                            const std::vector<std::pair<std::string, std::string>>& bootconfig) {
    struct FILE_deleter {
        void operator()(FILE* fp) const {
            ::fclose(fp);
        }
    };

    std::unique_ptr<FILE, FILE_deleter> srcRamdisk(::fopen(srcRamdiskPath, "rb"));
    if (!srcRamdisk) {
        fprintf(stderr, "%s:%d Can't open '%s' for reading\n", __func__, __LINE__, srcRamdiskPath);
        return 1;
    }

    std::unique_ptr<FILE, FILE_deleter> dstRamdisk(::fopen(dstRamdiskPath, "wb"));
    if (!dstRamdisk) {
        fprintf(stderr, "%s:%d Can't open '%s' for writing\n", __func__, __LINE__, dstRamdiskPath);
        return 1;
    }

    const auto r = copyFile(srcRamdisk.get(), dstRamdisk.get());
    if (r.first) {
        fprintf(stderr, "%s:%d Error copying '%s' into '%s'\n",
                __func__, __LINE__, srcRamdiskPath, dstRamdiskPath);
        return r.first;
    }

    return appendBootconfig(r.second, bootconfig, dstRamdisk.get());
}

