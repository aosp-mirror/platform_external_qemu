# Run CTS in a Linux Container on Windows

Running CTS (cts-tradefed) on Windows can be quite difficult, so a solution is to run the CTS
binaries in a Linux container. The only thing that the container would need from the host
environment, where the emulator lives, is the `adb` server port, of which can be tunneled via `ssh`.

## Prerequisites
### Docker for Windows
Follow the [steps to install Docker Desktop for Windows](https://docs.docker.com/desktop/install/windows-install/). Once installed, make sure Docker is set up to run Linux containers (Windows has a
toggle to switch between Windows containers and Linux containers).

## Building the Docker Image
Construct an image from the Dockerfile and name it `cts-windows`:
```
cd android/docker/windows/cts && docker build -t cts-windows .
```

## Create a Container from the Docker Image
Create and run the container, and port forward host port `3000` (or a port of your choosing) to the
container's ssh port `22`:
```
docker run -p 3000:22 -dt cts-windows
```

## SSH Port Forward ADB Server Port
In order for the container above to access your emulator running on the Windows host, one way is to
forward the ADB server port (`5037` is the default server port). Open a new `cmd.exe`, and connect
to the container via `ssh`:
```
C:\your\host> ssh -l root -p 3000 -R 5037:127.0.0.1:5037 localhost
root@localhost's password: password (you can change the password in the Dockerfile)
```

***Note: `-p 3000` is because we port forward 3000 to ssh port `22` in the container.
`-R 5037:127.0.0.1:5037` means `adb` in the container will see the host's `adb` server by default.***

## Run CTS
The running container now has access to the emulator. To login to the container, you can run:
```
docker exec -it <container_id> bash
```
***Note: you can obtain the `<container_id>` by running `docker ps`.***

The environment will have everything you need to run `cts-tradefed`, including `adb` and `aapt`. The
Android SDK folder is at `/android-sdk`. `cts-tradefed` should be located in
`/android-cts/android-cts/tools/cts-tradefed`. Enjoy!