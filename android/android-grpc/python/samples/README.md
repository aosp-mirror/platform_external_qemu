# Emulated Bluetooth

This contains an example of a working heartrate monitor. The samples
are guaranteed to work with the (limited) python interpreter that
is available here: https://android.googlesource.com/platform/prebuilts/python/

The wheels contained in the zip file are build against pyhthon 3.10.6, which
you can obtain here: https://www.python.org/downloads/release/python-3106/

## Development

If you wish to do development you can create a virtual
environment by running:

    cd samples
    python -m venv .venv
    source .venv/bin/activate

You will find a set of wheels that need to be installed.
If you extracted this from a zip, you will need to install all the wheels that
come in this zip file.

    pip install ../*whl

The emulator must be launched with netsim:

    $ANDROID_SDK_ROOT/emulator/emulator @MY_AVD -packet-streamer-endpoint default

Next you can run the sample with:

    python emulated_bluetooth/hr.py

