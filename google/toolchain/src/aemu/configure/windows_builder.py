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
from aemu.configure.base_builder import QemuBuilder, LibInfo


class WindowsBuilder(QemuBuilder):
    def packages(self):
        # So bazel does not give the right includes, so we just shim them here.
        includes = [
            self.aosp / "external" / "glib",
            self.aosp / "external" / "glib" / "glib" / "dirent",
            self.aosp / "external" / "glib" / "gmodule",
            self.aosp / "external" / "glib" / "os" / "windows",
            self.aosp / "external" / "glib" / "os" / "windows" / "glib",
            self.aosp / "external" / "glib" / "os" / "windows" / "gmodule",
            "${libdir}",
        ]

        bazel_configs = [
            LibInfo(
                "@zlib//:zlib",
                "1.2.10",
                {
                    "includes": [str(self.aosp / "external" / "zlib")],
                },
            ),
            LibInfo("@glib//:gmodule-static", "2.77.2", {}),
            LibInfo("@glib//:gnulib", "2.77.2", {}),
            LibInfo("@glib//:dirent", "2.77.2", {}),
            LibInfo(
                "@glib//:glib-static",
                "2.77.2",
                {
                    "includes": [str(x) for x in includes],
                    "Requires": "pcre2, zlib, gmodule-static, gnulib, dirent",
                    "name": "glib-2.0",
                    "link_name": "glib-2.0.lib",
                    "cflags": "-DGLIB_STATIC_COMPILATION",
                    "dll_ext": "",
                },
            ),
            LibInfo(
                "@pixman//:pixman-1",
                "0.42.3",
                {
                    "Requires": "pixman_simd",
                    "includes": [
                        str(self.aosp / "external" / "pixman" / "pixman"),
                        str(
                            self.aosp
                            / "external"
                            / "pixman"
                            / "os"
                            / "windows"
                            / "pixman"
                        ),
                    ],
                },
            ),
            LibInfo("@pixman//:pixman_simd", "0.42.3", {}),
            LibInfo("@pcre2//:pcre2", "10.42", {}),
            LibInfo("//external/dtc:libfdt", "1.6.0", {}),
        ]
        return bazel_configs

    def meson_config(self):
        prefix = (self.dest / "release").as_posix()
        return [
            "-Dandroid=enabled",
            "-Db_pie=false",
            "-Daudio_drv_list=default",
            "-Dfdt=auto",
            "-Dalsa=disabled",
            "-Dattr=disabled",
            "-Dauth_pam=disabled",
            # TODO(jansene): Figure out how we get these turned on in the build bots!
            "-Davx2=auto",
            "-Davx512bw=auto",
            "-Davx512f=auto",
            "-Dblkio=disabled",
            "-Dbochs=disabled",
            "-Dbpf=disabled",
            "-Dbrlapi=disabled",
            "-Dbzip2=disabled",
            "-Dcanokey=disabled",
            "-Dcap_ng=disabled",
            "-Dcapstone=disabled",
            "-Dcloop=disabled",
            "-Dcocoa=disabled",
            "-Dcolo_proxy=disabled",
            "-Dcoreaudio=disabled",
            "-Dcrypto_afalg=disabled",
            "-Dcurl=disabled",
            "-Dcurses=disabled",
            "-Ddbus_display=disabled",
            "-Ddmg=disabled",
            "-Ddsound=enabled",
            "-Dfuse=disabled",
            "-Dfuse_lseek=disabled",
            "-Dgcrypt=disabled",
            "-Dgettext=disabled",
            "-Dgio=disabled",
            "-Dglusterfs=disabled",
            "-Dgnutls=disabled",
            "-Dgtk=disabled",
            "-Dgtk_clipboard=disabled",
            "-Dguest_agent=disabled",
            "-Dguest_agent_msi=disabled",
            "-Dhax=enabled",
            "-Dhvf=disabled",
            "-Diconv=disabled",
            "-Djack=disabled",
            "-Dkeyring=disabled",
            "-Dkvm=disabled",
            "-Dl2tpv3=disabled",
            "-Dlibdaxctl=disabled",
            "-Dlibdw=disabled",
            "-Dlibiscsi=disabled",
            "-Dlibnfs=disabled",
            "-Dlibpmem=disabled",
            "-Dlibssh=disabled",
            "-Dlibudev=disabled",
            "-Dlibusb=disabled",
            "-Dlibvduse=disabled",
            "-Dlinux_aio=disabled",
            "-Dlinux_io_uring=disabled",
            "-Dlive_block_migration=enabled",
            "-Dlzfse=disabled",
            "-Dlzo=disabled",
            "-Dmalloc_trim=disabled",
            "-Dmembarrier=disabled",
            "-Dmodules=disabled",
            "-Dmpath=disabled",
            "-Dmultiprocess=disabled",
            "-Dnetmap=disabled",
            "-Dnettle=disabled",
            "-Dnuma=disabled",
            "-Dnvmm=disabled",
            "-Dopengl=disabled",
            "-Doss=disabled",
            "-Dpa=disabled",
            "-Dparallels=disabled",
            "-Dpipewire=disabled",
            "-Dpng=disabled",
            "-Dpvrdma=disabled",
            "-Dqcow1=disabled",
            "-Dqed=disabled",
            "-Dqga_vss=disabled",
            "-Drbd=disabled",
            "-Drdma=disabled",
            "-Dreplication=disabled",
            "-Dsdl=disabled",
            "-Dsdl_image=disabled",
            "-Dseccomp=disabled",
            "-Dselinux=disabled",
            "-Dslirp=disabled",
            "-Dslirp_smbd=disabled",
            "-Dsmartcard=disabled",
            "-Dsnappy=disabled",
            "-Dsndio=disabled",
            "-Dsparse=disabled",
            "-Dspice=disabled",
            "-Dspice_protocol=disabled",
            "-Dstack_protector=disabled",
            "-Dtools=enabled",
            "-Dtpm=disabled",
            "-Du2f=disabled",
            "-Dusb_redir=disabled",
            "-Dvde=disabled",
            "-Dvdi=disabled",
            "-Dvduse_blk_export=disabled",
            "-Dvfio_user_server=disabled",
            "-Dvhdx=disabled",
            "-Dvhost_crypto=disabled",
            "-Dvhost_kernel=disabled",
            "-Dvhost_net=enabled",
            "-Dvhost_user=disabled",
            "-Dvhost_user_blk_server=disabled",
            "-Dvhost_vdpa=disabled",
            "-Dvirglrenderer=disabled",
            "-Dvirtfs=disabled",
            "-Dvirtfs_proxy_helper=disabled",
            "-Dvmdk=disabled",
            "-Dvmnet=disabled",
            "-Dvnc=disabled",
            "-Dvnc_jpeg=disabled",
            "-Dvnc_sasl=disabled",
            "-Dvpc=disabled",
            "-Dvte=disabled",
            "-Dvvfat=disabled",
            "-Dwhpx=enabled",
            "-Dxen=disabled",
            "-Dxen_pci_passthrough=disabled",
            "-Dxkbcommon=disabled",
            "-Dzstd=disabled",
            "-Dauto_features=disabled",
            "-Dwerror=true",
            "-Ddocs=disabled",
            f"-Dprefix={prefix}",
            "--vsenv",
            "--native-file",
        ]

    def config_mak(self):
        return [
            "TARGET_DIRS=aarch64-softmmu riscv64-softmmu x86_64-softmmu",
            "CONFIG_WIN32=y",
            "GENISOIMAGE=False",
            "TCG_TESTS_TARGETS=x86_64-softmmu",
            "EXESUF=.exe",
        ]
