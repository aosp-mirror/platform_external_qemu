if libiscsi.found()
  executable('vhost-user-scsi', files('vhost-user-scsi.c'),
             dependencies: [qemuutil, print_stub, libiscsi, vhost_user],
             build_by_default: targetos == 'linux',
             install: false)
endif
