# Copyright 2020 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import subprocess
import sys
import lxml.etree as etree

def manifest_fetch_command(build_id):
    return "/google/data/ro/projects/android/fetch_artifact --bid %s --target gfxstream_sdk_tools_linux 'manifest_%s.xml'" % build_id

input_manifest_filename = sys.argv[1]
as_proj_list = (int(sys.argv[2]) == 1) if len(sys.argv) > 2 else True

# Don't need to check out the entire emulator repo to build gfxstream
min_projects = set([
  "device/generic/goldfish-opengl",
  "platform/external/angle",
  "platform/external/astc-codec",
  "platform/external/boringssl",
  "platform/external/c-ares",
  "platform/external/curl",
  "platform/external/deqp",
  "platform/external/ffmpeg",
  "platform/external/googletest",
  "platform/external/google-benchmark",
  "platform/external/google-breakpad",
  "platform/external/grpc-grpc",
  "platform/external/libffi",
  "platform/external/libvpx",
  "platform/external/libyuv",
  "platform/external/libpng",
  "platform/external/lz4",
  "platform/external/protobuf",
  "platform/external/qemu",
  "platform/external/tinyobjloader",
  "platform/external/nasm",
  "platform/external/zlib",
  "platform/hardware/google/gfxstream",
  "platform/prebuilts/android-emulator-build/common",
  "platform/prebuilts/android-emulator-build/curl",
  "platform/prebuilts/android-emulator-build/mesa",
  "platform/prebuilts/android-emulator-build/mesa-deps",
  "platform/prebuilts/android-emulator-build/protobuf",
  "platform/prebuilts/android-emulator-build/qemu-android-deps",
  "platform/prebuilts/clang/host/linux-x86",
  "platform/prebuilts/cmake/linux-x86",
  "platform/prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.17-4.8",
  "platform/prebuilts/ninja/linux-x86",
])

def generate_filtered_manifest(filename, as_proj_list=False):
    outs = []

    out = etree.Element("manifest")

    t = etree.parse(filename)
    r = t.getroot()

    for e in r.findall("project"):
        if e.attrib["name"] in min_projects:
            outp = etree.SubElement(out, "project")
            outs.append(outp)
            outp.set("name", e.attrib["name"])
            outp.set("path", e.attrib["path"])
            outp.set("revision", e.attrib["revision"])
            outp.set("clone-depth", "1")

    if as_proj_list:
        return outs
    else:
        return out

if as_proj_list:
    for e in generate_filtered_manifest(input_manifest_filename, as_proj_list=as_proj_list):
        print(etree.tostring(e, pretty_print=True, encoding="utf-8")),
else:
    print(etree.tostring(
        generate_filtered_manifest(input_manifest_filename, as_proj_list=as_proj_list),
            pretty_print=True,
            xml_declaration=True,encoding="utf-8"))



