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
from pathlib import Path

import aemu.discovery.emulator_discovery
import grpc
import psutil
import pytest
from aemu.discovery.emulator_discovery import EmulatorDescription, get_default_emulator


@pytest.fixture
def fake_emu_pid_file(tmpdir_factory, mocker):
    tmp_pid = Path(tmpdir_factory.mktemp("data") / "pid_1234.ini")
    with open(tmp_pid, "w") as w:
        w.write("grpc.port=8554\n")

    mocker.patch.object(
        aemu.discovery.emulator_discovery,
        "get_discovery_directories",
        return_value=[tmp_pid.parent],
    )
    mocker.patch.object(Path, "glob", return_value=[tmp_pid])
    mocker.patch.object(EmulatorDescription, "is_alive", return_value=True)
    mocker.patch.object(psutil, "pid_exists", return_value=True)


@pytest.fixture
def fake_emu_pid_file_with_token(tmpdir_factory, mocker):
    tmp_pid = Path(tmpdir_factory.mktemp("data") / "pid_1234.ini")
    with open(tmp_pid, "w") as w:
        w.write("grpc.port=8554\n")
        w.write("grpc.token=abcd")

    mocker.patch.object(
        aemu.discovery.emulator_discovery,
        "get_discovery_directories",
        return_value=[tmp_pid.parent],
    )
    mocker.patch.object(Path, "glob", return_value=[tmp_pid])
    mocker.patch.object(EmulatorDescription, "is_alive", return_value=True)
    mocker.patch.object(psutil, "pid_exists", return_value=True)


def test_get_default_channel(fake_emu_pid_file):
    """Test you gets a standard insecure channel."""
    channel = get_default_emulator().get_grpc_channel()
    assert channel is not None
    assert isinstance(channel, grpc.Channel)
    assert channel.__class__.__name__ == "Channel"


def test_get_default_async_channel(fake_emu_pid_file):
    """Test you gets a standard async insecure channel."""
    channel = get_default_emulator().get_async_grpc_channel()
    assert channel is not None
    assert isinstance(channel, grpc.aio.Channel)
    assert channel.__class__.__name__ == "Channel"


def test_get_token_channel(fake_emu_pid_file_with_token):
    """Test you gets a standard insecure channel with interceptor."""
    channel = get_default_emulator().get_grpc_channel()
    assert channel is not None
    # Note the hijacked channel class!
    assert channel.__class__.__name__ == "_Channel"


def test_get_token_async_channel(fake_emu_pid_file_with_token):
    """Test you gets a standard insecure channel with interceptor."""
    channel = get_default_emulator().get_async_grpc_channel()
    assert channel is not None
    # Note the hijacked channel class!
    assert channel.__class__.__name__ == "Channel"
