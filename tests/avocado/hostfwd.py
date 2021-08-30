# Hostfwd command tests
#
# Copyright 2021 Google LLC
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.


from avocado_qemu import Test


class Hostfwd(Test):
    """
    :avocado: tags=hostfwd
    """
    def hmc(self, cmd):
        return self.vm.command('human-monitor-command', command_line=cmd)

    def test_qmp_hostfwd_ipv4(self):
        self.vm.add_args('-nodefaults',
                         '-netdev', 'user,id=vnet',
                         '-device', 'virtio-net,netdev=vnet')
        self.vm.launch()
        self.assertEquals(self.hmc('hostfwd_add vnet tcp::65022-:22'), '')
        self.assertEquals(self.hmc('hostfwd_remove vnet tcp::65022'),
                          'host forwarding rule for tcp::65022 removed\r\n')
        self.assertEquals(self.hmc('hostfwd_add tcp::65022-:22'), '')
        self.assertEquals(self.hmc('hostfwd_remove tcp::65022'),
                          'host forwarding rule for tcp::65022 removed\r\n')
        self.assertEquals(self.hmc('hostfwd_add udp::65042-:42'), '')
        self.assertEquals(self.hmc('hostfwd_remove udp::65042'),
                          'host forwarding rule for udp::65042 removed\r\n')

    def test_qmp_hostfwd_ipv4_functional_errors(self):
        """Verify handling of various kinds of errors given valid addresses."""
        self.vm.add_args('-nodefaults',
                         '-netdev', 'user,id=vnet',
                         '-device', 'virtio-net,netdev=vnet')
        self.vm.launch()
        self.assertEquals(self.hmc('hostfwd_remove ::65022'),
                          'host forwarding rule for ::65022 not found\r\n')
        self.assertEquals(self.hmc('hostfwd_add udp::65042-:42'), '')
        self.assertEquals(self.hmc('hostfwd_add udp::65042-:42'),
                          "Could not set up host forwarding rule" + \
                          " 'udp::65042-:42': Address already in use\r\n")
        self.assertEquals(self.hmc('hostfwd_remove ::65042'),
                          'host forwarding rule for ::65042 not found\r\n')
        self.assertEquals(self.hmc('hostfwd_remove udp::65042'),
                          'host forwarding rule for udp::65042 removed\r\n')
        self.assertEquals(self.hmc('hostfwd_remove udp::65042'),
                          'host forwarding rule for udp::65042 not found\r\n')

    def test_qmp_hostfwd_ipv4_parsing_errors(self):
        """Verify handling of various kinds of parsing errors."""
        self.vm.add_args('-nodefaults',
                         '-netdev', 'user,id=vnet',
                         '-device', 'virtio-net,netdev=vnet')
        self.vm.launch()
        self.assertEquals(self.hmc('hostfwd_remove abc::42'),
                          "Invalid format: bad protocol name 'abc'\r\n")
        self.assertEquals(self.hmc('hostfwd_add abc::65022-:22'),
                          "Invalid host forwarding rule 'abc::65022-:22'" + \
                          " (bad protocol name 'abc')\r\n")
        self.assertEquals(self.hmc('hostfwd_add :foo'),
                          "Invalid host forwarding rule ':foo'" + \
                          " (missing host-guest separator)\r\n")
        self.assertEquals(self.hmc('hostfwd_add :a.b.c.d:66-:66'),
                          "Invalid host forwarding rule ':a.b.c.d:66-:66'" + \
                          " (For host address: address resolution failed for" \
                          " 'a.b.c.d:66': Name or service not known)\r\n")
        self.assertEquals(self.hmc('hostfwd_add ::66-a.b.c.d:66'),
                          "Invalid host forwarding rule '::66-a.b.c.d:66'" + \
                          " (For guest address: address resolution failed" + \
                          " for 'a.b.c.d:66': Name or service not known)\r\n")
        self.assertEquals(self.hmc('hostfwd_add ::-1-foo'),
                          "Invalid host forwarding rule '::-1-foo'" + \
                          " (For host address: error parsing port in" + \
                          " address ':')\r\n")
        self.assertEquals(self.hmc('hostfwd_add ::66-foo'),
                          "Invalid host forwarding rule '::66-foo' (For" + \
                          " guest address: error parsing address 'foo')\r\n")
        self.assertEquals(self.hmc('hostfwd_add ::66-:0'),
                          "Invalid host forwarding rule '::66-:0'" + \
                          " (For guest address: invalid port '0')\r\n")
