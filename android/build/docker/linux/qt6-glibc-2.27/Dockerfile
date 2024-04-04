# Copyright (C) 2024 The Android Open Source Project
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

# This Dockerfile sets up a linux image to build Qt 6 with glibc 2.27.

FROM ubuntu:18.04

ENV TZ=America/Los_Angeles
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt update

# packages to install repo and pull down source code
RUN apt -y install git python3 curl

RUN mkdir $HOME/bin
ENV PATH=$HOME/bin/repo:$PATH

# make python point to python3
RUN ln -sf /usr/bin/python3 $HOME/bin/python

# repo to pull down source code
RUN curl http://commondatastorage.googleapis.com/git-repo-downloads/repo > $HOME/bin/repo
RUN chmod +x $HOME/bin/repo

# To make repo not prompt for things
RUN git config --global user.email "you@example.com"
RUN git config --global user.name "Your Name"
RUN git config --global color.ui false

# packages required to build Qt 6
RUN apt -y install g++ unzip zip perl nodejs patchelf gperf bison flex

# libraries required to build Qt 6
RUN apt -y install \
  libfontconfig1-dev \
  libfreetype6-dev \
  libx11-dev \
  libx11-xcb-dev \
  libxext-dev \
  libxfixes-dev \
  libxi-dev \
  libxrender-dev \
  libxcb-cursor-dev \
  libxcb-glx0-dev \
  libxcb-keysyms1-dev \
  libxcb-image0-dev \
  libxcb-shm0-dev \
  libxcb-icccm4-dev \
  libxcb-sync-dev \
  libxcb-xfixes0-dev \
  libxcb-shape0-dev \
  libxcb-randr0-dev \
  libxcb-render-util0-dev \
  libxcb-util-dev \
  libxcb-xinerama0-dev \
  libxcb-xkb-dev \
  libxkbcommon-dev \
  libxkbcommon-x11-dev \
  libsm-dev \
  libice-dev \
  libdbus-1-dev \
  libxcomposite-dev \
  libxcursor-dev \
  libxrandr-dev \
  libxshmfence-dev \
  libcupsfilters-dev \
  libegl1-mesa-dev \
  libglib2.0-dev \
  libcups2-dev \
  libnss3-dev \
  libxtst-dev \
  libxkbfile-dev \
  libdrm-dev

# The below lines should run on the instance:
# Pull down the source code
RUN mkdir /emu-prebuilts \
    && cd /emu-prebuilts \
    && repo init -u https://android.googlesource.com/platform/manifest -b emu-prebuilts \
    && repo sync -c -j16

# Build it!
# I had to modify some things in qt.py to make it only build the no-webengine variant.
RUN cd /emu-prebuilts \
    && prebuilts/python/linux-x86/bin/python3 tools/buildSrc/servers/build_tools.py --out_dir /emu-prebuilts/out-prebuilts --build-id 12345678 --dist_dir /emu-prebuilts/dist/12345678 --prebuilts
