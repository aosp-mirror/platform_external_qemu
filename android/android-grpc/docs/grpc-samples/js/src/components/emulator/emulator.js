import PropTypes from "prop-types";
import React, { Component } from "react";
import * as Device from "../../android_emulation_control/emulator_controller_grpc_web_pb.js";
import * as Proto from "../../android_emulation_control/emulator_controller_pb.js";
import Log from "../../util/log.js";
import EmulatorPngView from "./views/simple_png_view.js";

export default class Emulator extends Component {
  state = {
    mouseDown: false,
    xpos: 0,
    ypos: 0
  };

  static propTypes = {
    uri: PropTypes.string,
    width: PropTypes.number,
    height: PropTypes.number,
    scale: PropTypes.number,
    refreshRate: PropTypes.number
  };

  static defaultProps = {
    width: 1080,
    height: 1920,
    scale: 0.4,   // Scale factor of the emulator image
    refreshRate: 5 // Desired refresh rate.
  };

  componentDidMount() {
    const { uri } = this.props;
    this.emulatorService = new Device.EmulatorControllerClient(uri);
  }

  setCoordinates = (down, xp, yp) => {
    // It is totally possible that we send clicks that are offscreen..
    const { width, height, scale } = this.props;
    const x = Math.round((xp * width) / (width * scale));
    const y = Math.round((yp * height) / (height * scale));

    // Make the grpc call.
    var request = new Proto.MouseEvent();
    request.setX(x);
    request.setY(y);
    request.setButtons(down ? 1 : 0);
    var call = this.emulatorService.sendMouse(request, {}, function(
      err,
      response
    ) {
      if (err) {
        Log.error(
          "Grpc: " + err.code + ", msg: " + err.message,
          "Emulator:setCoordinates"
        );
      }
    });

    // Status callback..
    call.on("status", function(status) {
      Log.debug("Status: " + JSON.stringify(status));
    });
  };

  // Properly handle the mouse events.
  handleMouseDown = e => {
    this.setState({ mouseDown: true });
    const { offsetX, offsetY } = e.nativeEvent;
    this.setCoordinates(true, offsetX, offsetY);
  };

  handleMouseUp = e => {
    this.setState({ mouseDown: false });
    const { offsetX, offsetY } = e.nativeEvent;
    this.setCoordinates(false, offsetX, offsetY);
  };

  handleMouseMove = e => {
    const { mouseDown } = this.state;
    if (!mouseDown) return;
    const { offsetX, offsetY } = e.nativeEvent;
    this.setCoordinates(true, offsetX, offsetY);
  };

  render() {
    const { width, height, scale, uri, refreshRate } = this.props;
    return (
      <div
        onMouseDown={this.handleMouseDown}
        onMouseMove={this.handleMouseMove}
        onMouseUp={this.handleMouseUp}
        onMouseOut={this.handleMouseUp}
      >
        <EmulatorPngView
          width={width * scale}
          height={height * scale}
          refreshRate={refreshRate}
          uri={uri}
        />
      </div>
    );
  }
}
