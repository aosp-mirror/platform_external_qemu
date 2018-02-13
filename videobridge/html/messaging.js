/**
 * @fileoverview Contains a simple pubsub module to talk with the emulator
 * and a simple class to setup the webrtc connection.
 *
 * You can set the variable pubsub_debug to log send and receive to the console.
 *
 * TODO(jansene): Improve error handling in websocket communication.
 * TODO(jansene): The emulator responds with "KO: .. failure message ..." to
 * indicate failure. It would be nice if we parsed those out for js.
 * TODO(jansene): Proper future usage in websocket send.
 */
'use strict';

class Pubsub {
  /**
   * Constructs a Pubsub module that can be used to
   * subscribe, unsubscribe and publish to topics.
   *
   * The websocket must be connected to the emulator.
   *
   * @param {websocket} A connected websocket
   */
  constructor(websocket) {
    var self = this;
    this.ws = websocket;
    this.callbacks = [];
    var rec = this.received.bind(this);
    this.ws.onmessage = rec;
  }


  received(e) {
    /**
     * Callback for incoming messages from the emulator.
     * Will invoke the callback of the registered topic
     *
     * @param {e} The data event
     */
    if (typeof pubsub_debug !== 'undefined')
      console.log("Pubsub.received: %s - %o", e.data, e);
    var msg = JSON.parse(e.data);
    for(var i = 0; i < this.callbacks.length; i++) {
      if (this.callbacks[i].topic == msg.topic) {
        this.callbacks[i].callback(msg.msg);
      }
    }
  }

  static connect(uri) {
    /**
     * Returns a promise containing a Pubsub object
     * when a connection to the emulator was successful.
     *
     * @param {uri} The uri where the emulator can be reached.
     *
     * For example:
     *
     * Pubsub.connect('ws://localhost:8080').then(function(pubsub) { .. })
     */
    return new Promise(function(resolve, reject) {
      var ws = new WebSocket(uri);
      ws.onopen  = function(e) { resolve(new Pubsub(ws)); }
      ws.onerror = function(e) { reject(e) }
    });
  }

  subscribe(topic, callback) {
    /**
     * Subscribes to the given topic. Invoking the callback when
     * a message was published to the given topic.
     *
     * @param {topic} The topic to subscribe to
     * @param {callback} The callback to be invoked when a message is published
     * to the given topic
     */
    this.callbacks.push({ topic: topic, callback: callback });
    this.send( { topic: topic, type: 'subscribe' });
  }

  unsubscribe(topic) {
    /**
     * Unsubscribes from the given topic. The callback will no longer
     * be invoked, and the server will be informed that we are no longer
     * interested in messages to this topic.
     *
     * @param {topic} The topic to unsubscribe from
     */
    var i = this.callbacks.length;
    while (i--) {
      if (this.callbacks[i].topic == msg.topic) {
        this.callbakcs.splice(i, 1);
      }
    }
    this.send( { topic: topic, type: 'unsubscribe' });
  }

  publish(topic, msg) {
    /**
     * Sends the message to the given topic. Serializing it
     * as json if it is not a string. All subscribers to
     * this topic will receive this message.
     *
     * @param {topic} The topic to broadcast the message to
     * @param {msg} The message to be broadcast
     */
    if (typeof msg !== 'string') msg = JSON.stringify(msg)
    this.send({ topic: topic, type: 'publish', msg: msg});
  }

  send(snd) {
    /**
     * Sends a raw message to the websocket, do not use this as
     * it does not follow the proper protocols.
     */
    if (typeof pubsub_debug !== 'undefined')
      console.log("Pubsub.send: %s - %o", JSON.stringify(snd), snd);
    this.ws.send(JSON.stringify(snd));
  }
}

class MouseForwarder {
  constructor(emu, videoWindow) {
     this.button = false;
     this.emulator = emu;

     // TODO(jansene): Need to catch actual size events
     // TODO(jansene): Need to communicate actual widht/height
     this.scaleX = 1080 / videoWindow.width;
     this.scaleY = 1920 / videoWindow.height;
     this.videoWindow = videoWindow;
     videoWindow.addEventListener("mousedown", e => this.mousedown(e));
     videoWindow.addEventListener("mouseup", e => this.mouseup(e));
     videoWindow.addEventListener("mousemove",e => this.mousemove(e));
  }

  disconnect() {
     this.videoWindow.removeEventListener("mousedown", e => this.mousedown(e));
     this.videoWindow.removeEventListener("mouseup", e => this.mouseup(e));
     this.videoWindow.removeEventListener("mousemove",e => this.mousemove(e));

  }
 /**
   * Retrieve the coordinates of the given event relative to the center
   * of the widget.
   *
   * @param event
   *   A mouse-related DOM event.
   * @param reference
   *   A DOM element whose position we want to transform the mouse coordinates to.
   * @return
   *    A hash containing keys 'x' and 'y'.
   */
  getRelativeCoordinates(event, reference) {
    var x, y;
    event = event || window.event;
    var el = event.target || event.srcElement;

    if (!window.opera && typeof event.offsetX != 'undefined') {
      // Use offset coordinates and find common offsetParent
      var pos = { x: event.offsetX, y: event.offsetY };

      // Send the coordinates upwards through the offsetParent chain.
      var e = el;
      while (e) {
        e.mouseX = pos.x;
        e.mouseY = pos.y;
        pos.x += e.offsetLeft;
        pos.y += e.offsetTop;
        e = e.offsetParent;
      }

      // Look for the coordinates starting from the reference element.
      var e = reference;
      var offset = { x: 0, y: 0 }
      while (e) {
        if (typeof e.mouseX != 'undefined') {
          x = e.mouseX - offset.x;
          y = e.mouseY - offset.y;
          break;
        }
        offset.x += e.offsetLeft;
        offset.y += e.offsetTop;
        e = e.offsetParent;
      }

      // Reset stored coordinates
      e = el;
      while (e) {
        e.mouseX = undefined;
        e.mouseY = undefined;
        e = e.offsetParent;
      }
    }
    else {
      // Use absolute coordinates
      var pos = getAbsolutePosition(reference);
      x = event.pageX  - pos.x;
      y = event.pageY - pos.y;
    }
    // Subtract distance to middle
    return { x: x, y: y };
  }

  mousedown(e) {
    this.button = true;
    var place = this.getRelativeCoordinates(e, this.videoWindow);
    this.mouse(place.x, place.y)
  }

  mouseup(e) {
    this.button = false;
    var place = this.getRelativeCoordinates(e, this.videoWindow);
    this.mouse(place.x, place.y)
  }

  mousemove(e) {
    if (this.button) {
      var place = this.getRelativeCoordinates(e, this.videoWindow);
      this.mouse(place.x, place.y)
    }
  }

  mouse(x, y) {
    // TODO(jansene): Do some sensible coordinate transformation
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
    var btn = (this.button) ? "1" : "0";
    var s = "event mouse " + Math.round(x * this.scaleX) + " " + Math.round(y * this.scaleY) + " 0 " + btn;
    this.emulator.send(s);
  }

}

class Emulator {
  /**
   * A class that can be used to interact with the emulator. It is responsible
   * for setting up the webrtc video, as well as sending commands to the
   * emulator.
   *
   * Communication with the emulator happens over a pubsub protocol, where the
   * following topics are of interest:
   *
   * 'emulator-video': topic used to setup webrtc communication.
   * '[guid]' : topic that indicates me, used to setup webrtc.
   * 'emulator-console-out': Results from a command are published here.
   * 'emulator': topic used to send commands to the emulator. Commands follow
   * the telnet protocol exposed by the emulator.
   */
  constructor(pubsub) {
    this.pubsub = pubsub;
    this.me = Emulator.guid();
    this.self = this;
    this.resolvers = []
        this.pubsub.subscribe('emulator-console-out', msg => {
          if (this.resolvers.length > 1) {
            var resolve = this.resolvers.shift();
            resolve(msg);
          }
        })
  }

  send(cmd) {
    /**
     * Sends the given command to the emulator. This follows the
     * same protocol as using telnet <emulator> <port> to the given emulator.
     *
     * @param {cmd} The command to be send to the emulator
     * @returns {Promise} Containing a string with the response from the
     * emulator.
     *
     * For example
     *
     *  e.send('help').then(msg => { console.log(msg); });
     *
     */
    return new Promise((resolve, reject) => {
      this.resolvers.push(resolve);
      this.pubsub.publish('emulator', cmd);
    });
  }

  show(videoElement) {
    /**
     * Sets up the webrtc communication, connecting the resulting video stream
     * to the video element.
     *
     * Calling this function will result in a series of messages being exchanged
     * that follow the webrtc protocol. Communcation will go vie the
     * 'emulator-video' topic.
     *
     * @param {videoElement} The video element to which we will connect the
     * video stream.
     * @return A promise contain the Emulator object.
     */
    return new Promise((resolve, reject) => {
      var webrtc_message = msg => {
        var signal = JSON.parse(msg);
        if (signal.start) {
	  
          this.pc = new RTCPeerConnection(signal.start);
          this.pc.ontrack = ev => {
            videoElement.srcObject = ev.streams[0];
            videoElement.play();
            this.forwarder = new MouseForwarder(this, videoElement);
            resolve(this);
          };
          this.pc.onicecandidate = evt => {
            if (evt.candidate != null) {
              this.pubsub.publish('emulator-video', { candidate: evt.candidate });
            }  else {
              // Finished ice package exchange. Nothing needs to happen.
            }
          }
          this.pubsub.publish('emulator-video', { start: this.me });
        }
        if (signal.sdp) {
          this.pc.setRemoteDescription(new RTCSessionDescription(signal));
          this.pc.createAnswer().then(answer => {
            this.pc.setLocalDescription(answer);
            this.pubsub.publish( 'emulator-video', { sdp: answer });
          }).catch(reject);
        }
        if (signal.bye) {
          // TODO(jansene): We can do better :-)
          this.forwarder.disconnect();
          this.forwarder = null;
          alert("The server kicked you out! Maybe someone else took over?");
        }
      if (signal.candidate)
        this.pc.addIceCandidate(new RTCIceCandidate(signal));
    };

    this.pubsub.subscribe(this.me, webrtc_message);
    this.pubsub.publish('turn-config',  this.me );
    
  });
}

  static guid() {
    /**
     * Generates a sort of unique guid. This is far from perfect but will suit
     * our needs.
     */
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
      var r = Math.random()*16|0, v = c == 'x' ? r : (r&0x3|0x8);
      return v.toString(16);
    });
  }

  static connect(uri) {
    /**
     * Connects to the emulator at the given uri.
     *
     * It will setup the proper subscriptions to topics to:
     * 1) Communicate with the emulator to control it.
     * 2) Communicate with the webrtc component to connect video.
     *
     * Returns a promise that will contain an Emulator object.
     *
     * @param {uri} The websocket uri where the emulator is hosted.
     */
    return new Promise(function(resolve, reject) {
      var ws = new WebSocket(uri);
      ws.onopen  = function(e) {
        var pubsub = new Pubsub(ws);
        var emu = new Emulator(pubsub);
        resolve(emu);
      }
      ws.onerror = function(e) { reject(e) }
    });
  }

  static connect_and_show(uri, videoElement) {
    /**
     * Connects to the emulator at the given uri and sets up the
     * video to be connected to the videoElement.
     *
     * @param {uri} The websocket uri where the emulator is hosted.
     * @param {videoElement} The video element where the video from the emulator
     * will be shown. Should refere to an html video tag.
     *
     * For example:
     *   var remoteVideo = document.getElementById('remoteVideo');
     *   Emulator.connect_and_show('ws://localhost:8080', remoteVideo).then(function(e) {
     *        // Video should be showing, and we can send commands to the
     *        // emulator.
     *       e.send('help');
     *   });
     */
    return Emulator.connect(uri).then(function(e) {
      return e.show(videoElement);
    });
  }
}
