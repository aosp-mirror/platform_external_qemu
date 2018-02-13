"""A very simple pubsub module.

This is a very basic pubsub module that understands:

- Subscribe
- Unsubscribe
- Publish

A message published to a topic will be received by subscribers in the following
json envelope:

# { 'topic' : topic, 'type' : 'publish', 'msg' : message }

"""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import asyncio
import json
import threading

from absl import logging


class Pubsub(object):
  """A very simple pubsub module.

     The wire format for messages is json and looks as follows:

       {
         'topic' : [string],
         'type'  : [publish|subscribe|unsubscribe],
         'msg'   : [string]
       }

     The wire format are actual strings. Connections themselves are responsible
     for lower level transport protocols.

     A Connection is a type that has the following characterstics:

     - A send(str) method.
     - A remote_address attribute

     Connections need to connect before they can start
     sending and receiving messages.

     Connections need to disconnect when they become inactive.

  """

  def __init__(self):
    self.topic_to_connection = {}
    self.connection_to_topic = {}
    self.lock = threading.RLock()

  async def publish(self, topic, message):
    """Publish the given message to a topic.

    Every subscribed connection will receive the message.

    Args:
      topic:  The topic to broadcast to.
      message: The message to send.

    """
    # TODO(jansene): Validate connect has been called?
    with self.lock:
      for connection in self.topic_to_connection.get(topic, []):
        logging.info('Publish: %s - %s to: %s', topic, message,
                     connection.remote_address)
        msg = {'topic': topic, 'type': 'publish', 'msg': message}
        # TODO(jansene): Probably should be in some kind of catch block.
        await connection.send(msg)

  def unsubscribe(self, topic, connection):
    """Unsubscribe the connection from a topic.

    The connection will no longer receive messages published to the
    given topic.

    Args:
      topic:  The topic to unsubscribe from.
      connection: The connection to unsubscribe.

    """
    # TODO(jansene): Validate connect has been called?
    with self.lock:
      logging.info('Unsubscribe: %s - %s', topic, connection.remote_address)
      if topic in self.connection_to_topic.get(topic, []):
        self.connection_to_topic[topic].remove(topic)

      connected = self.topic_to_connection.get(topic, [])
      if connection in connected:
        connected.remove(connection)

      # Cleanup the mess
      if not connected:
        del self.topic_to_connection[topic]

  def subscribe(self, topic, connection):
    """Subscribe the connection to a topic.

    The connection will receive messages published to the
    given topic.

    Args:
      topic:  The topic to subscribe to.
      connection: The connection to subscribe.
    """
    # TODO(jansene): Validate connect has been called?
    with self.lock:
      logging.info('Subscribe: %s - %s', topic, connection.remote_address)
      self.connection_to_topic.get(connection).append(topic)
      self.topic_to_connection.setdefault(topic, []).append(connection)

  async def execute(self, connection, json_str):
    """Interprets and executes the given json message.

    Clients will send message using the json format described above, this
    method will make it easy to interpret the received message from the
    connection.

    Args:
       connection: The connection from which the message came.
       json_str: The json protocol message to be interpreted and executed.
    """
    msg = {}
    try:
      msg = json.loads(json_str)
    except json.JSONDecodeError as e:
      logging.error('Failed to decode %s, details:%s', json_str, e)

    if msg['type'] == 'publish':
      await self.publish(msg['topic'], msg['msg'])
    elif msg['type'] == 'subscribe':
      self.subscribe(msg['topic'], connection)
    elif msg['type'] == 'unsubscribe':
      self.unsubscribe(msg['topic'], connection)
    else:
      logging.warn('Unknown protocol!')

  def connect(self, connection):
    """Connects the given connection to the pubsub system.

    After this the connection can start publishing, subscribing and
    unsubscribing to and from topics.

    Args:
       connection: The connection to connect to the pubsub system.
    """
    with self.lock:
      self.connection_to_topic[connection] = []

  def disconnect(self, connection):
    """Connects the given connection to the pubsub system.

    The connection will unsubscribe from all topics.

    Args:
       connection: The connection to remove from the pubsub system.
    """
    with self.lock:
      topics = self.connection_to_topic.pop(connection, [])
      for topic in topics:
        self.unsubscribe(topic, connection)

  def __str__(self):
    return 'topic_to_connection: {}, connection_to_topic: {}'.format(
        self.topic_to_connection, self.connection_to_topic)
