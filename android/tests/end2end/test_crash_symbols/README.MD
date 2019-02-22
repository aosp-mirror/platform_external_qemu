# Symbol crash test

This test validates that we:

- Generate usable symbols
- Minidump files correspond to the generated symbols

This test will only work when the following conditions are met:

- The emulator has been build with crash set to prod/staging
- Symbols have been generated during the build

## How?

This contains a test that validates that the generate symbols from breakpad contain stacktraces that we are looking for. The test does the following:

1. Next we setup a local symbol server by copying the symbol files generated during build to an approriate destination
2. Start an emulator instance, based on the first availabe avd.
    - Make sure this is not an ARM image, they take long time to boot.
    - We pick a random telnet/adb port
3. Wait until the emulator is booted.
4. Connect to the telnet console.
5. Send the crash command.
6. Decode the minidump

Since we have called the crash command we know what the trace should look like, and validate that we have a properly translated crash.

