# Emulated Bluetooth

This contains an example of a working heartrate monitor. The samples
are guaranteed to work with the (limited) python interpreter that
is available here: https://android.googlesource.com/platform/prebuilts/python/

## Development

If you wish to do development you can create a virtual
environment by running:

    python -m venv .venv
    source .venv/bin/activate

You will find a set of wheels that need to be installed.
If you extracted this from a zip, you will need to install all the wheels that
come in this zip file.

Next you can run the sample with:

    python -m emulated_bluetooth.hr

