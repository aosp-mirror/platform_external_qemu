executable('ivshmem-client', files('ivshmem-client.c', 'main.c'), genh,
           dependencies: [glib, threads],
           build_by_default: targetos == 'linux',
           install: false)
