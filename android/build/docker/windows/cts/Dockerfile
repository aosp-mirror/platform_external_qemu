# Copyright (C) 2023 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This Dockerfile builds a linux image on Windows that can run CTS against an emulator running
# on the Windows host.
#
# To build/run an image with the necessary settings for cts-tradefed:
# > cd android/docker/windows/cts
# > docker build -t cts-win32 .
# # forword host's localhost port 3000 to remote's localhost port 22 (ssh)
# > docker run -p 3000:22 -dt cts-win32
#
# Next, using a different terminal, forward the adb server port into the docker container
# through ssh:
# > ssh -l root -p 3000 -R 5037:127.0.0.1:5037 localhost
# root@localhost's password: password
#
# Now you can login to the container to run cts-tradefed:
# > docker exec -it <container_id> bash
# > /android-cts/android-cts/tools/cts-tradefed

FROM ubuntu:22.04

RUN apt update
RUN apt install -y unzip wget

# Download CTS for android 14 (U)
RUN mkdir /android-cts && cd /android-cts && \
    wget dl.google.com/dl/android/cts/android-cts-14_r2-linux_x86-x86.zip && \
    unzip android-cts-14_r2-linux_x86-x86.zip && \
    rm android-cts-14_r2-linux_x86-x86.zip

# Get Android Studio command line tools package to install build-tools and platform-tools
# This version of sdkmanager needs at least java 17.
# sdkmanager expects to be located in <sdk_root>/cmdline-tools/latest/bin/sdkmanager.
RUN apt install -y openjdk-17-jre-headless openjdk-17-jdk
RUN mkdir /android-sdk && cd /android-sdk && \
    wget https://dl.google.com/android/repository/commandlinetools-linux-10406996_latest.zip && \
    unzip commandlinetools-linux-10406996_latest.zip && \
    rm commandlinetools-linux-10406996_latest.zip && \
    mv /android-sdk/cmdline-tools /android-sdk/latest && \
    mkdir /android-sdk/cmdline-tools && \
    mv /android-sdk/latest /android-sdk/cmdline-tools

ENV PATH /android-sdk/cmdline-tools/latest/bin:$PATH

# Accept all licenses
RUN yes | sdkmanager --licenses

# Install platform-tools and build-tools 34.0.0
RUN sdkmanager "platform-tools" "build-tools;34.0.0"
ENV PATH /android-sdk/build-tools/34.0.0:/android-sdk/platform-tools:$PATH

RUN apt install -y openssh-server
# Configure ssh
RUN mkdir /var/run/ssh
# ssh command to login (forward adb server port):
# > ssh -l root -p <ssh port> -R 5037:127.0.0.1:5037 localhost
# root@localhost's password: password
RUN echo 'root:password' | chpasswd
RUN sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config

# For ssh connections; needed to forward adb server port for cts-tradefed to use
EXPOSE 22