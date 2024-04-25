cc_library(
    name = "compat",
    srcs =
        glob([
            "src/dirent/*.c",
            "src/*.c",
            "src/*.h",
            "src/*.cpp",
        ]),
    hdrs = glob(["includes/**/*.h"]),
    includes = [
        "includes",
        "includes/dirent",
        "src",
    ],
    linkopts = [
        "-DEFAULTLIB:ws2_32.lib",
        "-DEFAULTLIB:Pathcch.lib",
        "-DEFAULTLIB:ole32.lib",
        "-DEFAULTLIB:dxguid.lib",
        "-DEFAULTLIB:Winmm.lib",
    ],
    linkstatic = True,
    visibility = ["//visibility:public"],
)
