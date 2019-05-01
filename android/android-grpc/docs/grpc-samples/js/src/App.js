import React, { Component } from 'react';
import logo from './logo.svg';
import './App.css';

import Emulator from './components/emulator/emulator.js'
const environment = process.env.NODE_ENV || 'development';
var URI = window.location.protocol + "//" + window.location.hostname + ":" + window.location.port
if (environment === 'development') {
   URI = "http://" + window.location.hostname + ":8080"
}

export default class App extends Component {
  state = {}

  static propTypes = {}
  static defaultProps = {}

  componentDidMount() { }

  render() {
    return (
      <div>
        <Emulator uri={URI} />
      </div>
    )
  }
}
