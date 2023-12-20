# -*- coding: utf-8 -*-
# Copyright 2023 - The Android Open Source Project
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

from aemu.configure.base_builder import LibInfo
from aemu.configure.linux_builder import LinuxBuilder

class TrustyBuilder(LinuxBuilder):
    def meson_config(self):
        linux_cfg = super().meson_config()

        # Disable PulseAudio
        linux_cfg.remove("-Dpa=enabled")

        # Link statically
        return [
            "-Dpa=disabled",
            "-Dprefer_static=true",
            "-Db_pie=false",
        ] + linux_cfg

    def packages(self):
        # Similar to linux, but using static dependencies.
        super().packages();

        includes = [
            self.aosp / "external" / "glib",
            self.aosp / "external" / "glib" / "gmodule",
            self.aosp / "external" / "glib" / "os" / "linux",
            self.aosp / "external" / "glib" / "os" / "linux" / "glib",
            self.aosp / "external" / "glib" / "os" / "linux" / "gmodule",
            "${libdir}",
        ]
        # Next we have our dependencies.
        return [
            LibInfo("//external/dtc:libfdt", "1.6.0", {}),
            LibInfo("@glib//:gmodule-static", "2.77.2", {}),
            LibInfo("@glib//:glib-static", "2.77.2",
                {
                    "name": "glib-2.0",
                    "includes": [str(x) for x in includes],
                    "link_flags": "-pthread",
                    "Requires": "pcre2, gmodule-static",
                },
                    ),
            LibInfo("@zlib//:zlib", "1.2.10", {}),
            LibInfo("@pixman//:pixman-1", "0.42.3", {"Requires": "pixman_simd"}),
            LibInfo("@pixman//:pixman_simd", "0.42.3", {}),
            LibInfo("@pcre2//:pcre2", "10.42", {}),
        ]

