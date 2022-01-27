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

import argparse
import sys
import time
import wave
from timeit import default_timer as timer

from aemu.discovery.emulator_discovery import get_default_emulator
from aemu.proto.emulator_controller_pb2 import AudioFormat, AudioPacket


def read_wav_file(wavefile, realtime=False):
    """Reads a wav file and chunks it into AudioPackets.."""

    # Will send chunks of 300ms
    with wave.open(wavefile, "rb") as wav_r:

        # Print some general information about the wav file we just opened.
        bps = wav_r.getframerate() * wav_r.getnchannels() * wav_r.getsampwidth()

        if wav_r.getsampwidth() > 2:
            raise Exception(
                "Samplewidth has to be 1 or 2 not: {}".format(wav_r.getsampwidth())
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
            mode=AudioFormat.MODE_REAL_TIME
            if realtime
            else AudioFormat.MODE_UNSPECIFIED,
        )

        chunk_size = int(wav_r.getframerate() / 1000 * 300)
        print("Wav file of: {} hz,  with bps: {}".format(wav_r.getframerate(), bps))
        print(
            "Bytes per sample: {}, channels: {}, chunk size: {}".format(
                wav_r.getsampwidth(), wav_r.getnchannels(), chunk_size
            )
        )
        # Chunk them into audio packets.
        while wav_r.tell() < wav_r.getnframes():
            byts = wav_r.readframes(chunk_size)
            yield AudioPacket(format=aud_fmt, audio=byts)
            if realtime:
                time.sleep(0.300)


def main(args):
    start = timer()

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--realtime",
        dest="realtime",
        action="store_true",
        help="Set this flag to provide realtime audio.",
    )

    parser.add_argument("wav", help="Wav file to be injected into the emulator")

    args = parser.parse_args()

    # Connect to the default emulator.
    stub = get_default_emulator().get_emulator_controller()

    # Open the wav file, and generate a stream of audio packets.
    audio_packet_stream = read_wav_file(args.wav, args.realtime)

    # Send the audio to the gRPC "microphone"
    stub.injectAudio(audio_packet_stream)

    end = timer()
    print("Send audio for approx {} seconds.".format(end - start))


if __name__ == "__main__":
    main(sys.argv)
