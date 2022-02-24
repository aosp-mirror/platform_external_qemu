#include "android/bootconfig.h"

#include <numeric>
#include <memory>
#include <string_view>
#include <stdio.h>
#include "android/utils/debug.h"
#include "android/utils/file_io.h"

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

std::vector<char> flattenBootconfig(const std::vector<std::pair<std::string, std::string>>& bootconfig) {
    std::vector<char> bits;

    for (const auto& kv: bootconfig) {
        bits.insert(bits.end(), kv.first.begin(), kv.first.end());
        bits.push_back('=');
        bits.push_back('\"');
        bits.insert(bits.end(), kv.second.begin(), kv.second.end());
        bits.push_back('\"');
        bits.push_back('\n');
    }
    bits.push_back(0);  // it is ASCIIZ

    return bits;
}

int appendBootconfig(const size_t srcSize,
                     const std::vector<std::pair<std::string, std::string>>& bootconfig,
                     FILE* dst) {
    const std::vector<char> blob = buildBootconfigBlob(srcSize, bootconfig);

    if (blob.size() != ::fwrite(blob.data(), 1, blob.size(), dst)) {
        return ::ferror(dst);
    }

    return 0;
}
}  // namespace

std::vector<char> buildBootconfigBlob(const size_t srcSize,
                                      const std::vector<std::pair<std::string, std::string>>& bootconfig) {
    std::vector<char> blob = flattenBootconfig(bootconfig);

    const size_t unaligend = (srcSize + blob.size()) % kBootconfigAlign;
    if (unaligend) {
        blob.insert(blob.end(), kBootconfigAlign - unaligend, '+');
    }

    const uint32_t csum =
        std::accumulate(blob.begin(), blob.end(), 0,
        [](const uint32_t z, const char c){
            return z + static_cast<uint8_t>(c);
        });

    const size_t size = blob.size();

    blob.insert(blob.end(), 8, '+');    // size(u32, LE), csum(u32, LE)
    host2le32(size, &blob[blob.size() - 8]);
    host2le32(csum, &blob[blob.size() - 4]);

    blob.insert(blob.end(), kBootconfigMagic.begin(), kBootconfigMagic.end());

    return blob;
}

int createRamdiskWithBootconfig(const char* srcRamdiskPath,
                                const char* dstRamdiskPath,
                                const std::vector<std::pair<std::string, std::string>>& bootconfig) {
    struct FILE_deleter {
        void operator()(FILE* fp) const {
            ::fclose(fp);
        }
    };

    std::unique_ptr<FILE, FILE_deleter> srcRamdisk(android_fopen(srcRamdiskPath, "rb"));
    if (!srcRamdisk) {
        derror("%s Can't open '%s' for reading\n", __func__, srcRamdiskPath);
        return 1;
    }

    std::unique_ptr<FILE, FILE_deleter> dstRamdisk(android_fopen(dstRamdiskPath, "wb"));
    if (!dstRamdisk) {
        derror("%s:  Can't open '%s' for writing", __func__, dstRamdiskPath);
        return 1;
    }

    const auto r = copyFile(srcRamdisk.get(), dstRamdisk.get());
    if (r.first) {
        derror("%s Error copying '%s' into '%s'\n",
                __func__,  srcRamdiskPath, dstRamdiskPath);
        return r.first;
    }

    return appendBootconfig(r.second, bootconfig, dstRamdisk.get());
}

