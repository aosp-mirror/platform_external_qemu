/**
 * @fileoverview A simple example of how to use the emulator
 *
 */
'use strict';

/* The remote video element, we need this to display video
 * can be any <video> tag in the page*/
var remoteVideo = document.getElementById('remoteVideo');

var emulator;

var fast_tap = function(x, y) {
  /* Example function that shows how to do a tap on an x, y coordinate. */
  var z = 0;
  var s = "event mouse " + x + " " + y + " " + z;

  // We basically send an 'event mouse x y 0 1'
  // followed by an 'event mouse x y 0 0'
  // which means lbutton at x y, no button at x y

  /* The exact events come from here:
   *   #define MOUSE_EVENT_LBUTTON 0x01
   *   #define MOUSE_EVENT_RBUTTON 0x02
   *   #define MOUSE_EVENT_MBUTTON 0x04
   *   #define MOUSE_EVENT_WHEELUP 0x08
   *   #define MOUSE_EVENT_WHEELDN 0x10
   *
   *   0 indicates no buttons down.
   */
  emulator.send(s + " 1").then(emulator.send(s + " 0"))
}


// Construct the proper uri to connect to the emulator.
var host = location.host
if (host.indexOf(':') > 0) {
   host = host.split(':')[0]
}
var uri = "ws://" + host + ":8000"

// Set to false, or leave undefined for no logging.
var pubsub_debug = true;

Emulator.connect_and_show(uri, remoteVideo).then(function(e) {
  emulator = e;
  // First we authorize, so we can do the extensive commands.
  // Note: authorization differs from emulator to emulator so this might
  // not work..
  e.send('auth Uy1kLOX5tddG9Er0').then(msg => {
    console.log(msg); // Should display OK.
    fast_tap(1156, 2481);
  });

  // now you can send commands to the emulator
  // For example:
  // e.send('help').then(msg => { console.log(msg); });
  // e.send('ping').then(msg => { console.log(msg); });
});
