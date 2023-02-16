/* this header file can be included several times by the same source code.
 * it contains the list of support command-line options for the Android
 * emulator program
 */
#ifndef OPT_PARAM
#error OPT_PARAM is not defined
#endif
#ifndef OPT_LIST
#error OPT_LIST is not defined
#endif
#ifndef OPT_FLAG
#error OPT_FLAG is not defined
#endif
#ifndef CFG_PARAM
#define CFG_PARAM  OPT_PARAM
#endif
#ifndef CFG_FLAG
#define CFG_FLAG   OPT_FLAG
#endif

/* required to ensure that the CONFIG_XXX macros are properly defined */
#include "android/config/config.h"

/* Some options acts like flags, while others must be followed by a parameter
 * string. Nothing really new here.
 *
 * Some options correspond to AVD (Android Virtual Device) configuration
 * and will be ignored if you start the emulator with the -avd <name> flag.
 *
 * However, if you use them with -avd-create <name>, these options will be
 * recorded into the new AVD directory. Once an AVD is created, there is no
 * way to change these options.
 *
 * Several macros are used to define options:
 *
 *    OPT_FLAG( name, "description" )
 *       used to define a non-config flag option.
 *       * 'name' is the option suffix that must follow the dash (-)
 *          as well as the name of an integer variable whose value will
 *          be 1 if the flag is used, or 0 otherwise.
 *       * "description" is a short description string that will be
 *         displayed by 'emulator -help'.
 *
 *    OPT_PARAM( name, "<param>", "description" )
 *       used to define a non-config parameter option
 *        * 'name' will point to a char* variable (NULL if option is unused)
 *        * "<param>" is a template for the parameter displayed by the help
 *        * 'varname' is the name of a char* variable that will point
 *          to the parameter string, if any, or will be NULL otherwise.
 *
 *    CFG_FLAG( name, "description" )
 *        used to define a configuration-specific flag option.
 *
 *    CFG_PARAM( name, "<param>", "description" )
 *        used to define a configuration-specific parameter option.
 *
 * NOTE: Keep in mind that option names are converted by translating
 *       dashes into underscore.
 *
 *       This means that '-some-option' is equivalent to '-some_option'
 *       and will be backed by a variable name 'some_option'
 *
 */

OPT_FLAG( list_avds, "list available AVDs")
CFG_PARAM( sysdir,  "<dir>",  "search for system disk images in <dir>" )
CFG_PARAM( system,  "<file>", "read initial system image from <file>" )
CFG_PARAM( vendor,  "<file>", "read initial vendor image from <file>" )
CFG_FLAG ( writable_system, "make system & vendor image writable after 'adb remount'" )
CFG_FLAG ( delay_adb, "delay adb communication till boot completes" )
CFG_PARAM( datadir, "<dir>",  "write user data into <dir>" )
CFG_PARAM( kernel,  "<file>", "use specific emulated kernel" )
CFG_PARAM( ramdisk, "<file>", "ramdisk image (default <system>/ramdisk.img" )
CFG_PARAM( image,   "<file>", "obsolete, use -system <file> instead" )
CFG_PARAM( initdata, "<file>", "same as '-init-data <file>'" )
CFG_PARAM( data,     "<file>", "data image (default <datadir>/userdata-qemu.img" )
CFG_PARAM( encryption_key,     "<file>", "read initial encryption key image from <file>" )
CFG_PARAM( logcat_output, "<file>", "output file of logcat(default none)" )
CFG_PARAM( partition_size, "<size>", "system/data partition size in MBs" )
CFG_PARAM( cache,    "<file>", "cache partition image (default is temporary file)" )
OPT_PARAM( cache_size, "<size>", "cache partition size in MBs" )
CFG_FLAG ( no_cache, "disable the cache partition" )
CFG_FLAG ( nocache,  "same as -no-cache" )
OPT_PARAM( sdcard, "<file>", "SD card image (default <datadir>/sdcard.img")
CFG_PARAM( quit_after_boot, "<timeout>", "qeuit emulator after guest boots completely, or after timeout in seconds" )
CFG_PARAM( qemu_top_dir, "<dir>", "Use the emulator in the specified dir (relative or absolute path)" )
CFG_PARAM( monitor_adb, "<verbose_level>", "monitor the adb messages between guest and host, default not" )
OPT_PARAM( snapstorage,    "<file>", "file that contains all state snapshots (default <datadir>/snapshots.img)")
OPT_FLAG ( no_snapstorage, "do not mount a snapshot storage file (this disables all snapshot functionality)" )
OPT_PARAM( snapshot,       "<name>", "name of snapshot within storage file for auto-start and auto-save (default 'default-boot')" )
OPT_FLAG ( no_snapshot,    "perform a full boot and do not auto-save, but qemu vmload and vmsave operate on snapstorage" )
OPT_FLAG ( no_snapshot_save, "do not auto-save to snapshot on exit: abandon changed state" )
OPT_FLAG ( no_snapshot_load, "do not auto-start from snapshot: perform a full boot" )
OPT_FLAG(qcow2_for_userdata, "use qcow2 for userdata")
OPT_FLAG ( snapshot_list,  "show a list of available snapshots" )
OPT_FLAG ( no_snapshot_update_time, "do not try to correct snapshot time on restore" )
OPT_FLAG ( wipe_data, "reset the user data image (copy it from initdata)" )
CFG_PARAM( avd, "<name>", "use a specific android virtual device" )
CFG_PARAM( avd_arch, "<target>", "use a specific target architecture" )
CFG_PARAM( skindir, "<dir>", "search skins in <dir> (default <system>/skins)" )
CFG_PARAM( skin, "<name>", "select a given skin" )
CFG_FLAG ( no_skin, "deprecated: create an AVD with no skin instead" )
CFG_FLAG ( noskin, "same as -no-skin" )
CFG_PARAM( memory, "<size>", "physical RAM size in MBs" )
OPT_PARAM( ui_only, "<UI feature>", "run only the UI feature requested")
CFG_PARAM( id, "<name>", "assign an id to this virtual device (separate from the avd name)")

OPT_PARAM( cores, "<number>", "Set number of CPU cores to emulator" )
OPT_PARAM( accel, "<mode>", "Configure emulation acceleration" )
OPT_FLAG ( no_accel, "Same as '-accel off'" )
OPT_FLAG ( ranchu, "Use new emulator backend instead of the classic one" )
OPT_PARAM( engine, "<engine>", "Select engine. auto|classic|qemu2" )

OPT_PARAM( netspeed, "<speed>", "maximum network download/upload speeds" )
OPT_PARAM( netdelay, "<delay>", "network latency emulation" )
OPT_FLAG ( netfast, "disable network shaping" )

OPT_PARAM( code_profile, "<name>", "enable code profiling" )
OPT_FLAG ( show_kernel, "display kernel messages" )
OPT_FLAG ( shell, "enable root shell on current terminal" )
OPT_FLAG ( no_jni, "deprecated, see dalvik_vm_checkjni" )
OPT_FLAG ( nojni, "deprecated, see dalvik_vm_checkjni" )
OPT_FLAG ( dalvik_vm_checkjni, "Enable dalvik.vm.checkjni" )
OPT_PARAM( logcat, "<tags>", "enable logcat output with given tags" )
OPT_FLAG( log_nofilter, "Disable the duplicate log filter")
OPT_FLAG( log_detailed, "Enable detailed logging info" )

#ifdef __linux__
OPT_FLAG ( use_system_libs, "Use system libstdc++ instead of bundled one" )
CFG_PARAM( stdouterr_file, "<file-name>", "redirect stdout/stderr to a specific file" )
OPT_FLAG ( no_qt, "disable qt windowing system" )
#endif  // __linux__

OPT_FLAG ( no_audio, "disable audio support" )
OPT_FLAG ( noaudio,  "same as -no-audio" )
OPT_PARAM( audio,    "<backend>", "use specific audio backend" )

OPT_PARAM( radio, "<device>", "redirect radio modem interface to character device" )
OPT_PARAM( port, "<port>", "TCP port that will be used for the console" )
OPT_PARAM( ports, "<consoleport>,<adbport>", "TCP ports used for the console and adb bridge" )
OPT_PARAM( modem_simulator_port, "<port>", "TCP port that will be used for android modem simulator" )
OPT_PARAM( onion, "<image>", "use overlay PNG image over screen" )
OPT_PARAM( onion_alpha, "<%age>", "specify onion-skin translucency" )
OPT_PARAM( onion_rotation, "0|1|2|3", "specify onion-skin rotation" )

OPT_PARAM( dpi_device, "<dpi>", "specify device's resolution in dpi (default "
            STRINGIFY(DEFAULT_DEVICE_DPI) ")" )
OPT_PARAM( scale, "<scale>", "scale emulator window (deprecated)" )

OPT_PARAM( wifi_client_port, "<port>", "connect to other emulator for WiFi forwarding" )
OPT_PARAM( wifi_server_port, "<port>", "listen to other emulator for WiFi forwarding" )
OPT_PARAM( http_proxy, "<proxy>", "make TCP connections through a HTTP/HTTPS proxy" )
OPT_PARAM( timezone, "<timezone>", "use this timezone instead of the host's default" )
OPT_PARAM( change_language, "<language>", "use this language instead of the current one. Restarts the framework." )
OPT_PARAM( change_country, "<country>", "use this country instead of the current one. Restarts the framework." )
OPT_PARAM( change_locale, "<locale>", "use this locale instead of the current one. Restarts the framework." )
OPT_PARAM( dns_server, "<servers>", "use this DNS server(s) in the emulated system" )
OPT_PARAM( net_tap, "<interface>", "use this TAP interface for networking" )
OPT_PARAM( net_socket, "option", "use Socket interface for network, one of fd=, listen=, or connect=" )
OPT_PARAM( net_tap_script_up, "<script>", "script to run when the TAP interface goes up" )
OPT_PARAM( net_tap_script_down, "<script>", "script to run when the TAP interface goes down" )
OPT_PARAM( cpu_delay, "<cpudelay>", "throttle CPU emulation" )
OPT_FLAG ( no_boot_anim, "disable animation for faster boot" )

OPT_FLAG( no_window, "disable graphical window display" )
OPT_FLAG( qt_hide_window, "Start QT window but hide window display")
OPT_FLAG( no_sim, "device has no SIM card" )
OPT_FLAG( lowram, "device is a low ram device" )
OPT_FLAG( version, "display emulator version number" )
OPT_FLAG( no_passive_gps, "disable passive gps updates" )
CFG_PARAM( gnss_file_path, "<path>", "Use the specified filepath to read gnss data" )
CFG_PARAM( gnss_grpc_port, "<port number>", "Use the specified port number to start grpc service to receive gnss data" )
OPT_FLAG( virtio_console, "using virtio console as console")
OPT_FLAG( read_only, "allow running multiple instances of emulators on the same"
        " AVD, but cannot save snapshot.")
OPT_PARAM( is_restart, "<restart-pid>", "specifies that this emulator was a restart, and to wait out <restart-pid> before proceeding")

OPT_PARAM( report_console, "<socket>", "report console port to remote socket" )
OPT_PARAM( gps, "<device>", "redirect NMEA GPS to character device" )
OPT_PARAM( shell_serial, "<device>", "specific character device for root shell" )
OPT_PARAM( tcpdump, "<file>", "capture network packets to file" )

OPT_PARAM( bootchart, "<timeout>", "enable bootcharting")

OPT_PARAM( charmap, "<file>", "use specific key character map")
OPT_PARAM( studio_params, "<file>", "used by Android Studio to provide parameters" )

OPT_LIST(  prop, "<name>=<value>", "set system property on boot")

OPT_PARAM( shared_net_id, "<number>", "join the shared network, using IP address 10.1.2.<number>")

#ifndef _WIN32
 // the -nand-limits options can only work on non-windows systems
OPT_PARAM( nand_limits, "<nlimits>", "enforce NAND/Flash read/write thresholds" )
#endif

OPT_PARAM( gpu, "<mode>", "set hardware OpenGLES emulation mode" )
OPT_FLAG( use_host_vulkan, "use host for vulkan emulation regardless of 'gpu' mode")

OPT_PARAM( camera_back, "<mode>", "set emulation mode for a camera facing back" )
OPT_PARAM( camera_front, "<mode>", "set emulation mode for a camera facing front" )
OPT_FLAG( webcam_list, "lists web cameras available for emulation" )

OPT_LIST( virtualscene_poster, "<name>=<filename>", "Load a png or jpeg image as a poster in the virtual scene")

OPT_PARAM( screen, "<mode>", "set emulated screen mode" )

OPT_PARAM(selinux, "<disabled|permissive>", "Set SELinux to either disabled or permissive mode")
OPT_LIST(unix_pipe, "<path>", "Add <path> to the list of allowed Unix pipes")

CFG_FLAG (fixed_scale, "Use fixed 1:1 scale for the initial emulator window.")

OPT_FLAG(wait_for_debugger, "Pause on launch and wait for a debugger process to attach before resuming")

OPT_FLAG(skip_adb_auth, "Skip adb authentication dialogue")

OPT_FLAG(metrics_to_console, "Enable usage metrics and print the messages to stdout")
OPT_FLAG(metrics_collection, "Enable usage metrics and send them to google play")
OPT_PARAM(metrics_to_file, "<file>", "Enable usage metrics and write the messages into specified file")
OPT_FLAG(detect_image_hang, "Enable the detection of system image hangs.")

OPT_LIST(feature, "<name|-name>", "Force-enable or disable (-name) the features")

OPT_PARAM(icc_profile, "<file>", "Use icc profile from specified file")
OPT_PARAM(sim_access_rules_file, "<file>", "Use SIM access rules from specified file")
OPT_PARAM(phone_number, "<phone_number>", "Sets the phone number of the emulated device")

CFG_PARAM(acpi_config, "<file>", "specify acpi device proprerties (hierarchical key=value pair)")

OPT_FLAG(fuchsia, "Run Fuchsia image. Bypasses android-specific setup; args after are treated as standard QEMU args")

OPT_PARAM(window_size, "<size>", "Set window size for when bypassing android-specific setup.")

OPT_FLAG(allow_host_audio, "Allows sending of audio from audio input devices. Otherwise, zeroes out audio.")

OPT_FLAG(restart_when_stalled, "Allows restarting guest when it is stalled.")

OPT_PARAM(perf_stat, "<file>", "Run periodic perf stat reporter in the background and write output to specified file.")

OPT_FLAG(share_vid, "Share current video state in shared memory region.")

OPT_PARAM(grpc, "<port>", "TCP ports used for the gRPC bridge." )
OPT_PARAM(grpc_tls_key, "<pem>", "File with the private key used to enable gRPC TLS.")
OPT_PARAM(grpc_tls_cer, "<pem>", "File with the public X509 certificate used to enable gRPC TLS.")
OPT_PARAM(grpc_tls_ca, "<pem>", "File with the Certificate Authorities used to validate client certificates.")
OPT_FLAG(grpc_use_token, "Use the emulator console token for gRPC authentication.")
OPT_FLAG(grpc_use_jwt, "Use a signed JWT token for gRPC authentication.")
OPT_PARAM(idle_grpc_timeout, "<timeout>", "Terminate the emulator if there is no gRPC activity within <timeout> seconds.")
OPT_PARAM(waterfall, "<mode>", "Mode in which to run waterfall.")

#ifdef ANDROID_WEBRTC
OPT_PARAM(turncfg, "cmd", "Command to execute to obtain turn configuration for the webrtc connection.")
OPT_PARAM(dump_audio, "<file>", "Dump audio to apointed file for the webrtc connection.")
#endif

#ifdef ANDROID_BLUETOOTH
OPT_PARAM(rootcanal_controller_properties_file, "<file>", "Rootcanal controller_properties.json file.")
OPT_PARAM(rootcanal_default_commands_file, "<file>", "Rootcanal commands file to run on launch.")
OPT_FLAG(rootcanal_no_mesh, "Disable auto discovery and connection bluetooth enabled emulators (DEPRECATED).")
OPT_FLAG(forward_vhci, "Enable the VHCI grpc forwarding service (DEPRECATED).")
OPT_PARAM(packet_streamer_endpoint, "<uri>", "Use the provided packet streamer endpoint when present.")
#endif

OPT_PARAM(multidisplay, "index width height dpi flag", "config multiple displays.")

OPT_PARAM(google_maps_key, "<API key>", "API key to use with the Google Maps GUI.")

OPT_FLAG(no_location_ui, "Disable the location UI in the extended window.")

OPT_FLAG(use_keycode_forwarding,
         "Use keycode forwarding instead of host charmap translation.")

OPT_PARAM(record_session, "<file>,<delay>[,<duration>]", "Screen record the emulator session.")

OPT_FLAG(legacy_fake_camera, "Use legacy camera HAL for the emulated fake camera.")

OPT_FLAG(camera_hq_edge, "Enable high qualify edge processing for emulated camera.")

OPT_FLAG(no_direct_adb, "Use external adb executable for internal communication.")

OPT_PARAM(check_snapshot_loadable,
          "<snapshot name|exported snapshot tar file>",
          "Check if a snasphot is loadable.")

OPT_FLAG(no_hidpi_scaling, "Disable HiDPI scaling of guest display on macOS devices.")

OPT_FLAG(no_mouse_reposition,
         "Do not reposition the mouse to emulator window center if mouse "
         "pointer gets out of the window.")

OPT_FLAG(guest_angle,
         "Enable guest ANGLE as system driver.")

OPT_LIST(usb_passthrough, "VID PID BUS PORTS", "Host USB device Passthrough")

OPT_LIST(append_userspace_opt, "key=value", "Appends a property which is passed to the userspace.")

OPT_PARAM(save_path,
          "<file path>",
          "Override save path for screenshot and bug report. The value will "
          "not be persisted on host OS.")

OPT_FLAG(no_nested_warnings,
         "Disable the warning dialog when emulator is running in nested "
         "virtualization.")

OPT_PARAM( wifi_tap, "<interface>", "use this TAP interface for Virtio Wi-Fi" )
OPT_PARAM( wifi_tap_script_up, "<script>", "script to run when the TAP interface goes up" )
OPT_PARAM( wifi_tap_script_down, "<script>", "script to run when the TAP interface goes down" )
OPT_PARAM( wifi_socket, "<option>", "use Socket interface for Virtio Wi-Fi, one of fd=, listen=, or connect=")
#ifdef __APPLE__
OPT_PARAM( vmnet_bridged,
           "<host network interface>",
           "Enable vmnet framework in bridged mode as the backend of tap netdev on MacOS and provide a host network interface as argument.")

OPT_FLAG(vmnet_shared,
           "Enable vmnet framework in shared mode as the backend of tap netdev on MacOS.")

OPT_PARAM(vmnet_start_address,
          "<ipv4 address>",
          "If using vmnet in shared mode, starting IPv4 address to assing using DHCP. Must be in RFC 1918 private IP range and specified together with vmnet_end_address and vmnet_subnet_mask.")

OPT_PARAM(vmnet_end_address,
          "<ipv4 address>",
          "If using vmnet in shared mode, ending IPv4 address to assing using DHCP. Must be in RFC 1918 private IP range and specified together with vmnet_start_address and vmnet_subnet_mask.")

OPT_PARAM(vmnet_subnet_mask,
          "<ipv4 address>",
          "If using vmnet in shared mode, IPv4 netmask to assing using DHCP. Must be in RFC 1918 private IP range and specified together with vmnet_start_address and vmnet_end_address.")

OPT_FLAG(vmnet_isolated,
          "Enable isolation for the interface. Ensures that vmnet interface is not able to communicate with any other vmnet interface.")
#endif
OPT_PARAM(wifi_user_mode_options,
          "<option list>",
          "Override default user mode networking option for wifi network in "
          "Android Emulator for API 31 and above.")

OPT_PARAM(network_user_mode_options,
          "<option list>",
          "Override default user mode networking option for both radio and "
          "wifi network in "
          "Android Emulator for API 30 and below. Override default user mode "
          "networking option"
          "for radio network for API 31 and above.")

CFG_PARAM(adb_path, "<path>", "use adb binary from <path>" )

OPT_FLAG(hotplug_multi_display, "Use the HAL hotplug display to implement multiDisplay.")

OPT_FLAG(xts, "Use android64-xts CPU profile instead of the default one")

OPT_FLAG(redirect_to_netsim, "Forward network traffic to the netsim process via gRPC. ")

#undef CFG_FLAG
#undef CFG_PARAM
#undef OPT_FLAG
#undef OPT_PARAM
#undef OPT_LIST
