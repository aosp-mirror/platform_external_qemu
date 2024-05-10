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

# This docker image is for build Qt 6.5.3 for linux-aarch64 on an arm mac.
# We want to build on arm mac so we can use an arm linux image.
#
# To build the docker image for arm architecture:
# > docker build -t <image_name> .
# > docker run --mount type=bind,source="/Volumes/android/emu-prebuilts",target="/emu" -dt <image_name>
# > docker exec -it <container_id> bash
# $ cd emu && tools/buildSrc/servers/build_tools.py --out_dir /emu/out-prebuilts --dist_dir /emu/dist-prebuilts/12345678 --build-id 12345678 --prebuilts
FROM ubuntu:focal-20200423

# tzdata complains when running apt-get install below, so we need to set the timezone.
ENV TZ=America/Los_Angeles
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt update
# Install necessary utilities
RUN apt install -y software-properties-common curl git

# Install python 3.10, ninja
RUN add-apt-repository -y ppa:deadsnakes/ppa
RUN apt install -y python3.10 python3.10-distutils pip ninja-build

# Install deps for building Qt
RUN apt -y install \
  patchelf \
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

# Install cmake >= 3.19; apt version is 3.16
RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.29.3/cmake-3.29.3-linux-aarch64.tar.gz > /cmake-3.29.3-linux-aarch64.tar.gz
RUN tar -xvf /cmake-3.29.3-linux-aarch64.tar.gz
ENV PATH=/cmake-3.29.3-linux-aarch64/bin:$PATH
