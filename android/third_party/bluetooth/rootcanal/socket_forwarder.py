#  Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# A simple socket forwarder that can be used to connect root canal to the emulator.
# This can be used while the emulator bluetooth module is still under development.
#
# python3 socket_forwarder.py -h
#
import asyncio
import argparse


async def pipe(reader, writer):
    """A Pipe that reads from the reader and writes the contents to the writer."""
    try:
        while not reader.at_eof():
            writer.write(await reader.read(2048))
    finally:
        writer.close()


async def fwd_client(host, port):
    """A client that connects to the given host and port."""
    print("Connecting to {}:{}".format(host, port))
    reader, writer = await asyncio.open_connection(host, port)
    return reader, writer


async def connect_server_sockets(emu_port, root_port):
    emu_read, emu_write = await fwd_client("localhost", emu_port)
    rcl_read, rcl_write = await fwd_client("localhost", root_port)
    pipe1 = pipe(emu_read, rcl_write)
    pipe2 = pipe(rcl_read, emu_write)
    await asyncio.gather(pipe1, pipe2)


def main():
    parser = argparse.ArgumentParser(
        description="Bridge two server sockets together. This can be used to establish a pipe between two server sockets."
        "This is mainly useful to connect root canal to the emulator. For example the emulator can be launched as:\n"
        "",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--fst",
        type=int,
        default=9321,
        help="First port to connect to. Usually the emulator.",
    )
    parser.add_argument(
        "--snd",
        type=int,
        default=6402,
        help="Second port to connect to. Usually root-canal.",
    )

    args = parser.parse_args()
    loop = asyncio.get_event_loop()
    asyncio.run(connect_server_sockets(args.fst, args.snd))


if __name__ == "__main__":
    main()