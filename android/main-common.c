/* Copyright (C) 2011 The Android Open Source Project
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
#include "android/avd/util.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/path.h"
#include "android/utils/dirscanner.h"
#include "android/main-common.h"
#include "android/globals.h"
#include "android/resource.h"
#include "android/user-config.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#ifdef _WIN32
#include <process.h>
#endif
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>


/***********************************************************************/
/***********************************************************************/
/*****                                                             *****/
/*****            U T I L I T Y   R O U T I N E S                  *****/
/*****                                                             *****/
/***********************************************************************/
/***********************************************************************/

#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

void reassign_string(char** string, const char* new_value) {
    free(*string);
    *string = ASTRDUP(new_value);
}

unsigned convertBytesToMB( uint64_t  size )
{
    if (size == 0)
        return 0;

    size = (size + ONE_MB-1) >> 20;
    if (size > UINT_MAX)
        size = UINT_MAX;

    return (unsigned) size;
}

uint64_t convertMBToBytes( unsigned  megaBytes )
{
    return ((uint64_t)megaBytes << 20);
}

/* this function is used to perform auto-detection of the
 * system directory in the case of a SDK installation.
 *
 * we want to deal with several historical usages, hence
 * the slightly complicated logic.
 *
 * NOTE: the function returns the path to the directory
 *       containing 'fileName'. this is *not* the full
 *       path to 'fileName'.
 */
static char*
_getSdkImagePath( const char*  fileName )
{
    char   temp[MAX_PATH];
    char*  p   = temp;
    char*  end = p + sizeof(temp);
    char*  q;
    char*  app;

    static const char* const  searchPaths[] = {
        "",                                  /* program's directory */
        "/lib/images",                       /* this is for SDK 1.0 */
        "/../platforms/android-1.1/images",  /* this is for SDK 1.1 */
        NULL
    };

    app = bufprint_app_dir(temp, end);
    if (app >= end)
        return NULL;

    do {
        int  nn;

        /* first search a few well-known paths */
        for (nn = 0; searchPaths[nn] != NULL; nn++) {
            p = bufprint(app, end, "%s", searchPaths[nn]);
            q = bufprint(p, end, "/%s", fileName);
            if (q < end && path_exists(temp)) {
                *p = 0;
                goto FOUND_IT;
            }
        }

        /* hmmm. let's assume that we are in a post-1.1 SDK
         * scan ../platforms if it exists
         */
        p = bufprint(app, end, "/../platforms");
        if (p < end) {
            DirScanner*  scanner = dirScanner_new(temp);
            if (scanner != NULL) {
                int          found = 0;
                const char*  subdir;

                for (;;) {
                    subdir = dirScanner_next(scanner);
                    if (!subdir) break;

                    q = bufprint(p, end, "/%s/images/%s", subdir, fileName);
                    if (q >= end || !path_exists(temp))
                        continue;

                    found = 1;
                    p = bufprint(p, end, "/%s/images", subdir);
                    break;
                }
                dirScanner_free(scanner);
                if (found)
                    break;
            }
        }

        /* I'm out of ideas */
        return NULL;

    } while (0);

FOUND_IT:
    //D("image auto-detection: %s/%s", temp, fileName);
    return android_strdup(temp);
}

static char*
_getSdkImage( const char*  path, const char*  file )
{
    char  temp[MAX_PATH];
    char  *p = temp, *end = p + sizeof(temp);

    p = bufprint(temp, end, "%s/%s", path, file);
    if (p >= end || !path_exists(temp))
        return NULL;

    return android_strdup(temp);
}

static char*
_getSdkSystemImage( const char*  path, const char*  optionName, const char*  file )
{
    char*  image = _getSdkImage(path, file);

    if (image == NULL) {
        derror("Your system directory is missing the '%s' image file.\n"
               "Please specify one with the '%s <filepath>' option",
               file, optionName);
        exit(2);
    }
    return image;
}

void sanitizeOptions( AndroidOptions* opts )
{
    /* legacy support: we used to use -system <dir> and -image <file>
     * instead of -sysdir <dir> and -system <file>, so handle this by checking
     * whether the options point to directories or files.
     */
    if (opts->image != NULL) {
        if (opts->system != NULL) {
            if (opts->sysdir != NULL) {
                derror( "You can't use -sysdir, -system and -image at the same time.\n"
                        "You should probably use '-sysdir <path> -system <file>'.\n" );
                exit(2);
            }
        }
        dwarning( "Please note that -image is obsolete and that -system is now used to point\n"
                  "to the system image. Next time, try using '-sysdir <path> -system <file>' instead.\n" );
        opts->sysdir = opts->system;
        opts->system = opts->image;
        opts->image  = NULL;
    }
    else if (opts->system != NULL && path_is_dir(opts->system)) {
        if (opts->sysdir != NULL) {
            derror( "Option -system should now be followed by a file path, not a directory one.\n"
                    "Please use '-sysdir <path>' to point to the system directory.\n" );
            exit(1);
        }
        dwarning( "Please note that the -system option should now be used to point to the initial\n"
                  "system image (like the obsolete -image option). To point to the system directory\n"
                  "please now use '-sysdir <path>' instead.\n" );

        opts->sysdir = opts->system;
        opts->system = NULL;
    }

    if (opts->nojni) {
        opts->no_jni = opts->nojni;
        opts->nojni  = 0;
    }

    if (opts->nocache) {
        opts->no_cache = opts->nocache;
        opts->nocache  = 0;
    }

    if (opts->noaudio) {
        opts->no_audio = opts->noaudio;
        opts->noaudio  = 0;
    }

    if (opts->noskin) {
        opts->no_skin = opts->noskin;
        opts->noskin  = 0;
    }

    /* If -no-cache is used, ignore any -cache argument */
    if (opts->no_cache) {
        opts->cache = 0;
    }

    /* the purpose of -no-audio is to disable sound output from the emulator,
     * not to disable Audio emulation. So simply force the 'none' backends */
    if (opts->no_audio)
        opts->audio = "none";

    /* we don't accept -skindir without -skin now
     * to simplify the autoconfig stuff with virtual devices
     */
    if (opts->no_skin) {
        opts->skin    = "320x480";
        opts->skindir = NULL;
    }

    if (opts->skindir) {
        if (!opts->skin) {
            derror( "the -skindir <path> option requires a -skin <name> option");
            exit(1);
        }
    }

    if (opts->bootchart) {
        char*  end;
        int    timeout = strtol(opts->bootchart, &end, 10);
        if (timeout == 0)
            opts->bootchart = NULL;
        else if (timeout < 0 || timeout > 15*60) {
            derror( "timeout specified for -bootchart option is invalid.\n"
                    "please use integers between 1 and 900\n");
            exit(1);
        }
    }
}

AvdInfo* createAVD(AndroidOptions* opts, int* inAndroidBuild)
{
    AvdInfo* ret = NULL;
    char   tmp[MAX_PATH];
    char*  tmpend = tmp + sizeof(tmp);
    char*  android_build_root = NULL;
    char*  android_build_out  = NULL;

    /* If no AVD name was given, try to find the top of the
     * Android build tree
     */
    if (opts->avd == NULL) {
        do {
            char*  out = getenv("ANDROID_PRODUCT_OUT");

            if (out == NULL || out[0] == 0)
                break;

            if (!path_exists(out)) {
                derror("Can't access ANDROID_PRODUCT_OUT as '%s'\n"
                    "You need to build the Android system before launching the emulator",
                    out);
                exit(2);
            }

            android_build_root = getenv("ANDROID_BUILD_TOP");
            if (android_build_root == NULL || android_build_root[0] == 0)
                break;

            if (!path_exists(android_build_root)) {
                derror("Can't find the Android build root '%s'\n"
                    "Please check the definition of the ANDROID_BUILD_TOP variable.\n"
                    "It should point to the root of your source tree.\n",
                    android_build_root );
                exit(2);
            }
            android_build_out = out;
            D( "found Android build root: %s", android_build_root );
            D( "found Android build out:  %s", android_build_out );
        } while (0);
    }
    /* if no virtual device name is given, and we're not in the
     * Android build system, we'll need to perform some auto-detection
     * magic :-)
     */
    if (opts->avd == NULL && !android_build_out)
    {
        if (!opts->sysdir) {
            opts->sysdir = _getSdkImagePath("system.img");
            if (!opts->sysdir) {
                derror(
                "You did not specify a virtual device name, and the system\n"
                "directory could not be found.\n\n"
                "If you are an Android SDK user, please use '@<name>' or '-avd <name>'\n"
                "to start a given virtual device (see -help-avd for details).\n\n"

                "Otherwise, follow the instructions in -help-disk-images to start the emulator\n"
                );
                exit(2);
            }
            D("autoconfig: -sysdir %s", opts->sysdir);
        }

        if (!opts->system) {
            opts->system = _getSdkSystemImage(opts->sysdir, "-image", "system.img");
            D("autoconfig: -system %s", opts->system);
        }

        if (!opts->kernel) {
            opts->kernel = _getSdkSystemImage(opts->sysdir, "-kernel", "kernel-qemu");
            D("autoconfig: -kernel %s", opts->kernel);
        }

        if (!opts->ramdisk) {
            opts->ramdisk = _getSdkSystemImage(opts->sysdir, "-ramdisk", "ramdisk.img");
            D("autoconfig: -ramdisk %s", opts->ramdisk);
        }

        /* if no data directory is specified, use the system directory */
        if (!opts->datadir) {
            opts->datadir   = android_strdup(opts->sysdir);
            D("autoconfig: -datadir %s", opts->sysdir);
        }

        if (!opts->data) {
            /* check for userdata-qemu.img in the data directory */
            bufprint(tmp, tmpend, "%s/userdata-qemu.img", opts->datadir);
            if (!path_exists(tmp)) {
                derror(
                "You did not provide the name of an Android Virtual Device\n"
                "with the '-avd <name>' option. Read -help-avd for more information.\n\n"

                "If you *really* want to *NOT* run an AVD, consider using '-data <file>'\n"
                "to specify a data partition image file (I hope you know what you're doing).\n"
                );
                exit(2);
            }

            opts->data = android_strdup(tmp);
            D("autoconfig: -data %s", opts->data);
        }

        if (!opts->snapstorage && opts->datadir) {
            bufprint(tmp, tmpend, "%s/snapshots.img", opts->datadir);
            if (path_exists(tmp)) {
                opts->snapstorage = android_strdup(tmp);
                D("autoconfig: -snapstorage %s", opts->snapstorage);
            }
        }
    }

    /* setup the virtual device differently depending on whether
     * we are in the Android build system or not
     */
    if (opts->avd != NULL)
    {
        ret = avdInfo_new( opts->avd, android_avdParams );
        if (ret == NULL) {
            /* an error message has already been printed */
            dprint("could not find virtual device named '%s'", opts->avd);
            exit(1);
        }
    }
    else
    {
        if (!android_build_out) {
            android_build_out = android_build_root = opts->sysdir;
        }
        ret = avdInfo_newForAndroidBuild(
                            android_build_root,
                            android_build_out,
                            android_avdParams );

        if(ret == NULL) {
            D("could not start virtual device\n");
            exit(1);
        }
    }

    if (android_build_out) {
        *inAndroidBuild = 1;
    } else {
        *inAndroidBuild = 0;
    }

    return ret;
}




#ifdef CONFIG_STANDALONE_UI

#include "android/protocol/core-connection.h"
#include "android/protocol/fb-updates-impl.h"
#include "android/protocol/user-events-proxy.h"
#include "android/protocol/core-commands-proxy.h"
#include "android/protocol/ui-commands-impl.h"
#include "android/protocol/attach-ui-impl.h"

/* Emulator's core port. */
int android_base_port = 0;

// Base console port
#define CORE_BASE_PORT          5554

// Maximum number of core porocesses running simultaneously on a machine.
#define MAX_CORE_PROCS          16

// Socket timeout in millisec (set to 5 seconds)
#define CORE_PORT_TIMEOUT_MS    5000

#include "android/async-console.h"

typedef struct {
    LoopIo                 io[1];
    int                    port;
    int                    ok;
    AsyncConsoleConnector  connector[1];
} CoreConsole;

static void
coreconsole_io_func(void* opaque, int fd, unsigned events)
{
    CoreConsole* cc = opaque;
    AsyncStatus  status;
    status = asyncConsoleConnector_run(cc->connector);
    if (status == ASYNC_COMPLETE) {
        cc->ok = 1;
    }
}

static void
coreconsole_init(CoreConsole* cc, const SockAddress* address, Looper* looper)
{
    int fd = socket_create_inet(SOCKET_STREAM);
    AsyncStatus status;
    cc->port = sock_address_get_port(address);
    cc->ok   = 0;
    loopIo_init(cc->io, looper, fd, coreconsole_io_func, cc);
    if (fd >= 0) {
        status = asyncConsoleConnector_connect(cc->connector, address, cc->io);
        if (status == ASYNC_ERROR) {
            cc->ok = 0;
        }
    }
}

static void
coreconsole_done(CoreConsole* cc)
{
    socket_close(cc->io->fd);
    loopIo_done(cc->io);
}

/* List emulator core processes running on the given machine.
 * This routine is called from main() if -list-cores parameter is set in the
 * command line.
 * Param:
 *  host Value passed with -list-core parameter. Must be either "localhost", or
 *  an IP address of a machine where core processes must be enumerated.
 */
static void
list_running_cores(const char* host)
{
    Looper*         looper;
    CoreConsole     cores[MAX_CORE_PROCS];
    SockAddress     address;
    int             nn, found;

    if (sock_address_init_resolve(&address, host, CORE_BASE_PORT, 0) < 0) {
        derror("Unable to resolve hostname %s: %s", host, errno_str);
        return;
    }

    looper = looper_newGeneric();

    for (nn = 0; nn < MAX_CORE_PROCS; nn++) {
        int port = CORE_BASE_PORT + nn*2;
        sock_address_set_port(&address, port);
        coreconsole_init(&cores[nn], &address, looper);
    }

    looper_runWithTimeout(looper, CORE_PORT_TIMEOUT_MS*2);

    found = 0;
    for (nn = 0; nn < MAX_CORE_PROCS; nn++) {
        int port = CORE_BASE_PORT + nn*2;
        if (cores[nn].ok) {
            if (found == 0) {
                fprintf(stdout, "Running emulator core processes:\n");
            }
            fprintf(stdout, "Emulator console port %d\n", port);
            found++;
        }
        coreconsole_done(&cores[nn]);
    }
    looper_free(looper);

    if (found == 0) {
       fprintf(stdout, "There were no running emulator core processes found on %s.\n",
               host);
    }
}

/* Attaches starting UI to a running core process.
 * This routine is called from main() when -attach-core parameter is set,
 * indicating that this UI instance should attach to a running core, rather than
 * start a new core process.
 * Param:
 *  opts Android options containing non-NULL attach_core.
 * Return:
 *  0 on success, or -1 on failure.
 */
static int
attach_to_core(AndroidOptions* opts) {
    int iter;
    SockAddress console_socket;
    SockAddress** sockaddr_list;
    QEmulator* emulator;

    // Parse attach_core param extracting the host name, and the port name.
    char* console_address = strdup(opts->attach_core);
    char* host_name = console_address;
    char* port_num = strchr(console_address, ':');
    if (port_num == NULL) {
        // The host name is ommited, indicating the localhost
        host_name = "localhost";
        port_num = console_address;
    } else if (port_num == console_address) {
        // Invalid.
        derror("Invalid value %s for -attach-core parameter\n",
               opts->attach_core);
        return -1;
    } else {
        *port_num = '\0';
        port_num++;
        if (*port_num == '\0') {
            // Invalid.
            derror("Invalid value %s for -attach-core parameter\n",
                   opts->attach_core);
            return -1;
        }
    }

    /* Create socket address list for the given address, and pull appropriate
     * address to use for connection. Note that we're fine copying that address
     * out of the list, since INET and IN6 will entirely fit into SockAddress
     * structure. */
    sockaddr_list =
        sock_address_list_create(host_name, port_num, SOCKET_LIST_FORCE_INET);
    free(console_address);
    if (sockaddr_list == NULL) {
        derror("Unable to resolve address %s: %s\n",
               opts->attach_core, errno_str);
        return -1;
    }
    for (iter = 0; sockaddr_list[iter] != NULL; iter++) {
        if (sock_address_get_family(sockaddr_list[iter]) == SOCKET_INET ||
            sock_address_get_family(sockaddr_list[iter]) == SOCKET_IN6) {
            memcpy(&console_socket, sockaddr_list[iter], sizeof(SockAddress));
            break;
        }
    }
    if (sockaddr_list[iter] == NULL) {
        derror("Unable to resolve address %s. Note that 'port' parameter passed to -attach-core\n"
               "must be resolvable into an IP address.\n", opts->attach_core);
        sock_address_list_free(sockaddr_list);
        return -1;
    }
    sock_address_list_free(sockaddr_list);

    if (attachUiImpl_create(&console_socket)) {
        return -1;
    }

    // Save core's port, and set the title.
    android_base_port = sock_address_get_port(&console_socket);
    emulator = qemulator_get();
    qemulator_set_title(emulator);

    return 0;
}


void handle_ui_options( AndroidOptions* opts )
{
    // Lets see if user just wants to list core process.
    if (opts->list_cores) {
        fprintf(stdout, "Enumerating running core processes.\n");
        list_running_cores(opts->list_cores);
        exit(0);
    }
}

int attach_ui_to_core( AndroidOptions* opts )
{
    // Lets see if we're attaching to a running core process here.
    if (opts->attach_core) {
        if (attach_to_core(opts)) {
            return -1;
        }
        // Connect to the core's UI control services.
        if (coreCmdProxy_create(attachUiImpl_get_console_socket())) {
            return -1;
        }
        // Connect to the core's user events service.
        if (userEventsProxy_create(attachUiImpl_get_console_socket())) {
            return -1;
        }
    }
    return 0;
}

#else  /* !CONFIG_STANDALONE_UI */

void handle_ui_options( AndroidOptions* opts )
{
    return;
}

int attach_ui_to_core( AndroidOptions* opts )
{
    return 0;
}

#endif /* CONFIG_STANDALONE_UI */
