#!/usr/bin/env python3
"""TODO(jansene): DO NOT SUBMIT without one-line documentation for pubsub.

TODO(jansene): DO NOT SUBMIT without a detailed description of pubsub.
"""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import functools
import json

from absl import app
from absl import flags
from absl import logging

import asyncio
from videobridge.emulator_connection import EmulatorConnection
from videobridge.pubsub import Pubsub
from videobridge.video_connection import VideoConnection
import websockets

FLAGS = flags.FLAGS
flags.DEFINE_integer("port", 5000,
                     "Websocket port, web clients will connect to this port.")
flags.DEFINE_integer("emuport", 5554, "Console port of the emulator")
flags.DEFINE_integer("videoport", 8081,
                     "The webrtc binary will connect back to this port.")
flags.DEFINE_string(
    "webrtc", "peerconnection_client",
    "The executable that is responsible for streaming the video.")


class WebsocketConnection(object):

  def __init__(self, ws):
    self.ws = ws
    self.remote_address = ws.remote_address

  async def send(self, msg):
    await self.ws.send(json.dumps(msg))


async def handler(websocket, pubsub):
  logging.info("%s connected", websocket.remote_address)
  ws = WebsocketConnection(websocket)
  pubsub.connect(ws)
  try:
    while True:
      msg = await websocket.recv()
      await pubsub.execute(ws, msg)

  finally:
    logging.info("%s disconnected", websocket.remote_address)
    pubsub.disconnect(ws)


def main(argv):
  del argv  # Unused.

  pubsub = Pubsub()

  # Setup the emulator and video connections
  EmulatorConnection.connect_to_emulator(FLAGS.emuport, pubsub)
  VideoConnection.listen_for_video(FLAGS.videoport, FLAGS.webrtc, pubsub)

  # Setup the websocket connections.
  bound_handler = functools.partial(handler, pubsub=pubsub)
  start_server = websockets.serve(bound_handler, "0.0.0.0", FLAGS.port)
  asyncio.get_event_loop().run_until_complete(start_server)
  asyncio.get_event_loop().set_debug(enabled=True)
  asyncio.get_event_loop().run_forever()

def launch():
  app.run(main)

if __name__ == "__main__":
  launch()
