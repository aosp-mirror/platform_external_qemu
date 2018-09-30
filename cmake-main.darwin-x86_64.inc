## AUTO-GENERATED AT SHA: da0f73eab7181e857f0ee2c98ed66ff5635d38b0 ##
# Do not edit, instead run gen-cmake.py!
# Defines targets: qemu-io
#   qemu-img
#   qemu-system-aarch64
#   ivshmem-server
#   ivshmem-client
#   qemu-nbd
#   qemu-system-x86_64
#   qemu-system-mipsel
#   libqemustub
#   libqemuutil
#   qemu-system-i386
#   qemu-system-mips64el
#   qemu-system-arm

 ## TARGET: qemu2-shared ##

set(qemu2-shared_sources ""
blockdev.c
   blockdev-nbd.c
   iothread.c
   qdev-monitor.c
   device-hotplug.c
   os-posix.c
   bt-host.c
   bt-vhci.c
   dma-helpers.c
   tpm.c
   device_tree.c
   qmp.c
   hmp.c
   cpus-common.c
   audio/audio.c
   audio/noaudio.c
   audio/wavaudio.c
   audio/mixeng.c
   audio/coreaudio.c
   audio/wavcapture.c
   backends/rng.c
   backends/rng-egd.c
   backends/rng-random-generic.c
   backends/tpm.c
   backends/hostmem.c
   backends/hostmem-ram.c
   backends/cryptodev.c
   backends/cryptodev-builtin.c
   block/stream.c
   disas/i386.c
   hw/acpi/acpi_interface.c
   hw/acpi/bios-linker-loader.c
   hw/acpi/aml-build.c
   hw/audio/es1370.c
   hw/audio/ac97.c
   hw/audio/intel-hda.c
   hw/audio/hda-codec.c
   hw/audio/goldfish_audio.c
   hw/block/block.c
   hw/block/cdrom.c
   hw/block/hd-geometry.c
   hw/block/pflash_cfi01.c
   hw/block/nvme.c
   hw/bt/core.c
   hw/bt/l2cap.c
   hw/bt/sdp.c
   hw/bt/hci.c
   hw/bt/hid.c
   hw/bt/hci-csr.c
   hw/char/ipoctal232.c
   hw/char/serial.c
   hw/char/serial-isa.c
   hw/char/serial-pci.c
   hw/char/virtio-console.c
   hw/core/qdev.c
   hw/core/qdev-properties.c
   hw/core/bus.c
   hw/core/reset.c
   hw/core/fw-path-provider.c
   hw/core/irq.c
   hw/core/hotplug.c
   hw/core/sysbus.c
   hw/core/machine.c
   hw/core/loader.c
   hw/core/qdev-properties-system.c
   hw/core/register.c
   hw/core/or-irq.c
   hw/display/vga-pci.c
   hw/display/framebuffer.c
   hw/display/goldfish_fb.c
   hw/i2c/core.c
   hw/i2c/smbus.c
   hw/i2c/smbus_eeprom.c
   hw/ide/core.c
   hw/ide/atapi.c
   hw/ide/qdev.c
   hw/ide/pci.c
   hw/ide/ahci.c
   hw/ide/ich.c
   hw/input/hid.c
   hw/input/ps2.c
   hw/input/virtio-input.c
   hw/input/virtio-input-hid.c
   hw/intc/intc.c
   hw/ipack/ipack.c
   hw/ipack/tpci200.c
   hw/isa/isa-bus.c
   hw/misc/pci-testdev.c
   hw/misc/unimp.c
   hw/net/ne2000.c
   hw/net/eepro100.c
   hw/net/pcnet-pci.c
   hw/net/pcnet.c
   hw/net/net_tx_pkt.c
   hw/net/net_rx_pkt.c
   hw/net/rtl8139.c
   hw/net/vmxnet3.c
   hw/net/rocker/rocker.c
   hw/net/rocker/rocker_fp.c
   hw/net/rocker/rocker_desc.c
   hw/net/rocker/rocker_world.c
   hw/net/rocker/rocker_of_dpa.c
   hw/nvram/eeprom93xx.c
   hw/nvram/fw_cfg.c
   hw/nvram/chrp_nvram.c
   hw/pci-bridge/pci_bridge_dev.c
   hw/pci-host/pam.c
   hw/pci/pci.c
   hw/pci/pci_bridge.c
   hw/pci/msix.c
   hw/pci/msi.c
   hw/pci/shpc.c
   hw/pci/slotid_cap.c
   hw/pci/pci_host.c
   hw/pci/pcie_host.c
   hw/pci/pcie.c
   hw/pci/pcie_aer.c
   hw/pci/pcie_port.c
   hw/pcmcia/pcmcia.c
   hw/scsi/scsi-disk.c
   hw/scsi/scsi-generic.c
   hw/scsi/scsi-bus.c
   hw/scsi/lsi53c895a.c
   hw/scsi/mptsas.c
   hw/scsi/mptconfig.c
   hw/scsi/mptendian.c
   hw/scsi/megasas.c
   hw/scsi/vmw_pvscsi.c
   hw/scsi/esp.c
   hw/scsi/esp-pci.c
   hw/sd/sd.c
   hw/sd/core.c
   hw/sd/sdhci.c
   hw/usb/core.c
   hw/usb/combined-packet.c
   hw/usb/bus.c
   hw/usb/libhw.c
   hw/usb/desc.c
   hw/usb/desc-msos.c
   hw/usb/hcd-uhci.c
   hw/usb/hcd-ohci.c
   hw/usb/hcd-ehci.c
   hw/usb/hcd-ehci-pci.c
   hw/usb/hcd-xhci.c
   hw/usb/dev-hub.c
   hw/usb/dev-hid.c
   hw/usb/dev-wacom.c
   hw/usb/dev-storage.c
   hw/usb/dev-uas.c
   hw/usb/dev-audio.c
   hw/usb/dev-serial.c
   hw/usb/dev-network.c
   hw/usb/dev-bluetooth.c
   hw/usb/dev-smartcard-reader.c
   hw/usb/dev-mtp.c
   hw/usb/host-libusb.c
   hw/virtio/virtio-rng.c
   hw/virtio/virtio-pci.c
   hw/virtio/virtio-bus.c
   hw/virtio/virtio-mmio.c
   hw/virtio/vhost-stub.c
   hw/watchdog/watchdog.c
   hw/watchdog/wdt_i6300esb.c
   migration/migration.c
   migration/socket.c
   migration/fd.c
   migration/exec.c
   migration/tls.c
   migration/colo-comm.c
   migration/colo.c
   migration/colo-failover.c
   migration/vmstate.c
   migration/qemu-file.c
   migration/qemu-file-channel.c
   migration/xbzrle.c
   migration/postcopy-ram.c
   migration/qjson.c
   migration/block.c
   net/net.c
   net/queue.c
   net/checksum.c
   net/util.c
   net/hub.c
   net/socket.c
   net/dump.c
   net/eth.c
   net/tap.c
   net/vhost-user.c
   net/tap-bsd.c
   net/slirp.c
   net/filter.c
   net/filter-buffer.c
   net/filter-mirror.c
   net/colo-compare.c
   net/colo.c
   net/filter-rewriter.c
   net/filter-replay.c
   qom/cpu.c
   replay/replay.c
   replay/replay-internal.c
   replay/replay-events.c
   replay/replay-time.c
   replay/replay-input.c
   replay/replay-char.c
   replay/replay-snapshot.c
   replay/replay-net.c
   replay/replay-audio.c
   slirp/cksum.c
   slirp/if.c
   slirp/ip_icmp.c
   slirp/ip6_icmp.c
   slirp/ip6_input.c
   slirp/ip6_output.c
   slirp/ip_input.c
   slirp/ip_output.c
   slirp/dnssearch.c
   slirp/dhcpv6.c
   slirp/ip_icmp_ping.c
   slirp/slirp.c
   slirp/mbuf.c
   slirp/misc.c
   slirp/sbuf.c
   slirp/socket.c
   slirp/tcp_input.c
   slirp/tcp_output.c
   slirp/tcp_subr.c
   slirp/tcp_timer.c
   slirp/udp.c
   slirp/udp6.c
   slirp/bootp.c
   slirp/tftp.c
   slirp/arp_table.c
   slirp/ndp_table.c
   ui/keymaps.c
   ui/console.c
   ui/cursor.c
   ui/qemu-pixman.c
   ui/input.c
   ui/input-keymap.c
   ui/input-legacy.c
   ui/x_keymap.c
   ui/vnc.c
   ui/vnc-enc-zlib.c
   ui/vnc-enc-hextile.c
   ui/vnc-enc-tight.c
   ui/vnc-palette.c
   ui/vnc-enc-zrle.c
   ui/vnc-auth-vencrypt.c
   ui/vnc-ws.c
   ui/vnc-jobs.c
   qom/object.c
   qom/container.c
   qom/qom-qobject.c
   qom/object_interfaces.c
   block.c
   blockjob.c
   qemu-io-cmds.c
   replication.c
   block/raw-format.c
   block/qcow.c
   block/vdi.c
   block/vmdk.c
   block/cloop.c
   block/bochs.c
   block/vpc.c
   block/vvfat.c
   block/dmg.c
   block/cat.c
   block/qcow2.c
   block/qcow2-refcount.c
   block/qcow2-cluster.c
   block/qcow2-snapshot.c
   block/qcow2-cache.c
   block/qed.c
   block/qed-l2-cache.c
   block/qed-table.c
   block/qed-cluster.c
   block/qed-check.c
   block/vhdx.c
   block/vhdx-endian.c
   block/vhdx-log.c
   block/quorum.c
   block/parallels.c
   block/blkdebug.c
   block/blkverify.c
   block/blkreplay.c
   block/block-backend.c
   block/snapshot.c
   block/qapi.c
   block/file-posix.c
   block/null.c
   block/mirror.c
   block/commit.c
   block/io.c
   block/throttle-groups.c
   block/nbd.c
   block/nbd-client.c
   block/sheepdog.c
   block/accounting.c
   block/dirty-bitmap.c
   block/write-threshold.c
   block/backup.c
   block/replication.c
   block/crypto.c
   nbd/server.c
   nbd/client.c
   nbd/common.c
   chardev/char.c
   chardev/char-fd.c
   chardev/char-file.c
   chardev/char-io.c
   chardev/char-mux.c
   chardev/char-null.c
   chardev/char-parallel.c
   chardev/char-pipe.c
   chardev/char-pty.c
   chardev/char-ringbuf.c
   chardev/char-serial.c
   chardev/char-socket.c
   chardev/char-stdio.c
   chardev/char-udp.c
   crypto/init.c
   crypto/hash.c
   crypto/hash-glib.c
   crypto/hmac.c
   crypto/hmac-glib.c
   crypto/aes.c
   crypto/desrfb.c
   crypto/cipher.c
   crypto/tlscreds.c
   crypto/tlscredsanon.c
   crypto/tlscredsx509.c
   crypto/tlssession.c
   crypto/secret.c
   crypto/random-platform.c
   crypto/pbkdf.c
   crypto/ivgen.c
   crypto/ivgen-essiv.c
   crypto/ivgen-plain.c
   crypto/ivgen-plain64.c
   crypto/afsplit.c
   crypto/xts.c
   crypto/block.c
   crypto/block-qcow.c
   crypto/block-luks.c
   io/channel.c
   io/channel-buffer.c
   io/channel-command.c
   io/channel-file.c
   io/channel-socket.c
   io/channel-tls.c
   io/channel-watch.c
   io/channel-websock.c
   io/channel-util.c
   io/dns-resolver.c
   io/task.c)
set(qemu2-shared_generated_sources "" 
${ANDROID_AUTOGEN}/trace-root.c
  ${ANDROID_AUTOGEN}/util/trace.c
  ${ANDROID_AUTOGEN}/crypto/trace.c
  ${ANDROID_AUTOGEN}/io/trace.c
  ${ANDROID_AUTOGEN}/migration/trace.c
  ${ANDROID_AUTOGEN}/block/trace.c
  ${ANDROID_AUTOGEN}/hw/block/trace.c
  ${ANDROID_AUTOGEN}/hw/block/dataplane/trace.c
  ${ANDROID_AUTOGEN}/hw/char/trace.c
  ${ANDROID_AUTOGEN}/hw/intc/trace.c
  ${ANDROID_AUTOGEN}/hw/net/trace.c
  ${ANDROID_AUTOGEN}/hw/virtio/trace.c
  ${ANDROID_AUTOGEN}/hw/audio/trace.c
  ${ANDROID_AUTOGEN}/hw/misc/trace.c
  ${ANDROID_AUTOGEN}/hw/usb/trace.c
  ${ANDROID_AUTOGEN}/hw/scsi/trace.c
  ${ANDROID_AUTOGEN}/hw/nvram/trace.c
  ${ANDROID_AUTOGEN}/hw/display/trace.c
  ${ANDROID_AUTOGEN}/hw/input/trace.c
  ${ANDROID_AUTOGEN}/hw/timer/trace.c
  ${ANDROID_AUTOGEN}/hw/dma/trace.c
  ${ANDROID_AUTOGEN}/hw/sparc/trace.c
  ${ANDROID_AUTOGEN}/hw/sd/trace.c
  ${ANDROID_AUTOGEN}/hw/isa/trace.c
  ${ANDROID_AUTOGEN}/hw/mem/trace.c
  ${ANDROID_AUTOGEN}/hw/i386/trace.c
  ${ANDROID_AUTOGEN}/hw/i386/xen/trace.c
  ${ANDROID_AUTOGEN}/hw/9pfs/trace.c
  ${ANDROID_AUTOGEN}/hw/ppc/trace.c
  ${ANDROID_AUTOGEN}/hw/pci/trace.c
  ${ANDROID_AUTOGEN}/hw/s390x/trace.c
  ${ANDROID_AUTOGEN}/hw/vfio/trace.c
  ${ANDROID_AUTOGEN}/hw/acpi/trace.c
  ${ANDROID_AUTOGEN}/hw/arm/trace.c
  ${ANDROID_AUTOGEN}/hw/alpha/trace.c
  ${ANDROID_AUTOGEN}/hw/xen/trace.c
  ${ANDROID_AUTOGEN}/ui/trace.c
  ${ANDROID_AUTOGEN}/audio/trace.c
  ${ANDROID_AUTOGEN}/net/trace.c
  ${ANDROID_AUTOGEN}/target/arm/trace.c
  ${ANDROID_AUTOGEN}/target/i386/trace.c
  ${ANDROID_AUTOGEN}/target/mips/trace.c
  ${ANDROID_AUTOGEN}/target/sparc/trace.c
  ${ANDROID_AUTOGEN}/target/s390x/trace.c
  ${ANDROID_AUTOGEN}/target/ppc/trace.c
  ${ANDROID_AUTOGEN}/qom/trace.c
  ${ANDROID_AUTOGEN}/linux-user/trace.c
  ${ANDROID_AUTOGEN}/qapi/trace.c)

set(qemu2-shared_dep )

 ## TARGET: qemu-io ##

set(qemu-io_sources ""
qemu-io.c)
set(qemu-io_generated_sources "" 
${ANDROID_AUTOGEN}/)

set(qemu-io_dep libqemuutil qemu2-shared libqemustub)

 ## TARGET: qemu-img ##

set(qemu-img_sources ""
qemu-img.c)
set(qemu-img_generated_sources "" 
${ANDROID_AUTOGEN}/)

set(qemu-img_dep libqemuutil qemu2-shared libqemustub)

 ## TARGET: qemu-system-aarch64 ##

set(qemu-system-aarch64_sources ""
exec.c
   tcg/tcg.c
   tcg/tcg-op.c
   tcg/optimize.c
   tcg/tcg-common.c
   fpu/softfloat.c
   disas.c
   whpx-stub.c
   arch_init.c
   cpus.c
   monitor.c
   gdbstub.c
   balloon.c
   ioport.c
   numa.c
   qtest.c
   bootdevice.c
   memory.c
   memory_mapping.c
   dump.c
   migration/ram.c
   migration/savevm.c
   hw/adc/stm32f2xx_adc.c
   hw/block/virtio-blk.c
   hw/block/dataplane/virtio-blk.c
   hw/char/exynos4210_uart.c
   hw/char/omap_uart.c
   hw/char/digic-uart.c
   hw/char/stm32f2xx_usart.c
   hw/char/bcm2835_aux.c
   hw/char/goldfish_tty.c
   hw/char/virtio-serial-bus.c
   hw/core/nmi.c
   hw/core/generic-loader.c
   hw/core/null-machine.c
   hw/cpu/arm11mpcore.c
   hw/cpu/realview_mpcore.c
   hw/cpu/a9mpcore.c
   hw/cpu/a15mpcore.c
   hw/cpu/core.c
   hw/display/omap_dss.c
   hw/display/omap_lcdc.c
   hw/display/pxa2xx_lcd.c
   hw/display/bcm2835_fb.c
   hw/display/vga.c
   hw/display/virtio-gpu.c
   hw/display/virtio-gpu-3d.c
   hw/display/virtio-gpu-pci.c
   hw/display/dpcd.c
   hw/display/xlnx_dp.c
   hw/dma/xlnx_dpdma.c
   hw/dma/omap_dma.c
   hw/dma/soc_dma.c
   hw/dma/pxa2xx_dma.c
   hw/dma/bcm2835_dma.c
   hw/gpio/omap_gpio.c
   hw/gpio/imx_gpio.c
   hw/gpio/bcm2835_gpio.c
   hw/i2c/omap_i2c.c
   hw/input/pxa2xx_keypad.c
   hw/input/tsc210x.c
   hw/input/goldfish_events.c
   hw/input/goldfish_events_common.c
   hw/input/goldfish_rotary.c
   hw/intc/armv7m_nvic.c
   hw/intc/exynos4210_gic.c
   hw/intc/exynos4210_combiner.c
   hw/intc/omap_intc.c
   hw/intc/bcm2835_ic.c
   hw/intc/bcm2836_control.c
   hw/intc/allwinner-a10-pic.c
   hw/intc/aspeed_vic.c
   hw/intc/arm_gicv3_cpuif.c
   hw/intc/goldfish_pic.c
   hw/misc/arm_sysctl.c
   hw/misc/cbus.c
   hw/misc/exynos4210_pmu.c
   hw/misc/exynos4210_clk.c
   hw/misc/imx_ccm.c
   hw/misc/imx31_ccm.c
   hw/misc/imx25_ccm.c
   hw/misc/imx6_ccm.c
   hw/misc/imx6_src.c
   hw/misc/mst_fpga.c
   hw/misc/omap_clk.c
   hw/misc/omap_gpmc.c
   hw/misc/omap_l4.c
   hw/misc/omap_sdrc.c
   hw/misc/omap_tap.c
   hw/misc/bcm2835_mbox.c
   hw/misc/bcm2835_property.c
   hw/misc/bcm2835_rng.c
   hw/misc/zynq_slcr.c
   hw/misc/zynq-xadc.c
   hw/misc/stm32f2xx_syscfg.c
   hw/misc/edu.c
   hw/misc/auxbus.c
   hw/misc/aspeed_scu.c
   hw/misc/aspeed_sdmc.c
   hw/misc/goldfish_battery.c
   hw/misc/goldfish_pipe.c
   hw/misc/goldfish_sync.c
   hw/misc/goldfish_pstore.c
   hw/net/virtio-net.c
   hw/net/vhost_net.c
   hw/pcmcia/pxa2xx.c
   hw/scsi/virtio-scsi.c
   hw/scsi/virtio-scsi-dataplane.c
   hw/sd/omap_mmc.c
   hw/sd/pxa2xx_mmci.c
   hw/sd/bcm2835_sdhost.c
   hw/ssi/omap_spi.c
   hw/ssi/imx_spi.c
   hw/timer/exynos4210_mct.c
   hw/timer/exynos4210_pwm.c
   hw/timer/exynos4210_rtc.c
   hw/timer/omap_gptimer.c
   hw/timer/omap_synctimer.c
   hw/timer/pxa2xx_timer.c
   hw/timer/digic-timer.c
   hw/timer/allwinner-a10-pit.c
   hw/timer/goldfish_timer.c
   hw/usb/tusb6010.c
   hw/virtio/virtio.c
   hw/virtio/virtio-balloon.c
   hw/virtio/virtio-crypto.c
   hw/virtio/virtio-crypto-pci.c
   hw/arm/boot.c
   hw/arm/collie.c
   hw/arm/exynos4_boards.c
   hw/arm/gumstix.c
   hw/arm/highbank.c
   hw/arm/digic_boards.c
   hw/arm/integratorcp.c
   hw/arm/mainstone.c
   hw/arm/musicpal.c
   hw/arm/nseries.c
   hw/arm/omap_sx1.c
   hw/arm/palm.c
   hw/arm/realview.c
   hw/arm/spitz.c
   hw/arm/stellaris.c
   hw/arm/tosa.c
   hw/arm/versatilepb.c
   hw/arm/vexpress.c
   hw/arm/virt.c
   hw/arm/xilinx_zynq.c
   hw/arm/z2.c
   hw/arm/virt-acpi-build.c
   hw/arm/netduino2.c
   hw/arm/sysbus-fdt.c
   hw/arm/ranchu.c
   hw/arm/armv7m.c
   hw/arm/exynos4210.c
   hw/arm/pxa2xx.c
   hw/arm/pxa2xx_gpio.c
   hw/arm/pxa2xx_pic.c
   hw/arm/digic.c
   hw/arm/omap1.c
   hw/arm/omap2.c
   hw/arm/strongarm.c
   hw/arm/allwinner-a10.c
   hw/arm/cubieboard.c
   hw/arm/bcm2835_peripherals.c
   hw/arm/bcm2836.c
   hw/arm/raspi.c
   hw/arm/stm32f205_soc.c
   hw/arm/xlnx-zynqmp.c
   hw/arm/fsl-imx25.c
   hw/arm/imx25_pdk.c
   hw/arm/fsl-imx31.c
   hw/arm/kzm.c
   hw/arm/fsl-imx6.c
   hw/arm/sabrelite.c
   hw/arm/aspeed_soc.c
   hw/arm/aspeed.c
   target/arm/arm-semi.c
   target/arm/machine.c
   target/arm/psci.c
   target/arm/arch_dump.c
   target/arm/monitor.c
   target/arm/kvm-stub.c
   target/arm/translate.c
   target/arm/op_helper.c
   target/arm/helper.c
   target/arm/cpu.c
   target/arm/neon_helper.c
   target/arm/iwmmxt_helper.c
   target/arm/gdbstub.c
   target/arm/cpu64.c
   target/arm/translate-a64.c
   target/arm/helper-a64.c
   target/arm/gdbstub64.c
   target/arm/crypto_helper.c
   target/arm/arm-powerctl.c
   disas/arm.c
   disas/arm-a64.cc
   disas/libvixl/vixl/utils.cc
   disas/libvixl/vixl/compiler-intrinsics.cc
   disas/libvixl/vixl/a64/instructions-a64.cc
   disas/libvixl/vixl/a64/decoder-a64.cc
   disas/libvixl/vixl/a64/disasm-a64.cc
   hw/acpi/acpi-stub.c
   hw/acpi/ipmi-stub.c
   hw/audio/wm8750.c
   hw/audio/pl041.c
   hw/audio/lm4549.c
   hw/audio/marvell_88w8618.c
   hw/block/m25p80.c
   hw/block/nand.c
   hw/block/pflash_cfi02.c
   hw/block/ecc.c
   hw/block/onenand.c
   hw/char/pl011.c
   hw/char/cadence_uart.c
   hw/char/imx_serial.c
   hw/core/ptimer.c
   hw/core/platform-bus.c
   hw/display/ads7846.c
   hw/display/pl110.c
   hw/display/ssd0303.c
   hw/display/ssd0323.c
   hw/display/blizzard.c
   hw/display/exynos4210_fimd.c
   hw/display/tc6393xb.c
   hw/dma/pl080.c
   hw/dma/pl330.c
   hw/dma/xlnx-zynq-devcfg.c
   hw/gpio/max7310.c
   hw/gpio/pl061.c
   hw/gpio/zaurus.c
   hw/gpio/gpio_key.c
   hw/i2c/i2c-ddc.c
   hw/i2c/versatile_i2c.c
   hw/i2c/bitbang_i2c.c
   hw/i2c/exynos4210_i2c.c
   hw/i2c/imx_i2c.c
   hw/i2c/aspeed_i2c.c
   hw/ide/microdrive.c
   hw/input/lm832x.c
   hw/input/pl050.c
   hw/input/stellaris_input.c
   hw/input/tsc2005.c
   hw/intc/pl190.c
   hw/intc/imx_avic.c
   hw/intc/realview_gic.c
   hw/intc/arm_gic_common.c
   hw/intc/arm_gic.c
   hw/intc/arm_gicv2m.c
   hw/intc/arm_gicv3_common.c
   hw/intc/arm_gicv3.c
   hw/intc/arm_gicv3_dist.c
   hw/intc/arm_gicv3_redist.c
   hw/intc/arm_gicv3_its_common.c
   hw/misc/max111x.c
   hw/misc/tmp105.c
   hw/misc/arm_l2x0.c
   hw/misc/arm_integrator_debug.c
   hw/misc/a9scu.c
   hw/misc/arm11scu.c
   hw/net/smc91c111.c
   hw/net/lan9118.c
   hw/net/xgmac.c
   hw/net/allwinner_emac.c
   hw/net/imx_fec.c
   hw/net/cadence_gem.c
   hw/net/stellaris_enet.c
   hw/pci-bridge/pcie_root_port.c
   hw/pci-bridge/gen_pcie_root_port.c
   hw/pci-bridge/xio3130_upstream.c
   hw/pci-bridge/xio3130_downstream.c
   hw/pci-bridge/ioh3420.c
   hw/pci-bridge/i82801b11.c
   hw/pci-host/versatile.c
   hw/pci-host/gpex.c
   hw/sd/pl181.c
   hw/sd/ssi-sd.c
   hw/smbios/smbios.c
   hw/smbios/smbios_type_38-stub.c
   hw/ssi/pl022.c
   hw/ssi/ssi.c
   hw/ssi/xilinx_spips.c
   hw/ssi/aspeed_smc.c
   hw/ssi/stm32f2xx_spi.c
   hw/timer/arm_timer.c
   hw/timer/arm_mptimer.c
   hw/timer/armv7m_systick.c
   hw/timer/a9gtimer.c
   hw/timer/cadence_ttc.c
   hw/timer/ds1338.c
   hw/timer/pl031.c
   hw/timer/twl92230.c
   hw/timer/imx_epit.c
   hw/timer/imx_gpt.c
   hw/timer/stm32f2xx_timer.c
   hw/timer/aspeed_timer.c
   hw/usb/hcd-ehci-sysbus.c
   hw/usb/hcd-musb.c
   hw/watchdog/wdt_aspeed.c
   trace/control-target.c)
set(qemu-system-aarch64_generated_sources "" 
${ANDROID_AUTOGEN}/trace/generated-helpers.c)

set(qemu-system-aarch64_dep libqemuutil qemu2-shared libqemustub)

 ## TARGET: ivshmem-server ##

set(ivshmem-server_sources ""
contrib/ivshmem-server/ivshmem-server.c
   contrib/ivshmem-server/main.c)
set(ivshmem-server_generated_sources "" 
${ANDROID_AUTOGEN}/)

set(ivshmem-server_dep libqemuutil qemu2-shared libqemustub)

 ## TARGET: ivshmem-client ##

set(ivshmem-client_sources ""
contrib/ivshmem-client/ivshmem-client.c
   contrib/ivshmem-client/main.c)
set(ivshmem-client_generated_sources "" 
${ANDROID_AUTOGEN}/)

set(ivshmem-client_dep libqemuutil qemu2-shared libqemustub)

 ## TARGET: qemu-nbd ##

set(qemu-nbd_sources ""
qemu-nbd.c)
set(qemu-nbd_generated_sources "" 
${ANDROID_AUTOGEN}/)

set(qemu-nbd_dep libqemuutil qemu2-shared libqemustub)

 ## TARGET: qemu-system-x86_64 ##

set(qemu-system-x86_64_sources ""
exec.c
   tcg/tcg.c
   tcg/tcg-op.c
   tcg/optimize.c
   tcg/tcg-common.c
   fpu/softfloat.c
   disas.c
   whpx-stub.c
   arch_init.c
   cpus.c
   monitor.c
   gdbstub.c
   balloon.c
   ioport.c
   numa.c
   qtest.c
   bootdevice.c
   memory.c
   memory_mapping.c
   dump.c
   migration/ram.c
   migration/savevm.c
   hw/block/virtio-blk.c
   hw/block/dataplane/virtio-blk.c
   hw/char/goldfish_tty.c
   hw/char/virtio-serial-bus.c
   hw/core/nmi.c
   hw/core/generic-loader.c
   hw/core/null-machine.c
   hw/cpu/core.c
   hw/display/vga.c
   hw/display/virtio-gpu.c
   hw/display/virtio-gpu-3d.c
   hw/display/virtio-gpu-pci.c
   hw/display/virtio-vga.c
   hw/input/goldfish_events.c
   hw/input/goldfish_events_common.c
   hw/input/goldfish_rotary.c
   hw/intc/apic.c
   hw/intc/apic_common.c
   hw/intc/ioapic.c
   hw/intc/goldfish_pic.c
   hw/isa/lpc_ich9.c
   hw/misc/pvpanic.c
   hw/misc/edu.c
   hw/misc/goldfish_battery.c
   hw/misc/goldfish_pipe.c
   hw/misc/goldfish_sync.c
   hw/misc/goldfish_pstore.c
   hw/net/virtio-net.c
   hw/net/vhost_net.c
   hw/scsi/virtio-scsi.c
   hw/scsi/virtio-scsi-dataplane.c
   hw/timer/mc146818rtc.c
   hw/timer/goldfish_timer.c
   hw/virtio/virtio.c
   hw/virtio/virtio-balloon.c
   hw/virtio/virtio-crypto.c
   hw/virtio/virtio-crypto-pci.c
   hw/i386/multiboot.c
   hw/i386/pc.c
   hw/i386/pc_q35.c
   hw/i386/pc_sysfw.c
   hw/i386/x86-iommu.c
   hw/i386/intel_iommu.c
   hw/i386/amd_iommu.c
   hw/i386/kvmvapic.c
   target/i386/translate.c
   target/i386/helper.c
   target/i386/cpu.c
   target/i386/bpt_helper.c
   target/i386/excp_helper.c
   target/i386/fpu_helper.c
   target/i386/cc_helper.c
   target/i386/int_helper.c
   target/i386/svm_helper.c
   target/i386/smm_helper.c
   target/i386/misc_helper.c
   target/i386/mem_helper.c
   target/i386/seg_helper.c
   target/i386/mpx_helper.c
   target/i386/gdbstub.c
   target/i386/machine.c
   target/i386/arch_memory_mapping.c
   target/i386/arch_dump.c
   target/i386/monitor.c
   target/i386/kvm-stub.c
   target/i386/hvf-all.c
   target/i386/hvf-utils/x86.c
   target/i386/hvf-utils/x86_cpuid.c
   target/i386/hvf-utils/x86_decode.c
   target/i386/hvf-utils/x86_descr.c
   target/i386/hvf-utils/x86_emu.c
   target/i386/hvf-utils/x86_flags.c
   target/i386/hvf-utils/x86_mmu.c
   target/i386/hvf-utils/x86hvf.c
   target/i386/hax-all.c
   target/i386/hax-mem.c
   target/i386/hax-darwin.c
   hw/acpi/core.c
   hw/acpi/piix4.c
   hw/acpi/pcihp.c
   hw/acpi/ich9.c
   hw/acpi/tco.c
   hw/acpi/cpu_hotplug.c
   hw/acpi/memory_hotplug.c
   hw/acpi/cpu.c
   hw/acpi/nvdimm.c
   hw/acpi/vmgenid.c
   hw/acpi/ipmi.c
   hw/audio/sb16.c
   hw/audio/fmopl.c
   hw/audio/adlib.c
   hw/audio/gus.c
   hw/audio/gusemu_hal.c
   hw/audio/gusemu_mixer.c
   hw/audio/cs4231a.c
   hw/audio/pcspk.c
   hw/block/fdc.c
   hw/char/parallel.c
   hw/char/debugcon.c
   hw/display/cirrus_vga.c
   hw/display/vga-isa.c
   hw/display/vmware_vga.c
   hw/dma/i8257.c
   hw/i2c/smbus_ich9.c
   hw/i2c/pm_smbus.c
   hw/ide/isa.c
   hw/ide/piix.c
   hw/input/pckbd.c
   hw/intc/i8259_common.c
   hw/intc/i8259.c
   hw/intc/ioapic_common.c
   hw/ipmi/ipmi.c
   hw/ipmi/ipmi_bmc_sim.c
   hw/ipmi/ipmi_bmc_extern.c
   hw/ipmi/isa_ipmi_kcs.c
   hw/ipmi/isa_ipmi_bt.c
   hw/isa/apm.c
   hw/mem/pc-dimm.c
   hw/mem/nvdimm.c
   hw/misc/applesmc.c
   hw/misc/debugexit.c
   hw/misc/sga.c
   hw/misc/pc-testdev.c
   hw/net/ne2000-isa.c
   hw/pci-bridge/pcie_root_port.c
   hw/pci-bridge/gen_pcie_root_port.c
   hw/pci-bridge/pci_expander_bridge.c
   hw/pci-bridge/xio3130_upstream.c
   hw/pci-bridge/xio3130_downstream.c
   hw/pci-bridge/ioh3420.c
   hw/pci-bridge/i82801b11.c
   hw/pci-host/piix.c
   hw/pci-host/q35.c
   hw/smbios/smbios.c
   hw/smbios/smbios_type_38.c
   hw/timer/hpet.c
   hw/timer/i8254_common.c
   hw/timer/i8254.c
   hw/tpm/tpm_tis.c
   hw/watchdog/wdt_ib700.c
   trace/control-target.c)
set(qemu-system-x86_64_generated_sources "" 
${ANDROID_AUTOGEN}/trace/generated-helpers.c)

set(qemu-system-x86_64_dep libqemuutil qemu2-shared libqemustub)

 ## TARGET: qemu-system-mipsel ##

set(qemu-system-mipsel_sources ""
exec.c
   tcg/tcg.c
   tcg/tcg-op.c
   tcg/optimize.c
   tcg/tcg-common.c
   fpu/softfloat.c
   disas.c
   whpx-stub.c
   arch_init.c
   cpus.c
   monitor.c
   gdbstub.c
   balloon.c
   ioport.c
   numa.c
   qtest.c
   bootdevice.c
   memory.c
   memory_mapping.c
   dump.c
   migration/ram.c
   migration/savevm.c
   hw/block/virtio-blk.c
   hw/block/dataplane/virtio-blk.c
   hw/char/goldfish_tty.c
   hw/char/virtio-serial-bus.c
   hw/core/nmi.c
   hw/core/generic-loader.c
   hw/core/null-machine.c
   hw/cpu/core.c
   hw/display/vga.c
   hw/display/virtio-gpu.c
   hw/display/virtio-gpu-3d.c
   hw/display/virtio-gpu-pci.c
   hw/input/goldfish_events.c
   hw/input/goldfish_events_common.c
   hw/input/goldfish_rotary.c
   hw/intc/mips_gic.c
   hw/intc/goldfish_pic.c
   hw/misc/mips_cmgcr.c
   hw/misc/mips_cpc.c
   hw/misc/mips_itu.c
   hw/misc/edu.c
   hw/misc/goldfish_battery.c
   hw/misc/goldfish_pipe.c
   hw/misc/goldfish_sync.c
   hw/misc/goldfish_pstore.c
   hw/net/virtio-net.c
   hw/net/vhost_net.c
   hw/scsi/virtio-scsi.c
   hw/scsi/virtio-scsi-dataplane.c
   hw/timer/mips_gictimer.c
   hw/timer/mc146818rtc.c
   hw/timer/goldfish_timer.c
   hw/virtio/virtio.c
   hw/virtio/virtio-balloon.c
   hw/virtio/virtio-crypto.c
   hw/virtio/virtio-crypto-pci.c
   hw/mips/mips_r4k.c
   hw/mips/mips_malta.c
   hw/mips/mips_mipssim.c
   hw/mips/addr.c
   hw/mips/mips_int.c
   hw/mips/mips_ranchu.c
   hw/mips/gt64xxx_pci.c
   hw/mips/cps.c
   target/mips/translate.c
   target/mips/dsp_helper.c
   target/mips/op_helper.c
   target/mips/lmi_helper.c
   target/mips/helper.c
   target/mips/cpu.c
   target/mips/gdbstub.c
   target/mips/msa_helper.c
   target/mips/mips-semi.c
   target/mips/machine.c
   disas/mips.c
   hw/acpi/core.c
   hw/acpi/piix4.c
   hw/acpi/pcihp.c
   hw/acpi/cpu_hotplug.c
   hw/acpi/memory_hotplug.c
   hw/acpi/cpu.c
   hw/acpi/nvdimm.c
   hw/acpi/ipmi-stub.c
   hw/audio/sb16.c
   hw/audio/fmopl.c
   hw/audio/adlib.c
   hw/audio/gus.c
   hw/audio/gusemu_hal.c
   hw/audio/gusemu_mixer.c
   hw/audio/cs4231a.c
   hw/audio/pcspk.c
   hw/block/fdc.c
   hw/char/parallel.c
   hw/core/empty_slot.c
   hw/display/cirrus_vga.c
   hw/display/vga-isa.c
   hw/display/vga-isa-mm.c
   hw/display/vmware_vga.c
   hw/dma/i8257.c
   hw/i2c/smbus_ich9.c
   hw/i2c/pm_smbus.c
   hw/ide/isa.c
   hw/ide/piix.c
   hw/input/pckbd.c
   hw/intc/i8259_common.c
   hw/intc/i8259.c
   hw/isa/apm.c
   hw/isa/piix4.c
   hw/misc/pc-testdev.c
   hw/net/ne2000-isa.c
   hw/net/mipsnet.c
   hw/smbios/smbios-stub.c
   hw/timer/i8254_common.c
   hw/timer/i8254.c
   trace/control-target.c)
set(qemu-system-mipsel_generated_sources "" 
${ANDROID_AUTOGEN}/trace/generated-helpers.c)

set(qemu-system-mipsel_dep libqemuutil qemu2-shared libqemustub)

 ## TARGET: libqemustub ##

set(libqemustub_sources ""
crypto/pbkdf-stub.c
   stubs/arch-query-cpu-def.c
   stubs/arch-query-cpu-model-expansion.c
   stubs/arch-query-cpu-model-comparison.c
   stubs/arch-query-cpu-model-baseline.c
   stubs/bdrv-next-monitor-owned.c
   stubs/blk-commit-all.c
   stubs/blockdev-close-all-bdrv-states.c
   stubs/clock-warp.c
   stubs/cpu-get-clock.c
   stubs/cpu-get-icount.c
   stubs/dump.c
   stubs/error-printf.c
   stubs/fdset.c
   stubs/gdbstub.c
   stubs/get-vm-name.c
   stubs/iothread.c
   stubs/iothread-lock.c
   stubs/is-daemonized.c
   stubs/machine-init-done.c
   stubs/migr-blocker.c
   stubs/monitor.c
   stubs/notify-event.c
   stubs/qtest.c
   stubs/replay.c
   stubs/runstate-check.c
   stubs/set-fd-handler.c
   stubs/slirp.c
   stubs/sysbus.c
   stubs/trace-control.c
   stubs/uuid.c
   stubs/vm-stop.c
   stubs/vmstate.c
   stubs/target-monitor-defs.c
   stubs/target-get-monitor-def.c
   stubs/pc_madt_cpu_entry.c
   stubs/vmgenid.c
   stubs/migration.c)
set(libqemustub_generated_sources "" 
${ANDROID_AUTOGEN}/)

set(libqemustub_dep qemu2-shared)

 ## TARGET: libqemuutil ##

set(libqemuutil_sources ""
qapi/qapi-visit-core.c
   qapi/qapi-dealloc-visitor.c
   qapi/qobject-input-visitor.c
   qapi/qobject-output-visitor.c
   qapi/qmp-registry.c
   qapi/qmp-dispatch.c
   qapi/string-input-visitor.c
   qapi/string-output-visitor.c
   qapi/opts-visitor.c
   qapi/qapi-clone-visitor.c
   qapi/qmp-event.c
   qapi/qapi-util.c
   qobject/qnull.c
   qobject/qstring.c
   qobject/qdict.c
   qobject/qlist.c
   qobject/qbool.c
   qobject/qjson.c
   qobject/qobject.c
   qobject/json-lexer.c
   qobject/json-streamer.c
   qobject/json-parser.c
   trace/control.c
   trace/qmp.c
   util/osdep.c
   util/cutils.c
   util/unicode.c
   util/qemu-timer-common.c
   util/bufferiszero.c
   util/lockcnt.c
   util/aiocb.c
   util/async.c
   util/thread-pool.c
   util/qemu-timer.c
   util/main-loop.c
   util/iohandler.c
   util/aio-posix.c
   util/compatfd.c
   util/event_notifier-posix.c
   util/mmap-alloc.c
   util/oslib-posix.c
   util/qemu-openpty.c
   util/qemu-thread-posix.c
   util/memfd.c
   util/abort.c
   util/path.c
   util/module.c
   util/host-utils.c
   util/bitmap.c
   util/bitops.c
   util/hbitmap.c
   util/fifo8.c
   util/acl.c
   util/error.c
   util/qemu-error.c
   util/id.c
   util/iov.c
   util/qemu-config.c
   util/qemu-sockets.c
   util/uri.c
   util/notify.c
   util/qemu-option.c
   util/qemu-progress.c
   util/keyval.c
   util/hexdump.c
   util/crc32c.c
   util/uuid.c
   util/throttle.c
   util/getauxval.c
   util/readline.c
   util/rcu.c
   util/qemu-coroutine.c
   util/qemu-coroutine-lock.c
   util/qemu-coroutine-io.c
   util/qemu-coroutine-sleep.c
   util/coroutine-sigaltstack.c
   util/buffer.c
   util/timed-average.c
   util/base64.c
   util/log.c
   util/qdist.c
   util/qht.c
   util/range.c
   util/systemd.c)
set(libqemuutil_generated_sources "" 
${ANDROID_AUTOGEN}/)

set(libqemuutil_dep qemu2-shared)

 ## TARGET: qemu-system-i386 ##

set(qemu-system-i386_sources ""
exec.c
   tcg/tcg.c
   tcg/tcg-op.c
   tcg/optimize.c
   tcg/tcg-common.c
   fpu/softfloat.c
   disas.c
   whpx-stub.c
   arch_init.c
   cpus.c
   monitor.c
   gdbstub.c
   balloon.c
   ioport.c
   numa.c
   qtest.c
   bootdevice.c
   memory.c
   memory_mapping.c
   dump.c
   migration/ram.c
   migration/savevm.c
   hw/block/virtio-blk.c
   hw/block/dataplane/virtio-blk.c
   hw/char/goldfish_tty.c
   hw/char/virtio-serial-bus.c
   hw/core/nmi.c
   hw/core/generic-loader.c
   hw/core/null-machine.c
   hw/cpu/core.c
   hw/display/vga.c
   hw/display/virtio-gpu.c
   hw/display/virtio-gpu-3d.c
   hw/display/virtio-gpu-pci.c
   hw/display/virtio-vga.c
   hw/input/goldfish_events.c
   hw/input/goldfish_events_common.c
   hw/input/goldfish_rotary.c
   hw/intc/apic.c
   hw/intc/apic_common.c
   hw/intc/ioapic.c
   hw/intc/goldfish_pic.c
   hw/isa/lpc_ich9.c
   hw/misc/pvpanic.c
   hw/misc/edu.c
   hw/misc/goldfish_battery.c
   hw/misc/goldfish_pipe.c
   hw/misc/goldfish_sync.c
   hw/misc/goldfish_pstore.c
   hw/net/virtio-net.c
   hw/net/vhost_net.c
   hw/scsi/virtio-scsi.c
   hw/scsi/virtio-scsi-dataplane.c
   hw/timer/mc146818rtc.c
   hw/timer/goldfish_timer.c
   hw/virtio/virtio.c
   hw/virtio/virtio-balloon.c
   hw/virtio/virtio-crypto.c
   hw/virtio/virtio-crypto-pci.c
   hw/i386/multiboot.c
   hw/i386/pc.c
   hw/i386/pc_q35.c
   hw/i386/pc_sysfw.c
   hw/i386/x86-iommu.c
   hw/i386/intel_iommu.c
   hw/i386/amd_iommu.c
   hw/i386/kvmvapic.c
   target/i386/translate.c
   target/i386/helper.c
   target/i386/cpu.c
   target/i386/bpt_helper.c
   target/i386/excp_helper.c
   target/i386/fpu_helper.c
   target/i386/cc_helper.c
   target/i386/int_helper.c
   target/i386/svm_helper.c
   target/i386/smm_helper.c
   target/i386/misc_helper.c
   target/i386/mem_helper.c
   target/i386/seg_helper.c
   target/i386/mpx_helper.c
   target/i386/gdbstub.c
   target/i386/machine.c
   target/i386/arch_memory_mapping.c
   target/i386/arch_dump.c
   target/i386/monitor.c
   target/i386/kvm-stub.c
   target/i386/hvf-all.c
   target/i386/hvf-utils/x86.c
   target/i386/hvf-utils/x86_cpuid.c
   target/i386/hvf-utils/x86_decode.c
   target/i386/hvf-utils/x86_descr.c
   target/i386/hvf-utils/x86_emu.c
   target/i386/hvf-utils/x86_flags.c
   target/i386/hvf-utils/x86_mmu.c
   target/i386/hvf-utils/x86hvf.c
   target/i386/hax-all.c
   target/i386/hax-mem.c
   target/i386/hax-darwin.c
   hw/acpi/core.c
   hw/acpi/piix4.c
   hw/acpi/pcihp.c
   hw/acpi/ich9.c
   hw/acpi/tco.c
   hw/acpi/cpu_hotplug.c
   hw/acpi/memory_hotplug.c
   hw/acpi/cpu.c
   hw/acpi/nvdimm.c
   hw/acpi/vmgenid.c
   hw/acpi/ipmi.c
   hw/audio/sb16.c
   hw/audio/fmopl.c
   hw/audio/adlib.c
   hw/audio/gus.c
   hw/audio/gusemu_hal.c
   hw/audio/gusemu_mixer.c
   hw/audio/cs4231a.c
   hw/audio/pcspk.c
   hw/block/fdc.c
   hw/char/parallel.c
   hw/char/debugcon.c
   hw/display/cirrus_vga.c
   hw/display/vga-isa.c
   hw/display/vmware_vga.c
   hw/dma/i8257.c
   hw/i2c/smbus_ich9.c
   hw/i2c/pm_smbus.c
   hw/ide/isa.c
   hw/ide/piix.c
   hw/input/pckbd.c
   hw/intc/i8259_common.c
   hw/intc/i8259.c
   hw/intc/ioapic_common.c
   hw/ipmi/ipmi.c
   hw/ipmi/ipmi_bmc_sim.c
   hw/ipmi/ipmi_bmc_extern.c
   hw/ipmi/isa_ipmi_kcs.c
   hw/ipmi/isa_ipmi_bt.c
   hw/isa/apm.c
   hw/mem/pc-dimm.c
   hw/mem/nvdimm.c
   hw/misc/applesmc.c
   hw/misc/debugexit.c
   hw/misc/sga.c
   hw/misc/pc-testdev.c
   hw/net/ne2000-isa.c
   hw/pci-bridge/pcie_root_port.c
   hw/pci-bridge/gen_pcie_root_port.c
   hw/pci-bridge/pci_expander_bridge.c
   hw/pci-bridge/xio3130_upstream.c
   hw/pci-bridge/xio3130_downstream.c
   hw/pci-bridge/ioh3420.c
   hw/pci-bridge/i82801b11.c
   hw/pci-host/piix.c
   hw/pci-host/q35.c
   hw/smbios/smbios.c
   hw/smbios/smbios_type_38.c
   hw/timer/hpet.c
   hw/timer/i8254_common.c
   hw/timer/i8254.c
   hw/tpm/tpm_tis.c
   hw/watchdog/wdt_ib700.c
   trace/control-target.c)
set(qemu-system-i386_generated_sources "" 
${ANDROID_AUTOGEN}/trace/generated-helpers.c)

set(qemu-system-i386_dep libqemuutil qemu2-shared libqemustub)

 ## TARGET: qemu-system-mips64el ##

set(qemu-system-mips64el_sources ""
exec.c
   tcg/tcg.c
   tcg/tcg-op.c
   tcg/optimize.c
   tcg/tcg-common.c
   fpu/softfloat.c
   disas.c
   whpx-stub.c
   arch_init.c
   cpus.c
   monitor.c
   gdbstub.c
   balloon.c
   ioport.c
   numa.c
   qtest.c
   bootdevice.c
   memory.c
   memory_mapping.c
   dump.c
   migration/ram.c
   migration/savevm.c
   hw/block/virtio-blk.c
   hw/block/dataplane/virtio-blk.c
   hw/char/goldfish_tty.c
   hw/char/virtio-serial-bus.c
   hw/core/nmi.c
   hw/core/generic-loader.c
   hw/core/null-machine.c
   hw/cpu/core.c
   hw/display/vga.c
   hw/display/virtio-gpu.c
   hw/display/virtio-gpu-3d.c
   hw/display/virtio-gpu-pci.c
   hw/input/goldfish_events.c
   hw/input/goldfish_events_common.c
   hw/input/goldfish_rotary.c
   hw/intc/mips_gic.c
   hw/intc/goldfish_pic.c
   hw/misc/mips_cmgcr.c
   hw/misc/mips_cpc.c
   hw/misc/mips_itu.c
   hw/misc/edu.c
   hw/misc/goldfish_battery.c
   hw/misc/goldfish_pipe.c
   hw/misc/goldfish_sync.c
   hw/misc/goldfish_pstore.c
   hw/net/virtio-net.c
   hw/net/vhost_net.c
   hw/scsi/virtio-scsi.c
   hw/scsi/virtio-scsi-dataplane.c
   hw/timer/mips_gictimer.c
   hw/timer/mc146818rtc.c
   hw/timer/goldfish_timer.c
   hw/virtio/virtio.c
   hw/virtio/virtio-balloon.c
   hw/virtio/virtio-crypto.c
   hw/virtio/virtio-crypto-pci.c
   hw/mips/mips_r4k.c
   hw/mips/mips_malta.c
   hw/mips/mips_mipssim.c
   hw/mips/addr.c
   hw/mips/mips_int.c
   hw/mips/mips_ranchu.c
   hw/mips/mips_jazz.c
   hw/mips/mips_fulong2e.c
   hw/mips/gt64xxx_pci.c
   hw/mips/cps.c
   hw/mips/boston.c
   target/mips/translate.c
   target/mips/dsp_helper.c
   target/mips/op_helper.c
   target/mips/lmi_helper.c
   target/mips/helper.c
   target/mips/cpu.c
   target/mips/gdbstub.c
   target/mips/msa_helper.c
   target/mips/mips-semi.c
   target/mips/machine.c
   disas/mips.c
   hw/acpi/core.c
   hw/acpi/piix4.c
   hw/acpi/pcihp.c
   hw/acpi/cpu_hotplug.c
   hw/acpi/memory_hotplug.c
   hw/acpi/cpu.c
   hw/acpi/nvdimm.c
   hw/acpi/ipmi-stub.c
   hw/audio/sb16.c
   hw/audio/fmopl.c
   hw/audio/adlib.c
   hw/audio/gus.c
   hw/audio/gusemu_hal.c
   hw/audio/gusemu_mixer.c
   hw/audio/cs4231a.c
   hw/audio/pcspk.c
   hw/block/fdc.c
   hw/char/parallel.c
   hw/core/empty_slot.c
   hw/core/loader-fit.c
   hw/display/cirrus_vga.c
   hw/display/g364fb.c
   hw/display/jazz_led.c
   hw/display/vga-isa.c
   hw/display/vga-isa-mm.c
   hw/display/vmware_vga.c
   hw/dma/rc4030.c
   hw/dma/i8257.c
   hw/i2c/smbus_ich9.c
   hw/i2c/pm_smbus.c
   hw/ide/isa.c
   hw/ide/piix.c
   hw/ide/via.c
   hw/input/pckbd.c
   hw/intc/i8259_common.c
   hw/intc/i8259.c
   hw/isa/apm.c
   hw/isa/piix4.c
   hw/isa/vt82c686.c
   hw/misc/pc-testdev.c
   hw/net/dp8393x.c
   hw/net/ne2000-isa.c
   hw/net/mipsnet.c
   hw/nvram/ds1225y.c
   hw/pci-host/bonito.c
   hw/pci-host/xilinx-pcie.c
   hw/smbios/smbios-stub.c
   hw/timer/i8254_common.c
   hw/timer/i8254.c
   trace/control-target.c)
set(qemu-system-mips64el_generated_sources "" 
${ANDROID_AUTOGEN}/trace/generated-helpers.c)

set(qemu-system-mips64el_dep libqemuutil qemu2-shared libqemustub)

 ## TARGET: qemu-system-arm ##

set(qemu-system-arm_sources ""
exec.c
   tcg/tcg.c
   tcg/tcg-op.c
   tcg/optimize.c
   tcg/tcg-common.c
   fpu/softfloat.c
   disas.c
   whpx-stub.c
   arch_init.c
   cpus.c
   monitor.c
   gdbstub.c
   balloon.c
   ioport.c
   numa.c
   qtest.c
   bootdevice.c
   memory.c
   memory_mapping.c
   dump.c
   migration/ram.c
   migration/savevm.c
   hw/adc/stm32f2xx_adc.c
   hw/block/virtio-blk.c
   hw/block/dataplane/virtio-blk.c
   hw/char/exynos4210_uart.c
   hw/char/omap_uart.c
   hw/char/digic-uart.c
   hw/char/stm32f2xx_usart.c
   hw/char/bcm2835_aux.c
   hw/char/goldfish_tty.c
   hw/char/virtio-serial-bus.c
   hw/core/nmi.c
   hw/core/generic-loader.c
   hw/core/null-machine.c
   hw/cpu/arm11mpcore.c
   hw/cpu/realview_mpcore.c
   hw/cpu/a9mpcore.c
   hw/cpu/a15mpcore.c
   hw/cpu/core.c
   hw/display/omap_dss.c
   hw/display/omap_lcdc.c
   hw/display/pxa2xx_lcd.c
   hw/display/bcm2835_fb.c
   hw/display/vga.c
   hw/display/virtio-gpu.c
   hw/display/virtio-gpu-3d.c
   hw/display/virtio-gpu-pci.c
   hw/dma/omap_dma.c
   hw/dma/soc_dma.c
   hw/dma/pxa2xx_dma.c
   hw/dma/bcm2835_dma.c
   hw/gpio/omap_gpio.c
   hw/gpio/imx_gpio.c
   hw/gpio/bcm2835_gpio.c
   hw/i2c/omap_i2c.c
   hw/input/pxa2xx_keypad.c
   hw/input/tsc210x.c
   hw/input/goldfish_events.c
   hw/input/goldfish_events_common.c
   hw/input/goldfish_rotary.c
   hw/intc/armv7m_nvic.c
   hw/intc/exynos4210_gic.c
   hw/intc/exynos4210_combiner.c
   hw/intc/omap_intc.c
   hw/intc/bcm2835_ic.c
   hw/intc/bcm2836_control.c
   hw/intc/allwinner-a10-pic.c
   hw/intc/aspeed_vic.c
   hw/intc/arm_gicv3_cpuif.c
   hw/intc/goldfish_pic.c
   hw/misc/arm_sysctl.c
   hw/misc/cbus.c
   hw/misc/exynos4210_pmu.c
   hw/misc/exynos4210_clk.c
   hw/misc/imx_ccm.c
   hw/misc/imx31_ccm.c
   hw/misc/imx25_ccm.c
   hw/misc/imx6_ccm.c
   hw/misc/imx6_src.c
   hw/misc/mst_fpga.c
   hw/misc/omap_clk.c
   hw/misc/omap_gpmc.c
   hw/misc/omap_l4.c
   hw/misc/omap_sdrc.c
   hw/misc/omap_tap.c
   hw/misc/bcm2835_mbox.c
   hw/misc/bcm2835_property.c
   hw/misc/bcm2835_rng.c
   hw/misc/zynq_slcr.c
   hw/misc/zynq-xadc.c
   hw/misc/stm32f2xx_syscfg.c
   hw/misc/edu.c
   hw/misc/aspeed_scu.c
   hw/misc/aspeed_sdmc.c
   hw/misc/goldfish_battery.c
   hw/misc/goldfish_pipe.c
   hw/misc/goldfish_sync.c
   hw/misc/goldfish_pstore.c
   hw/net/virtio-net.c
   hw/net/vhost_net.c
   hw/pcmcia/pxa2xx.c
   hw/scsi/virtio-scsi.c
   hw/scsi/virtio-scsi-dataplane.c
   hw/sd/omap_mmc.c
   hw/sd/pxa2xx_mmci.c
   hw/sd/bcm2835_sdhost.c
   hw/ssi/omap_spi.c
   hw/ssi/imx_spi.c
   hw/timer/exynos4210_mct.c
   hw/timer/exynos4210_pwm.c
   hw/timer/exynos4210_rtc.c
   hw/timer/omap_gptimer.c
   hw/timer/omap_synctimer.c
   hw/timer/pxa2xx_timer.c
   hw/timer/digic-timer.c
   hw/timer/allwinner-a10-pit.c
   hw/timer/goldfish_timer.c
   hw/usb/tusb6010.c
   hw/virtio/virtio.c
   hw/virtio/virtio-balloon.c
   hw/virtio/virtio-crypto.c
   hw/virtio/virtio-crypto-pci.c
   hw/arm/boot.c
   hw/arm/collie.c
   hw/arm/exynos4_boards.c
   hw/arm/gumstix.c
   hw/arm/highbank.c
   hw/arm/digic_boards.c
   hw/arm/integratorcp.c
   hw/arm/mainstone.c
   hw/arm/musicpal.c
   hw/arm/nseries.c
   hw/arm/omap_sx1.c
   hw/arm/palm.c
   hw/arm/realview.c
   hw/arm/spitz.c
   hw/arm/stellaris.c
   hw/arm/tosa.c
   hw/arm/versatilepb.c
   hw/arm/vexpress.c
   hw/arm/virt.c
   hw/arm/xilinx_zynq.c
   hw/arm/z2.c
   hw/arm/virt-acpi-build.c
   hw/arm/netduino2.c
   hw/arm/sysbus-fdt.c
   hw/arm/ranchu.c
   hw/arm/armv7m.c
   hw/arm/exynos4210.c
   hw/arm/pxa2xx.c
   hw/arm/pxa2xx_gpio.c
   hw/arm/pxa2xx_pic.c
   hw/arm/digic.c
   hw/arm/omap1.c
   hw/arm/omap2.c
   hw/arm/strongarm.c
   hw/arm/allwinner-a10.c
   hw/arm/cubieboard.c
   hw/arm/bcm2835_peripherals.c
   hw/arm/bcm2836.c
   hw/arm/raspi.c
   hw/arm/stm32f205_soc.c
   hw/arm/fsl-imx25.c
   hw/arm/imx25_pdk.c
   hw/arm/fsl-imx31.c
   hw/arm/kzm.c
   hw/arm/fsl-imx6.c
   hw/arm/sabrelite.c
   hw/arm/aspeed_soc.c
   hw/arm/aspeed.c
   target/arm/arm-semi.c
   target/arm/machine.c
   target/arm/psci.c
   target/arm/arch_dump.c
   target/arm/monitor.c
   target/arm/kvm-stub.c
   target/arm/translate.c
   target/arm/op_helper.c
   target/arm/helper.c
   target/arm/cpu.c
   target/arm/neon_helper.c
   target/arm/iwmmxt_helper.c
   target/arm/gdbstub.c
   target/arm/crypto_helper.c
   target/arm/arm-powerctl.c
   disas/arm.c
   disas/arm-a64.cc
   disas/libvixl/vixl/utils.cc
   disas/libvixl/vixl/compiler-intrinsics.cc
   disas/libvixl/vixl/a64/instructions-a64.cc
   disas/libvixl/vixl/a64/decoder-a64.cc
   disas/libvixl/vixl/a64/disasm-a64.cc
   hw/acpi/acpi-stub.c
   hw/acpi/ipmi-stub.c
   hw/audio/wm8750.c
   hw/audio/pl041.c
   hw/audio/lm4549.c
   hw/audio/marvell_88w8618.c
   hw/block/m25p80.c
   hw/block/nand.c
   hw/block/pflash_cfi02.c
   hw/block/ecc.c
   hw/block/onenand.c
   hw/char/pl011.c
   hw/char/cadence_uart.c
   hw/char/imx_serial.c
   hw/core/ptimer.c
   hw/core/platform-bus.c
   hw/display/ads7846.c
   hw/display/pl110.c
   hw/display/ssd0303.c
   hw/display/ssd0323.c
   hw/display/blizzard.c
   hw/display/exynos4210_fimd.c
   hw/display/tc6393xb.c
   hw/dma/pl080.c
   hw/dma/pl330.c
   hw/dma/xlnx-zynq-devcfg.c
   hw/gpio/max7310.c
   hw/gpio/pl061.c
   hw/gpio/zaurus.c
   hw/gpio/gpio_key.c
   hw/i2c/versatile_i2c.c
   hw/i2c/bitbang_i2c.c
   hw/i2c/exynos4210_i2c.c
   hw/i2c/imx_i2c.c
   hw/i2c/aspeed_i2c.c
   hw/ide/microdrive.c
   hw/input/lm832x.c
   hw/input/pl050.c
   hw/input/stellaris_input.c
   hw/input/tsc2005.c
   hw/intc/pl190.c
   hw/intc/imx_avic.c
   hw/intc/realview_gic.c
   hw/intc/arm_gic_common.c
   hw/intc/arm_gic.c
   hw/intc/arm_gicv2m.c
   hw/intc/arm_gicv3_common.c
   hw/intc/arm_gicv3.c
   hw/intc/arm_gicv3_dist.c
   hw/intc/arm_gicv3_redist.c
   hw/intc/arm_gicv3_its_common.c
   hw/misc/max111x.c
   hw/misc/tmp105.c
   hw/misc/arm_l2x0.c
   hw/misc/arm_integrator_debug.c
   hw/misc/a9scu.c
   hw/misc/arm11scu.c
   hw/net/smc91c111.c
   hw/net/lan9118.c
   hw/net/xgmac.c
   hw/net/allwinner_emac.c
   hw/net/imx_fec.c
   hw/net/cadence_gem.c
   hw/net/stellaris_enet.c
   hw/pci-bridge/pcie_root_port.c
   hw/pci-bridge/gen_pcie_root_port.c
   hw/pci-bridge/xio3130_upstream.c
   hw/pci-bridge/xio3130_downstream.c
   hw/pci-bridge/ioh3420.c
   hw/pci-bridge/i82801b11.c
   hw/pci-host/versatile.c
   hw/pci-host/gpex.c
   hw/sd/pl181.c
   hw/sd/ssi-sd.c
   hw/smbios/smbios.c
   hw/smbios/smbios_type_38-stub.c
   hw/ssi/pl022.c
   hw/ssi/ssi.c
   hw/ssi/xilinx_spips.c
   hw/ssi/aspeed_smc.c
   hw/ssi/stm32f2xx_spi.c
   hw/timer/arm_timer.c
   hw/timer/arm_mptimer.c
   hw/timer/armv7m_systick.c
   hw/timer/a9gtimer.c
   hw/timer/cadence_ttc.c
   hw/timer/ds1338.c
   hw/timer/pl031.c
   hw/timer/twl92230.c
   hw/timer/imx_epit.c
   hw/timer/imx_gpt.c
   hw/timer/stm32f2xx_timer.c
   hw/timer/aspeed_timer.c
   hw/usb/hcd-ehci-sysbus.c
   hw/usb/hcd-musb.c
   hw/watchdog/wdt_aspeed.c
   trace/control-target.c)
set(qemu-system-arm_generated_sources "" 
${ANDROID_AUTOGEN}/trace/generated-helpers.c)

set(qemu-system-arm_dep libqemuutil qemu2-shared libqemustub)

set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/trace-root.c ${ANDROID_QEMU2_TOP_DIR}/dump.c ${ANDROID_QEMU2_TOP_DIR}/bt-vhci.c ${ANDROID_QEMU2_TOP_DIR}/gdbstub.c ${ANDROID_QEMU2_TOP_DIR}/arch_init.c ${ANDROID_QEMU2_TOP_DIR}/monitor.c ${ANDROID_QEMU2_TOP_DIR}/bt-host.c ${ANDROID_QEMU2_TOP_DIR}/blockdev-nbd.c ${ANDROID_QEMU2_TOP_DIR}/whpx-stub.c ${ANDROID_QEMU2_TOP_DIR}/block.c ${ANDROID_QEMU2_TOP_DIR}/hmp.c ${ANDROID_QEMU2_TOP_DIR}/qdev-monitor.c ${ANDROID_QEMU2_TOP_DIR}/device_tree.c ${ANDROID_QEMU2_TOP_DIR}/blockdev.c ${ANDROID_QEMU2_TOP_DIR}/ioport.c ${ANDROID_QEMU2_TOP_DIR}/numa.c ${ANDROID_QEMU2_TOP_DIR}/blockjob.c ${ANDROID_QEMU2_TOP_DIR}/dma-helpers.c ${ANDROID_QEMU2_TOP_DIR}/qmp.c ${ANDROID_QEMU2_TOP_DIR}/qemu-nbd.c ${ANDROID_QEMU2_TOP_DIR}/qemu-io-cmds.c ${ANDROID_QEMU2_TOP_DIR}/tpm.c ${ANDROID_QEMU2_TOP_DIR}/memory.c ${ANDROID_QEMU2_TOP_DIR}/cpus-common.c ${ANDROID_QEMU2_TOP_DIR}/iothread.c ${ANDROID_QEMU2_TOP_DIR}/qemu-img.c ${ANDROID_QEMU2_TOP_DIR}/os-posix.c ${ANDROID_QEMU2_TOP_DIR}/bootdevice.c ${ANDROID_QEMU2_TOP_DIR}/disas.c ${ANDROID_QEMU2_TOP_DIR}/balloon.c ${ANDROID_QEMU2_TOP_DIR}/replication.c ${ANDROID_QEMU2_TOP_DIR}/qemu-io.c ${ANDROID_QEMU2_TOP_DIR}/device-hotplug.c ${ANDROID_QEMU2_TOP_DIR}/qtest.c ${ANDROID_QEMU2_TOP_DIR}/exec.c ${ANDROID_QEMU2_TOP_DIR}/memory_mapping.c ${ANDROID_QEMU2_TOP_DIR}/cpus.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/ipack/ipack.c ${ANDROID_QEMU2_TOP_DIR}/hw/ipack/tpci200.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/ipack")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/vfio/trace.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/vfio")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/ssi/imx_spi.c ${ANDROID_QEMU2_TOP_DIR}/hw/ssi/aspeed_smc.c ${ANDROID_QEMU2_TOP_DIR}/hw/ssi/xilinx_spips.c ${ANDROID_QEMU2_TOP_DIR}/hw/ssi/stm32f2xx_spi.c ${ANDROID_QEMU2_TOP_DIR}/hw/ssi/pl022.c ${ANDROID_QEMU2_TOP_DIR}/hw/ssi/ssi.c ${ANDROID_QEMU2_TOP_DIR}/hw/ssi/omap_spi.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/ssi")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/adc/stm32f2xx_adc.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/adc")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/input/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/pl050.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/virtio-input.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/ps2.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/lm832x.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/goldfish_events_common.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/goldfish_rotary.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/stellaris_input.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/tsc2005.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/tsc210x.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/pckbd.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/pxa2xx_keypad.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/goldfish_events.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/virtio-input-hid.c ${ANDROID_QEMU2_TOP_DIR}/hw/input/hid.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/input")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/sparc/trace.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/sparc")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/disas/mips.c ${ANDROID_QEMU2_TOP_DIR}/disas/arm-a64.cc ${ANDROID_QEMU2_TOP_DIR}/disas/i386.c ${ANDROID_QEMU2_TOP_DIR}/disas/arm.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/disas")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/pci-host/gpex.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-host/q35.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-host/versatile.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-host/bonito.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-host/pam.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-host/xilinx-pcie.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-host/piix.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/pci-host")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/ppc/trace.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/ppc")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/net/rocker/rocker_of_dpa.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/rocker/rocker.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/rocker/rocker_world.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/rocker/rocker_fp.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/rocker/rocker_desc.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/net/rocker")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/ide/ahci.c ${ANDROID_QEMU2_TOP_DIR}/hw/ide/atapi.c ${ANDROID_QEMU2_TOP_DIR}/hw/ide/isa.c ${ANDROID_QEMU2_TOP_DIR}/hw/ide/pci.c ${ANDROID_QEMU2_TOP_DIR}/hw/ide/core.c ${ANDROID_QEMU2_TOP_DIR}/hw/ide/piix.c ${ANDROID_QEMU2_TOP_DIR}/hw/ide/microdrive.c ${ANDROID_QEMU2_TOP_DIR}/hw/ide/via.c ${ANDROID_QEMU2_TOP_DIR}/hw/ide/ich.c ${ANDROID_QEMU2_TOP_DIR}/hw/ide/qdev.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/ide")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/replay/replay.c ${ANDROID_QEMU2_TOP_DIR}/replay/replay-char.c ${ANDROID_QEMU2_TOP_DIR}/replay/replay-internal.c ${ANDROID_QEMU2_TOP_DIR}/replay/replay-time.c ${ANDROID_QEMU2_TOP_DIR}/replay/replay-events.c ${ANDROID_QEMU2_TOP_DIR}/replay/replay-audio.c ${ANDROID_QEMU2_TOP_DIR}/replay/replay-net.c ${ANDROID_QEMU2_TOP_DIR}/replay/replay-input.c ${ANDROID_QEMU2_TOP_DIR}/replay/replay-snapshot.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/replay")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/migration/trace.c ${ANDROID_QEMU2_TOP_DIR}/migration/savevm.c ${ANDROID_QEMU2_TOP_DIR}/migration/exec.c ${ANDROID_QEMU2_TOP_DIR}/migration/block.c ${ANDROID_QEMU2_TOP_DIR}/migration/colo-comm.c ${ANDROID_QEMU2_TOP_DIR}/migration/vmstate.c ${ANDROID_QEMU2_TOP_DIR}/migration/postcopy-ram.c ${ANDROID_QEMU2_TOP_DIR}/migration/xbzrle.c ${ANDROID_QEMU2_TOP_DIR}/migration/ram.c ${ANDROID_QEMU2_TOP_DIR}/migration/socket.c ${ANDROID_QEMU2_TOP_DIR}/migration/qemu-file.c ${ANDROID_QEMU2_TOP_DIR}/migration/migration.c ${ANDROID_QEMU2_TOP_DIR}/migration/fd.c ${ANDROID_QEMU2_TOP_DIR}/migration/qjson.c ${ANDROID_QEMU2_TOP_DIR}/migration/tls.c ${ANDROID_QEMU2_TOP_DIR}/migration/colo.c ${ANDROID_QEMU2_TOP_DIR}/migration/colo-failover.c ${ANDROID_QEMU2_TOP_DIR}/migration/qemu-file-channel.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/migration")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/io/trace.c ${ANDROID_QEMU2_TOP_DIR}/io/channel-socket.c ${ANDROID_QEMU2_TOP_DIR}/io/task.c ${ANDROID_QEMU2_TOP_DIR}/io/channel-tls.c ${ANDROID_QEMU2_TOP_DIR}/io/channel-websock.c ${ANDROID_QEMU2_TOP_DIR}/io/dns-resolver.c ${ANDROID_QEMU2_TOP_DIR}/io/channel-file.c ${ANDROID_QEMU2_TOP_DIR}/io/channel-command.c ${ANDROID_QEMU2_TOP_DIR}/io/channel-buffer.c ${ANDROID_QEMU2_TOP_DIR}/io/channel.c ${ANDROID_QEMU2_TOP_DIR}/io/channel-watch.c ${ANDROID_QEMU2_TOP_DIR}/io/channel-util.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/io")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/disas/libvixl/vixl/compiler-intrinsics.cc ${ANDROID_QEMU2_TOP_DIR}/disas/libvixl/vixl/utils.cc PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/disas/libvixl/vixl")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/i2c/bitbang_i2c.c ${ANDROID_QEMU2_TOP_DIR}/hw/i2c/core.c ${ANDROID_QEMU2_TOP_DIR}/hw/i2c/versatile_i2c.c ${ANDROID_QEMU2_TOP_DIR}/hw/i2c/omap_i2c.c ${ANDROID_QEMU2_TOP_DIR}/hw/i2c/imx_i2c.c ${ANDROID_QEMU2_TOP_DIR}/hw/i2c/aspeed_i2c.c ${ANDROID_QEMU2_TOP_DIR}/hw/i2c/exynos4210_i2c.c ${ANDROID_QEMU2_TOP_DIR}/hw/i2c/i2c-ddc.c ${ANDROID_QEMU2_TOP_DIR}/hw/i2c/smbus_ich9.c ${ANDROID_QEMU2_TOP_DIR}/hw/i2c/pm_smbus.c ${ANDROID_QEMU2_TOP_DIR}/hw/i2c/smbus.c ${ANDROID_QEMU2_TOP_DIR}/hw/i2c/smbus_eeprom.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/i2c")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/contrib/ivshmem-client/main.c ${ANDROID_QEMU2_TOP_DIR}/contrib/ivshmem-client/ivshmem-client.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/contrib/ivshmem-client")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/virtio/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/virtio/virtio-rng.c ${ANDROID_QEMU2_TOP_DIR}/hw/virtio/virtio.c ${ANDROID_QEMU2_TOP_DIR}/hw/virtio/virtio-mmio.c ${ANDROID_QEMU2_TOP_DIR}/hw/virtio/virtio-bus.c ${ANDROID_QEMU2_TOP_DIR}/hw/virtio/virtio-pci.c ${ANDROID_QEMU2_TOP_DIR}/hw/virtio/virtio-balloon.c ${ANDROID_QEMU2_TOP_DIR}/hw/virtio/vhost-stub.c ${ANDROID_QEMU2_TOP_DIR}/hw/virtio/virtio-crypto-pci.c ${ANDROID_QEMU2_TOP_DIR}/hw/virtio/virtio-crypto.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/virtio")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/i386/xen/trace.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/i386/xen")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/mips/trace.c ${ANDROID_QEMU2_TOP_DIR}/target/mips/helper.c ${ANDROID_QEMU2_TOP_DIR}/target/mips/op_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/mips/dsp_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/mips/cpu.c ${ANDROID_QEMU2_TOP_DIR}/target/mips/lmi_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/mips/gdbstub.c ${ANDROID_QEMU2_TOP_DIR}/target/mips/msa_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/mips/mips-semi.c ${ANDROID_QEMU2_TOP_DIR}/target/mips/translate.c ${ANDROID_QEMU2_TOP_DIR}/target/mips/machine.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/mips")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/isa/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/isa/lpc_ich9.c ${ANDROID_QEMU2_TOP_DIR}/hw/isa/piix4.c ${ANDROID_QEMU2_TOP_DIR}/hw/isa/isa-bus.c ${ANDROID_QEMU2_TOP_DIR}/hw/isa/vt82c686.c ${ANDROID_QEMU2_TOP_DIR}/hw/isa/apm.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/isa")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/qapi/trace.c ${ANDROID_QEMU2_TOP_DIR}/qapi/opts-visitor.c ${ANDROID_QEMU2_TOP_DIR}/qapi/qobject-input-visitor.c ${ANDROID_QEMU2_TOP_DIR}/qapi/string-input-visitor.c ${ANDROID_QEMU2_TOP_DIR}/qapi/qapi-util.c ${ANDROID_QEMU2_TOP_DIR}/qapi/qapi-dealloc-visitor.c ${ANDROID_QEMU2_TOP_DIR}/qapi/qapi-clone-visitor.c ${ANDROID_QEMU2_TOP_DIR}/qapi/qapi-visit-core.c ${ANDROID_QEMU2_TOP_DIR}/qapi/string-output-visitor.c ${ANDROID_QEMU2_TOP_DIR}/qapi/qmp-registry.c ${ANDROID_QEMU2_TOP_DIR}/qapi/qobject-output-visitor.c ${ANDROID_QEMU2_TOP_DIR}/qapi/qmp-event.c ${ANDROID_QEMU2_TOP_DIR}/qapi/qmp-dispatch.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/qapi")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/slirp/socket.c ${ANDROID_QEMU2_TOP_DIR}/slirp/ip6_input.c ${ANDROID_QEMU2_TOP_DIR}/slirp/cksum.c ${ANDROID_QEMU2_TOP_DIR}/slirp/bootp.c ${ANDROID_QEMU2_TOP_DIR}/slirp/ip_output.c ${ANDROID_QEMU2_TOP_DIR}/slirp/ip6_icmp.c ${ANDROID_QEMU2_TOP_DIR}/slirp/sbuf.c ${ANDROID_QEMU2_TOP_DIR}/slirp/ip6_output.c ${ANDROID_QEMU2_TOP_DIR}/slirp/mbuf.c ${ANDROID_QEMU2_TOP_DIR}/slirp/tcp_output.c ${ANDROID_QEMU2_TOP_DIR}/slirp/tftp.c ${ANDROID_QEMU2_TOP_DIR}/slirp/dnssearch.c ${ANDROID_QEMU2_TOP_DIR}/slirp/udp6.c ${ANDROID_QEMU2_TOP_DIR}/slirp/ndp_table.c ${ANDROID_QEMU2_TOP_DIR}/slirp/dhcpv6.c ${ANDROID_QEMU2_TOP_DIR}/slirp/tcp_input.c ${ANDROID_QEMU2_TOP_DIR}/slirp/ip_icmp_ping.c ${ANDROID_QEMU2_TOP_DIR}/slirp/slirp.c ${ANDROID_QEMU2_TOP_DIR}/slirp/udp.c ${ANDROID_QEMU2_TOP_DIR}/slirp/ip_icmp.c ${ANDROID_QEMU2_TOP_DIR}/slirp/misc.c ${ANDROID_QEMU2_TOP_DIR}/slirp/tcp_subr.c ${ANDROID_QEMU2_TOP_DIR}/slirp/ip_input.c ${ANDROID_QEMU2_TOP_DIR}/slirp/if.c ${ANDROID_QEMU2_TOP_DIR}/slirp/tcp_timer.c ${ANDROID_QEMU2_TOP_DIR}/slirp/arp_table.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/slirp")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/net/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/cadence_gem.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/ne2000-isa.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/vhost_net.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/dp8393x.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/imx_fec.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/net_rx_pkt.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/pcnet.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/virtio-net.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/vmxnet3.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/ne2000.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/mipsnet.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/stellaris_enet.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/smc91c111.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/net_tx_pkt.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/rtl8139.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/eepro100.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/pcnet-pci.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/lan9118.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/allwinner_emac.c ${ANDROID_QEMU2_TOP_DIR}/hw/net/xgmac.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/net")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/arm/trace.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/psci.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/crypto_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/op_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/helper.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/arm-powerctl.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/neon_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/gdbstub64.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/arch_dump.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/translate-a64.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/translate.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/machine.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/arm-semi.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/kvm-stub.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/cpu.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/cpu64.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/helper-a64.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/gdbstub.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/monitor.c ${ANDROID_QEMU2_TOP_DIR}/target/arm/iwmmxt_helper.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/arm")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/s390x/trace.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/s390x")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/block/dataplane/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/dataplane/virtio-blk.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/block/dataplane")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/smbios/smbios-stub.c ${ANDROID_QEMU2_TOP_DIR}/hw/smbios/smbios_type_38.c ${ANDROID_QEMU2_TOP_DIR}/hw/smbios/smbios.c ${ANDROID_QEMU2_TOP_DIR}/hw/smbios/smbios_type_38-stub.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/smbios")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/mem/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/mem/pc-dimm.c ${ANDROID_QEMU2_TOP_DIR}/hw/mem/nvdimm.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/mem")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/pci/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci/msi.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci/pci_bridge.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci/slotid_cap.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci/pcie_port.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci/pci_host.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci/pcie_aer.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci/pci.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci/shpc.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci/msix.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci/pcie_host.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci/pcie.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/pci")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/display/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/bcm2835_fb.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/vmware_vga.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/omap_dss.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/goldfish_fb.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/omap_lcdc.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/virtio-gpu-3d.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/blizzard.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/ssd0323.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/g364fb.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/vga-pci.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/ads7846.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/pl110.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/ssd0303.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/dpcd.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/virtio-gpu.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/jazz_led.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/xlnx_dp.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/tc6393xb.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/virtio-gpu-pci.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/pxa2xx_lcd.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/cirrus_vga.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/vga-isa-mm.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/exynos4210_fimd.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/vga.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/virtio-vga.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/vga-isa.c ${ANDROID_QEMU2_TOP_DIR}/hw/display/framebuffer.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/display")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/qobject/qbool.c ${ANDROID_QEMU2_TOP_DIR}/qobject/qlist.c ${ANDROID_QEMU2_TOP_DIR}/qobject/json-parser.c ${ANDROID_QEMU2_TOP_DIR}/qobject/qstring.c ${ANDROID_QEMU2_TOP_DIR}/qobject/qjson.c ${ANDROID_QEMU2_TOP_DIR}/qobject/qnull.c ${ANDROID_QEMU2_TOP_DIR}/qobject/json-streamer.c ${ANDROID_QEMU2_TOP_DIR}/qobject/json-lexer.c ${ANDROID_QEMU2_TOP_DIR}/qobject/qdict.c ${ANDROID_QEMU2_TOP_DIR}/qobject/qobject.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/qobject")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/fpu/softfloat.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/fpu")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/backends/hostmem.c ${ANDROID_QEMU2_TOP_DIR}/backends/rng-random-generic.c ${ANDROID_QEMU2_TOP_DIR}/backends/cryptodev.c ${ANDROID_QEMU2_TOP_DIR}/backends/rng-egd.c ${ANDROID_QEMU2_TOP_DIR}/backends/hostmem-ram.c ${ANDROID_QEMU2_TOP_DIR}/backends/rng.c ${ANDROID_QEMU2_TOP_DIR}/backends/tpm.c ${ANDROID_QEMU2_TOP_DIR}/backends/cryptodev-builtin.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/backends")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/core/reset.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/irq.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/hotplug.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/platform-bus.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/qdev-properties-system.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/generic-loader.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/loader.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/null-machine.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/empty_slot.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/bus.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/nmi.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/ptimer.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/register.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/sysbus.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/qdev.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/machine.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/or-irq.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/fw-path-provider.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/loader-fit.c ${ANDROID_QEMU2_TOP_DIR}/hw/core/qdev-properties.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/core")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/pcmcia/pcmcia.c ${ANDROID_QEMU2_TOP_DIR}/hw/pcmcia/pxa2xx.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/pcmcia")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/i386/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/i386/x86-iommu.c ${ANDROID_QEMU2_TOP_DIR}/hw/i386/pc_sysfw.c ${ANDROID_QEMU2_TOP_DIR}/hw/i386/amd_iommu.c ${ANDROID_QEMU2_TOP_DIR}/hw/i386/pc_q35.c ${ANDROID_QEMU2_TOP_DIR}/hw/i386/pc.c ${ANDROID_QEMU2_TOP_DIR}/hw/i386/multiboot.c ${ANDROID_QEMU2_TOP_DIR}/hw/i386/kvmvapic.c ${ANDROID_QEMU2_TOP_DIR}/hw/i386/intel_iommu.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/i386")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/9pfs/trace.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/9pfs")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/net/trace.c ${ANDROID_QEMU2_TOP_DIR}/net/dump.c ${ANDROID_QEMU2_TOP_DIR}/net/filter-mirror.c ${ANDROID_QEMU2_TOP_DIR}/net/queue.c ${ANDROID_QEMU2_TOP_DIR}/net/colo.c ${ANDROID_QEMU2_TOP_DIR}/net/filter-rewriter.c ${ANDROID_QEMU2_TOP_DIR}/net/checksum.c ${ANDROID_QEMU2_TOP_DIR}/net/hub.c ${ANDROID_QEMU2_TOP_DIR}/net/net.c ${ANDROID_QEMU2_TOP_DIR}/net/vhost-user.c ${ANDROID_QEMU2_TOP_DIR}/net/util.c ${ANDROID_QEMU2_TOP_DIR}/net/socket.c ${ANDROID_QEMU2_TOP_DIR}/net/colo-compare.c ${ANDROID_QEMU2_TOP_DIR}/net/filter.c ${ANDROID_QEMU2_TOP_DIR}/net/tap.c ${ANDROID_QEMU2_TOP_DIR}/net/filter-replay.c ${ANDROID_QEMU2_TOP_DIR}/net/slirp.c ${ANDROID_QEMU2_TOP_DIR}/net/filter-buffer.c ${ANDROID_QEMU2_TOP_DIR}/net/eth.c ${ANDROID_QEMU2_TOP_DIR}/net/tap-bsd.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/net")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/scsi/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/virtio-scsi.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/virtio-scsi-dataplane.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/megasas.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/scsi-generic.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/mptsas.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/mptendian.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/scsi-disk.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/esp.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/mptconfig.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/esp-pci.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/vmw_pvscsi.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/lsi53c895a.c ${ANDROID_QEMU2_TOP_DIR}/hw/scsi/scsi-bus.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/scsi")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/arm/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/exynos4_boards.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/virt-acpi-build.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/collie.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/omap2.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/sabrelite.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/versatilepb.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/exynos4210.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/digic.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/musicpal.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/realview.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/nseries.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/mainstone.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/vexpress.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/tosa.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/kzm.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/gumstix.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/fsl-imx6.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/xilinx_zynq.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/omap1.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/z2.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/highbank.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/stellaris.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/netduino2.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/ranchu.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/aspeed.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/pxa2xx_gpio.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/fsl-imx25.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/omap_sx1.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/pxa2xx.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/spitz.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/palm.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/fsl-imx31.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/sysbus-fdt.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/pxa2xx_pic.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/boot.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/cubieboard.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/integratorcp.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/aspeed_soc.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/bcm2835_peripherals.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/stm32f205_soc.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/allwinner-a10.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/strongarm.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/virt.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/raspi.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/bcm2836.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/digic_boards.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/armv7m.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/xlnx-zynqmp.c ${ANDROID_QEMU2_TOP_DIR}/hw/arm/imx25_pdk.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/arm")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/contrib/ivshmem-server/main.c ${ANDROID_QEMU2_TOP_DIR}/contrib/ivshmem-server/ivshmem-server.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/contrib/ivshmem-server")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/misc/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/arm_sysctl.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/mips_itu.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/bcm2835_rng.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/mst_fpga.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/stm32f2xx_syscfg.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/cbus.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/sga.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/goldfish_battery.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/max111x.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/zynq_slcr.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/arm11scu.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/edu.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/arm_l2x0.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/mips_cpc.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/goldfish_pstore.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/unimp.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/tmp105.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/auxbus.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/omap_l4.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/imx25_ccm.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/omap_gpmc.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/bcm2835_property.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/imx6_src.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/omap_clk.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/applesmc.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/mips_cmgcr.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/omap_tap.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/pc-testdev.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/goldfish_sync.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/imx31_ccm.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/goldfish_pipe.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/imx_ccm.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/debugexit.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/bcm2835_mbox.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/a9scu.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/exynos4210_pmu.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/omap_sdrc.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/aspeed_sdmc.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/pci-testdev.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/aspeed_scu.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/pvpanic.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/exynos4210_clk.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/arm_integrator_debug.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/imx6_ccm.c ${ANDROID_QEMU2_TOP_DIR}/hw/misc/zynq-xadc.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/misc")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/dma/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/dma/omap_dma.c ${ANDROID_QEMU2_TOP_DIR}/hw/dma/xlnx_dpdma.c ${ANDROID_QEMU2_TOP_DIR}/hw/dma/pl330.c ${ANDROID_QEMU2_TOP_DIR}/hw/dma/xlnx-zynq-devcfg.c ${ANDROID_QEMU2_TOP_DIR}/hw/dma/i8257.c ${ANDROID_QEMU2_TOP_DIR}/hw/dma/rc4030.c ${ANDROID_QEMU2_TOP_DIR}/hw/dma/pl080.c ${ANDROID_QEMU2_TOP_DIR}/hw/dma/pxa2xx_dma.c ${ANDROID_QEMU2_TOP_DIR}/hw/dma/soc_dma.c ${ANDROID_QEMU2_TOP_DIR}/hw/dma/bcm2835_dma.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/dma")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/i386/trace.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/hax-darwin.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/helper.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/arch_dump.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/kvm-stub.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/mpx_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/smm_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/monitor.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/hax-mem.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/excp_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/machine.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/cpu.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/hvf-all.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/gdbstub.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/svm_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/seg_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/translate.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/bpt_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/hax-all.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/fpu_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/cc_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/arch_memory_mapping.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/mem_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/int_helper.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/misc_helper.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/i386")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/sparc/trace.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/sparc")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/timer/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/exynos4210_mct.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/ds1338.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/omap_synctimer.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/exynos4210_rtc.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/exynos4210_pwm.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/twl92230.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/armv7m_systick.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/imx_gpt.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/arm_mptimer.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/imx_epit.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/digic-timer.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/mips_gictimer.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/i8254.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/pxa2xx_timer.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/a9gtimer.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/i8254_common.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/pl031.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/omap_gptimer.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/goldfish_timer.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/aspeed_timer.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/allwinner-a10-pit.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/mc146818rtc.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/stm32f2xx_timer.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/hpet.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/arm_timer.c ${ANDROID_QEMU2_TOP_DIR}/hw/timer/cadence_ttc.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/timer")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/xen/trace.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/xen")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/alpha/trace.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/alpha")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/tpm/tpm_tis.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/tpm")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/util/trace.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-coroutine-lock.c ${ANDROID_QEMU2_TOP_DIR}/util/throttle.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-coroutine-sleep.c ${ANDROID_QEMU2_TOP_DIR}/util/uuid.c ${ANDROID_QEMU2_TOP_DIR}/util/aiocb.c ${ANDROID_QEMU2_TOP_DIR}/util/bitmap.c ${ANDROID_QEMU2_TOP_DIR}/util/bitops.c ${ANDROID_QEMU2_TOP_DIR}/util/osdep.c ${ANDROID_QEMU2_TOP_DIR}/util/lockcnt.c ${ANDROID_QEMU2_TOP_DIR}/util/crc32c.c ${ANDROID_QEMU2_TOP_DIR}/util/systemd.c ${ANDROID_QEMU2_TOP_DIR}/util/aio-posix.c ${ANDROID_QEMU2_TOP_DIR}/util/qht.c ${ANDROID_QEMU2_TOP_DIR}/util/acl.c ${ANDROID_QEMU2_TOP_DIR}/util/iov.c ${ANDROID_QEMU2_TOP_DIR}/util/coroutine-sigaltstack.c ${ANDROID_QEMU2_TOP_DIR}/util/id.c ${ANDROID_QEMU2_TOP_DIR}/util/notify.c ${ANDROID_QEMU2_TOP_DIR}/util/oslib-posix.c ${ANDROID_QEMU2_TOP_DIR}/util/iohandler.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-thread-posix.c ${ANDROID_QEMU2_TOP_DIR}/util/fifo8.c ${ANDROID_QEMU2_TOP_DIR}/util/uri.c ${ANDROID_QEMU2_TOP_DIR}/util/main-loop.c ${ANDROID_QEMU2_TOP_DIR}/util/memfd.c ${ANDROID_QEMU2_TOP_DIR}/util/keyval.c ${ANDROID_QEMU2_TOP_DIR}/util/timed-average.c ${ANDROID_QEMU2_TOP_DIR}/util/rcu.c ${ANDROID_QEMU2_TOP_DIR}/util/thread-pool.c ${ANDROID_QEMU2_TOP_DIR}/util/abort.c ${ANDROID_QEMU2_TOP_DIR}/util/readline.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-config.c ${ANDROID_QEMU2_TOP_DIR}/util/event_notifier-posix.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-progress.c ${ANDROID_QEMU2_TOP_DIR}/util/cutils.c ${ANDROID_QEMU2_TOP_DIR}/util/unicode.c ${ANDROID_QEMU2_TOP_DIR}/util/log.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-openpty.c ${ANDROID_QEMU2_TOP_DIR}/util/qdist.c ${ANDROID_QEMU2_TOP_DIR}/util/async.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-sockets.c ${ANDROID_QEMU2_TOP_DIR}/util/host-utils.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-error.c ${ANDROID_QEMU2_TOP_DIR}/util/hexdump.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-timer.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-option.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-coroutine-io.c ${ANDROID_QEMU2_TOP_DIR}/util/buffer.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-coroutine.c ${ANDROID_QEMU2_TOP_DIR}/util/range.c ${ANDROID_QEMU2_TOP_DIR}/util/getauxval.c ${ANDROID_QEMU2_TOP_DIR}/util/compatfd.c ${ANDROID_QEMU2_TOP_DIR}/util/mmap-alloc.c ${ANDROID_QEMU2_TOP_DIR}/util/path.c ${ANDROID_QEMU2_TOP_DIR}/util/base64.c ${ANDROID_QEMU2_TOP_DIR}/util/module.c ${ANDROID_QEMU2_TOP_DIR}/util/qemu-timer-common.c ${ANDROID_QEMU2_TOP_DIR}/util/hbitmap.c ${ANDROID_QEMU2_TOP_DIR}/util/error.c ${ANDROID_QEMU2_TOP_DIR}/util/bufferiszero.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/util")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/watchdog/wdt_aspeed.c ${ANDROID_QEMU2_TOP_DIR}/hw/watchdog/watchdog.c ${ANDROID_QEMU2_TOP_DIR}/hw/watchdog/wdt_ib700.c ${ANDROID_QEMU2_TOP_DIR}/hw/watchdog/wdt_i6300esb.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/watchdog")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/ppc/trace.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/ppc")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/gpio/pl061.c ${ANDROID_QEMU2_TOP_DIR}/hw/gpio/imx_gpio.c ${ANDROID_QEMU2_TOP_DIR}/hw/gpio/max7310.c ${ANDROID_QEMU2_TOP_DIR}/hw/gpio/gpio_key.c ${ANDROID_QEMU2_TOP_DIR}/hw/gpio/zaurus.c ${ANDROID_QEMU2_TOP_DIR}/hw/gpio/omap_gpio.c ${ANDROID_QEMU2_TOP_DIR}/hw/gpio/bcm2835_gpio.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/gpio")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/nbd/client.c ${ANDROID_QEMU2_TOP_DIR}/nbd/server.c ${ANDROID_QEMU2_TOP_DIR}/nbd/common.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/nbd")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/usb/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/hcd-xhci.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/hcd-musb.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/dev-smartcard-reader.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/dev-network.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/dev-uas.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/hcd-ehci-pci.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/dev-audio.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/tusb6010.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/dev-hid.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/desc-msos.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/bus.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/desc.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/core.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/dev-bluetooth.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/hcd-ehci-sysbus.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/dev-serial.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/hcd-uhci.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/dev-wacom.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/hcd-ohci.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/hcd-ehci.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/dev-mtp.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/combined-packet.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/libhw.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/host-libusb.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/dev-storage.c ${ANDROID_QEMU2_TOP_DIR}/hw/usb/dev-hub.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/usb")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/pci-bridge/i82801b11.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-bridge/gen_pcie_root_port.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-bridge/xio3130_upstream.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-bridge/pcie_root_port.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-bridge/pci_expander_bridge.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-bridge/pci_bridge_dev.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-bridge/ioh3420.c ${ANDROID_QEMU2_TOP_DIR}/hw/pci-bridge/xio3130_downstream.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/pci-bridge")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/bt/core.c ${ANDROID_QEMU2_TOP_DIR}/hw/bt/hci-csr.c ${ANDROID_QEMU2_TOP_DIR}/hw/bt/hid.c ${ANDROID_QEMU2_TOP_DIR}/hw/bt/sdp.c ${ANDROID_QEMU2_TOP_DIR}/hw/bt/l2cap.c ${ANDROID_QEMU2_TOP_DIR}/hw/bt/hci.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/bt")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/nvram/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/nvram/ds1225y.c ${ANDROID_QEMU2_TOP_DIR}/hw/nvram/fw_cfg.c ${ANDROID_QEMU2_TOP_DIR}/hw/nvram/eeprom93xx.c ${ANDROID_QEMU2_TOP_DIR}/hw/nvram/chrp_nvram.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/nvram")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/ipmi/isa_ipmi_bt.c ${ANDROID_QEMU2_TOP_DIR}/hw/ipmi/isa_ipmi_kcs.c ${ANDROID_QEMU2_TOP_DIR}/hw/ipmi/ipmi_bmc_sim.c ${ANDROID_QEMU2_TOP_DIR}/hw/ipmi/ipmi.c ${ANDROID_QEMU2_TOP_DIR}/hw/ipmi/ipmi_bmc_extern.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/ipmi")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/linux-user/trace.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/linux-user")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/mips/mips_fulong2e.c ${ANDROID_QEMU2_TOP_DIR}/hw/mips/mips_mipssim.c ${ANDROID_QEMU2_TOP_DIR}/hw/mips/mips_malta.c ${ANDROID_QEMU2_TOP_DIR}/hw/mips/mips_ranchu.c ${ANDROID_QEMU2_TOP_DIR}/hw/mips/boston.c ${ANDROID_QEMU2_TOP_DIR}/hw/mips/mips_jazz.c ${ANDROID_QEMU2_TOP_DIR}/hw/mips/mips_int.c ${ANDROID_QEMU2_TOP_DIR}/hw/mips/addr.c ${ANDROID_QEMU2_TOP_DIR}/hw/mips/gt64xxx_pci.c ${ANDROID_QEMU2_TOP_DIR}/hw/mips/cps.c ${ANDROID_QEMU2_TOP_DIR}/hw/mips/mips_r4k.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/mips")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/tcg/optimize.c ${ANDROID_QEMU2_TOP_DIR}/tcg/tcg-common.c ${ANDROID_QEMU2_TOP_DIR}/tcg/tcg-op.c ${ANDROID_QEMU2_TOP_DIR}/tcg/tcg.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/tcg")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/s390x/trace.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/s390x")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/audio/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/gus.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/pl041.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/marvell_88w8618.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/hda-codec.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/adlib.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/wm8750.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/sb16.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/fmopl.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/gusemu_mixer.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/cs4231a.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/lm4549.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/pcspk.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/gusemu_hal.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/es1370.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/intel-hda.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/goldfish_audio.c ${ANDROID_QEMU2_TOP_DIR}/hw/audio/ac97.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/audio")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/block/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/cdrom.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/block.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/pflash_cfi01.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/m25p80.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/ecc.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/onenand.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/nvme.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/nand.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/pflash_cfi02.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/virtio-blk.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/hd-geometry.c ${ANDROID_QEMU2_TOP_DIR}/hw/block/fdc.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/block")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/stubs/qtest.c ${ANDROID_QEMU2_TOP_DIR}/stubs/iothread.c ${ANDROID_QEMU2_TOP_DIR}/stubs/arch-query-cpu-model-expansion.c ${ANDROID_QEMU2_TOP_DIR}/stubs/target-get-monitor-def.c ${ANDROID_QEMU2_TOP_DIR}/stubs/get-vm-name.c ${ANDROID_QEMU2_TOP_DIR}/stubs/runstate-check.c ${ANDROID_QEMU2_TOP_DIR}/stubs/error-printf.c ${ANDROID_QEMU2_TOP_DIR}/stubs/clock-warp.c ${ANDROID_QEMU2_TOP_DIR}/stubs/set-fd-handler.c ${ANDROID_QEMU2_TOP_DIR}/stubs/replay.c ${ANDROID_QEMU2_TOP_DIR}/stubs/migr-blocker.c ${ANDROID_QEMU2_TOP_DIR}/stubs/monitor.c ${ANDROID_QEMU2_TOP_DIR}/stubs/gdbstub.c ${ANDROID_QEMU2_TOP_DIR}/stubs/pc_madt_cpu_entry.c ${ANDROID_QEMU2_TOP_DIR}/stubs/slirp.c ${ANDROID_QEMU2_TOP_DIR}/stubs/notify-event.c ${ANDROID_QEMU2_TOP_DIR}/stubs/vmgenid.c ${ANDROID_QEMU2_TOP_DIR}/stubs/trace-control.c ${ANDROID_QEMU2_TOP_DIR}/stubs/cpu-get-clock.c ${ANDROID_QEMU2_TOP_DIR}/stubs/cpu-get-icount.c ${ANDROID_QEMU2_TOP_DIR}/stubs/arch-query-cpu-def.c ${ANDROID_QEMU2_TOP_DIR}/stubs/arch-query-cpu-model-comparison.c ${ANDROID_QEMU2_TOP_DIR}/stubs/target-monitor-defs.c ${ANDROID_QEMU2_TOP_DIR}/stubs/vm-stop.c ${ANDROID_QEMU2_TOP_DIR}/stubs/blockdev-close-all-bdrv-states.c ${ANDROID_QEMU2_TOP_DIR}/stubs/is-daemonized.c ${ANDROID_QEMU2_TOP_DIR}/stubs/sysbus.c ${ANDROID_QEMU2_TOP_DIR}/stubs/iothread-lock.c ${ANDROID_QEMU2_TOP_DIR}/stubs/vmstate.c ${ANDROID_QEMU2_TOP_DIR}/stubs/fdset.c ${ANDROID_QEMU2_TOP_DIR}/stubs/bdrv-next-monitor-owned.c ${ANDROID_QEMU2_TOP_DIR}/stubs/blk-commit-all.c ${ANDROID_QEMU2_TOP_DIR}/stubs/dump.c ${ANDROID_QEMU2_TOP_DIR}/stubs/machine-init-done.c ${ANDROID_QEMU2_TOP_DIR}/stubs/migration.c ${ANDROID_QEMU2_TOP_DIR}/stubs/uuid.c ${ANDROID_QEMU2_TOP_DIR}/stubs/arch-query-cpu-model-baseline.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/stubs")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/trace/generated-helpers.c ${ANDROID_QEMU2_TOP_DIR}/trace/qmp.c ${ANDROID_QEMU2_TOP_DIR}/trace/control-target.c ${ANDROID_QEMU2_TOP_DIR}/trace/control.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/trace")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/crypto/trace.c ${ANDROID_QEMU2_TOP_DIR}/crypto/hash.c ${ANDROID_QEMU2_TOP_DIR}/crypto/hmac-glib.c ${ANDROID_QEMU2_TOP_DIR}/crypto/desrfb.c ${ANDROID_QEMU2_TOP_DIR}/crypto/ivgen-plain.c ${ANDROID_QEMU2_TOP_DIR}/crypto/tlssession.c ${ANDROID_QEMU2_TOP_DIR}/crypto/block-luks.c ${ANDROID_QEMU2_TOP_DIR}/crypto/block-qcow.c ${ANDROID_QEMU2_TOP_DIR}/crypto/tlscredsanon.c ${ANDROID_QEMU2_TOP_DIR}/crypto/block.c ${ANDROID_QEMU2_TOP_DIR}/crypto/xts.c ${ANDROID_QEMU2_TOP_DIR}/crypto/afsplit.c ${ANDROID_QEMU2_TOP_DIR}/crypto/secret.c ${ANDROID_QEMU2_TOP_DIR}/crypto/ivgen-essiv.c ${ANDROID_QEMU2_TOP_DIR}/crypto/aes.c ${ANDROID_QEMU2_TOP_DIR}/crypto/random-platform.c ${ANDROID_QEMU2_TOP_DIR}/crypto/cipher.c ${ANDROID_QEMU2_TOP_DIR}/crypto/tlscreds.c ${ANDROID_QEMU2_TOP_DIR}/crypto/tlscredsx509.c ${ANDROID_QEMU2_TOP_DIR}/crypto/ivgen.c ${ANDROID_QEMU2_TOP_DIR}/crypto/ivgen-plain64.c ${ANDROID_QEMU2_TOP_DIR}/crypto/pbkdf.c ${ANDROID_QEMU2_TOP_DIR}/crypto/hash-glib.c ${ANDROID_QEMU2_TOP_DIR}/crypto/hmac.c ${ANDROID_QEMU2_TOP_DIR}/crypto/init.c ${ANDROID_QEMU2_TOP_DIR}/crypto/pbkdf-stub.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/crypto")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/hw/cpu/a9mpcore.c ${ANDROID_QEMU2_TOP_DIR}/hw/cpu/a15mpcore.c ${ANDROID_QEMU2_TOP_DIR}/hw/cpu/core.c ${ANDROID_QEMU2_TOP_DIR}/hw/cpu/realview_mpcore.c ${ANDROID_QEMU2_TOP_DIR}/hw/cpu/arm11mpcore.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/cpu")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/target/i386/hvf-utils/x86_cpuid.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/hvf-utils/x86hvf.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/hvf-utils/x86_decode.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/hvf-utils/x86_flags.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/hvf-utils/x86.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/hvf-utils/x86_descr.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/hvf-utils/x86_emu.c ${ANDROID_QEMU2_TOP_DIR}/target/i386/hvf-utils/x86_mmu.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/target/i386/hvf-utils")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/qom/trace.c ${ANDROID_QEMU2_TOP_DIR}/qom/object.c ${ANDROID_QEMU2_TOP_DIR}/qom/qom-qobject.c ${ANDROID_QEMU2_TOP_DIR}/qom/object_interfaces.c ${ANDROID_QEMU2_TOP_DIR}/qom/container.c ${ANDROID_QEMU2_TOP_DIR}/qom/cpu.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/qom")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/intc/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/arm_gic_common.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/bcm2836_control.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/omap_intc.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/i8259_common.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/exynos4210_gic.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/ioapic.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/armv7m_nvic.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/intc.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/arm_gicv3.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/aspeed_vic.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/arm_gicv3_cpuif.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/realview_gic.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/arm_gic.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/apic_common.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/allwinner-a10-pic.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/arm_gicv3_dist.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/arm_gicv3_common.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/goldfish_pic.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/bcm2835_ic.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/ioapic_common.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/exynos4210_combiner.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/arm_gicv3_redist.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/imx_avic.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/arm_gicv3_its_common.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/mips_gic.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/apic.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/pl190.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/arm_gicv2m.c ${ANDROID_QEMU2_TOP_DIR}/hw/intc/i8259.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/intc")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/ui/trace.c ${ANDROID_QEMU2_TOP_DIR}/ui/vnc-jobs.c ${ANDROID_QEMU2_TOP_DIR}/ui/input-legacy.c ${ANDROID_QEMU2_TOP_DIR}/ui/vnc-enc-zlib.c ${ANDROID_QEMU2_TOP_DIR}/ui/keymaps.c ${ANDROID_QEMU2_TOP_DIR}/ui/vnc-auth-vencrypt.c ${ANDROID_QEMU2_TOP_DIR}/ui/input-keymap.c ${ANDROID_QEMU2_TOP_DIR}/ui/vnc.c ${ANDROID_QEMU2_TOP_DIR}/ui/vnc-enc-zrle.c ${ANDROID_QEMU2_TOP_DIR}/ui/console.c ${ANDROID_QEMU2_TOP_DIR}/ui/cursor.c ${ANDROID_QEMU2_TOP_DIR}/ui/input.c ${ANDROID_QEMU2_TOP_DIR}/ui/vnc-ws.c ${ANDROID_QEMU2_TOP_DIR}/ui/qemu-pixman.c ${ANDROID_QEMU2_TOP_DIR}/ui/vnc-enc-hextile.c ${ANDROID_QEMU2_TOP_DIR}/ui/vnc-palette.c ${ANDROID_QEMU2_TOP_DIR}/ui/x_keymap.c ${ANDROID_QEMU2_TOP_DIR}/ui/vnc-enc-tight.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/ui")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/sd/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/sd/omap_mmc.c ${ANDROID_QEMU2_TOP_DIR}/hw/sd/bcm2835_sdhost.c ${ANDROID_QEMU2_TOP_DIR}/hw/sd/pl181.c ${ANDROID_QEMU2_TOP_DIR}/hw/sd/pxa2xx_mmci.c ${ANDROID_QEMU2_TOP_DIR}/hw/sd/sd.c ${ANDROID_QEMU2_TOP_DIR}/hw/sd/core.c ${ANDROID_QEMU2_TOP_DIR}/hw/sd/ssi-sd.c ${ANDROID_QEMU2_TOP_DIR}/hw/sd/sdhci.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/sd")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/chardev/char-ringbuf.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char-fd.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char-mux.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char-pipe.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char-parallel.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char-stdio.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char-io.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char-file.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char-null.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char-serial.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char-udp.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char-pty.c ${ANDROID_QEMU2_TOP_DIR}/chardev/char-socket.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/chardev")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/char/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/virtio-serial-bus.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/omap_uart.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/ipoctal232.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/serial-pci.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/parallel.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/imx_serial.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/cadence_uart.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/pl011.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/bcm2835_aux.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/virtio-console.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/goldfish_tty.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/digic-uart.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/serial.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/exynos4210_uart.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/stm32f2xx_usart.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/serial-isa.c ${ANDROID_QEMU2_TOP_DIR}/hw/char/debugcon.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/char")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/acpi/trace.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/cpu_hotplug.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/bios-linker-loader.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/memory_hotplug.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/aml-build.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/ich9.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/ipmi-stub.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/acpi-stub.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/pcihp.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/piix4.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/vmgenid.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/nvdimm.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/tco.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/cpu.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/core.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/ipmi.c ${ANDROID_QEMU2_TOP_DIR}/hw/acpi/acpi_interface.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/hw/acpi")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/audio/trace.c ${ANDROID_QEMU2_TOP_DIR}/audio/noaudio.c ${ANDROID_QEMU2_TOP_DIR}/audio/wavaudio.c ${ANDROID_QEMU2_TOP_DIR}/audio/audio.c ${ANDROID_QEMU2_TOP_DIR}/audio/wavcapture.c ${ANDROID_QEMU2_TOP_DIR}/audio/coreaudio.c ${ANDROID_QEMU2_TOP_DIR}/audio/mixeng.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/audio")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/disas/libvixl/vixl/a64/decoder-a64.cc ${ANDROID_QEMU2_TOP_DIR}/disas/libvixl/vixl/a64/disasm-a64.cc ${ANDROID_QEMU2_TOP_DIR}/disas/libvixl/vixl/a64/instructions-a64.cc PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/disas/libvixl/vixl/a64")
set_source_files_properties(${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/block/trace.c ${ANDROID_QEMU2_TOP_DIR}/block/vvfat.c ${ANDROID_QEMU2_TOP_DIR}/block/mirror.c ${ANDROID_QEMU2_TOP_DIR}/block/commit.c ${ANDROID_QEMU2_TOP_DIR}/block/qed-check.c ${ANDROID_QEMU2_TOP_DIR}/block/sheepdog.c ${ANDROID_QEMU2_TOP_DIR}/block/bochs.c ${ANDROID_QEMU2_TOP_DIR}/block/crypto.c ${ANDROID_QEMU2_TOP_DIR}/block/vhdx-log.c ${ANDROID_QEMU2_TOP_DIR}/block/snapshot.c ${ANDROID_QEMU2_TOP_DIR}/block/qed.c ${ANDROID_QEMU2_TOP_DIR}/block/io.c ${ANDROID_QEMU2_TOP_DIR}/block/replication.c ${ANDROID_QEMU2_TOP_DIR}/block/block-backend.c ${ANDROID_QEMU2_TOP_DIR}/block/cloop.c ${ANDROID_QEMU2_TOP_DIR}/block/file-posix.c ${ANDROID_QEMU2_TOP_DIR}/block/nbd.c ${ANDROID_QEMU2_TOP_DIR}/block/backup.c ${ANDROID_QEMU2_TOP_DIR}/block/qcow2-cache.c ${ANDROID_QEMU2_TOP_DIR}/block/stream.c ${ANDROID_QEMU2_TOP_DIR}/block/vdi.c ${ANDROID_QEMU2_TOP_DIR}/block/parallels.c ${ANDROID_QEMU2_TOP_DIR}/block/throttle-groups.c ${ANDROID_QEMU2_TOP_DIR}/block/nbd-client.c ${ANDROID_QEMU2_TOP_DIR}/block/qed-cluster.c ${ANDROID_QEMU2_TOP_DIR}/block/null.c ${ANDROID_QEMU2_TOP_DIR}/block/quorum.c ${ANDROID_QEMU2_TOP_DIR}/block/accounting.c ${ANDROID_QEMU2_TOP_DIR}/block/raw-format.c ${ANDROID_QEMU2_TOP_DIR}/block/vhdx.c ${ANDROID_QEMU2_TOP_DIR}/block/blkreplay.c ${ANDROID_QEMU2_TOP_DIR}/block/qcow2-cluster.c ${ANDROID_QEMU2_TOP_DIR}/block/vhdx-endian.c ${ANDROID_QEMU2_TOP_DIR}/block/vpc.c ${ANDROID_QEMU2_TOP_DIR}/block/qcow2-refcount.c ${ANDROID_QEMU2_TOP_DIR}/block/cat.c ${ANDROID_QEMU2_TOP_DIR}/block/write-threshold.c ${ANDROID_QEMU2_TOP_DIR}/block/vmdk.c ${ANDROID_QEMU2_TOP_DIR}/block/blkverify.c ${ANDROID_QEMU2_TOP_DIR}/block/qapi.c ${ANDROID_QEMU2_TOP_DIR}/block/blkdebug.c ${ANDROID_QEMU2_TOP_DIR}/block/dmg.c ${ANDROID_QEMU2_TOP_DIR}/block/dirty-bitmap.c ${ANDROID_QEMU2_TOP_DIR}/block/qcow2.c ${ANDROID_QEMU2_TOP_DIR}/block/qcow2-snapshot.c ${ANDROID_QEMU2_TOP_DIR}/block/qed-table.c ${ANDROID_QEMU2_TOP_DIR}/block/qcow.c ${ANDROID_QEMU2_TOP_DIR}/block/qed-l2-cache.c PROPERTIES COMPILE_FLAGS " -I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/block")
