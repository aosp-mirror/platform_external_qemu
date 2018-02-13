/**
 * @fileoverview A simple example of how to use the emulator
 *
 */
'use strict';

/* The remote video element, we need this to display video
 * can be any <video> tag in the page*/
var remoteVideo = document.getElementById('remoteVideo');

var emulator;


// Construct the proper uri to connect to the emulator websocket
var host = location.host;

// Note, we depend on some websocket passthrough under /socket
var uri = "ws://" + host + "/socket"
var pubsub_debug = true;

Emulator.connect(uri).then(function(e) {
  emulator = e;
  // Note, this will hookup the mouseforwarder, so you can click on things.
  emulator.show(remoteVideo);
  // now you can send commands to the emulator
  // For example:
  e.send('help').then(msg => { console.log(msg); });
  e.send('ping').then(msg => { console.log(msg); });
});
