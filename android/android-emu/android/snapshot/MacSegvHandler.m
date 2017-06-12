#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <pthread.h>
#include <assert.h>
#include <mach/mach.h>

#include <sys/types.h>
#include <sys/mman.h>


extern __attribute__((visibility("default"))) kern_return_t
catch_exception_raise(mach_port_t exception_port,
                      mach_port_t thread,
                      mach_port_t task,
                      exception_type_t exception,
                      exception_data_t code_vector,
                      mach_msg_type_number_t code_count) {
    fprintf(stderr, "%s: enter\n", __func__);
    if (exception == EXC_BAD_ACCESS) {
        fprintf(stderr, "BAD_ACCESS %d\n", exception);
        thread_state_flavor_t flavor = x86_EXCEPTION_STATE64;
        x86_exception_state64_t exc_state; 
        mach_msg_type_number_t exc_state_count = x86_EXCEPTION_STATE64_COUNT;
        thread_get_state(thread, x86_EXCEPTION_STATE64, (thread_state_t)&exc_state, &exc_state_count);
        fprintf(stderr, "BAD_ACCESS @ 0x%llx...should unprotecting 0x%llx\n", exc_state.__faultvaddr, exc_state.__faultvaddr & ~0xFFF);
        mprotect(exc_state.__faultvaddr & ~0xFFF, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
    }
    return KERN_SUCCESS;  // loops infinitely...
}

void *exception_handler(void *arg)
{
extern boolean_t exc_server();
mach_port_t port = (mach_port_t) arg;
fprintf(stderr, "%s: call\n", __func__);
mach_msg_server(exc_server, 2048, port, 0);
fprintf(stderr, "%s: abort!!!!!!!!!!!!!\n", __func__);
}

void setup_mac_segv_handler_native()
{
    static mach_port_t exception_port = MACH_PORT_NULL;
    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &exception_port);
    mach_port_insert_right(mach_task_self(), exception_port, exception_port, MACH_MSG_TYPE_MAKE_SEND);
    task_set_exception_ports(mach_task_self(), EXC_MASK_BAD_ACCESS, exception_port, EXCEPTION_DEFAULT, MACHINE_THREAD_STATE);
    pthread_t returned_thread;
    pthread_create(&returned_thread, NULL, exception_handler, (void*) exception_port);
    fprintf(stderr, "%s: done\n", __func__);
}

void test_crash()
{
    char* data = (char*)malloc(4096 * 2);
    data = (char*)(4096 * ((((uintptr_t)data) + 4096 - 1) / 4096));
    fprintf(stderr, "%s: data: %p\n", __func__, data);
    mprotect(data, 4096, PROT_NONE);
    data[234] = 0xff;
    fprintf(stderr, "%s: survived!\n", __func__);
}

