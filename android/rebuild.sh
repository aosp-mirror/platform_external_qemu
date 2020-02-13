#!/bin/sh
#
# this script is used to rebuild all QEMU binaries for the host
# platforms.
#
# assume that the device tree is in TOP
#
. $(dirname "$0")/scripts/unix/utils/common.shi
PROGDIR=$(dirname $0)
set -e
export LANG=C
export LC_ALL=C

PYTHON=python

exists() {
    # Returns true if $1 is on the path.
    command -v "$1" >/dev/null 2>&1
}

check_python_version() {
    PYV=$($PYTHON -c "import sys;t='{v[0]}'.format(v=list(sys.version_info[:2]));sys.stdout.write(t)")
    if [ "${PYV}" == "2" ]; then
        warn "Python 2, has been deprecated! See: https://python3statement.org/ and go/python-upgrade please upgrade to python 3!"
        install_deps enum enum34 # Needed compatibility layer for Python2..
    fi
}

python_module_exist() {
    #  returns true if $1 can be imported using the default python interpreter
    $PYTHON -c "import $1" >/dev/null 2>&1
}

install_deps() {
    # Checks to see if the given import is availabe from the current python install
    # if not, it will install the dependency using pip or try to use easy_install.
    # If that fails, you will have to install
    local import=$1
    local package=$2
    if ! python_module_exist ${import}; then
        echo "Missing dependency ${package} attempting to install dependency in user packages."
        python_module_exist pip || panic "No installer available (https://pip.pypa.io/en/stable/installing/)! Please run: curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py && $PYTHON get-pip.py"
        $PYTHON -m pip install --user $package
    fi
}

exists python3 && PYTHON=python3
exists $PYTHON || panic "No python interpreter available, please install python 3!"

# Check for python 3, and install compatibility layers if needed.
check_python_version

# Install needed depencies..
install_deps absl absl-py      # Used to parse parameters.
install_deps requests requests # We make outgoing calls..

$PYTHON $PROGDIR/build/python/cmake.py --noshowprefixforinfo $*
