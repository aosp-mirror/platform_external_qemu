#  Copyright (C) 2024 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE2.0
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
import time
import sys
import wave
from timeit import default_timer as timer

from aemu.discovery.emulator_discovery import get_default_emulator
from aemu.proto.emulator_controller_pb2 import AudioFormat, AudioPacket
from aemu.proto.emulator_controller_pb2_grpc import EmulatorControllerStub


def read_wav_file(wavefile, realtime=False):
    """
    Reads a WAV file and yields it in chunks as AudioPacket objects for streaming.

    Args:
        wavefile (str): The path to the WAV file.
        realtime (bool, optional): If True, simulates real-time audio by adding
                                   delays between chunks. Defaults to False.

    Yields:
        AudioPacket: An AudioPacket object containing the audio data and format.

    Raises:
        Exception: If the WAV file's sample width is not 1 or 2 bytes.
    """
    # Type hint for wav_r
    wav_r: wave.Wave_read
    with wave.open(wavefile, "rb") as wav_r:
        # Extract and validate WAV file properties
        sample_width = wav_r.getsampwidth()
        if sample_width > 2:
            raise Exception(f"Sample width must be 1 or 2 bytes, not {sample_width}")

        # Construct AudioFormat based on WAV file properties
        audio_format = AudioFormat(
            format=AudioFormat.AUD_FMT_S16
            if sample_width == 2
            else AudioFormat.AUD_FMT_U8,
            channels=AudioFormat.Stereo
            if wav_r.getnchannels() == 2
            else AudioFormat.Mono,
            samplingRate=wav_r.getframerate(),
            mode=AudioFormat.MODE_REAL_TIME
            if realtime
            else AudioFormat.MODE_UNSPECIFIED,
        )

        chunk_size = int(wav_r.getframerate() / 1000 * 300)  # 300ms chunks

        # Calculate bits per sample
        bps: int = wav_r.getsampwidth() * 8

        # Calculate bits per second
        bps_per_second: int = bps * wav_r.getnchannels() * wav_r.getframerate()

        # Print WAV file information
        print(f"WAV file: {wav_r.getframerate()} Hz, {bps_per_second} bps")
        print(f"Sample width: {sample_width} bytes, Channels: {wav_r.getnchannels()}")
        print(f"Chunk size: {chunk_size} bytes")

        # Yield audio data in chunks
        while wav_r.tell() < wav_r.getnframes():
            data = wav_r.readframes(chunk_size)
            yield AudioPacket(format=audio_format, audio=data)

            if realtime:
                time.sleep(0.300)  # Simulate real-time audio delay


def main(args):
    """
    Injects audio from a WAV file into the emulator's microphone over gRPC.

    Args:
        args: The parsed command-line arguments from argparse.
    """

    start_time = timer()

    # Parse command-line arguments
    parser = argparse.ArgumentParser(
        description="Inject audio from a WAV file into a gRPC emulator."
    )
    parser.add_argument(
        "wav", help="The path to the WAV file to inject into the emulator."
    )
    parser.add_argument(
        "--realtime",
        action="store_true",
        help="Inject audio in real time, with delays to simulate actual playback.",
    )
    args = parser.parse_args()

    # Connect to the emulator
    emulator = get_default_emulator()
    stub = EmulatorControllerStub(
        emulator.get_grpc_channel([("emulator.security", "token")])
    )

    # Send audio stream to the emulator's microphone
    try:
        audio_stream = read_wav_file(args.wav, args.realtime)
        stub.injectAudio(audio_stream)
    except Exception as e:
        print(f"Error injecting audio: {e}")
    else:
        end_time = timer()
        print(f"Audio injected for approximately {end_time - start_time:.2f} seconds.")


if __name__ == "__main__":
    main(sys.argv)
