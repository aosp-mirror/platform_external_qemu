/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import PropTypes from "prop-types";
import React, { Component } from "react";
import * as Device from "../../../android_emulation_control/emulator_controller_grpc_web_pb.js";

/**
 * A view on the emulator that is using WebRTC. It will use the Jsep protocol over gRPC to
 * establish the video streams.
 */
export default class EmulatorWebrtcView extends Component {

  constructor() {
    super()
    this.connected = false
    this.peerConnection = null
    /* eslint-disable */
    this.guid = new proto.android.emulation.control.RtcId()
  }

  static propTypes = {
    uri: PropTypes.string, // gRPC endpoint of the emulator
    width: PropTypes.number,
    height: PropTypes.number,
  };

  static defaultProps = {
    width: 1080,
    height: 1920,
  };

  componentDidMount() {
    const { uri, refreshRate } = this.props;
    this.emulatorService = new Device.EmulatorControllerClient(uri);
    this.startStream();
  }

  componentWillUnmount() {
    this.cleanup()
  }

  disconnect = () => {
    this.connected = false
    if (this.peerConnection) this.peerConnection.close()
  }

  onDisconnect = () => {
      this.video.stop()
  }

  onConnect = stream => {
    this.video.srcObject = stream
    var playPromise = this.video.play();

    if (playPromise !== undefined) {
      playPromise.then(_ => {
        console.log("Automatic playback started!")
      })
      .catch(error => {
        // Autoplay is likely disabled in chrome
        // https://developers.google.com/web/updates/2017/09/autoplay-policy-changes
        // so we should probably show something useful here.
        // We explicitly set the video stream to muted, so this shouldn't happen,
        // but is something you will have to fix once enabling audio.
        alert("code: " + error.code + ", msg: " + error.message + ", name: " + error.nane)
        })
    }
  }

  cleanup = () => {
    if (this.peerConnection) {
        this.peerConnection.removeEventListener('track', this.handlePeerConnectionTrack)
        this.peerConnection.removeEventListener('icecandidate', this.handlePeerIceCandidate)
        this.peerConnection = null
      }
  }

  handlePeerConnectionTrack = e => {
    this.connected = true
    this.onConnect(e.streams[0])
  }

  handlePeerIceCandidate = e => {
    if (e.candidate === null) return
    this.sendJsep({ candidate: e.candidate })
  }

  handleStart = signal => {
    this.peerConnection = new RTCPeerConnection(signal.start)
    this.peerConnection.addEventListener('track', this.handlePeerConnectionTrack, false)
    this.peerConnection.addEventListener('icecandidate', this.handlePeerIceCandidate, false)
    console.log("added peer connection: " + this.peerConnection)
  }

  handleSDP = async signal => {
    console.log("handleSDP: " + signal)
    this.peerConnection.setRemoteDescription(new RTCSessionDescription(signal))
    const answer = await this.peerConnection.createAnswer()
    if (answer) {
      this.peerConnection.setLocalDescription(answer)
      this.sendJsep({ sdp: answer })
    } else {
      this.disconnect()
    }
  }

  handleCandidate = signal => {
    console.log("handleCandidate: " + signal + " con: " + this.peerConnection)
    this.peerConnection.addIceCandidate(new RTCIceCandidate(signal))
  }

  handleJsepMessage = message => {
    try {
      const signal = JSON.parse(message)
      if (signal.start) this.handleStart(signal)
      if (signal.sdp) this.handleSDP(signal)
      if (signal.bye) this.handleBye()
      if (signal.candidate) this.handleCandidate(signal)
    } catch (e) {
      console.log("Failed to handle message: [" + message + "], due to: " + e)
    }
  }

  handleBye = () => {
    if (this.connected) {
      this.disconnect()
    }
  }

  sendJsep = jsonObject => {
    /* eslint-disable */
    var request = new proto.android.emulation.control.JsepMsg()
    request.setId(this.guid)
    request.setMessage(JSON.stringify(jsonObject))
    this.emulatorService.sendJsepMessage(request, {},
         function(err,response) {
            if (err) {
                console.error(
                    "Grpc: " + err.code + ", msg: " + err.message,
                    "Emulator:updateview"
                );
            }
        });
  }

  startStream() {
    var self = this
    var request = new proto.google.protobuf.Empty()
    var call = this.emulatorService.requestRtcStream(request, {},
      function(err,response) {
         if (err) {
             console.error(
                 "Grpc: " + err.code + ", msg: " + err.message,
                 "Emulator:updateview"
             );
         } else {
           // Configure
           self.guid.setGuid(response.getGuid())
           self.connected = true

           // And pump messages
           self.receiveJsepMessage()
         }
     });
  }

  receiveJsepMessage() {
    console.log("receiveJsepMessage")
    if (!this.connected)
      return

    /* eslint-disable */
    var self = this;

    // This is a blocking call, that will return as soon as a series
    // of messages have been made available.
    this.emulatorService.receiveJsepMessage(this.guid, {},
      function(err,response) {
         if (err) {
             console.error(
                 "Grpc: " + err.code + ", msg: " + err.message,
                 "Emulator:updateview"
             );
         } else {
           const msg = response.getMessage()

           // Handle only if we received a useful message.
           // it is possible to get nothing if the server decides
           // to kick us out.
           if (msg)
            self.handleJsepMessage(response.getMessage())

           // And pump messages
           self.receiveJsepMessage()
         }
     });
  }

  handleDrag = e => {
    e.preventDefault();
  }

  render() {
    const { width, height } = this.props;
    return (
      <video
        ref={node => (this.video = node)}
        width={width}
        height={height}
        muted="muted"
      />
    );
  }
}
