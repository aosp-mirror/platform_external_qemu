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
  # Returns true if $1 is on the path.
  command -v "$1" >/dev/null 2>&1
}

python_module_exist() {
    #  returns true if $1 can be imported using the default python interpreter
    python -c "import $1" > /dev/null 2>&1
}

install_deps() {
    # Checks to see if the given import is availabe from the current python install
    # if not, it will install the dependency using pip or try to use easy_install.
    # If that fails, you will have to install
    local import=$1
    local package=$2
    if ! python_module_exist ${import}; then
        echo "Missing dependency ${package} attempting to install dependency in user packages."
        python_module_exist pip || panic "No installer available (https://pip.pypa.io/en/stable/installing/)! Please run: curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py && python get-pip.py"
        python -m pip install --user $package
    fi
}


exists python || panic "No python interpreter available, please install python!"
install_deps absl absl-py
install_deps requests requests
install_deps enum enum34

python $PROGDIR/build/python/cmake.py --noshowprefixforinfo  $*
