/**
 * @fileoverview Description of this file.
 *
 * This should be run on the same host as where the emulator is running.
 * The idea is to run a screen capture, and make it available to everyone
 * using the pubsub protocol.
 */
'use strict';

var peerConnectionConfig = {'iceServers': [{'url': 'stun:stun.services.mozilla.com'}, {'url': 'stun:stun.l.google.com:19302'}]};
var videoElement = document.getElementById('video');
var extensionInstalled = false;
var who = null;
var pubsub = null;

var pc = new RTCPeerConnection();
pc.onicecandidate = function (evt) {
  pubsub.publish(who, { candidate: evt.candidate });
};



var pubsub_connected = function(ps) {
  pubsub = ps;
  ps.subscribe('emulator-video', webrtc_message);
}

var not_connected = function(e) {
   console.log("Bad news an error: %o", e);
   alert("Failed to connect to websocket");
}


var webrtc_message = function (msg) {
  var signal = JSON.parse(msg);
  if (signal.sdp)
    pc.setRemoteDescription(new RTCSessionDescription(signal.sdp));
  if (signal.candidate)
    pc.addIceCandidate(new RTCIceCandidate(signal.candidate));

  if (signal.start) {
    // Kick of the webrtc protocol
    who  = signal.start;
    pc.createOffer().then(
        function(desc) {
          pc.setLocalDescription(desc);
          pubsub.publish(who, { sdp: desc } );
        });
  }
};




/*** Stuff below here is screen capture selection, once we have a stream it will
 * setup the peerconnection object. Once that is done you can start streaming
 */
document.getElementById('start').addEventListener('click', function() {
  // send screen-sharer request to content-script
  if (!extensionInstalled) {
    var message = 'Please install the extension:\n' +
        '1. Go to chrome://extensions\n' +
        '2. Check: "Enable Developer mode"\n' +
        '3. Click: "Load the unpacked extension..."\n' +
        '4. Choose "extension" folder from the repository\n' +
        '5. Reload this page';
    alert(message);
  }
  window.postMessage({type: 'SS_UI_REQUEST', text: 'start'}, '*');
});

// listen for messages from the content-script
window.addEventListener('message', function(event) {
  if (event.origin !== window.location.origin) {
    return;
  }

  // content-script will send a 'SS_PING' msg if extension is installed
  if (event.data.type && (event.data.type === 'SS_PING')) {
    extensionInstalled = true;
  }

  // user chose a stream
  if (event.data.type && (event.data.type === 'SS_DIALOG_SUCCESS')) {
    startScreenStreamFrom(event.data.streamId);
  }

  // user clicked on 'cancel' in choose media dialog
  if (event.data.type && (event.data.type === 'SS_DIALOG_CANCEL')) {
    console.log('User cancelled!');
  }
});

function handleSuccess(screenStream) {
  videoElement.src = URL.createObjectURL(screenStream);
  videoElement.play();
  pc.addStream(screenStream);

  // Connect to the emulator.. We can now start streaming.
  Pubsub.connect('ws://localhost:8000').then(pubsub_connected).catch(not_connected);
}

function handleError(error) {
  alert('getUserMedia() failed: ' + error);
}

function startScreenStreamFrom(streamId) {
  var constraints = {
    audio: false,
    video: {
      mandatory: {
        chromeMediaSource: 'desktop',
        chromeMediaSourceId: streamId,
        maxWidth: window.screen.width,
        maxHeight: window.screen.height
      }
    }
  };
  navigator.mediaDevices.getUserMedia(constraints).
      then(handleSuccess).catch(handleError);
}

