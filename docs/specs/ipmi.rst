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

Wire protocol
=============

The wire protocol used between ``ipmi-bmc-extern`` and the external BMC
emulator is defined by `README.vm
<https://github.com/wrouesnel/openipmi/blob/master/lanserv/README.vm>`_ from
the OpenIPMI project.
