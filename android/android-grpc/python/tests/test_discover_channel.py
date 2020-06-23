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
from aemu.discovery.emulator_discovery import get_default_emulator


@pytest.fixture(scope="session")
def fake_emu_pid_file(tmpdir_factory):
    fn = str(tmpdir_factory.mktemp("data").join("pid_1234.ini"))
    with open(fn, "w") as w:
        w.write("grpc.port=8554\n")
    return fn


@pytest.fixture(scope="session")
def fake_emu_pid_file_with_token(tmpdir_factory):
    fn = str(tmpdir_factory.mktemp("data").join("pid_1234.ini"))
    with open(fn, "w") as w:
        w.write("grpc.port=8554\n")
        w.write("grpc.token=abcd")
    return fn


def test_get_default_channel(mocker, fake_emu_pid_file):
    """Test you gets a standard insecure channel."""
    path = os.path.dirname(fake_emu_pid_file)
    pid_file = os.path.basename(fake_emu_pid_file)
    mocker.patch.object(
        aemu.discovery.emulator_discovery, "get_discovery_directory", return_value=path
    )
    mocker.patch.object(os, "listdir", return_value=[pid_file])
    channel = get_default_emulator().get_grpc_channel()
    assert channel is not None
    assert channel.__class__.__name__ == "Channel"


def test_get_token_channel(mocker, fake_emu_pid_file_with_token):
    """Test you gets a standard insecure channel with interceptor."""
    path = os.path.dirname(fake_emu_pid_file_with_token)
    pid_file = os.path.basename(fake_emu_pid_file_with_token)
    mocker.patch.object(
        aemu.discovery.emulator_discovery, "get_discovery_directory", return_value=path
    )
    mocker.patch.object(os, "listdir", return_value=[pid_file])
    channel = get_default_emulator().get_grpc_channel()
    assert channel is not None
    # Note the hijacked channel class!
    assert channel.__class__.__name__ == "_Channel"
