# Symbol crash test

This test validates that we:

- Generate usable symbols
- Minidump files correspond to the generated symbols

This test will only work when the following conditions are met:

- The emulator has been build with crash set to prod/staging
- Symbols have been generated during the build
- You have access to these symbols.

## How?

This contains a test that validates that the generate symbols from breakpad contain stacktraces that we are looking for. The test does the following:

1. We setup a local symbol server by copying the symbol files generated during build to an approriate destination
2. Start an emulator instance that launches an arm instance with a kernel that can sort of boot (not really important to us)
3. Wait until the telnet console to the emulator is available.
4. Connect to the telnet console.
5. Send the crash command.
6. Quickly copy the minidump to a safe location.
7. Decode the minidump

Since we have called the crash command we know what the trace should look like, and validate that we have a properly translated crash.

Note: On windows it is possible that we do not immediately detect a crash, and will discover a hang through QT threads no longer responding.

You can also use this in a stand alone setting if you wish to decode traces.
