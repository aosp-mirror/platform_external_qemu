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

import argparse

from google.protobuf import empty_pb2

from aemu.proto.emulator_controller_pb2 import ClipData
from aemu.discovery.emulator_discovery import get_default_emulator

_EMPTY_ = empty_pb2.Empty()


def stream_clipboard(stub, args):
    for clip in stub.streamClipboard(_EMPTY_):
        print(clip.text)


def get_clipboard(stub, args):
    clip = stub.getClipboard(_EMPTY_)
    print(clip.text)


def set_clipboard(stub, args):
    clip = ClipData(text=args.text)
    stub.setClipboard(clip)


def main():
    parser = argparse.ArgumentParser(description="Sample to interact with clipboard")
    parser.add_argument(
        "-v",
        "--verbose",
        dest="verbose",
        action="store_true",
        help="Set verbose logging",
    )

    subparsers = parser.add_subparsers()
    set_clip = subparsers.add_parser("send", help="Set the emulator clipboard.")
    set_clip.add_argument(
        "text", help="Utf-8 encoded string to be send to the clipboard.",
    )
    set_clip.set_defaults(func=set_clipboard)

    get_clip = subparsers.add_parser(
        "read", help="Retrieve the contents of the clipboard."
    )
    get_clip.set_defaults(func=get_clipboard)

    stream_clip = subparsers.add_parser("stream", help="Stream the emulator clipboard")
    stream_clip.set_defaults(func=stream_clipboard)

    # Create a client
    stub = get_default_emulator().get_emulator_controller()

    args = parser.parse_args()
    if hasattr(args, "func"):
        args.func(stub, args)
    else:
        parser.print_help()


if __name__ == "__main__":
    # execute only if run as a script
    main()
