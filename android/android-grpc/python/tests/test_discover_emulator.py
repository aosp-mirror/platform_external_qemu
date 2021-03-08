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
import pytest

import aemu.discovery.emulator_discovery
from aemu.discovery.emulator_discovery import EmulatorDiscovery, get_default_emulator


@pytest.fixture(scope="session")
def fake_emu_pid_file(tmpdir_factory):
    fn = str(tmpdir_factory.mktemp("data").join("pid_1234.ini"))
    with open(fn, "w") as w:
        w.write("port.serial=5554\n")
        w.write("port.adb=5555\n")
        w.write("avd.name=Q\n")
        w.write("avd.dir=/home/.android/avd/Q.avd\n")
        w.write("avd.id=Q\n")
        w.write("cmdline=unused\n")
        w.write("grpc.port=8554\n")
    return fn


@pytest.fixture(scope="session")
def bad_emu_pid_file(tmpdir_factory):
    fn = str(tmpdir_factory.mktemp("data").join("pid_1234.ini"))
    with open(fn, "w") as w:
        w.write("bad_new_bears\n")
    return fn


def test_no_pid_file_means_no_emulator(mocker):
    mocker.patch.object(os.path, "exists", return_value=True)
    mocker.patch.object(os, "listdir", return_value=["pid_nope.ini"])
    emu = EmulatorDiscovery()
    assert emu.available() == 0


def test_bad_pid_file_means_no_emulator(mocker, bad_emu_pid_file):
    path = os.path.dirname(bad_emu_pid_file)
    mocker.patch.object(
        aemu.discovery.emulator_discovery, "get_discovery_directory", return_value=path
    )
    emu = EmulatorDiscovery()
    assert emu.available() == 0


def test_can_parse_and_read_emu_file(mocker, fake_emu_pid_file):
    path = os.path.dirname(fake_emu_pid_file)
    pid_file = os.path.basename(fake_emu_pid_file)
    mocker.patch.object(
        aemu.discovery.emulator_discovery, "get_discovery_directory", return_value=path
    )
    mocker.patch.object(os, "listdir", return_value=[pid_file])
    emu = EmulatorDiscovery()
    assert emu.available() == 1
    assert emu.find_by_pid(1234) is not None
    assert emu.find_by_pid(1234).name() == "emulator-5554"


def test_finds_default_emulator(mocker, fake_emu_pid_file):
    path = os.path.dirname(fake_emu_pid_file)
    pid_file = os.path.basename(fake_emu_pid_file)
    mocker.patch.object(
        aemu.discovery.emulator_discovery, "get_discovery_directory", return_value=path
    )
    mocker.patch.object(os, "listdir", return_value=[pid_file])
    assert get_default_emulator() is not None
    assert get_default_emulator().name() == "emulator-5554"
