#ifndef SLIRP_PROXY_H
#define SLIRP_PROXY_H

/* Type of a structure holding the interface to TCP proxy to be used
 * with the SLIRP stack. */
typedef struct SlirpProxyOps SlirpProxyOps;

/* Type of a function that is invoked when the proxy connection
 * attempt has completed. On success, |fd| will be >= 0 and the
 * descriptor for the active socket to use, |af| will be either
 * AF_INET or AF_INET6. On failure |fd| will be negative.
 * |connect_opaque| is the user-provided pointer that was passed
 * to slirp_proxy_try_connect().
 */
typedef void (SlirpProxyConnectFunc)(void* connect_opaque, int fd, int af);

/* Global SLIRP pointer to the proxy. If not-NULL, its methods will be
 * used to try to proxify TCP connections. */
extern const struct SlirpProxyOps *slirp_proxy;

/* A structure used to model the interface for any proxy implementation.
 * The default version will always return false in its |try_connect|
 * method. */
struct SlirpProxyOps {
    /* Initiate a proxy connection to address |addr|. |connect_func| is the
    * function that will be called to report the state of the connection,
    * and |connect_opaque| will be its first parameter, and used to
    * uniquely identify the connection (it should not be NULL).
    * Returns false if this address cannot be proxifed, in which case
    * a normal connection attempt should be performed. Return true
    * otherwise. */
    bool (*try_connect)(const struct sockaddr_storage *addr,
                        SlirpProxyConnectFunc *connect_func,
                        void *connect_opaque);

    /* Remove a proxy connection that was previously started with
    * a call to try_connect(). |connect_opaque| should be the same parameter
    * as the one passed to the function. */
    void (*remove)(void *connect_opaque);
};

extern bool http_proxy_on;

#endif /* SLIRP_PROXY_H */
