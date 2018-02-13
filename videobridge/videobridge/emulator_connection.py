"""A Connection to the emulator telnet console.
"""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from threading import Thread
from absl import logging

import asyncio


class EmulatorConnection(asyncio.Protocol):
  """Connects to the emulator telnet console.

  An EmulatorConnection forwards all messages received from the
  topic EMULATOR_BROADCAST and forwards it to the telnet console.
  Responses are broadcast back to the EMULATOR_RECEIVE topic.

  It will authenticate immediately so you should definitely trust
  everyone on your pubsub system.
  """

  EMULATOR_BROADCAST = "emulator-console-out"
  EMULATOR_RECEIVE = "emulator"

  def __init__(self, pubsub, loop):
    self.pubsub = pubsub
    self.loop = loop
    self.transport = None
    self.remote_address = "Emulator"

  def connection_made(self, transport):
    logging.info("Connected to emulator: %s", self)
    self.pubsub.connect(self)
    self.pubsub.subscribe(EmulatorConnection.EMULATOR_RECEIVE, self)
    self.transport = transport

  async def auth(self, fname):
    logging.info("Authenticating using %s", fname)
    with open(fname[1:-1], "r") as authfile:
      token = authfile.read()
      await self.send({"msg": "auth {}".format(token)})

  def data_received(self, data):
    msg = data.decode()
    if "Android Console: you can find your <auth_token> in" in msg:
      asyncio.async(self.auth(msg.split("\n")[-3].strip()))
      return

    asyncio.async(self.pubsub.publish(self.EMULATOR_BROADCAST, msg))

  def connection_lost(self, exc):
    logging.error("The emulator is gone!")
    self.pubsub.disconnect(self)
    self.loop.stop()

  async def send(self, msg):
    """Sends the message directly to the video connection"""
    message = msg["msg"]
    if self.transport:
      self.transport.write("{}\n".format(message).encode())

  @staticmethod
  def connect_to_emulator(port, pubsub):
    """Connects to the telnet console on the given port.

    Args:
      port: The port to which to connect to the emulator.
      pubsub: The pubsub system used to communicate
    """
    loop = asyncio.new_event_loop()
    coro = loop.create_connection(lambda: EmulatorConnection(pubsub, loop),
                                  "127.0.0.1", port)
    loop.run_until_complete(coro)
    t = Thread(target=lambda loop: loop.run_forever(), args=(loop,))
    t.start()
