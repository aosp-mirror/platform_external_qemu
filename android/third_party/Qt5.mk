QT_TOP_DIR := $(QT_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)
QT_TOP64_DIR := $(QT_PREBUILTS_DIR)/$(BUILD_TARGET_OS)-x86_64
QT_MOC_TOOL := $(QT_TOP64_DIR)/bin/moc
QT_RCC_TOOL := $(QT_TOP64_DIR)/bin/rcc
# Special-case: the 'uic' tool depends on Qt5Core: always ensure that the
# version that is being used is from the prebuilts directory. Otherwise
# the executable may fail to start due to dynamic linking problems.
QT_UIC_TOOL_LDPATH := $(QT_TOP64_DIR)/lib
QT_UIC_TOOL := $(QT_TOP64_DIR)/bin/uic

QT_LDLIBS := -lQt5Widgets -lQt5Gui -lQt5Core -lQt5Svg
ifeq ($(BUILD_TARGET_OS),windows)
    # On Windows, linking to mingw32 is required. The library is provided
    # by the toolchain, and depends on a main() function provided by qtmain
    # which itself depends on qMain(). These must appear in LDFLAGS and
    # not LDLIBS since qMain() is provided by object/libraries that
    # appear after these in the link command-line.
    QT_LDFLAGS += \
            -L$(QT_TOP_DIR)/bin \
            -lmingw32 \
            $(QT_TOP_DIR)/lib/libqtmain.a
else
    QT_LDFLAGS := -L$(QT_TOP_DIR)/lib
endif

QT_INCLUDE_DIR := $(QT_PREBUILTS_DIR)/common/include
QT_TARGET_INCLUDE_DIR := $(QT_TOP_DIR)/include.system
# Make sure the target-specific include directory is first one here, as that's
# where target-specific headers are.
QT_INCLUDES := \
    $(QT_TARGET_INCLUDE_DIR) \
    $(QT_TARGET_INCLUDE_DIR)/QtCore \
    $(QT_INCLUDE_DIR) \
    $(QT_INCLUDE_DIR)/QtCore \
    $(QT_INCLUDE_DIR)/QtGui \
    $(QT_INCLUDE_DIR)/QtSvg \
    $(QT_INCLUDE_DIR)/QtWidgets \
