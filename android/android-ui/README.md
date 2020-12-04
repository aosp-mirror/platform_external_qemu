Android Emulator UI
===================

This directory contains standalone UI implementation(s) that communicates with the qemu process via
gRPC.

# Why?
Decoupling the UI code will allow other users (e.g. Android Studio) to easily implement the UI using
a different UI framework.
