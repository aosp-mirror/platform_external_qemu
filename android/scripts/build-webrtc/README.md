Generate libwebrtc prebuilt
===========================

The CMake script will do the following:

- Obtain release branch M72 from WebRTC
- Fetch all the dependencies (very slow, migth include Xcode)
- Build the webrtc module
- Re-assemble a static archive
- Copy the archive & headers to the install directory passed into the cmake script.

Some things to be aware of:

- This is very slow, it obtains all the dependencies for webrtc.
- Pulling all the dependencies has failed at times, if this happens for you, you might need to tinker with the script.
- The generated archive can be very large, as it contains all essential dependencies.
- The generated archive will be constructed using the toolchain that comes with webrtc, this might not be the same toolchain as you are using.

