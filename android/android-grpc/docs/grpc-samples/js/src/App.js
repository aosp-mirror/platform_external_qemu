import './App.css';

import {blue, indigo} from '@material-ui/core/colors'
import {createMuiTheme, MuiThemeProvider} from '@material-ui/core/styles'
import React, {Component} from 'react';

import Emulator from './components/emulator/emulator.js'
import logo from './logo.svg';


const environment = process.env.NODE_ENV || 'development'
var URI = window.location.protocol + '//' + window.location.hostname + ':' +
        window.location.port
if (environment === 'development') {
    URI = 'http://' + window.location.hostname + ':8080'
}

const theme = createMuiTheme({
    palette: {secondary: {main: blue[900]}, primary: {main: indigo[700]}},
    typography: {
        // Use the system font instead of the default Roboto font.
        fontFamily: ['"Lato"', 'sans-serif'].join(',')
    }
})


export default class App extends Component {
    render() {
    return (
      <div>
         <MuiThemeProvider theme={theme}>
        <Emulator uri={
            URI}/>
        </MuiThemeProvider>
        </div>
    )
  }
}
