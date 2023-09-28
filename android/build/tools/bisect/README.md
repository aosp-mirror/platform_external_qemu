# Tool to bisect emulator issues

This is an internal tool that can be used to bisect issues you have
found with an emulator build.  This tool can only be used by googlers
as it will require access to non public apis.

## Usage

Say you have found a visual issue that can be reproduced as follows:

     emulator @my_avd -grpc-use-token -qt-hide-window

and you would like to find out which build introduced this. You can
run the following:

     . ./configure.sh
     emu-bisect --unzip --num 1024 '${ARTIFACT_UNZIP_DIR}/emulator/emulator @my_avd -qt-hide-window -no-snapshot -grpc-use-token; read -p "Is $X OK? (y/n): " ok < /dev/tty;  [[ "$ok" == "y" ]]'

This will do a bisection of the last 1024 emulator builds and run the given command. Once the emulator shuts down you will have to reply y/n to indicate
if the issue was pressent. Once the bisection is completed the tool will output something like this:

     10518258, not ok
     10517925, ok

Containing the first bad build, and the last good build.

Launch `emu-bisect --help` to get detailed usage information and examples

## Detailed usage

This tools is inspired by go/bisect, with the addition that it will work with builds from go/ab. In a nutshell it does this:

Fetch all build ids starting the provided build id (or latest)
Next we will do a binary search over all these ids to figure out which build id is causing a problem:

- Download the artifact, if it has not been downloaded already
- Unzip the artifact if it is a zipfile.
- Execute the shell command. The following environment variables will be set:
  - ARTIFACT_UNZIP_DIR variable to the directory of the unzipped artifact, if the artifact was a zip file and you passed in the --unzip flag
  - ARTIFACT_LOCATION variable containling the location of the downloaded artifact.
  - X the current build id under consideration.

The status code of the shell command is used to indicate failure or success. In theory you could do things with arbitary artifacts, targets and branches.
