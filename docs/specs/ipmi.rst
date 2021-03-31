=====================
IPMI device emulation
=====================

QEMU supports emulating many types of machines. This includes machines that may
serve as the main processor in an IPMI system, e.g. x86 or POWER server
processors, as well as machines emulating ARM-based Baseband Management
Controllers (BMCs), e.g. AST2xxx or NPCM7xxx systems-on-chip.

Main processor emulation
========================

A server platform may include one of the following system interfaces for
communicating with a BMC:

* A Keyboard Controller Style (KCS) Interface, accessible via ISA
  (``isa-ipmi-kcs``) or PCI (``pci-ipmi-kcs``).
* A Block Transfer (BT) Interface, accessible via ISA (``isa-ipmi-bt``) or PCI
  (``pci-ipmi-bt``).
* An SMBus System Interface (SSIF; ``smbus-ipmi``).

These interfaces can all be emulated by QEMU. To emulate the behavior of the
BMC, the messaging interface emulators use one of the following backends:

* A BMC simulator running within the QEMU process (``ipmi-bmc-sim``).
* An external BMC simulator or emulator, connected over a chardev
  (``ipmi-bmc-extern``). `ipmi_sim
  <https://github.com/wrouesnel/openipmi/blob/master/lanserv/README.ipmi_sim>`_
  from OpenIPMI is an example external BMC emulator.

The following diagram shows how these entities relate to each other.

.. blockdiag::

    blockdiag main_processor_ipmi {
        orientation = portrait
        default_group_color = "none";
        class msgif [color = lightblue];
        class bmc [color = salmon];

        ipmi_sim [color="aquamarine", label="External BMC"]
        ipmi-bmc-extern <-> ipmi_sim [label="chardev"];

        group {
            orientation = portrait

            ipmi-interface <-> ipmi-bmc;

            group {
                orientation = portrait

                ipmi-interface [class = "msgif"];
                isa-ipmi-kcs [class="msgif", stacked];

                ipmi-interface <- isa-ipmi-kcs [hstyle = generalization];
            }


            group {
                orientation = portrait

                ipmi-bmc [class = "bmc"];
                ipmi-bmc-sim [class="bmc"];
                ipmi-bmc-extern [class="bmc"];

                ipmi-bmc <- ipmi-bmc-sim [hstyle = generalization];
                ipmi-bmc <- ipmi-bmc-extern [hstyle = generalization];
            }

        }
    }

IPMI System Interfaces
----------------------

The system software running on the main processor may use a *system interface*
to communicate with the BMC. These are hardware devices attached to an ISA, PCI
or i2c bus, and in QEMU, they all need to implement ``ipmi-interface``.
This allows a BMC implementation to interact with the system interface in a
standard way.

IPMI BMC
--------

The system interface devices delegate emulation of BMC behavior to a BMC
device, that is a subclass of ``ipmi-bmc``. This type of device is called
a BMC because that's what it looks like to the main processor guest software.

The BMC behavior may be simulated within the qemu process (``ipmi-bmc-sim``) or
further delegated to an external emulator, or a real BMC. The
``ipmi-bmc-extern`` device has a required ``chardev`` property which specifies
the communications channel to the external BMC.

Baseband Management Controller (BMC) emulation
==============================================

This section is about emulation of IPMI-related devices in a System-on-Chip
(SoC) used as a Baseband Management Controller. This is not to be confused with
emulating the BMC device as seen by the main processor.

SoCs that are designed to be used as a BMC often have dedicated hardware that
allows them to be connected to one or more of the IPMI System Interfaces. The
BMC-side hardware interface is not standardized, so each type of SoC may need
its own device implementation in QEMU, for example:

* ``aspeed-ibt`` for emulating the Aspeed iBT peripheral.
* ``npcm7xx-kcs`` for emulating the Nuvoton NPCM7xx Host-to-BMC Keyboard
  Controller Style (KCS) channels.
* ``smbus-ipmi-host`` for emulating the SMBus System Interface(SSIF)
  Host-to-BMC smbus channel.

.. blockdiag::

    blockdiag bmc_ipmi {
        orientation = portrait
        default_group_color = "none";
        class responder [color = lightblue];
        class host [color = salmon];
        class i2c [color = yellow];

        host [color="aquamarine", label="External Host"]

        group {
            orientation = portrait

            group {
                orientation = portrait

                bmc-responder [class = "responder"]
                npcm7xx-ipmi-kcs [class = "responder", stacked]
                smbus-ipmi-host [class="responder"];
                smbus-device [class="i2c"];

                bmc-responder <- npcm7xx-ipmi-kcs [hstyle = generalization];

                bmc-responder <- smbus-ipmi-host [hstyle = generalization];
                smbus-ipmi-host -> smbus-device
            }

            group {
                orientation = portrait

                bmc-host [class = "host"];
                bmc-host-sim [class = "host"];
                bmc-host-extern [class = "host"];

                bmc-host <- bmc-host-sim [hstyle = generalization];
                bmc-host <- bmc-host-extern [hstyle = generalization];
            }

            bmc-responder <-> bmc-host
        }

        bmc-host-extern <-> host [label="chardev"];
    }

IPMI Responder Interface
------------------------

The software running on the BMC needs to intercept reads and writes to the
system interface registers on the main processor. This requires special
hardware that needs to be emulated by QEMU. We'll call these device *IPMI
responders*.

All *IPMI responder* devices should implement the ``ipmi-interface`` interface.
These devices are not supposed to work for main processor emulation and vice
versa.

IPMI Host
---------

Mirroring the main processor emulation, the responder devices delegate
emulation of host behavior to a Host device that is a subclass of
``ipmi-core``. This type of device is called a Host because that's what it
looks like to the BMC guest software.

The host behavior may be further delegated to an external emulator (e.g.
another QEMU VM) through the ``ipmi-host-extern`` host implementation. This
device has a required ``chardev`` property which specifies the communications
channel to the external host and a required ``responder`` property which
specifies the underlying IPMI responder. The wire format is the same as for
``ipmi-bmc-extern``.

Wire protocol
=============

The wire protocol used between ``ipmi-bmc-extern`` and the external BMC
emulator is defined by `README.vm
<https://github.com/wrouesnel/openipmi/blob/master/lanserv/README.vm>`_ from
the OpenIPMI project.
