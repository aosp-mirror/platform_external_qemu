"""A VideoConnection connects to the webrtc broadcaster.

A VideoConnection will spin up the process that will do the actual webrtc
broadcasting. The process is repsonsible for executing the webrtc handshake
protocol, and sending the actual video streams.
"""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import subprocess
from threading import Thread
from absl import logging

import asyncio

class VideoConnection(asyncio.Protocol):
  """A Connection forwarder to the video encoder process."""

  VIDEO_TOPIC = "emulator-video"

  def __init__(self, webrtc_exec, port, pubsub, loop):
    self.pubsub = pubsub
    self.port = port
    self.loop = loop
    self.transport = None
    self.webrtc_exec = webrtc_exec
    self.remote_address = "VideoConnection:local"

  def connection_made(self, transport):
    logging.info("Video client connected : %s", self)
    self.pubsub.connect(self)
    self.pubsub.subscribe(VideoConnection.VIDEO_TOPIC, self)
    self.transport = transport

  def data_received(self, data):
    msg = data.decode()
    logging.info("Received from video client: %s", msg)
    asyncio.ensure_future(self.pubsub.execute(self, msg))

  def connection_lost(self, exc):
    logging.error("The video client is gone (crash?), due to %s", exc)
    self.pubsub.disconnect(self)

    # Launch it again!
    VideoConnection.launch_webrtc_connector(self.webrtc_exec, self.port)

  async def send(self, msg):
    """Sends the message to the webrtc process."""
    message = msg["msg"].encode()
    if self.transport:
      self.transport.write("{:04x}".format(len(message)).encode())
      self.transport.write(message)
    logging.info("Send %s to video adapter", message)

  @staticmethod
  def launch_webrtc_connector(webrtc, port):
    # launcher = "{} --port {}".format(webrtc, port)
    # process = "{} 2>&1 | egrep 'conductor'".format(launcher)
    process = [webrtc, "--port", str(port)]
    Thread(target=lambda: subprocess.run(process, shell=True)).start()
    logging.info("Launched %s", webrtc)

  @staticmethod
  def listen_for_video(port, webrtc, pubsub):
    loop = asyncio.new_event_loop()
    coro = loop.create_server(lambda: VideoConnection(webrtc, port, pubsub, loop),
                              "127.0.0.1", port)
    loop.run_until_complete(coro)

    Thread(target=lambda loop: loop.run_forever(), args=(loop,)).start()
    VideoConnection.launch_webrtc_connector(webrtc, port)
