cc_library(
    name = "compat",
    srcs = glob(
        ["src/*.cc"],
        ["linux/**/*.h"],
    ),
    linkstatic = True,
    visibility = ["//visibility:public"],
    deps = [
        "@gcc_lib//:libpulse",
        "@gcc_lib//:libutil",
    ],
)
