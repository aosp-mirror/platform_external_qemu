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
import shutil

from aemu.configure.base_builder import QemuBuilder
from aemu.configure.libraries import BazelLib, CMakeLib, CargoLib


class LinuxBuilder(QemuBuilder):
    def meson_config(self):
        return [
            "-Dandroid=enabled",
            "-Daudio_drv_list=default",
            "-Dfdt=auto",
            "-Dalsa=disabled",
            "-Dattr=disabled",
            "-Dauth_pam=disabled",
            "-Davx2=enabled",
            "-Davx512bw=enabled",
            "-Davx512f=enabled",
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
            "-Dcrypto_afalg=enabled",
            "-Dcurl=disabled",
            "-Dcurses=disabled",
            "-Ddbus_display=disabled",
            "-Ddmg=disabled",
            "-Ddsound=disabled",
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
            "-Dhvf=disabled",
            "-Diconv=disabled",
            "-Djack=disabled",
            "-Dkeyring=disabled",
            "-Dkvm=enabled",
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
            "-Dmalloc_trim=enabled",
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
            "-Dpa=enabled",
            "-Dparallels=disabled",
            "-Dpipewire=disabled",
            "-Dpixman=enabled",
            "-Dpng=disabled",
            "-Dpvrdma=disabled",
            "-Dqcow1=disabled",
            "-Dqed=disabled",
            "-Dqga_vss=disabled",
            "-Drbd=disabled",
            "-Drdma=disabled",
            "-Dreplication=disabled",
            "-Drutabaga_gfx=enabled",
            "-Dsdl=disabled",
            "-Dsdl_image=disabled",
            "-Dseccomp=disabled",
            "-Dselinux=disabled",
            "-Dslirp=enabled",
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
            "-Dvhost_crypto=enabled",
            "-Dvhost_kernel=enabled",
            "-Dvhost_net=enabled",
            "-Dvhost_user=enabled",
            "-Dvhost_user_blk_server=enabled",
            "-Dvhost_vdpa=disabled",
            "-Dvirglrenderer=disabled",
            "-Dvirtfs=disabled",
            "-Dvirtfs_proxy_helper=disabled",
            "-Dvmdk=disabled",
            "-Dvmnet=disabled",
            "-Dvnc=enabled",
            "-Dvnc_jpeg=disabled",
            "-Dvnc_sasl=disabled",
            "-Dvpc=disabled",
            "-Dvte=disabled",
            "-Dvvfat=disabled",
            "-Dwhpx=disabled",
            "-Dxen=disabled",
            "-Dxen_pci_passthrough=disabled",
            "-Dxkbcommon=disabled",
            "-Dzstd=disabled",
            "-Dauto_features=disabled",
            "-Dwerror=true",
            "-Ddocs=disabled",
            f"-Dprefix={self.dest / 'release'}",
            "--native-file",
        ]

    def packages(self):
        # We need the original .pc from our aosp sysroot, so let's copy them over.
        destination_directory = self.toolchain_generator.pkgconfig_directory
        destination_directory.mkdir(parents=True, exist_ok=True)
        source_directory = (
            self.toolchain_generator.system_root / "usr" / "lib" / "pkgconfig"
        )
        for source_file in source_directory.iterdir():
            destination_file = destination_directory / source_file.name
            shutil.copy2(source_file, destination_file)

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
            BazelLib(
                "//hardware/google/gfxstream/host:gfxstream_backend",
                "0.1.2",
                {},
            ),
            CargoLib(
                "/external/crosvm/rutabaga_gfx/ffi:rutabaga_gfx_ffi",
                "0.1.2",
                {"archive": "rutabaga_gfx_ffi"},
            ),  # Must be after libgxstream!
            BazelLib("//external/dtc:libfdt", "1.6.0", {}),
            BazelLib("@glib//:gmodule-static", "2.77.2", {}),
            BazelLib("@zlib//:zlib", "1.2.10", {}),
            BazelLib(
                "@glib//:glib-static",
                "2.77.2",
                {
                    "name": "glib-2.0",
                    "includes": [str(x) for x in includes],
                    "link_flags": "-pthread",
                    "Requires": "pcre2, gmodule-static",
                },
            ),
            BazelLib("@pixman//:pixman-1", "0.42.3", {"Requires": "pixman_simd"}),
            BazelLib("@pixman//:pixman_simd", "0.42.3", {}),
            BazelLib("@pcre2//:pcre2", "10.42", {}),
        ]

    def config_mak(self):
        return [
            "TARGET_DIRS=aarch64-softmmu riscv64-softmmu x86_64-softmmu",
            "TCG_TESTS_TARGETS= x86_64-softmmu",
            "CONFIG_POSIX=y",
            "CONFIG_LINUX=y",
            "GENISOIMAGE=",
            "TCG_TESTS_TARGETS=",
            f"CC={self.dest / 'toolchain' / 'gcc'}",
        ]
