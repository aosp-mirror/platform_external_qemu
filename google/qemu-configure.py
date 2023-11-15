#!/usr/bin/env python3
#
# Copyright 2023 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import argparse
import logging
import platform
import subprocess
import os
from pathlib import Path
from typing import Dict


here = Path(__file__).resolve().parent

class CustomFormatter(
    argparse.RawTextHelpFormatter, argparse.ArgumentDefaultsHelpFormatter
):
    pass

# Map which contains the configure flags for each target.
config = {
    "linux": [
        "--disable-download", # Let's not accidentally fetch external things
        "--target-list=x86_64-softmmu",  # We only want x86
        "--audio-drv-list=pa",  # Pulse is our linux audio driver
        "--enable-fdt=internal",  # Pick up the internal fdt
        "--without-default-features",  # Disable everything
        "--cpu=x86_64",  # We only support x86 host architecture.
        "--disable-alsa",  # ALSA sound support
        "--disable-attr",  # attr/xattr support
        "--disable-auth-pam",  # PAM access control
        "--disable-blkio",  # libblkio block device driver
        "--disable-bochs",  # bochs image format support
        "--disable-bpf",  # eBPF support
        "--disable-brlapi",  # brlapi character device driver
        "--disable-bsd-user",  # all BSD usermode emulation targets
        "--disable-bzip2",  # bzip2 support for DMG images
        "--disable-canokey",  # CanoKey support
        "--disable-cap-ng",  # cap_ng support
        "--disable-capstone",  # Whether and how to find the capstone library
        "--disable-cloop",  # cloop image format support
        "--disable-cocoa",  # Cocoa user interface (macOS only)
        "--disable-colo-proxy",  # colo-proxy support
        "--disable-coreaudio",  # CoreAudio sound support
        "--disable-curl",  # CURL block device driver
        "--disable-curses",  # curses UI
        "--disable-dbus-display",  # -display dbus support
        "--disable-debug-tcg",  # TCG debugging (default is disabled)
        "--disable-dmg",  # dmg image format support
        "--disable-docs",  # Documentations build support
        "--disable-dsound",  # DirectSound sound support
        "--disable-fuse-lseek",  # SEEK_HOLE/SEEK_DATA support for FUSE exports
        "--disable-fuse",  # FUSE block device export
        "--disable-gcrypt",  # libgcrypt cryptography support
        "--disable-gettext",  # Localization of the GTK+ user interface
        "--disable-gio",  # use libgio for D-Bus support
        "--disable-glusterfs",  # Glusterfs block device driver
        "--disable-gnutls",  # GNUTLS cryptography support
        "--disable-gtk-clipboard",  # clipboard support for the gtk UI (EXPERIMENTAL, MAY HANG)
        "--disable-gtk",  # GTK+ user interface
        "--disable-guest-agent-msi",  # Build MSI package for the QEMU Guest Agent
        "--disable-guest-agent",  # Build QEMU Guest Agent
        "--disable-hax",  # HAX acceleration support
        "--disable-hvf",  # HVF acceleration support
        "--disable-iconv",  # Font glyph conversion support
        "--disable-jack",  # JACK sound support
        "--disable-keyring",  # Linux keyring support
        "--disable-l2tpv3",  # l2tpv3 network backend support
        "--disable-libdaxctl",  # libdaxctl support
        "--disable-libdw",  # debuginfo support
        "--disable-libiscsi",  # libiscsi userspace initiator
        "--disable-libnfs",  # libnfs block device driver
        "--disable-libpmem",  # libpmem support
        "--disable-libssh",  # ssh block device support
        "--disable-libudev",  # Use libudev to enumerate host devices
        "--disable-libusb",  # libusb support for USB passthrough
        "--disable-libvduse",  # build VDUSE Library
        "--disable-linux-aio",  # Linux AIO support
        "--disable-linux-io-uring",  #  Linux io_uring support
        "--disable-linux-user",  # all linux usermode emulation targets
        "--disable-lzfse",  # lzfse support for DMG images
        "--disable-lzo",  # lzo compression support
        "--disable-membarrier",  # membarrier system call (for Linux 4.14+ or Windows
        "--disable-modules",  # modules support (non Windows)
        "--disable-mpath",  # Multipath persistent reservation passthrough
        "--disable-netmap",  # netmap network backend support
        "--disable-nettle",  # nettle cryptography support
        "--disable-numa",  # libnuma support
        "--disable-nvmm",  # NVMM acceleration support
        "--disable-opengl",  # OpenGL support
        "--disable-oss",  # OSS sound support
        "--disable-parallels",  # parallels image format support
        "--disable-pipewire",  # PipeWire sound support
        "--disable-png",  # PNG support with libpng
        "--disable-pvrdma",  # Enable PVRDMA support
        "--disable-qcow1",  # qcow1 image format support
        "--disable-qed",  # qed image format support
        "--disable-qga-vss",  # build QGA VSS support (broken with MinGW)
        "--disable-rbd",  # Ceph block device driver
        "--disable-rdma",  # Enable RDMA-based migration
        "--disable-replication",  # replication support
        "--disable-sdl-image",  # SDL Image support for icons
        "--disable-sdl",  # SDL user interface
        "--disable-seccomp",  # seccomp support
        "--disable-selinux",  # SELinux support in qemu-nbd
        "--disable-slirp-smbd",  # use smbd (at path --smbd=*) in slirp networking
        "--disable-slirp",  # libslirp user mode network backend support
        "--disable-smartcard",  # CA smartcard emulation support
        "--disable-snappy",  # snappy compression support
        "--disable-sndio",  # sndio sound support
        "--disable-sparse",  # sparse checker
        "--disable-spice-protocol",  #  Spice protocol support
        "--disable-spice",  # Spice server support
        "--disable-stack-protector",  # compiler-provided stack protection
        "--disable-tpm",  # TPM support
        "--disable-u2f",  # U2F emulation support
        "--disable-usb-redir",  # libusbredir support
        "--disable-vde",  # vde network backend support
        "--disable-vdi",  # vdi image format support
        "--disable-vduse-blk-export",  # VDUSE block export support
        "--disable-vfio-user-server",  # vfio-user server support
        "--disable-vhdx",  # vhdx image format support
        "--disable-virglrenderer",  # virgl rendering support
        "--disable-virtfs-proxy-helper",  # virtio-9p proxy helper support
        "--disable-virtfs",  # virtio-9p support
        "--disable-vmdk",  # vmdk image format support
        "--disable-vmnet",  # vmnet.framework network backend support
        "--disable-vnc-jpeg",  # JPEG lossy compression for VNC server
        "--disable-vnc-sasl",  # SASL authentication for VNC server
        "--disable-vnc",  # VNC server
        "--disable-vpc",  # vpc image format support
        "--disable-vte",  # vte support for the gtk UI
        "--disable-vvfat",  # vvfat image format support
        "--disable-whpx",  # WHPX acceleration support
        "--disable-xen-pci-passthrough",  # Xen PCI passthrough support
        "--disable-xen",  # Xen backend support
        "--disable-xkbcommon",  # xkbcommon support
        "--disable-zstd",  # zstd compression support
        # --- Set of enabled options ---
        "--enable-avx2",  # AVX2 optimizations
        "--enable-avx512bw",  # AVX512BW optimizations
        "--enable-avx512f",  # AVX512F optimizations
        "--enable-crypto-afalg",  # Linux AF_ALG crypto backend driver
        "--enable-kvm",  # KVM acceleration support
        "--enable-live-block-migration",  # block migration in the main migration stream
        "--enable-malloc-trim",  # enable libc malloc_trim() for memory optimization
        "-disable-multiprocess",  # Out of process device emulation support
        "--enable-pa",  # PulseAudio sound support
        "--enable-pie",  # Position Independent Executables
        "--enable-system",  # all system emulation targets
        "--enable-tcg",  # TCG support
        "--enable-tools",  # build support utilities that come with QEMU
        "--enable-user",  # supported user emulation targets
        "--enable-vhost-crypto",  # vhost-user crypto backend support
        "--enable-vhost-kernel",  # vhost kernel backend support
        "--enable-vhost-net",  # vhost-net kernel acceleration support
        "--enable-vhost-user-blk-server",  # build vhost-user-blk server
        "--enable-vhost-user",  # vhost-user backend support
        "--enable-vhost-vdpa",  # vhost-vdpa kernel backend support
        "--extra-ldflags=-pthread",  # Make sure to always link agains pthread
    ],
     "darwin": [
        # "--disable-download", # TODO: Somehow this is needed on MacOs..
        "--target-list=aarch64-softmmu",  # We only want x86
        "--audio-drv-list=coreaudio",  # Pulse is our linux audio driver
        "--enable-fdt=internal",  # Pick up the internal fdt
        "--without-default-features",  # Disable everything
        "--cpu=aarch64",
        "--disable-alsa",  # ALSA sound support
        "--disable-attr",  # attr/xattr support
        "--disable-auth-pam",  # PAM access control
        "--disable-blkio",  # libblkio block device driver
        "--disable-bochs",  # bochs image format support
        "--disable-bpf",  # eBPF support
        "--disable-brlapi",  # brlapi character device driver
        "--disable-bsd-user",  # all BSD usermode emulation targets
        "--disable-bzip2",  # bzip2 support for DMG images
        "--disable-canokey",  # CanoKey support
        "--disable-cap-ng",  # cap_ng support
        "--disable-capstone",  # Whether and how to find the capstone library
        "--disable-cloop",  # cloop image format support
        "--enable-cocoa",  # Cocoa user interface (macOS only)
        "--disable-colo-proxy",  # colo-proxy support
        "--enable-coreaudio",  # CoreAudio sound support
        "--disable-curl",  # CURL block device driver
        "--disable-curses",  # curses UI
        "--disable-dbus-display",  # -display dbus support
        "--disable-debug-tcg",  # TCG debugging (default is disabled)
        "--disable-dmg",  # dmg image format support
        "--disable-docs",  # Documentations build support
        "--disable-dsound",  # DirectSound sound support
        "--disable-fuse-lseek",  # SEEK_HOLE/SEEK_DATA support for FUSE exports
        "--disable-fuse",  # FUSE block device export
        "--disable-gcrypt",  # libgcrypt cryptography support
        "--disable-gettext",  # Localization of the GTK+ user interface
        "--disable-gio",  # use libgio for D-Bus support
        "--disable-glusterfs",  # Glusterfs block device driver
        "--disable-gnutls",  # GNUTLS cryptography support
        "--disable-gtk-clipboard",  # clipboard support for the gtk UI (EXPERIMENTAL, MAY HANG)
        "--disable-gtk",  # GTK+ user interface
        "--disable-guest-agent-msi",  # Build MSI package for the QEMU Guest Agent
        "--disable-guest-agent",  # Build QEMU Guest Agent
        "--disable-hax",  # HAX acceleration support
        "--enable-hvf",  # HVF acceleration support
        "--disable-iconv",  # Font glyph conversion support
        "--disable-jack",  # JACK sound support
        "--disable-keyring",  # Linux keyring support
        "--disable-l2tpv3",  # l2tpv3 network backend support
        "--disable-libdaxctl",  # libdaxctl support
        "--disable-libdw",  # debuginfo support
        "--disable-libiscsi",  # libiscsi userspace initiator
        "--disable-libnfs",  # libnfs block device driver
        "--disable-libpmem",  # libpmem support
        "--disable-libssh",  # ssh block device support
        "--disable-libudev",  # Use libudev to enumerate host devices
        "--disable-libusb",  # libusb support for USB passthrough
        "--disable-libvduse",  # build VDUSE Library
        "--disable-linux-aio",  # Linux AIO support
        "--disable-linux-io-uring",  #  Linux io_uring support
        "--disable-linux-user",  # all linux usermode emulation targets
        "--disable-lzfse",  # lzfse support for DMG images
        "--disable-lzo",  # lzo compression support
        "--disable-membarrier",  # membarrier system call (for Linux 4.14+ or Windows
        "--disable-modules",  # modules support (non Windows)
        "--disable-mpath",  # Multipath persistent reservation passthrough
        "--disable-netmap",  # netmap network backend support
        "--disable-nettle",  # nettle cryptography support
        "--disable-numa",  # libnuma support
        "--disable-nvmm",  # NVMM acceleration support
        "--disable-opengl",  # OpenGL support
        "--disable-oss",  # OSS sound support
        "--disable-parallels",  # parallels image format support
        "--disable-pipewire",  # PipeWire sound support
        "--disable-png",  # PNG support with libpng
        "--disable-pvrdma",  # Enable PVRDMA support
        "--disable-qcow1",  # qcow1 image format support
        "--disable-qed",  # qed image format support
        "--disable-qga-vss",  # build QGA VSS support (broken with MinGW)
        "--disable-rbd",  # Ceph block device driver
        "--disable-rdma",  # Enable RDMA-based migration
        "--disable-replication",  # replication support
        "--disable-sdl-image",  # SDL Image support for icons
        "--disable-sdl",  # SDL user interface
        "--disable-seccomp",  # seccomp support
        "--disable-selinux",  # SELinux support in qemu-nbd
        "--disable-slirp-smbd",  # use smbd (at path --smbd=*) in slirp networking
        "--disable-slirp",  # libslirp user mode network backend support
        "--disable-smartcard",  # CA smartcard emulation support
        "--disable-snappy",  # snappy compression support
        "--disable-sndio",  # sndio sound support
        "--disable-sparse",  # sparse checker
        "--disable-spice-protocol",  #  Spice protocol support
        "--disable-spice",  # Spice server support
        "--disable-stack-protector",  # compiler-provided stack protection
        "--disable-tpm",  # TPM support
        "--disable-u2f",  # U2F emulation support
        "--disable-usb-redir",  # libusbredir support
        "--disable-vde",  # vde network backend support
        "--disable-vdi",  # vdi image format support
        "--disable-vduse-blk-export",  # VDUSE block export support
        "--disable-vfio-user-server",  # vfio-user server support
        "--disable-vhdx",  # vhdx image format support
        "--disable-virglrenderer",  # virgl rendering support
        "--disable-virtfs-proxy-helper",  # virtio-9p proxy helper support
        "--disable-virtfs",  # virtio-9p support
        "--disable-vmdk",  # vmdk image format support
        "--enable-vmnet",  # vmnet.framework network backend support
        "--disable-vnc-jpeg",  # JPEG lossy compression for VNC server
        "--disable-vnc-sasl",  # SASL authentication for VNC server
        "--disable-vnc",  # VNC server
        "--disable-vpc",  # vpc image format support
        "--disable-vte",  # vte support for the gtk UI
        "--disable-vvfat",  # vvfat image format support
        "--disable-whpx",  # WHPX acceleration support
        "--disable-xen-pci-passthrough",  # Xen PCI passthrough support
        "--disable-xen",  # Xen backend support
        "--disable-xkbcommon",  # xkbcommon support
        "--disable-zstd",  # zstd compression support
        # --- Set of enabled options ---
        "--disable-avx2",  # AVX2 optimizations
        "--disable-avx512bw",  # AVX512BW optimizations
        "--disable-avx512f",  # AVX512F optimizations
        "--disable-crypto-afalg",  # Linux AF_ALG crypto backend driver
        "--disable-kvm",  # KVM acceleration support
        "--enable-live-block-migration",  # block migration in the main migration stream
        "--disable-malloc-trim",  # enable libc malloc_trim() for memory optimization
        "--disable-multiprocess",  # Out of process device emulation support
        "--disable-pa",  # PulseAudio sound support
        # "--enable-pie",  # Position Independent Executables
        "--enable-system",  # all system emulation targets
        "--enable-tcg",  # TCG support
        "--enable-tools",  # build support utilities that come with QEMU
        "--enable-user",  # supported user emulation targets
        "--disable-vhost-crypto",  # vhost-user crypto backend support
        "--disable-vhost-kernel",  # vhost kernel backend support
        "--disable-vhost-net",  # vhost-net kernel acceleration support
        "--disable-vhost-user-blk-server",  # build vhost-user-blk server
        "--disable-vhost-user",  # vhost-user backend support
        "--disable-vhost-vdpa",  # vhost-vdpa kernel backend support
        "--extra-ldflags=-pthread",  # Make sure to always link agains pthread
    ]
}


meson_config = {
    "linux": {
        "config_mak": [
            "all:",
            "CONFIG_POSIX=y",
            "CONFIG_LINUX=y",
            "SRC_PATH=/mnt/ssd/src/emu-dev/external/qemu",
            "TARGET_DIRS=x86_64-softmmu",
            "HAVE_GDB_BIN=/usr/bin/gdb",
            "ENGINE=docker",
            "RUNC=docker",
            "ROMS=",
            "PYTHON=/mnt/ram/tst/pyvenv/bin/python3 -B",
            "GENISOIMAGE=",
            "MESON=/mnt/ram/tst/pyvenv/bin/meson",
            "NINJA=/usr/local/google/home/jansene/bin/ninja",
            "PKG_CONFIG=pkg-config",
            "CC=/mnt/ram/tst/toolchain/linux-cc",
            "EXESUF=",
            "TCG_TESTS_TARGETS= x86_64-softmmu",
        ],
        "config-meson.cross": [
            "            # Automatically generated by configure - do not modify",
            "[properties]",
            "[built-in options]",
            "c_args = []",
            "cpp_args = []",
            "objc_args = []",
            "c_link_args = ['-pthread']",
            "cpp_link_args = ['-pthread']",
            "[binaries]",
            "c = ['/mnt/ram/tst/toolchain/linux-cc','-m64','-mcx16']",
            "cpp = ['/mnt/ram/tst/toolchain/linux-c++','-m64','-mcx16']",
            "objc = ['clang','-m64','-mcx16']",
            "ar = ['ar']",
            "nm = ['nm']",
            "pkgconfig = ['pkg-config']",
            "ranlib = ['ranlib']",
            "strip = ['strip']",
            "widl = ['widl']",
            "windres = ['windres']",
            "windmc = ['windmc']",
        ],
        "meson": [
            # "NINJA=/usr/local/google/home/jansene/bin/ninja",
            # "/mnt/ram/tst/pyvenv/bin/meson",
            # "--prefix",
            # "/usr/local",
            # "setup",
            "-Daudio_drv_list=default",
            "-Dfdt=internal",
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
            "-Dhax=disabled",
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
            "-Dvnc=disabled",
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
            "--native-file",
            "config-meson.cross",
        ],
    }
}


def run(cmd, env: Dict[str, str], cwd: Path = None) -> int:
    """Runs the given command

    Args:
        cmd: A list of strings representing the command to run.
        env: A dictionary of environment variables to set.
        cwd: The working directory to run the command in.

    Returns:
        The exit code of the command.
    """

    cmd = [str(x) for x in cmd]
    logging.info("Run: %s", " ".join(cmd))

    # Create a subprocess to run the command.
    process = subprocess.Popen(
        cmd,
        env=env,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        bufsize=1,
    )

    # Log the output of the command line by line.
    for line in process.stdout:
        logging.info(line.strip())

    # Wait for the command to finish and get the exit code.
    return_code = process.wait()

    # Log the final output and return code.
    logging.info(f"Command completed with exit code: {return_code}")

    if return_code != 0:
        raise subprocess.CalledProcessError(return_code, cmd)

    return return_code


def create_toolchain(out: Path, target: str) -> None:
    """Creates a compiler toolchain for the given target.

    Includes the proper pkgconfig files to discover dependencies
    to build QEMU for the target. The pkgconfig files will be
    autogenerated from the bazel targets contained in AOSP.

    Args:
        out: The output directory for the toolchain.
        target: The target architecture to create the toolchain for.
    """

    env = os.environ.copy()

    run(
        [
            str(here / "toolchain" / "src" / "aemu" / "toolchain.py"),
            str(out / "toolchain"),
            "--target",
            target,
            "--prefix",
            f"{target}-",
        ],
        env=env,
        cwd=here / "toolchain",
    )


def run_configure(out: Path, target: str) -> None:
    """Runs the configure script for QEMU, using the given target architecture and toolchain.

    Args:
        out: The output directory for the QEMU build.
        target: The target architecture to build QEMU for.
    """

    # First we create our toolchain with pkgconfig/meson and compilers
    create_toolchain(out, target)
    env = os.environ.copy()

    # Make sure we pick up the right tools.
    env["PATH"] = str(out / "toolchain") + os.pathsep + env["PATH"]
    run(
        [
            here.parent / "configure",
            f"--cc={out / 'toolchain' / f'{target}-cc'}",
            f"--host-cc={out / 'toolchain' / f'{target}-cc'}",
            f"--cxx={out / 'toolchain' / f'{target}-c++'}",
            f"--objcc={out / 'toolchain' / f'{target}-cc'}",
        ]
        + config[target],
        env=env,
        cwd=out,
    )


def main():
    parser = argparse.ArgumentParser(
        formatter_class=CustomFormatter,
        description=f"""
       QEMU configuration script
        """,
    )
    parser.add_argument(
        "--target",
        type=str,
        default=platform.system().lower(),
        help="Toolchain target",
    )
    parser.add_argument(
        "out",
        default=f"build/{platform.system().lower()}",
        type=str,
        help="Destination directory this build should be written",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        dest="verbose",
        default=False,
        action="store_true",
        help="Verbose logging",
    )

    args = parser.parse_args()
    lvl = logging.DEBUG if args.verbose else logging.INFO
    logging.basicConfig(level=lvl)

    out = Path(args.out).absolute()

    if out.exists():
        logging.fatal(
            "The directory %s already exists, please delete it first.",
            out,
        )
        return

    run_configure(out, args.target)


if __name__ == "__main__":
    main()
