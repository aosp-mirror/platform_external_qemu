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
import os
from pathlib import Path

import aemu.discovery.emulator_discovery
import psutil
import pytest
from aemu.discovery.emulator_discovery import (
    EmulatorDescription,
    EmulatorDiscovery,
    get_default_emulator,
)


@pytest.fixture
def tmp_directory(tmp_path_factory):
    return tmp_path_factory.mktemp("data")


@pytest.fixture
def fake_emu_pid_file(tmp_directory, mocker):
    def write_pid_file(number):
        fn = Path(tmp_directory / f"pid_123{number}.ini")
        with open(fn, "w") as w:
            w.write("port.serial={}\n".format(5554 + number * 2))
            w.write("port.adb={}\n".format(5555 + number * 2))
            w.write("avd.name=Q\n")
            w.write("avd.dir=/home/.android/avd/Q.avd\n")
            w.write("avd.id=Q\n")
            w.write("cmdline=unused\n")
            w.write("grpc.port={}\n".format(8554 + number * 2))

        mocker.patch.object(
            aemu.discovery.emulator_discovery,
            "get_discovery_directories",
            return_value=[tmp_directory],
        )
        mocker.patch.object(EmulatorDescription, "is_alive", return_value=True)
        mocker.patch.object(psutil, "pid_exists", return_value=True)

    return write_pid_file


@pytest.fixture
def bad_emu_pid_file(tmp_directory, mocker):
    bad_file = Path(tmp_directory / "pid_1234.ini")
    with open(bad_file, "w") as w:
        w.write("bad_new_bears\n")

    mocker.patch.object(
        aemu.discovery.emulator_discovery,
        "get_discovery_directories",
        return_value=[bad_file],
    )


def test_no_pid_file_means_no_emulator(mocker):
    mocker.patch.object(os.path, "exists", return_value=True)
    mocker.patch.object(os, "listdir", return_value=["pid_nope.ini"])
    emu = EmulatorDiscovery()
    assert emu.available() == 0


def test_bad_pid_file_means_no_emulator(bad_emu_pid_file):
    assert EmulatorDiscovery().available() == 0


def test_can_parse_and_read_emu_file(fake_emu_pid_file):
    fake_emu_pid_file(0)
    emu = EmulatorDiscovery()
    assert emu.available() == 1
    assert emu.find_by_pid(1230) is not None
    assert emu.find_by_pid(1230).name() == "emulator-5554"


def test_finds_default_emulator(mocker, fake_emu_pid_file):
    fake_emu_pid_file(0)
    assert get_default_emulator() is not None
    assert get_default_emulator().name() == "emulator-5554"


def test_finds_two_emulators(fake_emu_pid_file):
    fake_emu_pid_file(0)
    fake_emu_pid_file(1)

    discovery = EmulatorDiscovery()
    assert discovery.available() == 2
    assert discovery.find_by_pid("1230").name() == "emulator-5554"
    assert discovery.find_by_pid("1231").name() == "emulator-5556"


def test_create_description_from_uri():
    assert EmulatorDiscovery.connection("localhost:8444") is not None


def test_create_description_from_uri_is_same():
    assert EmulatorDiscovery.connection(
        "localhost:8444"
    ) == EmulatorDiscovery.connection("localhost:8444")
