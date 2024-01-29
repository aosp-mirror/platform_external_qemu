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
import platform
from pathlib import Path

import pytest
from aemu.discovery.emulator_discovery import get_discovery_directories

test_expected = [
    ("Linux", {"XDG_RUNTIME_DIR": "foo"}, "foo"),
    (
        "Darwin",
        {"HOME": "foo"},
        os.path.join("foo", "Library", "Caches", "TemporaryItems"),
    ),
    ("Windows", {"LOCALAPPDATA": "foo"}, os.path.join("foo", "Temp")),
]


@pytest.mark.parametrize("target_os,env,expected", test_expected)
def test_get_discovery_directory(target_os, env, expected, mocker):
    env["ANDROID_SDK_HOME"] = "bar"
    mocker.patch.dict(os.environ, env)
    mocker.patch.object(Path, "home", return_value=Path("foo"))
    mocker.patch.object(Path, "exists", return_value=True)
    mocker.patch("platform.system", return_value=target_os)
    disc = get_discovery_directories()
    assert Path(expected) / "avd" / "running" in disc


test_fallback = [
    ("Linux", {"XDG_RUNTIME_DIR": "foo"}),
    ("Darwin", {"HOME": "foo"}),
    ("Windows", {"LOCALAPPDATA": "foo"}),
]


@pytest.mark.parametrize("target_os,env", test_fallback)
def test_get_discovery_directory_fallback(target_os, env, mocker):
    if platform.system() == "Windows" and target_os != "Windows":
        pytest.skip()

    env["ANDROID_EMULATOR_HOME"] = "bar"
    mocker.patch.dict(os.environ, env)
    mocker.patch.object(os, "getuid", return_value="bar", create=True)
    mocker.patch("platform.system", return_value=target_os)
    disc = get_discovery_directories()
    assert Path("bar") / "avd" / "running" in disc


@pytest.mark.parametrize("target_os,env", test_fallback)
def test_get_discovery_directory_fallback(target_os, env, mocker):
    env["ANDROID_SDK_HOME"] = "bar"
    mocker.patch.dict(os.environ, env)
    mocker.patch.object(os, "getuid", return_value="bar", create=True)
    mocker.patch("platform.system", return_value=target_os)
    disc = get_discovery_directories()
    assert Path("bar") / ".android" / "avd" / "running" in disc
