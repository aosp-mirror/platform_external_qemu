#include "android/help.h"

#include <stdio.h>                                   // for snprintf
#include <stdlib.h>                                  // for NULL
#include <string.h>                                  // for strlen, strcmp

#include "host-common/constants.h"                       // for CORE_HARDWARE_INI
#include "android/emulation/bufprint_config_dirs.h"  // for bufprint_config_...
#include "android/network/constants.h"               // for AndroidNetworkLa...
#include "android/utils/bufprint.h"                  // for bufprint
#include "android/utils/debug.h"                     // for VERBOSE_adb, VER...
#include "android/utils/misc.h"                      // for buffer_translate...
#include "android/utils/path.h"                      // for MAX_PATH, PATH_SEP
#include "android/utils/system.h"                    // for STRINGIFY

/* XXX: TODO: put most of the help stuff in auto-generated files */

#define  PRINTF(...)  stralloc_add_format(out,__VA_ARGS__)

static void
help_list_avds( stralloc_t* out ) {
    PRINTF(
    "  List all available AVDs\n\n"

    "  When this flag is used, the emulator will not start, but instead\n"
    "  probe the Android 'home' directory and print the names of all AVDs\n"
    "  that were created by the user, and which can be started with\n"
    "  '@<name>', or the equivalent '-name <avd>'.\n\n"

    "  Note that the home directory can be overriden by defining the\n"
    "  ANDROID_SDK_HOME environment variable.\n\n"
    );
}

static void
help_studio_params(stralloc_t*  out)
{
    PRINTF(
    "  Indicates a temporary file containing parameters from Android Studio.\n\n"
    "  This should be used only by Android Studio.\n\n"
    );
}

static void
help_virtual_device( stralloc_t*  out )
{
    PRINTF(
    "  An Android Virtual Device (AVD) models a single virtual\n"
    "  device running the Android platform that has, at least, its own\n"
    "  kernel, system image and data partition.\n\n"

    "  Only one emulator process can run a given AVD at a time, but\n"
    "  you can create several AVDs and run them concurrently.\n\n"

    "  You can invoke a given AVD at startup using either '-avd <name>'\n"
    "  or '@<name>', both forms being equivalent. For example, to launch\n"
    "  the AVD named 'foo', type:\n\n"

    "      emulator @foo\n\n"

    "  The 'android' helper tool can be used to manage virtual devices.\n"
    "  For example:\n\n"

    "    android create avd -n <name> -t 1  # creates a new virtual device.\n"
    "    android list avd                   # list all virtual devices available.\n\n"

    "  Try 'android --help' for more commands.\n\n"

    "  Each AVD really corresponds to a content directory which stores\n"
    "  persistent and writable disk images as well as configuration files.\n"

    "  Each AVD must be created against an existing SDK platform or add-on.\n"
    "  For more information on this topic, see -help-sdk-images.\n\n"

    "  SPECIAL NOTE: in the case where you are *not* using the emulator\n"
    "  with the Android SDK, but with the Android build system, you will\n"
    "  need to define the ANDROID_PRODUCT_OUT variable in your environment.\n"
    "  See -help-build-images for the details.\n"
    );
}


static void
help_sdk_images( stralloc_t*  out )
{
    PRINTF(
    "  The Android SDK now supports multiple versions of the Android platform.\n"
    "  Each SDK 'platform' corresponds to:\n\n"

    "    - a given version of the Android API.\n"
    "    - a set of corresponding system image files.\n"
    "    - build and configuration properties.\n"
    "    - an android.jar file used when building your application.\n"
    "    - skins.\n\n"

    "  The Android SDK also supports the concept of 'add-ons'. Each add-on is\n"
    "  based on an existing platform, and provides replacement or additional\n"
    "  image files, android.jar, hardware configuration options and/or skins.\n\n"

    "  The purpose of add-ons is to allow vendors to provide their own customized\n"
    "  system images and APIs without needing to package a complete SDK.\n\n"

    "  Before using the SDK, you need to create an Android Virtual Device (AVD)\n"
    "  (see -help-virtual-device for details). Each AVD is created in reference\n"
    "  to a given SDK platform *or* add-on, and will search the corresponding\n"
    "  directories for system image files, in the following order:\n\n"

    "    - in the AVD's content directory.\n"
    "    - in the AVD's SDK add-on directory, if any.\n"
    "    - in the AVD's SDK platform directory, if any.\n\n"

    "  The image files are documented in -help-disk-images. By default, an AVD\n"
    "  content directory will contain the following persistent image files:\n\n"

    "     userdata-qemu.img     - the /data partition image file\n"
    "     cache.img             - the /cache partition image file\n\n"

    "  You can use -wipe-data to re-initialize the /data partition to its factory\n"
    "  defaults. This will erase all user settings for the virtual device.\n\n"
    );
}

static void
help_build_images( stralloc_t*  out )
{
    PRINTF(
    "  The emulator detects that you are working from the Android build system\n"
    "  by looking at the ANDROID_PRODUCT_OUT variable in your environment.\n\n"

    "  If it is defined, it should point to the product-specific directory that\n"
    "  contains the generated system images.\n"

    "  In this case, the emulator will look by default for the following image\n"
    "  files there:\n\n"

    "    - system.img   : the *initial* system image.\n"
    "    - vendor.img   : the *initial* vendor image.\n"
    "    - ramdisk.img  : the ramdisk image used to boot the system.\n"
    "    - userdata.img : the *initial* user data image (see below).\n"
    "    - kernel-qemu  : the emulator-specific Linux kernel image.\n\n"

    "  If the kernel image is not found in the out directory, then it is searched\n"
    "  in <build-root>/prebuilts/qemu-kernel/.\n\n"

    "  You can use the -sysdir, -system, -vendor -kernel, -ramdisk, -datadir, -data options\n"
    "  to specify different search directories or specific image files. You can\n"
    "  also use the -cache and -sdcard options to indicate specific cache partition\n"
    "  and SD Card image files.\n\n"

    "  For more details, see the corresponding -help-<option> section.\n\n"

    "  Note that the following behaviour is specific to 'build mode':\n\n"

    "  - the *initial* system image is copied to a temporary file which is\n"
    "    automatically removed when the emulator exits. There is thus no way to\n"
    "    make persistent changes to this image through the emulator, even if\n"
    "    you use the '-image <file>' option.\n\n"

    "  - unless you use the '-cache <file>' option, the cache partition image\n"
    "    is backed by a temporary file that is initially empty and destroyed on\n"
    "    program exit.\n\n"

    "  SPECIAL NOTE: If you are using the emulator with the Android SDK, the\n"
    "  information above doesn't apply. See -help-sdk-images for more details.\n"
    );
}

static void
help_disk_images( stralloc_t*  out )
{
    char  datadir[256];

    bufprint_config_path( datadir, datadir + sizeof(datadir) );

    PRINTF(
    "  The emulator needs several key image files to run appropriately.\n"
    "  Their exact location depends on whether you're using the emulator\n"
    "  from the Android SDK, or not (more details below).\n\n"

    "  The minimal required image files are the following:\n\n"

    "    kernel-qemu      the emulator-specific Linux kernel image\n"
    "    ramdisk.img      the ramdisk image used to boot the system\n"
    "    system.img       the *initial* system image\n"
    "    vendor.img       the *initial* vendor image\n"
    "    userdata.img     the *initial* data partition image\n\n"

    "  It will also use the following writable image files:\n\n"

    "    userdata-qemu.img  the persistent data partition image\n"
    "    system-qemu.img    an *optional* persistent system image\n"
    "    vendor-qemu.img    an *optional* persistent vendor image\n"
    "    cache.img          an *optional* cache partition image\n"
    "    sdcard.img         an *optional* SD Card partition image\n\n"
    "    snapshots.img      an *optional* state snapshots image\n\n"

    "  If you use a virtual device, its content directory should store\n"
    "  all writable images, and read-only ones will be found from the\n"
    "  corresponding platform/add-on directories. See -help-sdk-images\n"
    "  for more details.\n\n"

    "  If you are building from the Android build system, you should\n"
    "  have ANDROID_PRODUCT_OUT defined in your environment, and the\n"
    "  emulator shall be able to pick-up the right image files automatically.\n"
    "  See -help-build-images for more details.\n\n"

    "  If you're neither using the SDK or the Android build system, you\n"
    "  can still run the emulator by explicitely providing the paths to\n"
    "  *all* required disk images through a combination of the following\n"
    "  options: -sysdir, -datadir, -kernel, -ramdisk, -system, -data, -cache\n"
    "  -sdcard and -snapstorage.\n\n"

    "  The actual logic being that the emulator should be able to find all\n"
    "  images from the options you give it.\n\n"

    "  For more detail, see the corresponding -help-<option> entry.\n\n"

    "  Other related options are:\n\n"

    "      -init-data       Specify an alternative *initial* user data image\n\n"

    "      -wipe-data       Copy the content of the *initial* user data image\n"
"                           (userdata.img) into the writable one (userdata-qemu.img)\n\n"

    "      -no-cache        do not use a cache partition, even if one is\n"
    "                       available.\n\n"

    "      -no-snapstorage  do not use a state snapshot image, even if one is\n"
    "                       available.\n\n"
    ,
    datadir );
}

static void
help_environment(stralloc_t*  out)
{
    PRINTF(
    "  The Android emulator looks at various environment variables when it starts:\n\n"

    "  If ANDROID_LOG_TAGS is defined, it will be used as in '-logcat <tags>'.\n\n"

    "  If 'http_proxy' is defined, it will be used as in '-http-proxy <proxy>'.\n\n"

    "  If ANDROID_VERBOSE is defined, it can contain a comma-separated list of\n"
    "  verbose items. for example:\n\n"

    "      ANDROID_VERBOSE=socket,radio\n\n"

    "  is equivalent to using the '-verbose -verbose-socket -verbose-radio'\n"
    "  options together. unsupported items will be ignored.\n\n"

    "  If ANDROID_LOG_TAGS is defined, it will be used as in '-logcat <tags>'.\n\n"

    "  If ANDROID_EMULATOR_HOME is defined, it replaces the path of the '$HOME/.android'\n"
    "  directory which contains the emulator config data (key stores, etc.).\n\n"

    "  If ANDROID_SDK_ROOT is defined, it indicates the path of the SDK\n"
    "  installation directory.\n\n"

    );
}


static void
help_debug_tags(stralloc_t*  out)
{
    int  n;

#define  _VERBOSE_TAG(x,y)   { #x, VERBOSE_##x, y },
    static const struct { const char*  name; int  flag; const char*  text; }
    verbose_options[] = {
        VERBOSE_TAG_LIST
        { 0, 0, 0 }
    };
#undef _VERBOSE_TAG

    PRINTF(
    "  the '-debug <tags>' option can be used to enable or disable debug\n"
    "  messages from specific parts of the emulator. <tags> must be a list\n"
    "  (separated by space/comma/column) of <component> names, which can be one of:\n\n"
    );

    for (n = 0; n < VERBOSE_MAX; n++)
        PRINTF( "    %-12s    %s\n", verbose_options[n].name, verbose_options[n].text );
    PRINTF( "    %-12s    %s\n", "all", "all components together\n" );

    PRINTF(
    "\n"
    "  each <component> can be prefixed with a single '-' to indicate the disabling\n"
    "  of its debug messages. for example:\n\n"

    "    -debug all,-socket,-keys\n\n"

    "  enables all debug messages, except the ones related to network sockets\n"
    "  and key bindings/presses\n\n"
    );
}

static void
help_char_devices(stralloc_t*  out)
{
    PRINTF(
    "  various emulation options take a <device> specification that can be used to\n"
    "  specify something to hook to an emulated device or communication channel.\n"
    "  here is the list of supported <device> specifications:\n\n"

    "      stdio\n"
    "          standard input/output. this may be subject to character\n"
    "          translation (e.g. LN <=> CR/LF)\n\n"

    "      COM<n>   [Windows only]\n"
    "          where <n> is a digit. host serial communication port.\n\n"

    "      pipe:<filename>\n"
    "          named pipe <filename>\n\n"

    "      file:<filename>\n"
    "          write output to <filename>, no input can be read\n\n"

    "      pty  [Linux only]\n"
    "          pseudo TTY (a new PTY is automatically allocated)\n\n"

    "      /dev/<file>  [Unix only]\n"
    "          host char device file, e.g. /dev/ttyS0. may require root access\n\n"

    "      /dev/parport<N>  [Linux only]\n"
    "          use host parallel port. may require root access\n\n"

    "      unix:<path>[,server][,nowait]]     [Unix only]\n"
    "          use a Unix domain socket. if you use the 'server' option, then\n"
    "          the emulator will create the socket and wait for a client to\n"
    "          connect before continuing, unless you also use 'nowait'\n\n"

    "      tcp:[<host>]:<port>[,server][,nowait][,nodelay]\n"
    "          use a TCP socket. 'host' is set to localhost by default. if you\n"
    "          use the 'server' option will bind the port and wait for a client\n"
    "          to connect before continuing, unless you also use 'nowait'. the\n"
    "          'nodelay' option disables the TCP Nagle algorithm\n\n"

    "      telnet:[<host>]:<port>[,server][,nowait][,nodelay]\n"
    "          similar to 'tcp:' but uses the telnet protocol instead of raw TCP\n\n"

    "      udp:[<remote_host>]:<remote_port>[@[<src_ip>]:<src_port>]\n"
    "          send output to a remote UDP server. if 'remote_host' is no\n"
    "          specified it will default to '0.0.0.0'. you can also receive input\n"
    "          through UDP by specifying a source address after the optional '@'.\n\n"

    "      fdpair:<fd1>,<fd2>  [Unix only]\n"
    "          redirection input and output to a pair of pre-opened file\n"
    "          descriptors. this is mostly useful for scripts and other\n"
    "          programmatic launches of the emulator.\n\n"

    "      none\n"
    "          no device connected\n\n"

    "      null\n"
    "          the null device (a.k.a /dev/null on Unix, or NUL on Win32)\n\n"

    "  NOTE: these correspond to the <device> parameter of the QEMU -serial option\n"
    "        as described on http://bellard.org/qemu/qemu-doc.html#SEC10\n\n"
    );
}

static void
help_avd(stralloc_t*  out)
{
    PRINTF(
    "  use '-avd <name>' to start the emulator program with a given Android\n"
    "  Virtual Device (a.k.a. AVD), where <name> must correspond to the name\n"
    "  of one of the existing AVDs available on your host machine.\n\n"

      "See -help-virtual-device to learn how to create/list/manage AVDs.\n\n"

    "  As a special convenience, using '@<name>' is equivalent to using\n"
    "  '-avd <name>'.\n\n"
    );
}

static void
help_avd_arch(stralloc_t*  out)
{
    PRINTF(
    "  use '-avd-arch <target>' to start the emulator program with a specific\n"
    "  target architecture and without having to create an AVD.\n\n"
    );
}

static void
help_sysdir(stralloc_t*  out)
{
    char   systemdir[MAX_PATH];
    char   *p = systemdir, *end = p + sizeof(systemdir);

    p = bufprint_app_dir( p, end );
    p = bufprint( p, end, PATH_SEP "lib" PATH_SEP "images" );

    PRINTF(
    "  use '-sysdir <dir>' to specify a directory where system read-only\n"
    "  image files will be searched. on this system, the default directory is:\n\n"
    "      %s\n\n", systemdir );

    PRINTF(
    "  see '-help-disk-images' for more information about disk image files\n\n" );
}

static void
help_datadir(stralloc_t*  out)
{
    char  datadir[MAX_PATH];

    bufprint_config_path(datadir, datadir + sizeof(datadir));

    PRINTF(
    "  use '-datadir <dir>' to specify a directory where writable image files\n"
    "  will be searched. on this system, the default directory is:\n\n"
    "      %s\n\n", datadir );

    PRINTF(
    "  see '-help-disk-images' for more information about disk image files\n\n" );
}

static void
help_kernel(stralloc_t*  out)
{
    PRINTF(
    "  use '-kernel <file>' to specify a Linux kernel image to be run.\n"
    "  the default image is 'kernel-qemu' from the system directory.\n\n"

    "  you can use '-debug-kernel' to see debug messages from the kernel\n"
    "  to the terminal\n\n"

    "  see '-help-disk-images' for more information about disk image files\n\n"
    );
}

static void
help_ramdisk(stralloc_t*  out)
{
    PRINTF(
    "  use '-ramdisk <file>' to specify a Linux ramdisk boot image to be run in\n"
    "  the emulator. the default image is 'ramdisk.img' from the system directory.\n\n"

    "  see '-help-disk-images' for more information about disk image files\n\n"
    );
}

static void
help_logcat_output(stralloc_t*  out)
{
    PRINTF(
    "  use '-logcat-output <file>' to specify a path to save logcat output\n\n"
    );
}

static void
help_system(stralloc_t*  out)
{
    PRINTF(
    "  use '-system <file>' to specify the intial system image that will be loaded.\n"
    "  the default image is 'system.img' from the system directory.\n\n"

    "  NOTE: In previous releases of the Android SDK, this option was named '-image'.\n"
    "        And using '-system <path>' was equivalent to using '-sysdir <path>' now.\n\n"

    "  see '-help-disk-images' for more information about disk image files\n\n"
    );
}

static void
help_vendor(stralloc_t*  out)
{
    PRINTF(
    "  use '-vendor <file>' to specify the intial system image that will be loaded.\n"
    "  the default image is 'vendor.img' from the system directory.\n\n"

    "  see '-help-disk-images' for more information about disk image files\n\n"
    );
}

static void
help_encryption_key(stralloc_t*  out)
{
    PRINTF(
    "  use '-encryption-key <file>' to specify the intial encryption key image that will be loaded.\n"
    "  the default image is 'encryptionkey.img' from the system directory.\n\n"

    "  see '-help-disk-images' for more information about disk image files\n\n"
    );
}

static void
help_image(stralloc_t*  out)
{
    PRINTF(
    "  This option is obsolete, you should use '-system <file>' instead to point\n"
    "  to the initial system image.\n\n"

    "  see '-help-disk-images' for more information about disk image files\n\n"
    );
}

static void
help_data(stralloc_t*  out)
{
    PRINTF(
    "  use '-data <file>' to specify a different /data partition image file.\n\n"

    "  see '-help-disk-images' for more information about disk image files\n\n"
    );
}

static void
help_wipe_data(stralloc_t*  out)
{
    PRINTF(
    "  use '-wipe-data' to reset your /data partition image to its factory\n"
    "  defaults. this removes all installed applications and settings.\n\n"

    "  see '-help-disk-images' for more information about disk image files\n\n"
    );
}

static void
help_writable_system(stralloc_t* out)
{
    PRINTF(
    "  use -writable-system to have a writable system image during your\n"
    "  emulation session. More specifically, one should:\n\n"

    "  - Start the AVD with the '-writable-system' flag.\n"
    "  - Use 'adb remount' from a host terminal to tell the guest system to\n"
    "    remount /system as read/write (it is mounted as readonly by default).\n"
    "\n"
    "  Note that using this flag will create a temporary copy of the system\n"
    "  image that can be very large (several hundreds of MBs), but will be\n"
    "  destroyed at emulator exit.\n"
    );
}

static void
help_cache(stralloc_t*  out)
{
    PRINTF(
    "  use '-cache <file>' to specify a /cache partition image. if <file> does\n"
    "  not exist, it will be created empty. by default, the cache partition is\n"
    "  backed by a temporary file that is deleted when the emulator exits.\n"
    "  using the -cache option allows it to be persistent.\n\n"

    "  the '-no-cache' option can be used to disable the cache partition.\n\n"

    "  see '-help-disk-images' for more information about disk image files\n\n"
    );
}

static void
help_cache_size(stralloc_t*  out)
{
    PRINTF(
    "  use '-cache <size>' to specify a /cache partition size in MB. By default,\n"
    "  the cache partition size is set to 66MB\n\n"
    );
}

static void
help_no_cache(stralloc_t*  out)
{
    PRINTF(
    "  use '-no-cache' to disable the cache partition in the emulated system.\n"
    "  the cache partition is optional, but when available, is used by the browser\n"
    "  to cache web pages and images\n\n"

    "  see '-help-disk-images' for more information about disk image files\n\n"
    );
}

static void
help_sdcard(stralloc_t*  out)
{
    PRINTF(
    "  use '-sdcard <file>' to specify a SD Card image file that will be attached\n"
    "  to the emulator. By default, the 'sdcard.img' file is searched in the data\n"
    "  directory.\n\n"

    "  if the file does not exist, the emulator will still start, but without an\n"
    "  attached SD Card.\n\n"

    "  see '-help-disk-images' for more information about disk image files\n\n"
    );
}

static void
help_snapstorage(stralloc_t*  out)
{
    PRINTF(
    "  Use '-snapstorage <file>' to specify a repository file for snapshots.\n"
    "  All snapshots made during execution will be saved in this file, and only\n"
    "  snapshots in this file can be restored during the emulator run.\n\n"

    "  If the option is not specified, it defaults to 'snapshots.img' in the\n"
    "  data directory. If the specified file does not exist, the emulator will\n"
    "  start, but without support for saving or loading state snapshots.\n\n"

    "  see '-help-disk-images' for more information about disk image files\n"
    "  see '-help-snapshot' for more information about snapshots\n\n"
    );
}

static void
help_no_snapstorage(stralloc_t*  out)
{
    PRINTF(
    "  This starts the emulator without mounting a file to store or load state\n"
    "  snapshots, forcing a full boot and disabling state snapshot functionality.\n\n"
    ""
    "  This command overrides the configuration specified by the parameters\n"
    "  '-snapstorage' and '-snapshot'. A warning will be raised if either\n"
    "  of those parameters was specified anyway.\n\n"
     );
 }

static void
help_snapshot(stralloc_t*  out)
{
    PRINTF(
    "  Rather than executing a full boot sequence, the Android emulator can\n"
    "  resume execution from an earlier state snapshot (which is usually\n"
    "  significantly faster). When the parameter '-snapshot <name>' is given,\n"
    "  the emulator loads the snapshot of that name from the snapshot image,\n"
    "  and saves it back under the same name on exit.\n\n"

    "  If the option is not specified, it defaults to 'default-boot'. If the\n"
    "  specified snapshot does not exist, the emulator will perform a full boot\n"
    "  sequence instead, but will still save.\n\n"

    "  WARNING: In the process of loading, all contents of the system, userdata\n"
    "           and SD card images will be OVERWRITTEN with the contents they\n"
    "           held when the snapshot was made. Unless saved in a different\n"
    "           snapshot, any changes since will be lost!\n\n"

    "  If you want to create a snapshot manually, connect to the emulator console:\n\n"

    "      telnet localhost <port>\n\n"

    "  Then execute the command 'avd snapshot save <name>'. See '-help-port' for\n"
    "  information on obtaining <port>.\n\n"
    );
}

static void
help_no_snapshot(stralloc_t*  out)
{
    PRINTF(
    "  This inhibits both the autoload and autosave operations, forcing\n"
    "  emulator to perform a full boot sequence and losing state on close.\n"
    "  It overrides the '-snapshot' parameter.\n"
    "  If '-snapshot' was specified anyway, a warning is raised.\n\n"
    );
}

static void
help_no_snapshot_load(stralloc_t*  out)
{
    PRINTF(
    "  Prevents the emulator from loading the AVD's state from the snapshot\n"
    "  storage on start.\n\n"
    );
}

static void help_qcow2_for_userdata(stralloc_t* out) {
    PRINTF("  Use Qcow2 format for userdata (instead of the default "
           "ext4).\n\n");
}

static void
help_no_snapshot_save(stralloc_t*  out)
{
    PRINTF(
    "  Prevents the emulator from saving the AVD's state to the snapshot\n"
    "  storage on exit, meaning that all changes will be lost.\n\n"
    );
}

static void
help_no_snapshot_update_time(stralloc_t*  out)
{
    PRINTF(
    "  Prevent the emulator from sending an unsolicited time update\n"
    "  in response to the first signal strength query after loadvm,\n"
    "  to avoid a sudden time jump that might upset testing. (Signal\n"
    "  strength is queried approximately every 15 seconds)\n\n"
    );
}

static void
help_snapshot_list(stralloc_t*  out)
{
    PRINTF(
    "  This prints a table of snapshots that are stored in the snapshot storage\n"
    "  file that the emulator was started with, then exits. Values from the 'ID'\n"
    "  and 'TAG' columns can be used as arguments for the '-snapshot' parameter.\n\n"

    "  If '-snapstorage <file>' was specified as well, this command prints a "
    "  table of the snapshots stored in <file>.\n\n"

    "  See '-help-snapshot' for more information on snapshots.\n\n"
    );
}

static void
help_accel(stralloc_t *out)
{
    PRINTF(
        "  Use '-accel <mode>' to control how CPU emulation can be accelerated\n"
        "  when launching the Android emulator. Accelerated emulation only works\n"
        "  for x86 and x86_64 system images. On Linux, it relies on KVM being\n"
        "  installed. On Windows and OS X, it relies on an Intel CPU and the\n"
        "  Intel HAXM driver being installed on your development machine.\n"
        "  Valid values for <mode> are:\n\n"

        "     auto   The default, determines automatically if acceleration\n"
        "            is supported, and uses it when possible.\n\n"

        "     off    Disables acceleration entirely. Mostly useful for debugging.\n\n"

        "     on     Force acceleration. If KVM/HAXM/WHPX is not installed or usable,\n"
        "            the emulator will refuse to start and print an error message.\n\n"

        "  Note that this flag is ignored if you're not emulating an x86 or x86_64\n"
    );
}

static void
help_no_accel(stralloc_t* out)
{
    PRINTF(
        "  Use '-no-accel' as a shortcut to '-accel off', i.e. to disable accelerated\n"
        "  CPU emulation, when emulating an x86 or x86_64 system image. Only useful\n"
        "  for debugging.\n"
    );
}

static void
help_ranchu(stralloc_t* out)
{
    PRINTF(
        "  DEPRECATED: Previously used to use the new emulator engine as the\n"
        "  preferred backend. Now this option is simply ignored and the 'emulator'\n"
        "  launcher will auto-detect the engine, favoring QEMU2 over the classic one\n"
    );
}

static void
help_engine(stralloc_t* out)
{
    PRINTF(
        "  Select the emulation engine to use. Valid values are\n\n"
        "      auto    -> perform auto-detection (default)\n"
        "      classic -> use the classic emulator engine\n"
        "      qemu2   -> use the newer emulator engine\n\n"

        "  Auto-detection should provide the value that provides the best\n"
        "  performance when emulating a given AVD. Only use this option\n"
        "  for debugging / comparison purposes.\n\n"
    );
}


static void
help_skindir(stralloc_t*  out)
{
    PRINTF(
    "  use '-skindir <dir>' to specify a directory that will be used to search\n"
    "  for emulator skins. each skin must be a subdirectory of <dir>. by default\n"
    "  the emulator will look in the 'skins' sub-directory of the system directory\n\n"

    "  the '-skin <name>' option is required when -skindir is used.\n"
    );
}

static void
help_skin(stralloc_t*  out)
{
    PRINTF(
    "  use '-skin <skin>' to specify an emulator skin, each skin corresponds to\n"
    "  the visual appearance of a given device, including buttons and keyboards,\n"
    "  and is stored as subdirectory <skin> of the skin root directory\n"
    "  (see '-help-skindir')\n\n" );

    PRINTF(
    "  note that <skin> can also be '<width>x<height>' (e.g. '320x480') to\n"
    "  specify an exact framebuffer size, without any visual ornaments.\n\n" );
}

static void
help_shaper(stralloc_t*  out)
{
    int  n;
    PRINTF(
    "  the Android emulator supports network throttling, i.e. slower network\n"
    "  bandwidth as well as higher connection latencies. this is done either through\n"
    "  skin configuration, or with '-netspeed <speed>' and '-netdelay <delay>'.\n\n"

    "  the format of -netspeed is one of the following (numbers are kbits/s):\n\n" );

    for (n = 0; n < android_network_speeds_count; ++n) {
        const AndroidNetworkSpeed* android_netspeed =
                &android_network_speeds[n];
        PRINTF( "    -netspeed %-12s %-15s  (up: %.1f KiB/s, down: %.1f KiB/s)\n",
                        android_netspeed->name,
                        android_netspeed->display_name,
                        android_netspeed->upload_bauds/8192.,
                        android_netspeed->download_bauds/8192. );
    }
    PRINTF( "\n" );
    PRINTF( "    -netspeed %-12s %s", "<num>", "select both upload and download speed\n");
    PRINTF( "    -netspeed %-12s %s", "<up>:<down>", "select individual up and down speed\n");

    PRINTF( "\n  The format of -netdelay is one of the following (numbers are msec):\n\n" );
    for (n = 0; n < android_network_latencies_count; ++n) {
        const AndroidNetworkLatency* android_netdelay =
                &android_network_latencies[n];
        PRINTF( "    -netdelay %-10s   %-15s  (min %d, max %d)\n",
                        android_netdelay->name, android_netdelay->display_name,
                        android_netdelay->min_ms, android_netdelay->max_ms );
    }
    PRINTF( "    -netdelay %-10s   %s", "<num>", "select exact latency\n");
    PRINTF( "    -netdelay %-10s   %s", "<min>:<max>", "select min and max latencies\n\n");

    PRINTF( "  the emulator uses the following defaults:\n\n" );
    PRINTF( "    Default network speed   is '%s'\n",   kAndroidNetworkDefaultSpeed);
    PRINTF( "    Default network latency is '%s'\n\n", kAndroidNetworkDefaultLatency);
}

static void
help_wifi_client_port(stralloc_t* out)
{
    PRINTF(
    "  the Android emulator allows forwarding of WiFi traffic from one emulator\n"
    "  to another emulator. This makes it appear as if the two emulators are on\n"
    "  the same WiFi network. In order to do this one emulator acts as a server\n"
    "  and the other emulators acts as a client. The client emulator connects\n"
    "  to the server emulator and the WiFi traffic is then sent between them.\n"
    "  Setting this parameter indicates that this emulator should act as a client\n"
    "  and attempt to connect to a server emulator on the provided port. The\n"
    "  client emulator will continually attempt to connect on the given port\n"
    "  so the order that the emulators start in does not matter. If the server\n"
    "  emulator is shut down the client will be disconnected but will try to\n"
    "  reconnect again. Starting a new server emulator will establish the\n"
    "  connection once more.\n\n" );
}

static void
help_wifi_server_port(stralloc_t* out)
{
    PRINTF(
    "  the Android emulator allows forwarding of WiFi traffic from one emulator\n"
    "  to another emulator. This makes it appear as if the two emulators are on\n"
    "  the same WiFi network. In order to do this one emulator acts as a server\n"
    "  and the other emulators acts as a client. The client emulator connects\n"
    "  to the server emulator and the WiFi traffic is then sent between them.\n"
    "  Setting this parameter indicates that this emulator should act as a server\n"
    "  and listen to connection attempts from client emulators on the provided\n"
    "  port. The server emulator will stop listening as soon as a client emulator\n"
    "  connects, allowing only one client at a time. If a client disconnects the\n"
    "  server emulator will start listening for connections again and a new client\n"
    "  can connect.\n\n" );
}

static void
help_http_proxy(stralloc_t*  out)
{
    PRINTF(
    "  the Android emulator allows you to redirect all TCP connections through\n"
    "  a HTTP/HTTPS proxy. this can be enabled by using the '-http-proxy <proxy>'\n"
    "  option, or by defining the 'http_proxy' environment variable.\n\n"

    "  <proxy> can be one of the following:\n\n"
    "    http://<server>:<port>\n"
    "    http://<username>:<password>@<server>:<port>\n\n"

    "  the 'http://' prefix can be omitted. If '-http-proxy <proxy>' is not used,\n"
    "  the 'http_proxy' environment variable is looked up and any value matching\n"
    "  the <proxy> format will be used automatically\n\n" );
}

static void
help_report_console(stralloc_t*  out)
{
    PRINTF(
    "  the '-report-console <socket>' option can be used to report the\n"
    "  automatically-assigned console port number to a remote third-party\n"
    "  before starting the emulation. <socket> must be in one of these\n"
    "  formats:\n\n"

    "      tcp:<port>[,server][,max=<seconds>][,ipv6]\n"
    "      unix:<path>[,server][,max=<seconds>][,ipv6]\n"
    "\n"
    "  if the 'server' option is used, the emulator opens a server socket\n"
    "  and waits for an incoming connection to it. by default, it will instead\n"
    "  try to make a normal client connection to the socket, and, in case of\n"
    "  failure, will repeat this operation every second for 10 seconds.\n"
    "  the 'max=<seconds>' option can be used to modify the timeout\n\n"

    "  if the 'ipv6' option is used, the interface used will be ::1, otherwise\n"
    "  it will be 127.0.0.1\n\n"

    "  when the connection is established, the emulator sends its console port\n"
    "  number as text to the remote third-party, then closes the connection and\n"
    "  starts the emulation as usual. *any* failure in the process described here\n"
    "  will result in the emulator aborting immediately\n\n"

    "  as an example, here's a small Unix shell script that starts the emulator in\n"
    "  the background and waits for its port number with the help of the 'netcat'\n"
    "  utility:\n\n"

    "      MYPORT=5000\n"
    "      emulator -no-window -report-console tcp:$MYPORT &\n"
    "      CONSOLEPORT=`nc -l localhost $MYPORT`\n"
    "\n"
    );
}

static void
help_ui_only(stralloc_t*  out)
{
    PRINTF(
    "  the '-ui-only <UI feature>' option is used to run only that part\n"
    "  of the emulator that displays the requested UI feature.\n"
    "  No virtual device is launched.\n"
    "  When the UI feature is closed, the emulator exits.\n\n"
    "  Supported UI features are:\n"
    "      snapshot-control\n"
    "\n"
    );
}

static void
help_id(stralloc_t*  out)
{
    PRINTF(
    "  '-id <name>' specifies the ID of the virtual device\n"
    "  separate from the AVD name. If not specified, the ID\n"
    "  defaults to the AVD name.\n"
    "  This can be useful when running multiple clones of the same device.\n"
    "  The ID associated with $device_serial can be queried via\n"
    "  adb -s $device_serial emu avd id\n\n"
    );
}

static void
help_dpi_device(stralloc_t*  out)
{
    PRINTF(
    "  the '-dpi-device <dpi>' option is obsolete as of Emulator 2.0 and will be ignored\n\n" );
}

static void
help_audio(stralloc_t*  out)
{
    PRINTF(
    "  the '-audio <backend>' option allows you to select a specific backend\n"
    "  to be used to both play and record audio in the Android emulator.\n\n"

    "  use '-audio none' to disable audio completely.\n\n"
    );
}

static void
help_scale(stralloc_t*  out)
{
    PRINTF(
    "  the '-scale <scale>' option is obsolete as of Emulator 2.0 and will be ignored\n\n" );
}

static void
help_code_profile(stralloc_t*  out)
{
    PRINTF(
    "  use '-code_profile <name>' to start the emulator with runtime code profiling support.\n\n"
    "  Profiles are stored in directory <name>, each executable will have its own profile.\n"
    "  The profile can be further processed to generate an AutoFDO profile, which can be \n"
    "  used to drive feedback directed optimizations.\n"
    "  More details can be found from https://gcc.gnu.org/wiki/AutoFDO.\n\n"
    "  IMPORTANT: This feature is *experimental* and requires a patched kernel.\n"
    "  as such, it will not work on regular SDK system images.\n\n"
    );
}

static void
help_show_kernel(stralloc_t*  out)
{
    PRINTF(
    "  use '-show-kernel' to redirect debug messages from the kernel to the current\n"
    "  terminal. this is useful to check that the boot process works correctly.\n\n"
    );
}

static void
help_shell(stralloc_t*  out)
{
    PRINTF(
    "  use '-shell' to create a root shell console on the current terminal.\n"
    "  this is unlike the 'adb shell' command for the following reasons:\n\n"

    "  * this is a *root* shell that allows you to modify many parts of the system\n"
    "  * this works even if the ADB daemon in the emulated system is broken\n"
    "  * pressing Ctrl-C will stop the emulator, instead of the shell.\n\n"
    "  See also '-shell-serial'.\n\n" );
}

static void help_virtio_console(stralloc_t* out) {
    PRINTF("  use '-virtio-console' to use virtio console hvc0 instead of "
           "default serial line\n\n");
}

static void
help_shell_serial(stralloc_t*  out)
{
    PRINTF(
    "  use '-shell-serial <device>' instead of '-shell' to open a root shell\n"
    "  to the emulated system, while specifying an external communication\n"
    "  channel / host device.\n\n"

    "  '-shell-serial stdio' is identical to '-shell', while you can use\n"
    "  '-shell-serial tcp::4444,server,nowait' to talk to the shell over local\n"
    "  TCP port 4444.  '-shell-serial fdpair:3:6' would let a parent process\n"
    "  talk to the shell using fds 3 and 6.\n\n"

    "  see -help-char-devices for a list of available <device> specifications.\n\n"
    "  NOTE: you can have only one shell per emulator instance at the moment\n\n"
    );
}

static void
help_logcat(stralloc_t*  out)
{
    PRINTF(
    "  use '-logcat <tags>' to redirect log messages from the emulated system to\n"
    "  the current terminal. <tags> is a list of space/comma-separated log filters\n"
    "  where each filter has the following format:\n\n"

    "     <componentName>:<logLevel>\n\n"

    "  where <componentName> is either '*' or the name of a given component,\n"
    "  and <logLevel> is one of the following letters:\n\n"

    "      v          verbose level\n"
    "      d          debug level\n"
    "      i          informative log level\n"
    "      w          warning log level\n"
    "      e          error log level\n"
    "      s          silent log level\n\n"

    "  for example, the following only displays messages from the 'GSM' component\n"
    "  that are at least at the informative level:\n\n"

    "    -logcat '*:s GSM:i'\n\n"

    "  if '-logcat <tags>' is not used, the emulator looks for ANDROID_LOG_TAGS\n"
    "  in the environment. if it is defined, its value must match the <tags>\n"
    "  format and will be used to redirect log messages to the terminal.\n\n"

    "  note that this doesn't prevent you from redirecting the same, or other,\n"
    "  log messages through the ADB too.\n\n");
}

static void
help_no_audio(stralloc_t*  out)
{
    PRINTF(
    "  use '-no-audio' to disable all audio support in the emulator. this may be\n"
    "  unfortunately be necessary in some cases:\n\n"

    "  * at least two users have reported that their Windows machine rebooted\n"
    "    instantly unless they used this option when starting the emulator.\n"
    "    it is very likely that the problem comes from buggy audio drivers.\n\n"

    "  * on some Linux machines, the emulator might get stuck at startup with\n"
    "    audio support enabled. this problem is hard to reproduce, but seems to\n"
    "    be related too to flaky ALSA / audio driver support.\n\n"

    "  on Linux, another option is to try to change the default audio backend\n"
    "  used by the emulator. you can do that by setting the QEMU_AUDIO_DRV\n"
    "  environment variables to one of the following values:\n\n"

    "    alsa        (use the ALSA backend)\n"
    "    esd         (use the EsounD backend)\n"
    "    sdl         (use the SDL audio backend, no audio input supported)\n"
    "    oss         (use the OSS backend)\n"
    "    none        (do not support audio)\n"
    "\n"
    "  the very brave can also try to use distinct backends for audio input\n"
    "  and audio outputs, this is possible by selecting one of the above values\n"
    "  into the QEMU_AUDIO_OUT_DRV and QEMU_AUDIO_IN_DRV environment variables.\n\n"
    );
}

static void
help_radio(stralloc_t*  out)
{
    PRINTF(
    "  use '-radio <device>' to redirect the GSM modem emulation to an external\n"
    "  character device or program. this bypasses the emulator's internal modem\n"
    "  and should only be used for testing.\n\n"

    "  see '-help-char-devices' for the format of <device>\n\n"

    "  the data exchanged with the external device/program are GSM AT commands\n\n"

    "  note that, when running in the emulator, the Android GSM stack only supports\n"
    "  a *very* basic subset of the GSM protocol. trying to link the emulator to\n"
    "  a real GSM modem is very likely to not work properly.\n\n"
    );
}


static void
help_port(stralloc_t*  out)
{
    PRINTF(
    "  at startup, the emulator tries to bind its control console at a free port\n"
    "  starting from 5554, in increments of two (i.e. 5554, then 5556, 5558, etc..)\n"
    "  this allows several emulator instances to run concurrently on the same\n"
    "  machine, each one using a different console port number.\n\n"

    "  use '-port <port>' to force an emulator instance to use a given console port\n\n"

    "  note that <port> must be an *even* integer between 5554 and 5584 included.\n"
    "  <port>+1 must also be free and will be reserved for ADB. if any of these\n"
    "  ports is already used, the emulator will fail to start.\n\n" );
}

static void
help_ports(stralloc_t*  out)
{
    PRINTF(
    "  the '-ports <consoleport>,<adbport>' option allows you to explicitely set\n"
    "  the TCP ports used by the emulator to implement its control console and\n"
    "  communicate with the ADB tool.\n\n"

    "  This is a very special option that should probably *not* be used by typical\n"
    "  developers using the Android SDK (use '-port <port>' instead), because the\n"
    "  corresponding instance is probably not going to be seen from adb. \n"
    "  It requires you to explicitly enable the gRPC port if you need it. Its\n"
    "  purpose is to use the emulator in very specific network configurations.\n\n"

    "    <consoleport> is the TCP port used to bind the control console\n"
    "    <adbport> is the TCP port used to bind the ADB local transport/tunnel.\n\n"

    "  If both ports aren't available on startup, the emulator will exit.\n\n");
}

static void
help_modem_simulator_port(stralloc_t*  out)
{
    PRINTF(
    "  by default the emulator tries to bind a free port for modem simulator.\n\n"

    "  use '-modem-simulaotor-port <port>' to force an emulator instance\n"
    "  to use a given modem simulaotor port\n\n"

    "  If the specified port isn't available on startup, the emulator will exit.\n\n");
}

static void
help_log_nofilter(stralloc_t*  out)
{
    PRINTF(
    "  Disables the duplicate logline filter.\n\n");
}

static void
help_log_detailed(stralloc_t*  out)
{
    PRINTF(
    "  Enable detailed logging.\n\n");
}


static void
help_grpc(stralloc_t*  out)
{
    PRINTF(
    "  Enables the gRPC service to control the emulator.\n\n"
    "    <port> is the TCP port used to bind the gRPC service\n\n"
    "  Note: You will have to explicitly turn on jwt authorization with the -grpc-use-jwt flag\n\n"
    "  The gRPC service will not be started if the port is not available on startup.\n\n");
}

static void
help_grpc_tls_key(stralloc_t*  out)
{
    PRINTF(
    "  Use the PEM file containing a private key for TLS.\n\n"
    "    <pem> a PEM file containing a private key used for TLS.\n\n"
    "  You must provide the -grpc-tls-cer flag as well.\n\n"
    );
}

static void
help_grpc_tls_cer(stralloc_t*  out)
{
    PRINTF(
    "  Use the given file containing a X509 certificate for TLS.\n\n"
    "    <pem> a PEM file with a X.509 public key certificate.\n\n"
    "  The X509 certificate can be self signed.\n\n"
    "  You must provide the -grpc-tls-key flag as well.\n\n"
    );
}

static void
help_grpc_tls_ca(stralloc_t*  out)
{
    PRINTF(
    "  Use the file containing a series of certificate authorities for client validation.\n\n"
    "    <pem> The file with a root certificate chain to use.\n\n"
    "  This file must be a PEM encoded file with all the roots such as the one that\n"
    "  can be downloaded from https://pki.google.com/roots.pem.\n\n"
    "  The emulator will request and validate the client certificate.\n"
    "  Clients with invalid certificates will be rejected.\n"
    );

}

static void
help_grpc_use_token(stralloc_t*  out)
{
    PRINTF(
    "  Require an authorization header with a valid token for every grpc call.\n\n"
    "  Every grpc request expects the following header:\n\n"
    "    authorization: Bearer <token>\n\n"
    "  The token can be found in the android studio discovery file under the key: grpc.token.\n"
    "  If an incorrect token is present the status UNAUTHORIZED will be returned.\n\n"
    "  The location of the discovery directory is %s/avd/running\n"
    "  The file will be named `pid_%%d_info.ini` where %%d is the process id of the emulator. \n\n"
    "  Note: Token based security can only be installed if you are using localhost or TLS.\n\n",
#ifdef __linux__
    "$XDG_RUNTIME_DIR"
#endif
#ifdef __APPLE__
    "$HOME/Library/Caches/TemporaryItems"
#endif
#ifdef _WIN32
    "\%LOCALAPPDATA\%/Temp"
#endif
    );
}


static void
help_grpc_use_jwt(stralloc_t*  out)
{
    PRINTF(
    "  Require an authorization header with a valid jwt token for every grpc call.\n\n"
    "  Every grpc request expects the following header:\n\n"
    "    authorization: Bearer <token>\n\n"
    "  Where token is a valid signed JSON Web Token (JWT)\n\n"
    "  If an incorrect token is present the status UNAUTHORIZED will be returned.\n\n"
    "  In order to make a succesful call a JWT token must be presented with the following fields:\n\n"
    "  - aud: must contain a list of allowed methods. \n"
    "  - iat: must have an issue in the past. \n"
    "  - exp: must have an expiration date. \n\n"
    "  In order to validate tokens the emulator will load all JSON Web Key (JWK) files from\n"
    "  a directory that is only accessible by the user who launched the emulator.\n"
    "  The location of this directory is defined in the emulator discovery file under the key grpc.jwks\n"
    "  Placing a valid JWK file with the name ${grpc.jwks}/my_filename.jwk will result in the JWK \n"
    "  being made available to the emulator. Note that `my_filename` can be an arbitrary file name.\n\n"
    "  The emulator will merge all discovered keys and write them to the file in the JWK format. The file\n"
    "  is defined in the emulator discovery file under the key grpc.jwk_active.\n\n"
    "  Developers can use this to make sure that the key they generated has been picked up by the emulator.\n\n"
    "  The location of the discovery file can be found in the directory is %s/avd/running\n"
    "  The file will be named `pid_%%d_info.ini` where %%d is the process id of the emulator. \n\n"
    "  Notes\n"
    "   - Tokens must be signed with one of the following algorithms: ES256, ES384, ES512, RS256, RS384, RS512\n"
    "   - Token based security can only be installed if you are using localhost or TLS.\n\n",
#ifdef __linux__
    "$XDG_RUNTIME_DIR"
#endif
#ifdef __APPLE__
    "$HOME/Library/Caches/TemporaryItems"
#endif
#ifdef _WIN32
    "\%LOCALAPPDATA\%/Temp"
#endif
    );
}

static void
help_grpc_allowlist(stralloc_t*  out)
{
    PRINTF(
    "  A list that defines which gRPC methods can be accessed by whom.\n\n"
    "    <path_to_json_file> A json file describing the access rules.\n"
    "    see details in lib/emulator_access.json on how to specify rules.\n\n");
}

static void
help_idle_grpc_timeout(stralloc_t*  out)
{
    PRINTF(
    "  Terminates the emulator if there is no interaction on the gRPC endpoint.\n\n"
    "    <timeout> Time in seconds before the emulator should terminate.\n"
    "    at first a clean shutdown will be attempted, if this does not succeed\n"
    "    within <timeout> seconds, the process will be killed.\n\n"
    "  Make sure that timeout is large enough to give the emulator a chance to \n"
    "  attempt a clean shutdown.\n\n");
}

static void
help_waterfall(stralloc_t*  out)
{
    PRINTF(
    "  Mode in which to run the waterfall grpc endpoint..\n\n"
    "    \n\n"
    "    fwd:  Forward the waterfall commands to the waterfall service\n"
    "          running inside the emulator.\n"
    "    adb:  Translate the waterfall commands to ADB.\n\n"
    );
}

static void
help_grpc_ui(stralloc_t*  out)
{
    PRINTF(
    "  Use the experimental gRPC based ui. This ui will make gRPC calls to accomplish tasks.\n\n");
}


static void
help_share_vid(stralloc_t*  out)
{
    PRINTF(
    "  Share the current video state in a shared memory region..\n\n"
    "  A πexshared memory region will be created that is available under\n"
    "  the handle videmulatorXXXX where XXXX is the port number of the emulator\n\n"
    "  For example:\n\n"
    "  -ports 6000,6001 -share-vid\n\n"
    "  will create the shared memory region videmulator6000\n"
    );
}

static void
help_turncfg(stralloc_t*  out)
{
    PRINTF(
    "  Execute the given command to obtain turn configuration..\n\n"
    "    <cmd> is the command to execute\n\n"
    "  Parameters in <cmd> can be grouped with a single (') or double (\") quote\n\n"
    "  This command must do the following:\n\n"
    "  - Produce a result on stdout.\n"
    "  - Produce a result within 1000 ms.\n"
    "  - Produce a valid JSON RTCConfiguration object "
    " (See https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/RTCPeerConnection).\n"
    "  - That contain at least an \"iceServers\" array.\n"
    "  - The exit value should be 0 on success\n\n"
    "  For example:\n\n"
    "  -turncfg \"sh -c 'curl -s -X POST https://api.twilio.com/2010-04-01/Accounts/some_account/Tokens.json -u some_account:some_secret | sed \\'s/ice_servers/iceServers/g\\''\"\n\n"
    );
}

static void
help_rootcanal_controller_properties_file(stralloc_t*  out)
{
    PRINTF(
    "  Loads the controller properties on launch. \n\n"
    "  This is not officially supported, and is for internal testing only. \n\n"
    );
}

static void
help_rootcanal_no_mesh(stralloc_t*  out)
{
    PRINTF(
    "  Disables the disovery of running emulators. \n\n"
    "  ** DEPRECATED **\n\n"
    "  This flag does nothing and will be removed in future versions of the emulator.\n\n"
    );
}

static void
help_forward_vhci(stralloc_t*  out)
{
    PRINTF(
    "  Enables the gRPC service that gives access to /dev/vhci\n\n"
    "  \n\n"
    "  ** DEPRECATED **\n\n"
    "  Please implement the packet streamer interface as defined in:\n"
    "  https://android.googlesource.com/platform/tools/netsim/+/refs/heads/master/src/proto/packet_streamer.proto\n"
    "  and use the -packet-streamer-endpoint flag to provide uri to the service.\n\n"
    );
}

static void
help_packet_streamer_endpoint(stralloc_t*  out)
{
    PRINTF(
    "  Use the packet stream at the given endpoint\n\n"
    "  A packet streamer provides the emulator with a set of emulated radio devices\n"
    "  The address should follow the gRPC name resolution as explained in detail:\n"
    "  here: https://grpc.github.io/grpc/cpp/md_doc_naming.html. \n\n"
    "  NOTE: This is currently under active development (b/265451720). \n"
    "  providing this parameter will currently enable the default packet streamer. \n\n"
    );
}




static void
help_onion(stralloc_t*  out)
{
    PRINTF(
    "  use '-onion <file>' to specify a PNG image file that will be displayed on\n"
    "  top of the emulated framebuffer with translucency. this can be useful to\n"
    "  check that UI elements are correctly positioned with regards to a reference\n"
    "  graphics specification.\n\n"

    "  the default translucency is 50%%, but you can use '-onion-alpha <%%age>' to\n"
    "  select a different one, or even use keypresses at runtime to alter it\n"
    "  (see -help-keys for details)\n\n"

    "  finally, the onion image can be rotated (see -help-onion-rotate)\n\n"
    );
}

static void
help_onion_alpha(stralloc_t*  out)
{
    PRINTF(
    "  use '-onion-alpha <percent>' to change the translucency level of the onion\n"
    "  image that is going to be displayed on top of the framebuffer (see also\n"
    "  -help-onion). the default is 50%%.\n\n"

    "  <percent> must be an integer between 0 and 100.\n\n"

    "  you can also change the translucency dynamically (see -help-keys)\n\n"
    );
}

static void
help_onion_rotation(stralloc_t*  out)
{
    PRINTF(
    "  use '-onion-rotation <rotation>' to change the rotation of the onion\n"
    "  image loaded through '-onion <file>'. valid values for <rotation> are:\n\n"

    "   0        no rotation\n"
    "   1        90  degrees clockwise\n"
    "   2        180 degrees\n"
    "   3        270 degrees clockwise\n\n"
    );
}


static void
help_timezone(stralloc_t*  out)
{
    PRINTF(
    "  by default, the emulator tries to detect your current timezone to report\n"
    "  it to the emulated system. use the '-timezone <timezone>' option to choose\n"
    "  a different timezone, or if the automatic detection doesn't work correctly.\n\n"

    "  VERY IMPORTANT NOTE:\n\n"
    "  the <timezone> value must be in zoneinfo format, i.e. it should look like\n"
    "  Area/Location or even Area/SubArea/Location. valid examples are:\n\n"

    "    America/Los_Angeles\n"
    "    Europe/Paris\n\n"

    "  using a human-friendly abbreviation like 'PST' or 'CET' will not work, as\n"
    "  well as using values that are not defined by the zoneinfo database.\n\n"

    "  NOTE: unfortunately, this will not work on M5 and older SDK releases\n\n"
    );
}

static void
help_change_language(stralloc_t* out)
{
    PRINTF(
    "  Change the device's language to the language code specified.\n"
    "  The emulator will cold boot and then restart the framework again for\n"
    "  the setting to take effect.\n"
    "  Note that when doing so, the Quickboot snapshot will be overwritten\n"
    "  unless -no-snapshot-save is also issued.");
}

static void
help_change_country(stralloc_t* out)
{
    PRINTF(
    "  Change the device's country to the country code specified.\n"
    "  The emulator will cold boot and then restart the framework again for\n"
    "  the setting to take effect.\n"
    "  Note that when doing so, the Quickboot snapshot will be overwritten\n"
    "  unless -no-snapshot-save is also issued.");
}

static void
help_change_locale(stralloc_t* out)
{
    PRINTF(
    "  Change the device's locale to the locale code specified.\n"
    "  The emulator will cold boot and then restart the framework again for\n"
    "  the setting to take effect.\n"
    "  Note that when doing so, the Quickboot snapshot will be overwritten\n"
    "  unless -no-snapshot-save is also issued.");
}

static void
help_dns_server(stralloc_t*  out)
{
    PRINTF(
    "  by default, the emulator tries to detect the DNS servers you're using and\n"
    "  will setup special aliases in the emulated firewall network to allow the\n"
    "  Android system to connect directly to them. use '-dns-server <servers>' to\n"
    "  select a different list of DNS servers to be used.\n\n"

    "  <servers> must be a comma-separated list of up to 4 DNS server names or\n"
    "  IP addresses.\n\n"

    "  NOTE: on M5 and older SDK releases, only the first server in the list will\n"
    "        be used.\n\n"
    );
}

static void
help_net_tap(stralloc_t*  out)
{
    PRINTF(
    "  by default, the emulator uses user mode networking which does address\n"
    "  translation of all network traffic. Use -net-tap <tap interface> to switch\n"
    "  to TAP network mode where Android will use a TAP interface to connect to\n"
    "  the host network directly. Android will attempt to configure the TAP\n"
    "  network using DHCP for IPv4 and SLAAC for IPv6. To be able to access the\n"
    "  network that the host is connected to the TAP interface should be bridged\n"
    "  with another network interface on the host.\n\n"

    "  <tap interface> is the name of a TAP network interface such as tap0. It\n"
    "  is NOT the path to the TAP device, just the name of the device.\n\n"

    "  NOTE: Using this disables other options such as net-speed, net-delay,\n"
    "        -dns-server, -http-proxy, -tcpdump, and -shared-net-id\n\n"
    );
}

static void
help_net_tap_script_up(stralloc_t*  out)
{
    PRINTF(
    "  when using the -net-tap option it is sometimes useful to run a script when\n"
    "  TAP interface is enabled. Depending on the host system the TAP interface\n"
    "  might not be up until the emulator enables it. A common operation in such\n"
    "  a script is to add the TAP interface to a bridge so that it can be part of\n"
    "  the host network.\n"

    "  <script> the path to a script to be run when the TAP interface goes up.\n"

    "  NOTE: The script will be run with the same privileges as the emulator.\n"
    "        Ensure that this is sufficient for whatever operations the script\n"
    "        performs.\n\n"
    );
}

static void
help_net_tap_script_down(stralloc_t*  out)
{
    PRINTF(
    "  when using the -net-tap option it is sometimes useful to run a script when\n"
    "  TAP interface is disabled. Depending on the host system the TAP interface\n"
    "  might be disabled when the emulator exits. A common operation in such\n"
    "  a script is to remove the TAP interface from a bridge that it was previously\n"
    "  part of.\n"

    "  <script> the path to a script to be run when the TAP interface goes down.\n"

    "  NOTE: The script will be run with the same privileges as the emulator.\n"
    "        Ensure that this is sufficient for whatever operations the script\n"
    "        performs.\n\n"
    );
}

static void
help_net_socket(stralloc_t* out)
{
    PRINTF(
    "  by default, the emulator's network device uses user mode networking which\n"
    "  does address translation of all network traffic. Use -net-socket <option>\n"
    "  to switch to Socket network mode where network will use a socket interface to\n"
    "  connect to the host network directly. Android will attempt to configure the Socket\n"
    "  network using DHCP for IPv4 and SLAAC for IPv6. To be able to access the\n"
    "  network that the host is connected to the Socket interface should be backed\n"
    "  with another network interface on the host.\n\n"

    "  <option> depends on how the socket is configured. It could be one of fd=, listen=,\n"
    "  or connect=. See https://wiki.qemu.org/Documentation/Networking#Socket\n\n"
    );
}

static void
help_cpu_delay(stralloc_t*  out)
{
    PRINTF(
    "  this option is purely experimental, probably doesn't work as you would\n"
    "  expect, and may even disappear in a later emulator release.\n\n"

    "  use '-cpu-delay <delay>' to throttle CPU emulation. this may be useful\n"
    "  to detect weird race conditions that only happen on 'lower' CPUs. note\n"
    "  that <delay> is a unit-less integer that doesn't even scale linearly\n"
    "  to observable slowdowns. use trial and error to find something that\n"
    "  suits you, the 'correct' machine is very probably dependent on your\n"
    "  host CPU and memory anyway...\n\n"
    );
}


static void
help_no_boot_anim(stralloc_t*  out)
{
    PRINTF(
    "  use '-no-boot-anim' to disable the boot animation (red bouncing ball) when\n"
    "  starting the emulator. on slow machines, this can surprisingly speed up the\n"
    "  boot sequence in tremendous ways.\n\n"

    "  NOTE: unfortunately, this will not work on M5 and older SDK releases\n\n"
    );
}


static void
help_gps(stralloc_t*  out)
{
    PRINTF(
    "  use '-gps <device>' to emulate an NMEA-compatible GPS unit connected to\n"
    "  an external character device or socket. the format of <device> is the same\n"
    "  than the one used for '-radio <device>' (see -help-char-devices for details)\n\n"
    );
}


#ifndef _WIN32
static void
help_nand_limits(stralloc_t*  out)
{
    PRINTF(
    "  use '-nand-limits <limits>' to enable a debugging feature that sends a\n"
    "  signal to an external process once a read and/or write limit is achieved\n"
    "  in the emulated system. the format of <limits> is the following:\n\n"

    "     pid=<number>,signal=<number>,[reads=<threshold>][,writes=<threshold>]\n\n"

    "  where 'pid' is the target process identifier, 'signal' the number of the\n"
    "  target signal. the read and/or write threshold'reads' are a number optionally\n"
    "  followed by a K, M or G suffix, corresponding to the number of bytes to be\n"
    "  read or written before the signal is sent.\n\n"
    );
}
#endif /* CONFIG_NAND_LIMITS */

static void
help_bootchart(stralloc_t  *out)
{
    PRINTF(
    "  some Android system images have a modified 'init' system that  integrates\n"
    "  a bootcharting facility (see http://www.bootchart.org/). You can pass a\n"
    "  bootcharting period to the system with the following:\n\n"

    "    -bootchart <timeout>\n\n"

    "  where 'timeout' is a period expressed in seconds. Note that this won't do\n"
    "  anything if your init doesn't have bootcharting activated.\n\n"
    );
}

static void
help_tcpdump(stralloc_t  *out)
{
    PRINTF(
    "  use the -tcpdump <file> option to start capturing all network packets\n"
    "  that are sent through the emulator's virtual Ethernet LAN. You can later\n"
    "  use tools like WireShark to analyze the traffic and understand what\n"
    "  really happens.\n\n"

    "  note that this captures all Ethernet packets, and is not limited to TCP\n"
    "  connections.\n\n"

    "  you can also start/stop the packet capture dynamically through the console;\n"
    "  see the 'network capture start' and 'network capture stop' commands for\n"
    "  details.\n\n"
    );
}

static void
help_charmap(stralloc_t  *out)
{
    PRINTF(
    "  use '-charmap <file>' to use key character map specified in that file.\n"
    "  <file> must be a full path to a kcm file, containing desired character map.\n\n"
    );
}

static void
help_prop(stralloc_t  *out)
{
    PRINTF(
    "  use '-prop <name>=<value>' to set a boot-time system property.\n"
    "  <name> must be a property name of at most %d characters, without any\n"
    "  space in it, and <value> must be a string of at most %d characters.\n\n",
    BOOT_PROPERTY_MAX_NAME, BOOT_PROPERTY_MAX_VALUE );

    PRINTF(
    "  the corresponding system property will be set at boot time in the\n"
    "  emulated system. This can be useful for debugging purposes.\n\n"

    "  note that you can use several -prop options to define more than one\n"
    "  boot property.\n\n"
    );
}

static void
help_shared_net_id(stralloc_t*  out)
{
    PRINTF(
    "  Normally, Android instances running in the emulator cannot talk to each other\n"
    "  directly, because each instance is behind a virtual router. However, sometimes\n"
    "  it is necessary to test the behaviour of applications if they are directly\n"
    "  exposed to the network.\n\n"

    "  This option instructs the emulator to join a virtual network shared with\n"
    "  emulators also using this option. The number given is used to construct\n"
    "  the IP address 10.1.2.<number>, which is bound to a second interface on\n"
    "  the emulator. Each emulator must use a different number.\n\n"
    );
}

static void
help_gpu(stralloc_t* out)
{
    PRINTF(
    "  Use -gpu <mode> to override the mode of hardware OpenGL ES emulation\n"
    "  indicated by the AVD. The following <mode> values should cover most\n"
    "  use cases:\n\n"

    "     auto (default)       -> Auto-select the renderer.\n"
    "     auto-no-window       -> Auto-select the renderer when\n"
    "                             running headless. This will use the same\n"
    "                             gpu selection mechanism as running without\n"
    "                             the \"-no-window\" flag and the \"-gpu auto\"\n"
    "                             option. See auto for details on the behavior.\n"
    "     host                 -> Use the host system's OpenGL driver.\n"
    "     swiftshader_indirect -> Use SwiftShader software renderer on the\n"
    "                             host, which can be beneficial if you are\n"
    "                             experiencing issues with your GPU drivers\n"
    "                             or need to run on systems without GPUs.\n"
    "     angle_indirect       -> Use ANGLE, an OpenGL ES to D3D11 renderer\n"
    "                             (Windows 7 SP1 + Platform update, \n"
    "                             Windows 8.1+, or Windows 10 only).\n"
    "     guest                -> Use guest-side software rendering. For\n"
    "                             advanced users only. Warning: slow!\n"
    "                             In API 28 and later, guest rendering\n"
    "                             is not supported, and will fall back\n"
    "                             automatically to swiftshader_indirect.\n"
    "\n"
    "  Note that enabling GPU emulation if the system image does not support it\n"
    "  will prevent the proper display of the emulated framebuffer.\n\n"

    "  The 'auto' mode is the default. In this mode, the hw.gpu.enabled setting\n"
    "  in the AVD's " CORE_HARDWARE_INI " file will determine whether GPU emulation\n"
    "  is enabled.\n\n"
    );
}

static void
help_use_host_vulkan(stralloc_t* out)
{
    PRINTF(
    "  Use '-use_host_vulkan' to force the use of host GPU for vulkan calls.\n\n"
    );
}

static void
help_camera_back(stralloc_t* out)
{
    PRINTF(
    "  Use -camera-back <mode> to control emulation of a camera facing back.\n"
    "  Valid values for <mode> are:\n\n"

    "     emulated     -> camera will be emulated using software ('fake') camera emulation\n"
    "     webcam<N>    -> camera will be emulated using a webcamera connected to the host\n"
    "     virtualscene -> If the feature is enabled, camera will render a virtual scene\n"
    "     videoplayback -> If the feature is enabled, camera will support playing back a video\n"
    "     none         -> camera emulation will be disabled\n\n"
    );
}

static void
help_camera_front(stralloc_t* out)
{
    PRINTF(
    "  Use -camera-front <mode> to control emulation of a camera facing front.\n"
    "  Valid values for <mode> are:\n\n"

    "     emulated  -> camera will be emulated using software ('fake') camera emulation\n"
    "     webcam<N> -> camera will be emulated using a webcamera connected to the host\n"
    "     none      -> camera emulation will be disabled\n\n"
    );
}

static void
help_webcam_list(stralloc_t* out)
{
    PRINTF(
    "  Use -webcam-list to list web cameras available for emulation.\n\n"
    );
}

static void
help_virtualscene_poster(stralloc_t* out)
{
    PRINTF(
    "  Use -virtualscene-poster <name>=<filename> to load an image as a poster in the\n"
    "  virtual scene camera environment. At a minimum, each scene supports these\n"
    "  <name> values:\n\n"

    "     wall  -> A poster on a vertical plane.\n"
    "     table -> A poster on a horizontal plane.\n\n"
    );
}

static void
help_screen(stralloc_t* out)
{
    PRINTF(
    "  Use -screen <mode> to set the emulated screen mode.\n"
    "  Valid values for <mode> are:\n\n"

    "     touch       -> emulate a touch screen\n"
    "     multi-touch -> emulate a multi-touch screen\n"
    "     no-touch    -> disable touch and multi-touch screen emulation\n\n"

    "  Default mode for screen emulation is 'multi-touch'.\n\n"
    );
}

static void help_gnss_file_path(stralloc_t* out) {
    PRINTF(" Use -gnss-file-path <path>  to set the gnss file path.\n\n");
}

static void help_gnss_grpc_port(stralloc_t* out) {
    PRINTF(" Use -gnss-grpc-port <port>  to set the gnss grpc port "
           "number.\n\n");
}

static void
help_selinux(stralloc_t* out)
{
    PRINTF(
    "  Use -selinux to control the SELinux enforcement mode.\n"
    "  By default, SELinux is in enforcing mode. Other modes available are:\n"
    "     -selinux permissive   -> Load the SELinux policy, but do not enforce it.\n"
    "                              Policy violations are logged, but not rejected.\n"
    "     -selinux disabled     -> Disable kernel support for SELinux.\n"
    );
}

#ifdef __linux__
static void
help_use_system_libs(stralloc_t* out)
{
    PRINTF(
    "  (DEPRECATED)\n\n"
    "  The emulator now uses libc++ and is no longer relying on libstdc++.\n"
    "  This parameter no longer has any effect and might be removed in future versions.\n\n"
    "  NOTE2: For now, it's not possible to use system Qt libraries as the ones\n"
    "  shipped with the emulator are compiled against libc++.\n"
    "\n"
    );
}

static void
help_stdouterr_file(stralloc_t*  out)
{
    PRINTF(
    "  use '-stdouterr-file' to redirect stdout and stderr to it.\n\n"
    );
}

static void
help_no_qt(stralloc_t*  out)
{
    PRINTF(
    "  use '-no-qt' to disable qt windowing system.\n\n"
    );
}
#endif  // __linux__

static void
help_unix_pipe(stralloc_t* out)
{
    PRINTF(
    "  Guest systems can directly connect to host Unix domain sockets using\n"
    "  the 'unix:' QEMU pipe protocol. As a security measure, only host path\n"
    "  values listed through the '-unix-pipe <path>' option can be used\n\n"

    "  One can use the option several times, each use adds a new path to the\n"
    "  whitelist that will be checked at runtime\n\n"
    );
}

static void
help_fixed_scale(stralloc_t* out)
{
    PRINTF(
    "  By default, we automatically scale the initial emulator window if it\n"
    "  doesn't fit into the screen. Use '-fixed-scale' to disable this.\n\n"
    );
}

static void
help_wait_for_debugger(stralloc_t* out)
{
    PRINTF(
    "  Request emulator process to pause at launch and wait for a debugger.\n"
    "  This option is useful for debugging it having a custom environment.\n\n"
    );
}

static void
help_cores(stralloc_t* out)
{
    PRINTF(
    "  Request the emulator to simulate <count> virtual CPU cores. This is\n"
    "  only useful when using accelerated CPU emulation, since it allows\n"
    "  mapping virtual CPU cores to individual host ones for improved\n"
    "  performance. Default value comes from your AVD configuration, and\n"
    "  is typically 2.\n\n"
    );
}

static void
help_detect_image_hang(stralloc_t* out)
{
    PRINTF(
    "  Enable the detection of overflowing goldfish event buffers.\n"
    "  An overflowing event buffer could indicate that the system image"
    "  unable to process any events and is hung. \n"
    "  Detection of hangs will result in crash and associated bug report. \n"
    );
}

static void
help_metrics_to_console(stralloc_t* out)
{
    PRINTF(
    "  Enable metrics reporting in the emulator and print the collected\n"
    "  data to stdout in JSON format.\n"
    "  Mutually exclusive with \"-metrics-to-file <file>\" and \"-metrics-collection\"\n\n"
    );
}


static void
help_metrics_collection(stralloc_t* out)
{
    PRINTF(
    "  Help make the emulator better by sending usage statistics\n"
    "  and crash reports to Google upon (graceful) emulator exit.\n"
    "  Mutually exclusive with \"-metrics-to-file <file>\" and \"-metrics-to-console\"\n\n"
    );
}

static void
help_metrics_to_file(stralloc_t* out)
{
    PRINTF(
    "  Enable metrics reporting in the emulator and write the collected\n"
    "  data to <file> in JSON format. Old contents of <file> are destroyed.\n"
    "  Mutually exclusive with \"-metrics-to-console\" and \"-metrics-collection\"\n\n"
    );
}

static void
help_feature(stralloc_t* out)
{
    PRINTF(
    "  Force-enable or disable an emulator feature by name. Also use the\n"
    "  ANDROID_EMULATOR_FEATURES environment variable for the same purpose.\n\n"
    "  E.g. \"-feature HAXM,-HVF -feature Wifi\""
    );
}

static void
help_icc_profile(stralloc_t* out)
{
    PRINTF(
    "  ICC configuration file for SIM card\n\n"
    );
}

static void
help_sim_access_rules_file(stralloc_t* out)
{
    PRINTF(
    "  Textproto configuration file for overriding SIM access rules (see\n"
    "  sim_access_rules.proto for structure). Used for granting Carrier\n"
    "  Privileges: https://source.android.com/devices/tech/config/uicc\n\n"
    );
}

static void
help_multidisplay(stralloc_t* out)
{
    PRINTF(
    "  Create displays besides the default android display. Format:\n"
    "  -multidisplay <index> <width> <height> <dpi> <flag>\n"
    "    1 <= index <= 3\n"
    "  as an example,\n"
    "  -multidisplay 1,1200,800,240,0 will create display 1 with dimension\n"
    "  1200x800, dpi 240 and the default flag\n\n"
    );
}

static void
help_google_maps_key(stralloc_t* out)
{
    PRINTF(
    "  User-provided API key to use with the Google maps GUI.\n"
    "  -google_maps_key <your_key>\n"
    "  Note that providing this key will override the one provided by Android Studio.\n\n"
    );
}

static void
help_no_location_ui(stralloc_t* out)
{
    PRINTF(
    "  Disables the location UI in the extended window.\n"
    "  -no-location-ui\n"
    "  This is useful if the location UI is interfering with your own location mocking.\n\n"
    );
}

static void help_use_keycode_forwarding(stralloc_t* out) {
    PRINTF("  Directly forward keycode generated by keypress event to guest.\n"
           "  -use-keycode-forwarding\n"
           "  This is useful if the you want to use a different key character\n"
           "  mapping from the default one. You will need to manually set the\n"
           "  keyboard layout in the guest system.\n\n"

    );
}

static void
help_record_session(stralloc_t* out)
{
    PRINTF(
    "  Screen record the emulator session. Format:\n"
    "  -record-session <file>,<delay>[,<duration>]\n"
    "  Examples:\n"
    "  -record-session /tmp/tmp.webm,20 will start recording after 20 second delay until shutdown.\n"
    "  -record-session /tmp/tmp.webm,0,20 will start recording immediately for 20 seconds.\n\n"
    );
}

static void
help_legacy_fake_camera(stralloc_t* out)
{
    PRINTF(
    "  Emulates the fake camera by legacy camera HAL.\n\n"
    );
}

static void
help_camera_hq_edge(stralloc_t* out)
{
    PRINTF(
    "  Enable high quality edge processing for emulated camera.\n\n"
    );
}

static void
help_no_direct_adb(stralloc_t* out)
{
    PRINTF(
    "  Disables the internal ADB bridge and use the external adb bridge.\n"
    );
}

static void help_check_snapshot_loadable(stralloc_t* out) {
    PRINTF("  Check if a snapshot is loadable.\n"
           "  -check-snapshot-loadable <snapshot name|snapshot TAR file>\n");
}

static void help_no_hidpi_scaling(stralloc_t* out) {
    PRINTF("  Disable HiDPI scaling of guest display on macOS devices.\n");
}

static void help_no_mouse_reposition(stralloc_t* out) {
    PRINTF("  Do not reposition the mouse to emulator window center if mouse\n"
           "  pointer gets out of the window.\n"
           "  Enable this if experiencing mouse issues on remote desktops\n"
           "  like VNC or Chrome remote desktop.\n");
}

static void help_guest_angle(stralloc_t* out) {
    PRINTF(" Enables ANGLE as the guest system GLES driver, using Vulkan to render on the host.\n");
}

static void help_usb_passthrough(stralloc_t* out) {
    PRINTF("  Pass Through a HOST USB device to the guest.\n"
           "  -usb-passthrough vendorid=VID,productid=PID[,hostbus=BUS,hostport=PORT]\n");
}

static void help_save_path(stralloc_t* out) {
    PRINTF(" Override save path for screenshot and bug report. The value will "
           " not be persisted on host OS.\n"
           " -save-path <path>\n"
           " <path> must be a writable full path. If <path> is invalid, "
           "default path will still be used.\n");
}

static void
help_wifi_tap(stralloc_t*  out)
{
    PRINTF(
    "  by default, the emulator's virtio wifi device uses user mode networking which\n"
    "  does address translation of all network traffic. Use -wifi-tap <tap interface>\n"
    "  to switch to TAP network mode where Virtio Wi-Fi will use a TAP interface to\n"
    "  connect to the host network directly. Android will attempt to configure the TAP\n"
    "  network using DHCP for IPv4 and SLAAC for IPv6. To be able to access the\n"
    "  network that the host is connected to the TAP interface should be bridged\n"
    "  with another network interface on the host.\n\n"

    "  <tap interface> is the name of a TAP network interface such as tap0. It\n"
    "  is NOT the path to the TAP device, just the name of the device. Also, the name\n"
    "  should Not be the same as the network interface used by net-tap option. \n\n"
    );
}

static void
help_wifi_tap_script_up(stralloc_t*  out)
{
    PRINTF(
    "  when using the -wifi-tap option it is sometimes useful to run a script when\n"
    "  TAP interface is enabled. Depending on the host system the TAP interface\n"
    "  might not be up until the emulator enables it. A common operation in such\n"
    "  a script is to add the TAP interface to a bridge so that it can be part of\n"
    "  the host network.\n"

    "  <script> the path to a script to be run when the TAP interface goes up.\n"

    "  NOTE: The script will be run with the same privileges as the emulator.\n"
    "        Ensure that this is sufficient for whatever operations the script\n"
    "        performs.\n\n"
    );
}

static void
help_wifi_tap_script_down(stralloc_t*  out)
{
    PRINTF(
    "  when using the -wifi-tap option it is sometimes useful to run a script when\n"
    "  TAP interface is disabled. Depending on the host system the TAP interface\n"
    "  might be disabled when the emulator exits. A common operation in such\n"
    "  a script is to remove the TAP interface from a bridge that it was previously\n"
    "  part of.\n"

    "  <script> the path to a script to be run when the TAP interface goes down.\n"

    "  NOTE: The script will be run with the same privileges as the emulator.\n"
    "        Ensure that this is sufficient for whatever operations the script\n"
    "        performs.\n\n"
    );
}

static void
help_wifi_socket(stralloc_t* out)
{
    PRINTF(
    "  by default, the emulator's virtio wifi device uses user mode networking which\n"
    "  does address translation of all network traffic. Use -wifi-socket <option>\n"
    "  to switch to Socket network mode where Virtio Wi-Fi will use a socket interface to\n"
    "  connect to the host network directly. Android will attempt to configure the Socket\n"
    "  network using DHCP for IPv4 and SLAAC for IPv6. To be able to access the\n"
    "  network that the host is connected to the Socket interface should be backed\n"
    "  with another network interface on the host.\n\n"

    "  <option> depends on how the socket is configured. It could be one of fd=, listen=,\n"
    "  or connect=. See https://wiki.qemu.org/Documentation/Networking#Socket\n\n"
    );
}

#ifdef __APPLE__
static void
help_vmnet_bridged(stralloc_t*  out) {
     PRINTF(" Enable vmnet framework in bridged mode as the backend of tap netdev on MacOS and provide a host network interface as argument.\n\n");
}

static void
help_vmnet_shared(stralloc_t*  out) {
     PRINTF(" Enable vmnet framework in shared mode as the backend for tap network on MacOS\n\n");
}

static void
help_vmnet_start_address(stralloc_t* out)
{
    PRINTF("  If using vmnet in shared mode, starting IPv4 address to assing using DHCP.\n"
           "  Must be in RFC 1918 private IP range and specified together with vmnet_end_address and vmnet_subnet_mask.\n\n");
}

static void
help_vmnet_end_address(stralloc_t* out)
{
    PRINTF("  If using vmnet in shared mode, ending IPv4 address to assing using DHCP.\n"
           "  Must be in RFC 1918 private IP range and specified together with vmnet_start_address and vmnet_subnet_mask.\n\n");
}

static void
help_vmnet_subnet_mask(stralloc_t* out)
{
    PRINTF("  If using vmnet in shared mode, IPv4 netmask to assing using DHCP.\n"
           "  Must be in RFC 1918 private IP range and specified together with vmnet_start_address and vmnet_end_address.\n\n");
}

static void
help_vmnet_isolated(stralloc_t* out)
{
    PRINTF(  "Enable isolation for the interface. Ensures that vmnet interface is not able to communicate with any other vmnet interface.\n\n");
}
#endif
static void help_wifi_user_mode_options(stralloc_t* out) {
    PRINTF("Override default user mode networking option for wifi network in \n"
           "Android Emulator for API 31 and above. It has no effect on wifi "
           "network\n"
           "for API 30 and below"
           " Example: -wifi-user-mode-options "
           "ipv6=off,host=10.4.4.7,dhcpstart=10.4.4.4\n\n");
}

static void help_network_user_mode_options(stralloc_t* out) {
    PRINTF("Override default user mode networking option for both radio and "
           "wifi network in "
           "Android Emulator for API 30 and below. Override default user mode "
           "networking option"
           "for radio network for API 31 and above."
           " Example: -radio-user-mode-options "
           "ipv6=off,host=10.4.4.7,dhcpstart=10.4.4.4\n\n");
}

static void help_adb_path(stralloc_t* out) {
    PRINTF("Set a customized adb path. Emulator will use the provided adb "
           "instead of automatically searching for an adb binary.\n\n");
}

static void help_hotplug_multi_display(stralloc_t* out) {
    PRINTF(" Use the HAL hotplug display to implement multiDisplay.\n\n");
}

static void help_xts(stralloc_t* out) {
    PRINTF(" Turn on tweaks made for running xTS.\n\n");
}

static void help_redirect_to_netsim(stralloc_t* out) {
    PRINTF(" All WiFi network traffic will be redirected to the external "
           " netsim process which is a full stack network service. By default,"
           " Emulator will use its own in process network stack.\n\n");
}

static void help_wifi_mac_address(stralloc_t* out) {
    PRINTF(" Override the mac address of the wlan0 interface in AVD. "
           " This option only applies to situation where AVD is loaded "
           " from snapshot using read-only mode."
           " Example: -wifi-mac-address 00:11:22:33:44:55 \n\n");
}

static void help_no_ethernet(stralloc_t* out) {
    PRINTF(" Disable the virtual Ethernet interface en0 in AVD. "
           " This option is only effective when feature VirtioWifi is "
           " used. Otherwise, this option is a no-op. \n\n");
}

#define  help_no_skin   NULL
#define  help_netspeed  help_shaper
#define  help_netdelay  help_shaper
#define  help_netfast   help_shaper

#define  help_noaudio      NULL
#define  help_noskin       NULL
#define  help_nocache      NULL
#define  help_no_jni       NULL
#define  help_nojni        NULL
#define  help_dalvik_vm_checkjni NULL
#define  help_initdata     NULL
#define  help_no_sim       NULL
#define  help_lowram       NULL
#define  help_no_window    NULL
#define  help_qt_hide_window NULL
#define  help_version      NULL
#define  help_no_passive_gps NULL
#define  help_read_only    NULL
#define  help_is_restart    NULL
#define  help_memory       NULL
#define  help_partition_size NULL

#define help_skip_adb_auth NULL
#define help_quit_after_boot NULL
#define help_delay_adb NULL

#define help_phone_number NULL
#define help_monitor_adb NULL
#define help_qemu_top_dir NULL

#define help_acpi_config NULL
#define help_fuchsia NULL
#define help_window_size NULL
#define help_allow_host_audio NULL
#define help_restart_when_stalled NULL
#define help_perf_stat NULL
#define help_append_userspace_opt NULL
#define help_no_nested_warnings NULL
#define help_dump_audio NULL

typedef struct {
    const char*  name;
    const char*  template;
    const char*  descr;
    void (*func)(stralloc_t*);
} OptionHelp;

static const OptionHelp    option_help[] = {
#define  OPT_FLAG(_name,_descr)             { STRINGIFY(_name), NULL, _descr, help_##_name },
#define  OPT_PARAM(_name,_template,_descr)  { STRINGIFY(_name), _template, _descr, help_##_name },
#define  OPT_LIST                           OPT_PARAM
#include "android/cmdline-options.h"

    { NULL, NULL, NULL, NULL }
};

typedef struct {
    const char*  name;
    const char*  desc;
    void (*func)(stralloc_t*);
} TopicHelp;


static const TopicHelp    topic_help[] = {
    { "disk-images",  "about disk images",      help_disk_images },
    { "debug-tags",   "debug tags for -debug <tags>", help_debug_tags },
    { "char-devices", "character <device> specification", help_char_devices },
    { "environment",  "environment variables",  help_environment },
    { "virtual-device", "virtual device management", help_virtual_device },
    { "sdk-images",   "about disk images when using the SDK", help_sdk_images },
    { "build-images", "about disk images when building Android", help_build_images },
    { NULL, NULL, NULL }
};

int
android_help_for_option( const char*  option, stralloc_t*  out )
{
    OptionHelp const*  oo;
    char               temp[32];

    /* the names in the option_help table use underscore instead
     * of dashes, so create a tranlated copy of the option name
     * before scanning the table for matches
     */
    buffer_translate_char( temp, sizeof temp, option, '-', '_' );

    for ( oo = option_help; oo->name != NULL; oo++ ) {
        if ( !strcmp(oo->name, temp) ) {
            if (oo->func)
                oo->func(out);
            else
                stralloc_add_str(out, oo->descr);
            return 0;
        }
    }
    return -1;
}


int
android_help_for_topic( const char*  topic, stralloc_t*  out )
{
    const TopicHelp*  tt;

    for ( tt = topic_help; tt->name != NULL; tt++ ) {
        if ( !strcmp(tt->name, topic) ) {
            tt->func(out);
            return 0;
        }
    }
    return -1;
}


extern void
android_help_list_options( stralloc_t*  out )
{
    const OptionHelp*  oo;
    const TopicHelp*   tt;
    int                maxwidth = 0;

    for ( oo = option_help; oo->name != NULL; oo++ ) {
        int  width = strlen(oo->name);
        if (oo->template != NULL)
            width += strlen(oo->template);
        if (width > maxwidth)
            maxwidth = width;
    }

    for (oo = option_help; oo->name != NULL; oo++) {
        char  temp[32];
        /* the names in the option_help table use underscores instead
         * of dashes, so create a translated copy of the option's name
         */
        buffer_translate_char(temp, sizeof temp, oo->name, '_', '-');

        stralloc_add_format( out, "    -%s %-*s %s\n",
            temp,
            (int)(maxwidth - strlen(oo->name)),
            oo->template ? oo->template : "",
            oo->descr );
    }

    PRINTF( "\n" );
    PRINTF( "     %-*s  %s\n", maxwidth, "-qemu args...",    "pass arguments to qemu");
    PRINTF( "     %-*s  %s\n", maxwidth, "-qemu -h", "display qemu help");
    PRINTF( "\n" );
    PRINTF( "     %-*s  %s\n", maxwidth, "-verbose",       "same as '-debug-init'");
    PRINTF( "     %-*s  %s\n", maxwidth, "-debug <tags>",  "enable/disable debug messages");
    PRINTF( "     %-*s  %s\n", maxwidth, "-debug-<tag>",   "enable specific debug messages");
    PRINTF( "     %-*s  %s\n", maxwidth, "-debug-no-<tag>","disable specific debug messages");
    PRINTF( "\n" );
    PRINTF( "     %-*s  %s\n", maxwidth, "-help",    "print this help");
    PRINTF( "     %-*s  %s\n", maxwidth, "-help-<option>", "print option-specific help");
    PRINTF( "\n" );

    for (tt = topic_help; tt->name != NULL; tt += 1) {
        char    help[32];
        snprintf(help, sizeof(help), "-help-%s", tt->name);
        PRINTF( "     %-*s  %s\n", maxwidth, help, tt->desc );
    }
    PRINTF( "     %-*s  %s\n", maxwidth, "-help-all", "prints all help content");
    PRINTF( "\n");
}


void
android_help_main( stralloc_t*  out )
{
    stralloc_add_str(out, "Android Emulator usage: emulator [options] [-qemu args]\n");
    stralloc_add_str(out, "  options:\n" );

    android_help_list_options(out);

    /*printf( "%.*s", out->n, out->s );*/
}


void
android_help_all( stralloc_t*  out )
{
    const OptionHelp*  oo;
    const TopicHelp*   tt;

    for (oo = option_help; oo->name != NULL; oo++) {
        PRINTF( "========= help for option -%s:\n\n", oo->name );
        android_help_for_option( oo->name, out );
    }

    for (tt = topic_help; tt->name != NULL; tt++) {
        PRINTF( "========= help for -help-%s\n\n", tt->name );
        android_help_for_topic( tt->name, out );
    }
    PRINTF( "========= top-level help\n\n" );
    android_help_main(out);
}
