
// #include "FenceSync.h"
// 
// #include "android/base/synchronization/Lock.h"
// 
// #include <unordered_map>
// 
// class SyncDriver {
// private:
//     std::unordered_map<int, FenceSync*> mSyncs;
// };
// 
// int goldfish_sync_open() {
// }
// 
// int goldfish_sync_close() {
//     return 0;
// }
// 
// int goldfish_sync_queue_work(int goldfish_sync_fd, uint64_t host_glsync, uint64_t host_thread, int* fd_out);
