#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""A Connection to the emulator telnet console.
"""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os
import sys
import time
import datetime
from threading import Thread

try:
  from absl import logging
except ImportError:
  import logging

try:
  import asyncio
except ImportError:
  import trollius as asyncio
  from trollius import From


class EmulatorConnection(asyncio.Protocol):
  """Connects to the emulator telnet console.

  It will authenticate immediately.
  """

  def __init__(self, loop, callback):
    self.loop = loop
    self.transport = None
    self.callback = callback
    self.start = time.time()
    self.connected = False

  def is_connected(self):
    return self.connected

  def connection_made(self, transport):
    logging.info("Connected to emulator: %s", self)
    self.transport = transport
    self.connected = True
    self.start = time.time()

  @asyncio.coroutine
  def auth(self, fname):
    logging.info("Authenticating using %s", fname)
    with open(fname[1:-1], "r") as authfile:
      token = authfile.read()
      msg = "auth {}".format(token).strip()
      yield From(self.send(msg))

  def data_received(self, data):
    msg = data.decode()
    # send the auth token if needed
    if "Android Console: you can find your <auth_token> in" in msg:
      fname = msg.split("\n")[-3].strip()
      asyncio.async(self.auth(fname), self.loop)

    # do something with the received data
    if self.callback:
      asyncio.async(self.callback(msg))

  def connection_lost(self, exc):
    total = time.time() - self.start
    logging.error("The emulator is gone, we were alive for: %d seconds (%s)!",
                  total, str(datetime.timedelta(seconds=total)))
    self.connected = False
    self.loop.stop()
    try:
      # Best effort cleanup
      pending = asyncio.Task.all_tasks()
      for task in pending:
        task.cancel()
        # Now we should await task to execute its cancellation.
        # Cancelled task raises asyncio.CancelledError that we can suppress:
        try:
          self.loop.run_until_complete(task)
        except (asyncio.CancelledError):
          pass
    except RuntimeError:
      pass

  @asyncio.coroutine
  def send(self, msg):
    """Sends plain text to the emulator"""
    if self.connected:
      logging.info("Sending %s", msg)
      self.transport.write("{}\n".format(msg).encode())

  @staticmethod
  def connect(port, callback=None):
    """Connects to the telnet console on the given port and authenticates.

    Args:
      port:     The port to which to connect to the emulator.
      callback: Function to be called when the telnet console has data

    Returns:
      Thread that is running the event loop
    """
    loop = asyncio.new_event_loop()
    emulator = EmulatorConnection(loop, callback)
    coro = loop.create_connection(lambda: emulator, "127.0.0.1", port)
    loop.run_until_complete(coro)
    t = Thread(target=lambda loop: loop.run_forever(), args=(loop,))
    t.start()
    return emulator
