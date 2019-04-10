import React, { Component } from 'react';
import logo from './logo.svg';
import './App.css';

import Emulator from './components/emulator/emulator.js'
const URI = "http://" + window.location.hostname + ":8080"

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
