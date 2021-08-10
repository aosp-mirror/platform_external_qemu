#  Copyright (C) 2021 The Android Open Source Project
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

"""A simple example that demonstrates how to inject audio through the gRPC endpoint.

   This example basically reads a .WAV file and pushes the audio packets
   to emulator microphone.
"""
import sys
import time
import wave
from timeit import default_timer as timer

from aemu.discovery.emulator_discovery import get_default_emulator
from aemu.proto.emulator_controller_pb2 import AudioFormat, AudioPacket


def readAudioFile(wavefile):
    """Reads a wav file and chunks it into AudioPackets.."""

    # Will send chunks of +/- 1 kb
    chunk_size = 1024
    with wave.open(wavefile, "rb") as wav_r:

        # Print some general information about the wav file we just opened.
        bps = wav_r.getframerate() * wav_r.getnchannels() * 2
        print("Wav file of: {} hz,  with bps: {}".format(wav_r.getframerate(), bps))
        print(
            "Bytes per sample: {}, channels: {}".format(
                wav_r.getsampwidth(), wav_r.getnchannels()
            )
        )

        # Construct the audio format, extracted from the wav file.
        aud_fmt = AudioFormat(
            format=AudioFormat.AUD_FMT_S16
            if wav_r.getsampwidth() == 2
            else AudioFormat.AUD_FMT_U8,
            channels=AudioFormat.Stereo
            if wav_r.getnchannels() == 2
            else AudioFormat.Mono,
            samplingRate=wav_r.getframerate(),
        )

        # Chunk them into audio packets.
        while wav_r.tell() < wav_r.getnframes():
            byts = wav_r.readframes(chunk_size)
            yield AudioPacket(format=aud_fmt, audio=byts)



def main(args):
    start = timer()

    # Connect to the default emulator.
    stub = get_default_emulator().get_emulator_controller()

    # Open the wav file, and generate a stream of audio packets.
    audio_packet_stream = readAudioFile(args[1])

    # Send the audio to the gRPC "microphone"
    stub.injectAudio(audio_packet_stream)

    end = timer()
    print("Send audio for approx {} seconds.".format(end - start))


if __name__ == "__main__":
    main(sys.argv)
