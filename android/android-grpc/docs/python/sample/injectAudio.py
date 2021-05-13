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
import time
from timeit import default_timer as timer
import google.protobuf.text_format
from aemu.discovery.emulator_discovery import get_default_emulator
from aemu.proto.emulator_controller_pb2 import AudioFormat, AudioPacket


def readAudioFile(wavefile):
    """Produces the audio stream, from the gRPC endpoint."""
    chunk_size = 1024
    with wave.open(wavefile, "rb") as wav_r:
        bps = wav_r.getframerate() * wav_r.getnchannels() * 2
        print("Got bps: {} -> {}".format(wav_r.getframerate(), bps))
        print(
            "SR: {}, width: {}, channels: {}".format(
                wav_r.getframerate(), wav_r.getsampwidth(), wav_r.getnchannels()
            )
        )
        wav_r.getsampwidth()
        aud_fmt = AudioFormat(
            format=AudioFormat.AUD_FMT_S16
            if wav_r.getsampwidth() == 2
            else AudioFormat.AUD_FMT_U8,
            channels=AudioFormat.Stereo
            if wav_r.getnchannels() == 2
            else AudioFormat.Mono,
            samplingRate=wav_r.getframerate(),
        )

        while wav_r.tell() < wav_r.getnframes():
            byts = wav_r.readframes(chunk_size)
            yield AudioPacket(format=aud_fmt, audio=byts)
            wait = len(byts) / bps
            time.sleep(wait)


def main(args):
    # Create a client
    stub = get_default_emulator().get_emulator_controller()
    wav_iterator = readAudioFile(args[1])
    start = timer()
    stub.injectAudio(wav_iterator)
    end = timer()
    print(end - start)


if __name__ == "__main__":
    main(sys.argv)
