# Nothing yet.
android_pc_ss = ss.source_set()

android_pc_ss.add(when: 'CONFIG_PC', if_true: files(
  'hw/i386/goldfish_board.c',
  'hw/acpi/goldfish_acpi.c',))

android_pc_ss.add_all(hw_arch.get('i386', []))
hw_arch += {'i386': android_pc_ss}
