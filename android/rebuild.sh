#!/bin/sh
#
# this script is used to rebuild all QEMU binaries for the host
# platforms.
#
# assume that the device tree is in TOP
#
. $(dirname "$0")/scripts/unix/utils/common.shi
PROGDIR=`dirname $0`
set -e
export LANG=C
export LC_ALL=C

exists() {
  command -v "$1" >/dev/null 2>&1
}

python_module_exist() {
    python -c "import $1" > /dev/null 2>&1
}

exists python || panic "No python interpreter available, please install python!"

install_deps() {
    local import=$1
    local package=$2
    if ! python_module_exist ${import}; then
        echo "Missing dependency ${package} using ${use_pip}"
        if python_module_exist pip; then
            python -m pip install --user $package
        else
            python_module_exist easy_install || panic "Need pip, or easy_install to make ${package} available, please install ${package} manually"
            python -m easy_install --user $package
        fi
    fi
}

install_deps absl absl-py
install_deps urlfetch urlfetch

python $PROGDIR/build/python/cmake.py --noshowprefixforinfo  $*
