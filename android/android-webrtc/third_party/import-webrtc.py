#!/usr/bin/env python
# Copyright 2019 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Converts a bazel BUILD file to CMakeLists.txt.

This is specifically tailored to converting G3 webrtc build
to a cmake representation that can be used to build webrtc modules.

Bazel BUILD files are basically simplistic python scripts.
Due to this we can just load them in, and transform them.

"""
import sys
import os.path
import argparse
import platform
import logging
from glob import iglob
from datetime import date


# The starting location of the webrtc modules.
PREFIX = "//third_party/webrtc/files/stable/webrtc"
WEBRTC_PREFIX = "webrtc_"

# Platform definitions, used by bazel to make selections on copt, defs etc.
# This mimics the effects of calling select:
# https://docs.bazel.build/versions/master/be/functions.html#select
PLATFORMS = {
    "Darwin": ["apple", "posix", "mac", "_all", "intel_cpu", ":cpu_x86_64"],
    "Darwin-aarch64": ["apple", "posix", "mac", "_all", "arm64", ":cpu_arm64"],
    "Linux-aarch64": ["linux", "posix", "linux_kernel", "arm64", "_all", ":cpu_arm64"],
    "Linux": ["linux", "posix", "linux_kernel", "_all", "intel_cpu", ":cpu_x86_64"],
    "Windows": ["windows", "windows_x64", "intel_cpu", "_all", ":cpu_x86_64"],
}

PLATFORM_NAME = {
    "Darwin": "darwin_x86_64.cmake",
    "Darwin-aarch64": "darwin_aarch64.cmake",
    "Linux-aarch64": "linux_aarch64.cmake",
    "Linux": "linux_x86_64.cmake",
    "Windows": "windows_x86_64.cmake",
}

# External dependencies that we translate to local ones..
DEP_MAP = {
    ":AppRTCMobile_lib": "unknown",
    "//testing/base/public:gunit_for_library_testonly": "gmock gtest",
    "//third_party/absl/algorithm:container": "absl::container",
    "//third_party/absl/algorithm": "absl::algorithm",
    "//third_party/absl/base:config": "absl::config",
    "//third_party/absl/base:core_headers": "absl::core_headers",
    "//third_party/absl/container:flat_hash_map": "absl::flat_hash_map",
    "//third_party/absl/container:inlined_vector": "absl::container",
    "//third_party/absl/debugging:failure_signal_handler": "absl::failure_signal_handler",
    "//third_party/absl/debugging:symbolize": "absl::debugging",
    "//third_party/absl/flags:config": "absl::flags_config",
    "//third_party/absl/flags:flag": "absl::flags",
    "//third_party/absl/flags:parse": "absl::flags_parse",
    "//third_party/absl/flags:usage": "absl::flags_usage",
    "//third_party/absl/memory": "absl::memory",
    "//third_party/absl/meta:type_traits": "absl::type_traits",
    "//third_party/absl/strings": "absl::strings",
    "//third_party/absl/synchronization": "absl::synchronization",
    "//third_party/absl/types:optional": "absl::optional",
    "//third_party/absl/types:variant": "absl::variant",
    "//third_party/catapult/tracing:histogram": "webrtc_catapult_histogram",
    "//third_party/catapult/tracing:reserved_infos": "webrtc_catapult_reserved_infos",
    "//third_party/catapult/tracing/tracing/proto:histogram_proto_bridge": "webrtc_histogram_proto_bridge",
    "//third_party/isac_fft:fft": "webrtc_fft",  # "unknown_fft",
    "//third_party/jsoncpp:json": "jsoncpp",
    "//third_party/libevent": "libevent",
    "//third_party/libopus": "opus",
    "//third_party/libsrtp:srtp": "libsrtp",
    "//third_party/libyuv": "webrtc-yuv",
    "//third_party/objective_c/ocmock/v3:OCMock": "OCMock",
    "//third_party/ooura_fft:fft_size_128": "webrtc_fft_size_128",
    "//third_party/ooura_fft:fft_size_256": "webrtc_fft_size_256",
    "//third_party/openssl:ssl": "ssl",
    "//third_party/pffft": "webrtc_pffft",
    "//third_party/protobuf:protobuf-lite_legacy": "libprotobuf",
    "//third_party/protobuf:py_proto_runtime": "unknown_py_proto_runtime",
    "//third_party/rnnoise:rnn_vad": "webrtc_rnnoise",
    "//third_party/spl_sqrt_floor": "webrtc_spl_sqrt_floor",
    "//third_party/usrsctp": "usrsctp",
    "//third_party/webrtc:webrtc_libvpx": "libvpx",
    "//third_party/webrtc/files/override/webrtc/rtc_base:protobuf_utils": "libprotobuf",
    "//third_party/webrtc/files/testing:fileutils_override_google3": "emulator_test_overrides",
    "//third_party/libjpeg_turbo/src:jpeg": "emulator-libjpeg",
    "//third_party/libaom:aom_highbd": "webrtc_libaom",  #
    # We rely on Qt5 to provide all these..
    "//third_party/GL:GLX_headers": "Qt5::Core",
    "//third_party/Xorg:Xorg_static": "Qt5::Core",
    "//third_party/Xorg:libX11_static": "Qt5::Core",
    "//third_party/Xorg:libXcomposite_static": "Qt5::Core",
    "//third_party/Xorg:libXdamage_static": "Qt5::Core",
    "//third_party/Xorg:libXfixes_static": "Qt5::Core",
    "//third_party/Xorg:libXrandr_static": "Qt5::Core",
    "//third_party/Xorg:libXtst_static": "Qt5::Core",
    "//third_party/mesa:GL": "mesa",
}


SOURCE_EXT = [".c", ".cc", ".cpp", ".m", ".mm"]


def is_source_file(name):
    return any([name.endswith(x) for x in SOURCE_EXT])


def bazel_package_name(target):
    """Returns the package name"""
    idx = target.find(":")
    if idx < 0:
        return target
    return target[:idx]


def bazel_label_name(target):
    """Gets the label from the complete bazel target."""
    label = target[target.rfind("/") + 1 :]
    return label[label.find(":") + 1 :]


def cmake_target_to_bazel_target(path, label):
    if os.path.basename(path) == label:
        return "{}/{}".format(PREFIX, path)
    return "{}/{}:{}".format(PREFIX, path, label)


def bazel_target_to_cmake_target(name):
    if name in DEP_MAP:
        return DEP_MAP[name]

    pkg = bazel_package_name(name)
    lbl = bazel_label_name(name)

    if not pkg.startswith(PREFIX):
        logging.error("Missing library: %s", name)
        return "missing_{}".format(name).replace("/", "_")
        # raise Exception('Unknown dependency: "{}" --> {}:{}'.format(name, pkg, lbl))

    strip_prefix = pkg[len(PREFIX) + 1 :]
    return "{}{}_{}".format(WEBRTC_PREFIX, strip_prefix, lbl).replace("/", "_")


def clang_opts_fixer(copts):
    """Fixes clang options, by doing variable substition and / to -/"""
    flag_replace = {
        "/GR-": "-GR-",  # We are using clang, not msvc.
        "/arch:AVX2": "-mavx2 -mfma",  # clang vs cl
        "/TC": "-TC",
        "$(WEBRTC_RTTI)": "",  # No exceptions already.
    }
    return [flag_replace[x] if x in flag_replace else x for x in copts]


class Target(object):
    def __init__(self, path, name, typ, srcs, hdrs, defs, deps, copts, includes, lopts):
        self.path = path
        self.label = name
        self.name = "{}{}_{}".format(WEBRTC_PREFIX, path, name).replace("/", "_")
        self.typ = typ
        self.srcs = srcs
        self.hdrs = hdrs
        self.defs = defs
        self.deps = deps
        self.includes = [
            "${{WEBRTC_ROOT}}/{}/{}".format(self.path, x) for x in includes
        ] + ["${WEBRTC_ROOT}", "${CMAKE_CURRENT_BINARY_DIR}"]
        # TODO variable replacement should be done for all.
        self.copts = clang_opts_fixer(copts)
        self.lopts = [
            "{lib}::{lib}".format(lib=x[:-4]) for x in lopts if x.endswith(".lib")
        ]

    def _add_deps(self, name, deps, sort="PUBLIC"):
        common = [bazel_target_to_cmake_target(x) for x in deps]
        return "target_link_libraries({} {} {} {})\n".format(
            name, sort, " ".join(common), " ".join(self.lopts)
        )

    def is_interface(self):
        files = self.srcs + self.hdrs
        return not any([is_source_file(x) for x in files])

    def cmake_snippet(self):
        snippet = "# {}\n".format(
            cmake_target_to_bazel_target(self.path, self.label)[len(PREFIX) + 1 :]
        )

        if self.is_interface() and self.typ != "add_executable":
            snippet += "add_library({} INTERFACE)\n".format(self.name)
            if self.deps:
                snippet += self._add_deps(self.name, self.deps, "INTERFACE")
            snippet += "target_include_directories({} INTERFACE {})\n".format(
                self.name, " ".join(self.includes)
            )
            return snippet

        sources = ["${{WEBRTC_ROOT}}/{}/{}".format(self.path, x) for x in self.srcs]

        if self.is_interface():
            sources = ["${CMAKE_CURRENT_BINARY_DIR}/dummy.cc"]

        if self.typ == "android_add_test":
            snippet += "  android_add_executable(TARGET {} NODISTRIBUTE SRC {})\n".format(
                self.name, " ".join(sources)
            )
        else:
            snippet += "{}({} {})\n".format(self.typ, self.name, " ".join(sources))
        snippet += "target_include_directories({} PRIVATE {})\n".format(
            self.name, " ".join(self.includes)
        )

        if self.defs:
            snippet += "target_compile_definitions({} PRIVATE {})\n".format(
                self.name, " ".join(self.defs)
            )

        if self.copts:
            snippet += "target_compile_options({} PRIVATE {})\n".format(
                self.name, " ".join(self.copts)
            )

        if self.deps:
            snippet += self._add_deps(self.name, self.deps)

        return snippet


class ObjcTarget(Target):
    def __init__(self, path, name, typ, srcs, hdrs, defs, deps, copts, includes, sdk):
        super(ObjcTarget, self).__init__(
            path, name, typ, srcs, hdrs, defs, deps, copts, includes, []
        )
        self.sdk = sdk

    def cmake_snippet(self):
        snippet = super(ObjcTarget, self).cmake_snippet()
        sdks = ['"-framework {}"'.format(x) for x in self.sdk]
        sort = "INTERFACE" if self.is_interface() else "PRIVATE"
        snippet += "target_link_libraries({} {} {})\n".format(
            self.name, sort, " ".join(sdks)
        )
        return snippet


class ProtobufTarget(object):
    def __init__(self, path, name, srcs):
        self.path = path
        self.label = name
        self.name = "{}{}_{}".format(WEBRTC_PREFIX, path, name).replace("/", "_")
        self.srcs = srcs
        self.deps = []

    def cmake_snippet(self):
        snippet = "# {}\n".format(cmake_target_to_bazel_target(self.path, self.label))
        snippet += "add_library({})\n".format(self.name)

        src_snippet = ""
        path_snippet = ""
        dest_path = ""
        for src in self.srcs:
            # Hack attack! We assume that all protobufs are from the same
            # dir..
            dest_path = os.path.dirname(os.path.join(self.path, src))
            src_snippet += " ${{WEBRTC_ROOT}}/{}/{}".format(self.path, src)
            path_snippet += " -I${{WEBRTC_ROOT}}/{}".format(dest_path)

        snippet += """protobuf_generate_with_plugin(
  TARGET {tgt}
  PROTOS {src}
  HEADERFILEEXTENSION .pb.h
  APPEND_PATH
  PROTOPATH {include_path}
  PROTOC_OUT_DIR ${{CMAKE_CURRENT_BINARY_DIR}}/{path})\n""".format(
            tgt=self.name, path=dest_path, include_path=path_snippet, src=src_snippet
        )
        snippet += "target_include_directories({} PUBLIC  ${{CMAKE_CURRENT_BINARY_DIR}}/{})\n".format(
            self.name, dest_path
        )
        snippet += "target_link_libraries({} PUBLIC libprotobuf)\n".format(self.name)
        return snippet


class Targets(object):
    def __init__(self):
        self.targets = {}

    def add(self, tgt):
        logging.debug("Adding target: %s", tgt.name)
        self.targets[tgt.name] = tgt

    def _calc_closure(self, name, seen):
        if name not in self.targets:
            return set()

        tgt = self.targets[name]
        if tgt in seen:
            return set()

        seen.add(tgt)
        for dep in tgt.deps:
            # Normalize the dependency
            seen |= self._calc_closure(bazel_target_to_cmake_target(dep), seen)

        return seen

    def closure(self, name):
        """Returns the target closure for the target of the given name."""
        return self._calc_closure(name, set())

    def platform_definition_snippet(self):
        """Writes out the platform target definitions, extracted from webrtc_lib_link_test"""
        target = self.targets["webrtc__webrtc_lib_link_test"]
        snippet = "# Platform specific set of defines\n"
        snippet += "add_library(webrtc_platform_defs INTERFACE)\n"
        snippet += (
            "target_compile_definitions(webrtc_platform_defs INTERFACE {}\n)".format(
                " ".join(target.defs)
            )
        )
        return snippet


class BuildFileFunctions(object):
    def __init__(self, targets, fname, start_dir, platforms):
        self.targets = targets
        self.start_dir = os.path.abspath(start_dir)
        self.rel = os.path.relpath(
            os.path.dirname(os.path.abspath(fname)),
            self.start_dir,
        )
        if self.rel == ".":
            self.rel = ""
        self.platforms = platforms

    def _add_target(self, typ, name, srcs, hdrs, defs, deps, copts, lopts, includes):
        self.targets.add(
            Target(self.rel, name, typ, srcs, hdrs, defs, deps, copts, includes, lopts)
        )

    def load(self, *args):
        pass

    def android_jni_library(self, **kwargs):
        pass

    def ios_application(self, **kwargs):
        pass

    def alias(self, **kwargs):
        pass

    def ios_unit_test(self, **kwargs):
        pass

    def android_instrumentation_test(self, **kwargs):
        pass

    def java_cpp_enum(self, **args):
        pass

    def setup_webrtc_build(self, *args):
        pass

    def android_library(self, **kwargs):
        pass

    def go_proto_library(self, **kwargs):
        pass

    def filegroup(self, **kwargs):
        pass

    def platform_select(self, **kwargs):
        selected = []
        for plf in self.platforms:
            selected += kwargs.get(plf, [])
        return selected

    def generated_jar_jni_library(self, **kwargs):
        pass

    def select(self, **kwargs):
        selected = []
        for plf in self.platforms:
            selected += kwargs.get(plf, [])
        return selected

    def generated_jni_library(self, **kwargs):
        pass

    def android_binary(self, **kwargs):
        pass

    def java_import(self, **kwargs):
        pass

    def webrtc_proto_library(self, **kwargs):
        self.targets.add(
            ProtobufTarget(self.rel, kwargs["name"] + "_bridge", kwargs["srcs"])
        )

    def cc_library(self, **kwargs):
        self._add_target(
            "add_library",
            kwargs["name"],
            kwargs.get("srcs", []),
            kwargs.get("hdrs", []),
            kwargs.get("defines", []),
            kwargs.get("deps", []),
            kwargs.get("copts", []),
            kwargs.get("linkopts", []),
            kwargs.get("includes", []),
        )

    def objc_library(self, **kwargs):
        if "apple" in self.platforms:
            self.targets.add(
                ObjcTarget(
                    self.rel,
                    kwargs["name"],
                    "add_library",
                    kwargs.get("srcs", []),
                    kwargs.get("hdrs", []),
                    kwargs.get("defines", []),
                    kwargs.get("deps", []),
                    kwargs.get("copts", []) + ["-fobjc-weak"],
                    kwargs.get("includes", []),
                    kwargs.get("sdk_frameworks", []),
                )
            )

    def cc_binary(self, **kwargs):
        self._add_target(
            "add_executable",
            kwargs["name"],
            kwargs.get("srcs", []),
            kwargs.get("hdrs", []),
            kwargs.get("defines", []),
            kwargs.get("deps", []),
            kwargs.get("copts", []),
            kwargs.get("linkopts", []),
            kwargs.get("includes", []),
        )

    def cc_test(self, **kwargs):
        self._add_target(
            "add_executable",
            kwargs["name"],
            kwargs.get("srcs", []),
            kwargs.get("hdrs", []),
            kwargs.get("defines", []),
            kwargs.get("deps", []),
            kwargs.get("copts", []),
            kwargs.get("linkopts", []),
            kwargs.get("includes", []),
        )

    def cc_and_android_test(self, **kwargs):
        self._add_target(
            "android_add_test",
            kwargs["name"],
            kwargs.get("srcs", []),
            kwargs.get("hdrs", []),
            kwargs.get("defines", []),
            kwargs.get("deps", []),
            kwargs.get("copts", []),
            kwargs.get("linkopts", []),
            kwargs.get("includes", []),
        )

    def build_test(self, **kwargs):
        pass

    def py_library(self, **kwargs):
        pass

    def py_binary(self, **kwargs):
        pass

    def package(self, **kwargs):
        pass

    def lua_cclibrary(self, **kwargs):
        pass

    def lua_library(self, **kwargs):
        pass

    def lua_binary(self, **kwargs):
        pass

    def lua_test(self, **kwargs):
        pass

    def sh_test(self, **kwargs):
        pass

    def make_shell_script(self, **kwargs):
        pass

    def exports_files(self, files, **kwargs):
        pass

    def proto_library(self, **kwargs):
        self.targets.add(ProtobufTarget(self.rel, kwargs["name"], kwargs["srcs"]))

    def generated_file_staleness_test(self, **kwargs):
        pass

    def upb_amalgamation(self, **kwargs):
        pass

    def upb_proto_library(self, **kwargs):
        pass

    def upb_proto_reflection_library(self, **kwargs):
        pass

    def genrule(self, **kwargs):
        pass

    def config_setting(self, **kwargs):
        pass

    def select(self, arg_dict):
        return []

    def glob(self, *args):
        return []

    def licenses(self, *args):
        pass

    def map_dep(self, arg):
        return arg


def write_cmake_project(targets, platform):
    cmake =  "# Generated on {} for target: {}\n".format(date.today().strftime("%m/%d/%y"), platform)
    cmake += "# This is an autogenerated file by calling:\n\n"
    cmake += "# {}\n\n".format(" ".join(sys.argv))
    cmake += "# Re-running this script will require you to merge in the latest upstream-master for webrtc\n\n"
    cmake += """
# Create a symlink so webrtc can find required includes, we already have all
# the required dependencies in our tree.
message("Creating webrtc test symlinks ${CMAKE_CURRENT_BINARY_DIR}/testing/")
android_symlink("${AOSP_ROOT}/external/googletest/googletest/" "gtest" ${CMAKE_CURRENT_BINARY_DIR}/testing)
android_symlink("${AOSP_ROOT}/external/googletest/googlemock/" "gmock" ${CMAKE_CURRENT_BINARY_DIR}/testing)

# ==== Generated modules are coming next ====
    """

    for target in targets:
        cmake += "\n" + target.cmake_snippet()

    return cmake


def GetDict(obj):
    ret = {}
    for k in dir(obj):
        if not k.startswith("_"):
            ret[k] = getattr(obj, k)
    return ret


def transform_bazel(bld_file, root_dir, platform, converter):
    logging.info("  %s - %s", platform, bld_file)
    exec(
        compile(open(bld_file, "rb").read(), bld_file, "exec"),
        GetDict(BuildFileFunctions(converter, bld_file, root_dir, PLATFORMS[platform])),
    )


def main():
    if sys.version_info[0] == 2:
        print("This only runs under Python3")
        sys.exit(1)

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--verbose", help="increase output verbosity", action="store_true"
    )
    parser.add_argument(
        "--platform",
        default=platform.system(),
        choices=(PLATFORMS.keys()),
        help="Target platform to generate cmake file for.",
    )
    parser.add_argument(
        "--root",
        default=os.path.abspath(os.getcwd()),
        help="Root directory to map the BUILD root to.",
    )

    parser.add_argument(
        "--target",
        action="append",
        help="Target closure to extract.",
    )

    parser.add_argument(
        "source",
        default="BUILD",
        help="First BUILD file to process, will descent recursively to load all of them.",
    )

    parser.add_argument(
        "out",
        default=None,
        help="Dir, or cmake file to write to.",
    )

    args = parser.parse_args()

    # Configure logger.
    lvl = logging.DEBUG if args.verbose else logging.WARNING
    handler = logging.StreamHandler()
    logging.root = logging.getLogger("root")
    logging.root.addHandler(handler)
    logging.root.setLevel(lvl)

    targets = Targets()

    source = os.path.join(args.root, args.source)
    transform_bazel(source, args.root, args.platform, targets)
    start_dir = os.path.dirname(source)
    for fn in iglob(start_dir + "/**/BUILD", recursive=True):
        if os.path.isfile(fn):
            transform_bazel(fn, args.root, args.platform, targets)

    cmake = set()
    for x in args.target:
        cmake |= targets.closure(x)

    out = sys.stdout

    if args.out:
        if os.path.isdir(args.out):
            out = open(os.path.join(args.out, PLATFORM_NAME[args.platform]), "w")
        else:
            out = open(args.out, "w")

    out.write(write_cmake_project(cmake, args.platform))
    out.write(targets.platform_definition_snippet())


if __name__ == "__main__":
    main()
