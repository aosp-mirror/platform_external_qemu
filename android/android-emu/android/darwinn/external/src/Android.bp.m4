cc_defaults {
    name: "third_party_darwinn_src_defaults",
    defaults: [
      "darwinn_driver_defaults",
    ],
    shared_libs: [
        "libcrypto",
        "libcutils",
        "libutils",
    ],
    static_libs: [
        "libneuralnetworks_common",
    ],
}

cc_library_static {
    name: "darwinn_beagle_all_driver_provider",
    defaults: [
        "third_party_darwinn_src_defaults",
    ],
    header_libs: [
        "darwinn_flatbuffer_headers",
    ],
    static_libs: [
        "libusb",
        "darwinn_port_default_port",
    ],
    srcs: [
__BEAGLE_DEP__
    ],
}

// Provides Noronha Driver.
cc_library_static {
    name: "darwinn_driver_noronha_noronha_driver_provider",
    defaults: [
        "third_party_darwinn_src_defaults",
    ],
    header_libs: [
        "darwinn_flatbuffer_headers",
    ],
    srcs: [
__NORONHA_DEP__
    ],
}

cc_library_static {
    name: "darwinn_driver_test_util",
    defaults: [
        "third_party_darwinn_src_defaults",
    ],
    header_libs: [
        "darwinn_flatbuffer_headers",
    ],
    static_libs: [
        "darwinn_port_default_port",
    ],
    srcs: [
__TEST_UTIL_DEP__
    ],
}

cc_library_shared {
    name: "libbeagle",
    defaults: [
        "third_party_darwinn_src_defaults",
    ],
    shared_libs: [
        "liblog",
    ],
    static_libs: [
        "libusb",
    ],
    whole_static_libs: [
        "darwinn_beagle_all_driver_provider",
        // This dependeny only shows up when
        // -DDARWINN_PORT_ANDROID_SYSTEM=1, so we'd manually add it here.
        "darwinn_port_default_port",
    ],
}

cc_library_shared {
    name: "libnoronha-pci",
    defaults: [
        "third_party_darwinn_src_defaults",
    ],
    shared_libs: [
        "liblog",
    ],
    whole_static_libs: [
        "darwinn_driver_noronha_noronha_driver_provider",
        // This dependeny only shows up when
        // -DDARWINN_PORT_ANDROID_SYSTEM=1, so we'd manually add it here.
        "darwinn_port_default_port",
    ],
}

// End of Generated Targets
//
subdirs = [
    "third_party"
]
