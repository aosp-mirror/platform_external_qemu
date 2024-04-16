executable('ivshmem-server', files('ivshmem-server.c', 'main.c'), genh,
           dependencies: [qemuutil, print_stub, rt],
           build_by_default: targetos == 'linux',
           install: false)
