# Build rules for prebuilt libxml2 static library

LIBXML2_TOP_DIR := $(LIBXML2_PREBUILTS_DIR)/$(HOST_OS)-$(HOST_ARCH)

$(call define-emulator-prebuilt-library, \
    emulator-libxml2, \
    $(LIBXML2_TOP_DIR)/lib/libxml2.a)

# Required on Windows to indicate that the code will link against a static
# version of libxml2. Otherwise, the linker complains about undefined
# references to '_imp__xmlFree'.
LIBXML2_CFLAGS := -DLIBXML_STATIC

LIBXML2_INCLUDES := $(LIBXML2_TOP_DIR)/include
LIBXML2_STATIC_LIBRARIES := emulator-libxml2
