#ifndef QEMU_THREAD_LOCAL_H
#define QEMU_THREAD_LOCAL_H

/* MinGW's implementation of __thread is very slow as of now: it's compiled into
 * a call to __emutls_get_address(), which itself calls pthread_getspecific(),
 * which is a very slow wrapper over Windows's Tls*() APIs.
 * Let's create a set of lightweight wrappers instead to use in
 * performance-critical places.
 *
 * The use of these macros is straightforward: instead of declaring a variable
 * like
 *      __thread int var;
 * use
 *      QEMU_THREAD_LOCAL_DECLARE(int, var);
 *
 * Read values with QEMU_THREAD_LOCAL_GET(), write with QEMU_THREAD_LOCAL_SET()
 *
 * There's an optimization for variables larger than pointer size: use
 * QEMU_THREAD_LOCAL_GET_PTR(var) to get a pointer to the variable in memory
 * instead of copying it with QEMU_THREAD_LOCAL_GET():
 *      big_struct* pval = QEMU_THREAD_LOCAL_GET_PTR(val);
 *      pval->blah = foo;
 *
 * For non-Windows cases it's all compiled back to the regular __thread use.
 */

#if !defined(_WIN32)

#define QEMU_THREAD_LOCAL_EXTERN(type, name) extern __thread type name;
#define QEMU_THREAD_LOCAL_DECLARE(type, name) __thread type name;
#define QEMU_THREAD_LOCAL_DECLARE_INIT(type, name, init) __thread type name = (init);

#define QEMU_THREAD_LOCAL_GET(name) (name)
#define QEMU_THREAD_LOCAL_GET_PTR(name) (&(name))
#define QEMU_THREAD_LOCAL_SET(name, value) ((name) = (value))

#else  /* _WIN32 */

#include "qemu/notify.h"
#include "qemu/thread.h"

/****************************************************************************/
/*
 * Private definitions
 *
 */

/* Define a struct to hold the thread-local variable and the corresponding
 * exit notifier per-thread.
 *
 * Note: struct is used in several macros, so we can't define it with a name -
 *       C forbids redefinitions even if struct bodies are the same.
 */
#define QEMU_THREAD_LOCAL_BIG_STRUCT(name) \
    struct { __typeof__(name ## _tls_empty) item; Notifier exit_notifier; }

#define QEMU_THREAD_LOCAL_DECLARE_COMMON(type, name) \
    DWORD name ## _tls_key; \
    type name ## _tls_empty; \
    void qemu_tls_free_ ## name(Notifier *notifier, void *data) { \
        void* val = TlsGetValue(name ## _tls_key); \
        if (val) { \
            qemu_thread_atexit_remove(&(((QEMU_THREAD_LOCAL_BIG_STRUCT(name)*)val)->exit_notifier)); \
            free(val); \
            TlsSetValue(name ## _tls_key, NULL); \
        } \
    } \

#define QEMU_THREAD_LOCAL_ALLOC_BIG(name) \
    ({ \
        QEMU_THREAD_LOCAL_BIG_STRUCT(name)* ptr = calloc(1, sizeof(QEMU_THREAD_LOCAL_BIG_STRUCT(name))); \
        TlsSetValue(name ## _tls_key, ptr); \
        ptr->exit_notifier.notify = &(qemu_tls_free_ ## name); \
        qemu_thread_atexit_add(&(ptr->exit_notifier)); \
        __typeof__(name ## _tls_empty)* val = &(ptr->item); \
        val; \
    })

/****************************************************************************/
/*
 * Public interface
 */

#define QEMU_THREAD_LOCAL_EXTERN(type, name) \
    extern DWORD name ## _tls_key; \
    extern type name ## _tls_empty; \
    extern void qemu_tls_free_ ## name(Notifier *notifier, void *data); \

#define QEMU_THREAD_LOCAL_DECLARE(type, name) \
    QEMU_THREAD_LOCAL_DECLARE_COMMON(type, name); \
    static void __attribute__((constructor)) qemu_tls_init_ ## name(void) { \
        name ## _tls_key = TlsAlloc(); \
    }

#define QEMU_THREAD_LOCAL_DECLARE_INIT(type, name, value) \
    QEMU_THREAD_LOCAL_DECLARE_COMMON(type, name); \
    static void __attribute__((constructor)) qemu_tls_init_ ## name(void) { \
        name ## _tls_key = TlsAlloc(); \
        QEMU_THREAD_LOCAL_SET(name, value); \
    }

#define QEMU_THREAD_LOCAL_GET_PTR(name) \
    ({ \
        /* make sure it's a big variable, otherwise *_GET_PTR makes no sense */ \
        { \
            char __attribute__((unused)) \
                assert_thread_local_var_is_big_ ## name [ \
                    sizeof(__typeof__(name ## _tls_empty)) > sizeof(void*) \
                        ? 1 : -1 ]; \
        } \
        __typeof__(name ## _tls_empty)* val = \
            (__typeof__(val)) TlsGetValue(name ## _tls_key); \
        if (!val) { \
            val = QEMU_THREAD_LOCAL_ALLOC_BIG(name); \
        } \
        val; \
    })

#define QEMU_THREAD_LOCAL_GET(name) \
    ({ \
        __typeof__(name ## _tls_empty) res; \
        void* val = TlsGetValue(name ## _tls_key); \
        if (sizeof(__typeof__(res)) > sizeof(void*)) { \
            if (val) { \
                res = *(__typeof__(res)*)val; \
            } else { \
                res = name ## _tls_empty; \
            } \
        } else { \
            res = *(__typeof__(res)*)&val; \
        } \
        res; \
    })

#define QEMU_THREAD_LOCAL_SET(name, value) \
    do { \
        void* val; \
        if (sizeof(__typeof__(name ## _tls_empty)) > sizeof(void*)) { \
            val = TlsGetValue(name ## _tls_key); \
            if (!val) { \
                val = QEMU_THREAD_LOCAL_ALLOC_BIG(name); \
            } \
            *(__typeof__(name ## _tls_empty)*)val = value; \
        } else { \
            *(__typeof__(name ## _tls_empty)*)&val = value; \
            TlsSetValue(name ## _tls_key, val); \
        } \
    } while (0)

#endif  /* _WIN32 */

#endif /* QEMU_THREAD_LOCAL_H */
