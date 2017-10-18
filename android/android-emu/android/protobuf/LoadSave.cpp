#include "android/protobuf/LoadSave.h"

#include "android/base/files/ScopedFd.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/utils/fd.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"

#include "google/protobuf/io/zero_copy_stream_impl.h"

#include <fcntl.h>
#include <sys/mman.h>

using android::base::ScopedCPtr;
using android::base::ScopedFd;
using android::base::StringView;
using android::base::System;
using android::base::makeCustomScopedPtr;

namespace android {
namespace protobuf {

ProtobufLoadResult loadProtobufFileImpl(android::base::StringView fileName,
                                        System::FileSize* bytesUsed,
                                        ProtobufLoadCallback loadCb) {
    const auto file =
            ScopedFd(::open(fileName.c_str(),
                            O_RDONLY | O_BINARY | O_CLOEXEC, 0755));

    System::FileSize size;
    if (!System::get()->fileSize(file.get(), &size)) {
        return ProtobufLoadResult::FileNotFound;
    }

    if (bytesUsed) *bytesUsed = size;

    const auto fileMap =
        makeCustomScopedPtr(
            mmap(nullptr, size, PROT_READ, MAP_PRIVATE, file.get(), 0),
            [size](void* ptr) { if (ptr != MAP_FAILED) munmap(ptr, size); });

    if (!fileMap || fileMap.get() == MAP_FAILED) {
        return ProtobufLoadResult::FileMapFailed;
    }

    return loadCb(fileMap.get(), size);
}

ProtobufSaveResult saveProtobufFileImpl(android::base::StringView fileName,
                                        System::FileSize* bytesUsed,
                                        ProtobufSaveCallback saveCb) {
    if (bytesUsed) *bytesUsed = 0;

    google::protobuf::io::FileOutputStream stream(
            ::open(fileName.c_str(),
                   O_WRONLY | O_BINARY | O_CREAT | O_TRUNC | O_CLOEXEC, 0755));

    stream.SetCloseOnDelete(true);

    return saveCb(stream, bytesUsed);
}

} // namespace android
} // namespace protobuf

