# Rules for prebuilt breakpad client library.

BREAKPAD_TOP_DIR := $(BREAKPAD_PREBUILTS_DIR)/$(HOST_OS)-$(HOST_ARCH)

$(call define-emulator-prebuilt-library,\
    emulator-libbreakpad_client, \
    $(BREAKPAD_TOP_DIR)/lib/libbreakpad_client.a)

BREAKPAD_STATIC_LIBRARIES := emulator-libbreakpad_client
BREAKPAD_INCLUDES := $(BREAKPAD_TOP_DIR)/include/breakpad
BREAKPAD_LDLIBS :=
ifeq ($(HOST_OS),windows)
    BREAKPAD_LDLIBS += -lstdc++
endif
