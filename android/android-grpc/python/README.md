# Embedded Emulator gRPC

This contains a series of classes to interact with gRPC android emulators

You can run the tests as follows:

    $ tox

## Development

If you wish to do development you can create a virtual
environment by running:

    $ . ./configure.sh

You can now run the tests by executing

    $ pytest

You can use the standard pytest commands to run specific tests, and
reconfigure the runner by modifying tox.ini

## Usage

To discover all emulators:


```python
   from aemu.discovery.emulator_discovery import EmulatorDiscovery

   discovery = EmulatorDiscovery()
   emulators = discovery.emulators()
```

What you usually want to is get a grpc stub to interact with the emulator.
For example to send an sms to the first discovered emulator:

```python
   from aemu.discovery.emulator_discovery import get_default_emulator
   from aemu.proto.emulator_controller_pb2 import SmsMessage

   stub = get_default_emulator().get_emulator_controller()
   stub.sendSms(SmsMessage(srcAddress="(650) 555-1212", text="Hello!"))
```

The following services are available:

- `get_emulator_controller()`: a stub to the emulator controller service.
- `get_snapshot_service()`: a stub to the snapshot service.
- `get_waterfall_service()`:  a stub to the waterfall service. Usually not available.
- `get_rtc_service()`: a stub to the WebRTC engine. Usually only supported on linux.
