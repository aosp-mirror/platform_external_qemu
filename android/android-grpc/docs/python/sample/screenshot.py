#  Copyright (C) 2019 The Android Open Source Project
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

"""A simple demonstration on how to construct a UI from the gRPC endpoint.

It uses the producer/consumer pattern to pull images from the gRPC stream.

The producer produces the frames from the gRPC endpoint.
The consumer displays the images on the UI

Note: Move the mouse around if the screen does not update.
"""
import argparse
import logging
import mmap
import sys
import time
from threading import Thread

import grpc

from queue import Queue

from threading import Thread
from tkinter import NW, Canvas, Tk

import aemu.proto.emulator_controller_pb2 as p
from aemu.discovery.emulator_discovery import get_default_emulator
from PIL import Image, ImageTk

IMAGE_FORMATS = {
    "RGB": p.ImageFormat.RGB888,
    "RGBA": p.ImageFormat.RGBA8888,
}

TRANSPORT = {
    "gRPC": p.ImageTransport.gRPC,
    "mmap": p.ImageTransport.mmap,
}


tmp_file = "/tmp/current_screen.img"


def prepare_shm(args):
    with open(args.mmap_file, "wb") as out:
        out.truncate(args.width * args.height * 4 + 1024)


def center_window(root, width=300, height=200):
    """Centers the tk window."""
    # get screen width and height
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()

    # calculate position x and y coordinates
    x = (screen_width / 2) - (width / 2)
    y = (screen_height / 2) - (height / 2)
    root.geometry("%dx%d+%d+%d" % (width, height, x, y))


def stream_screenshots(args):
    stub = get_default_emulator().get_emulator_controller()
    fmt = p.ImageFormat(
        format=IMAGE_FORMATS[args.format],
        width=args.width,
        height=args.height,
        display=0,
        transport=p.ImageTransport(
            channel=TRANSPORT[args.transport], handle="file://" + args.mmap_file
        ),
    )
    # We should get a continous sequence of frames..
    for img in stub.streamScreenshot(fmt):
        yield img


def _img_consumer(canvas, root, args):
    """Consumes the images from the queue, and displaying them on the ui."""
    use_mmap = args.transport == "mmap"
    mm = None
    if use_mmap:
        print("Using mmap: {}".format(args.mmap_file))
        f = open(args.mmap_file, "r+b")
        mm = mmap.mmap(f.fileno(), 0)

    emu = None
    visual = None

    for img in stream_screenshots(args):
        img_bytes = img.image
        if mm:
            mm.seek(0)
            img_bytes = mm.read()

        # Python has poor UI support, this takes up soo much time
        # that the type of call does not really make a difference.
        if emu:
            emu.frombytes(img_bytes)
            visual.paste(emu)
        else:
            # First time, create objects..
            emu = Image.frombytes(
                args.format, (img.format.width, img.format.height), img_bytes
            )
            visual = ImageTk.PhotoImage(emu)
            canvas.create_image(0, 0, anchor=NW, image=visual)
            canvas.image = visual
            canvas.pack()

        root.update()


def main():
    if sys.version_info[0] == 2:
        print("This sample only runs under Python3")
        sys.exit(1)

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--width",
        type=int,
        default=540,
        dest="width",
        help="Width of the retrieved images.",
    )
    parser.add_argument(
        "--height",
        type=int,
        default=960,
        dest="height",
        help="height of the retrieved images.",
    )
    parser.add_argument(
        "--format",
        default="RGBA",
        type=str.upper,
        choices=("RGB", "RGBA"),
        help="Imageformat to use when streaming screenshots.",
    )
    parser.add_argument(
        "--transport",
        default="gRPC",
        choices=("gRPC", "mmap"),
        help="Transport mechanism to use when retrieving images.",
    )
    parser.add_argument(
        "--mmap_file",
        default="/tmp/current_screen.img",
        help="Memory mapped file used to transport images.",
    )
    args = parser.parse_args()

    if args.transport == "mmap":
        prepare_shm(args)

    root = Tk()
    canvas = Canvas(root, width=args.width, height=args.height)
    canvas.pack()
    Thread(target=_img_consumer, args=[canvas, root, args]).start()
    center_window(root, args.width, args.height)
    root.mainloop()


if __name__ == "__main__":
    main()
