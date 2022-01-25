#ifndef QEMU_BASE_BUILD
#include "android/utils/file_io.h"

// Set of redefines for posix file calls. These redirects
// will make the windows posix calls unicode compliant.
//
// Note this is disabled when were a building qemu.
//
#define fopen(path, mode) android_fopen( (path), (mode) )
#define popen(path, mode) android_popen( (path), (mode) )
#define stat(path, buf) android_stat( (path), (buf))
#define lstat(path, buf) android_lstat ( (path), (buf) )
#define access(path, mode) android_access( (path), (mode))
#define mkdir(path, mode) android_mkdir( (path), (mode))
#define mkdir(path) android_mkdir( (path), 0755)
#define creat(path, mode) android_creat( (path), (mode))
#define unlink(path) android_unlink((path))
#define chmod(path, mode) android_chmod( (path), (mode))
#define rmdir(path) android_rmdir((path))
#else
// So we are in the qemu build..
#endif
