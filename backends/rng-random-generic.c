/*
 * QEMU Random Number Generator Backend for Windows
 *
 * Copyright 2018 The Android Open Source Project
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "qemu/osdep.h"
#include "sysemu/rng-random-generic.h"
#include "sysemu/rng-random.h"
#include "sysemu/rng.h"
#include "crypto/random.h"
#include "qapi/error.h"
#include "qapi/qmp/qerror.h"
#include "qemu/main-loop.h"

struct RngRandom
{
    RngBackend parent;
};

static QemuRngRandomGenericRandomFunc rng_random_generic_random_func;
void qemu_set_rng_random_generic_random_func(QemuRngRandomGenericRandomFunc setup_func)
{
    rng_random_generic_random_func = setup_func;
}

/**
 * A simple backend to request entropy on windows
 *
 */

static void read_random_bytes(void* buf, ssize_t size)
{
    if (rng_random_generic_random_func) {
        rng_random_generic_random_func(buf, size);
    }else if (qcrypto_random_bytes(buf, size, NULL)){
        printf("failed to read_random_bytes %d\n", (int)size);
    }
}

static gboolean entropy_available(void *opaque)
{
    RngRandom *s = RNG_RANDOM(opaque);

    while (!QSIMPLEQ_EMPTY(&s->parent.requests)) {
        RngRequest *req = QSIMPLEQ_FIRST(&s->parent.requests);
        ssize_t len;

        /* this will always succeed */
        read_random_bytes(req->data, req->size);

        req->receive_entropy(req->opaque, req->data, req->size);

        rng_backend_finalize_request(&s->parent, req);
    }

    return FALSE;
}

static void rng_random_request_entropy(RngBackend *b, RngRequest *req)
{
    RngRandom *s = RNG_RANDOM(b);

    if (QSIMPLEQ_EMPTY(&s->parent.requests)) {
        /* If there are no pending requests yet, we need to
         * install our timer. */
        g_timeout_add(1 /* ms */, entropy_available, s);
    }
}

static void rng_random_opened(RngBackend *b, Error **errp)
{
    RngRandom *s = RNG_RANDOM(b);

}


static void rng_random_init(Object *obj)
{
    RngRandom *s = RNG_RANDOM(obj);

}

static void rng_random_finalize(Object *obj)
{
    RngRandom *s = RNG_RANDOM(obj);

}

static void rng_random_class_init(ObjectClass *klass, void *data)
{
    RngBackendClass *rbc = RNG_BACKEND_CLASS(klass);

    rbc->request_entropy = rng_random_request_entropy;
    rbc->opened = rng_random_opened;
}

static const TypeInfo rng_random_info = {
    .name = TYPE_RNG_RANDOM,
    .parent = TYPE_RNG_BACKEND,
    .instance_size = sizeof(RngRandom),
    .class_init = rng_random_class_init,
    .instance_init = rng_random_init,
    .instance_finalize = rng_random_finalize,
};

static void register_types(void)
{
    type_register_static(&rng_random_info);
}

type_init(register_types);
