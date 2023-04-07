
// This files need to be included before EmulatedBluetoothService.h
// because of windows winnt.h header issues.
// #include "model/hci/hci_sniffer.h"
#include <grpcpp/grpcpp.h>
#include <unistd.h>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "aemu/base/files/PathUtils.h"
#include "aemu/base/files/ScopedFd.h"
#include "aemu/base/process/Command.h"
#include "aemu/base/process/Process.h"
#include "android/base/system/System.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/bluetooth/EmulatedBluetoothService.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"
#include "emulated_bluetooth.pb.h"
#include "emulated_bluetooth_device.pb.h"

#define DEBUG 0
/* set  >1 for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#define DD_BUF(buf, len) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#define DD_BUF(buf, len)                                \
    do {                                                \
        printf("Channel %s (%p):", __func__, this);     \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x]))                   \
                printf("%c", buf[x]);                   \
            else                                        \
                printf("[0x%02x]", 0xff & (int)buf[x]); \
        }                                               \
        printf("\n");                                   \
    } while (0)
#endif
namespace android {
namespace emulation {
namespace bluetooth {
class EmulatedBluetoothServiceImpl : public EmulatedBluetoothService::Service {
public:
    ::grpc::Status registerGattDevice(
            ::grpc::ServerContext* ctx,
            const ::android::emulation::bluetooth::GattDevice* request,
            ::android::emulation::bluetooth::RegistrationStatus* response)
            override {
        const std::string kNimbleBridgeExe = "nimble_bridge";
        std::string executable =
                android::base::System::get()->findBundledExecutable(
                        kNimbleBridgeExe);
        // No nimble bridge?
        if (executable.empty()) {
            return ::grpc::Status(::grpc::StatusCode::INTERNAL,
                                  "Nimble bridge executable missing", "");
        }
        auto tmp_dir = android::base::PathUtils::join(
                android::ConfigDirs::getDiscoveryDirectory(),
                std::to_string(android::base::Process::me()->pid()));
        path_mkdir_if_needed(tmp_dir.c_str(), 0700);
        // Get temporary file path
        std::string temp_filename_pattern = "nimble_device_XXXXXX";
        std::string temp_file_path =
                android::base::PathUtils::join(tmp_dir, temp_filename_pattern);
        auto tmpfd =
                android::base::ScopedFd(mkstemp((char*)temp_file_path.data()));
        if (!tmpfd.valid()) {
            return ::grpc::Status(
                    ::grpc::StatusCode::INTERNAL,
                    "Unable to create temporary file: " + temp_file_path +
                            " with device description",
                    "");
        }
        std::ofstream out(
                android::base::PathUtils::asUnicodePath(temp_file_path.data())
                        .c_str(),
                std::ios::out | std::ios::binary);
        request->SerializeToOstream(&out);
        std::vector<std::string> cmdArgs{executable, "--with_device_proto",
                                         temp_file_path};
        auto process =
                android::base::Command::create(cmdArgs).asDeamon().execute();
        if (!process || !process->isAlive()) {
            return ::grpc::Status(::grpc::StatusCode::INTERNAL,
                                  "Failed to launch nimble bridge", "");
        }
        response->mutable_callback_device_id()->set_identity(
                std::to_string(process->pid()));
        VERBOSE_INFO(bluetooth, "Launched nimble bridge pid:%d, with %s",
                     process->pid(), temp_file_path.c_str());
        return ::grpc::Status::OK;
    }
};
EmulatedBluetoothService::Service* getEmulatedBluetoothService() {
    return new EmulatedBluetoothServiceImpl();
}
}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
