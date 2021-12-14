#!/bin/bash

PROGDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
QT_SRC_DIR=$PROGDIR/src/qt-everywhere-src-6.2.1
QT_BUILD_DIR=$PROGDIR/build-darwin-aarch64
QT_INSTALL_DIR=$PROGDIR/install-darwin-aarch64
QT_PREBUILTS_DIR=$HOME/emu-master-dev/prebuilts/android-emulator-build/qt/darwin-aarch64

OPT_SKIP_CONFIGURE=
OPT_DRY_RUN=
function parse_args() {
    while [ -n "$1" ]; do
        case "$1" in
            "--skip-configure")
                OPT_SKIP_CONFIGURE=1
                ;;
            "--dry-run")
                OPT_DRY_RUN=1
                echo "DRY RUN"
                ;;
            "*")
               panic "Unrecognized option [$1]" 
               ;;
        esac
        shift
    done
}

var_value () {
    eval printf %s \"\$$1\"
}

_var_quote_value () {
    printf %s "$1" | sed -e "s|'|\\'\"'\"\\'|g"
}

var_append () {
    local _var_append_varname
    _var_append_varname=$1
    shift
    if test "$(var_value $_var_append_varname)"; then
        eval $_var_append_varname=\$$_var_append_varname\'\ $(_var_quote_value "$*")\'
    else
        eval $_var_append_varname=\'$(_var_quote_value "$*")\'
    fi
}

function run() {
    echo "> $@"
    if [ -z "$OPT_DRY_RUN" ]; then
        $@
    fi
}

function panic() {
    echo "$@"
    exit 1
}

function get_build_num_cores() {
    sysctl -n hw.ncpu 2>/dev/null || echo 1
}

function run_configure() {
    EXTRA_CONFIGURE_FLAGS=
    var_append EXTRA_CONFIGURE_FLAGS \
                -opensource \
                -confirm-license \
                -force-debug-info \
                -release \
                -shared \
                -nomake examples \
                -nomake tests \
                -nomake tools \
                -skip qtdoc \
                -skip qttranslations \
                -skip qttools \
                -no-webengine-pepper-plugins \
                -no-webengine-printing-and-pdf \
                -no-webengine-webrtc \
                -no-strip \
                -qtlibinfix AndroidEmu \
                -no-framework \
                -no-webengine-spellchecker
    (
        run mkdir -p "$QT_BUILD_DIR" &&
        run cd "$QT_BUILD_DIR" &&
        run $QT_SRC_DIR/configure -prefix $QT_INSTALL_DIR $EXTRA_CONFIGURE_FLAGS
    ) || panic "Could not configure Qt build!"
}

function run_make() {
    QT_MODULES="qtbase qtsvg qtimageformats qtdeclarative qtlocation qtwebchannel qtwebsockets qtwebengine"
    QT_MAKE_FLAGS="--parallel $(get_build_num_cores)"

    (
        run cd "$QT_BUILD_DIR" &&
        run cmake --build . $QT_MAKE_FLAGS --target $QT_MODULES
    ) || panic "Could not build Qt binaries!"

    (
        run cd "$QT_BUILD_DIR" &&
        for QT_MODULE in $QT_MODULES; do
            run cmake --install $QT_MODULE
        done
    ) || panic "Could not install Qt binaries!"
}

function copy_installed_files_to_prebuilts() {
    cp -r $QT_INSTALL_DIR $QT_PREBUILTS_DIR
}

parse_args $@

if [ -z "$OPT_SKIP_CONFIGURE" ]; then
    run_configure
else
    echo "Skipping configure step"
fi

run_make
copy_installed_files_to_prebuilts
