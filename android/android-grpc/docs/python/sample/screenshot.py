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

import sys
from threading import Lock, Thread
from tkinter import Label, Tk

from PIL import Image, ImageTk

import proto.emulator_controller_pb2 as p
import proto.emulator_controller_pb2_grpc
import google.protobuf.text_format
from channel.channel_provider import getEmulatorChannel
from google.protobuf import empty_pb2

SAMPLE_WIDTH = 540
SAMPLE_HEIGHT = 960


def center_window(root, width=300, height=200):
    """Centers the tk window."""
    # get screen width and height
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()

    # calculate position x and y coordinates
    x = (screen_width / 2) - (width / 2)
    y = (screen_height / 2) - (height / 2)
    root.geometry("%dx%d+%d+%d" % (width, height, x, y))


def _img_producer(queue, lock):
    """Produces the image stream, from the gRPC endpoint."""
    # Open a grpc channel
    channel = getEmulatorChannel()

    # Create a client
    stub = proto.emulator_controller_pb2_grpc.EmulatorControllerStub(channel)

    fmt = p.ImageFormat(
        format=p.ImageFormat.RGBA8888, width=SAMPLE_WIDTH, height=SAMPLE_HEIGHT, display=0
    )
    i = 0
    for img in stub.streamScreenshot(fmt):
        lock.acquire()
        i = i + 1
        fmt = google.protobuf.text_format.MessageToString(img.format, as_one_line=True)
        print("Enquing image {}:{}- {}".format(i, img.seq, fmt))
        queue.append(img)
        lock.release()


def _img_consumer(queue, lock, label, root):
    """Consumes the images from the queue, and displaying them on the ui."""
    while True:
        lock.acquire()
        if not queue:
            lock.release()
            continue

        img = queue.pop(0)
        if len(queue) > 0:
            print("Dropping {} frames.".format(len(queue)))
        queue.clear()
        lock.release()


        if img.format.width != 0:
            # Note, this thing is slow and probably cannot keep up..
            emu = Image.frombytes("RGBA", (img.format.width, img.format.height), img.image)
            visual = ImageTk.PhotoImage(emu)
            label.configure(image=visual)
            label.image = visual
            label.pack()
            root.update()
        else:
            print("Ignorning empty image.")
            label.image = None
            label.configure(image=None)


def main():
    if sys.version_info[0] == 2:
        print("This sample only runs under Python3")
        sys.exit(1)
    root = Tk()
    label = Label(root)
    queue = []
    lock = Lock()
    label.pack()
    Thread(target=_img_producer, args=[queue, lock]).start()
    Thread(target=_img_consumer, args=[queue, lock, label, root]).start()

    center_window(root, SAMPLE_WIDTH, SAMPLE_HEIGHT)
    root.mainloop()


if __name__ == "__main__":
    main()
