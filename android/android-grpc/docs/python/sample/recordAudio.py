#  Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""A simple example that demonstrates how to record audio through the gRPC endpoint.

   The audio is recorded to a .wav file.
"""

import sys
import wave

import proto.emulator_controller_pb2 as p
import proto.emulator_controller_pb2_grpc
import google.protobuf.text_format
from channel.channel_provider import getEmulatorChannel
from google.protobuf import empty_pb2

def streamAudioToWave(wavefile, bitrate=44100):
    """Produces the audio stream, from the gRPC endpoint."""
    # Open a grpc channel
    channel = getEmulatorChannel()

    # Create a client
    stub = proto.emulator_controller_pb2_grpc.EmulatorControllerStub(channel)

    # Desired format.
    fmt = p.AudioFormat(
        format=p.AudioFormat.AUD_FMT_S16, channels=p.AudioFormat.Stereo, samplingRate=bitrate)

    with wave.open(wavefile, 'wb') as wav:
        wav.setnchannels(2)
        wav.setframerate(fmt.samplingRate)
        wav.setsampwidth(2)
        for frame in stub.streamAudio(fmt):
            fmt = google.protobuf.text_format.MessageToString(frame.format, as_one_line=True)
            print("Packet: {} - {}".format(frame.timestamp, fmt))
            wav.writeframes(frame.audio)


def main(args):
  streamAudioToWave(args[1])


if __name__ == "__main__":
    main(sys.argv)
