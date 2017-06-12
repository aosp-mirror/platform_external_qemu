
#ifndef MAC_SEGV_HANDLER
#define MAC_SEGV_HANDLER

typedef void (*mac_bad_access_callback_t)(void* faultvaddr);

extern "C" {

void install_bad_access_callback(mac_bad_access_callback_t f);
void setup_mac_segv_handler();

}

#endif
