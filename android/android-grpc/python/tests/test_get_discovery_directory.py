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
import pytest
import os
import platform
from aemu.discovery.emulator_discovery import get_discovery_directory

test_expected = [
    ("Linux", {"XDG_RUNTIME_DIR": "foo"}, "foo"),
    (
        "Darwin",
        {"HOME": "foo"},
        os.path.join("foo", "Library", "Caches", "TemporaryItems"),
    ),
    ("Windows", {"LOCALAPPDATA": "foo"}, os.path.join("foo", "Temp")),
]


@pytest.mark.parametrize("platform,env,expected", test_expected)
def test_get_discovery_directory(platform, env, expected, mocker):
    env["ANDROID_SDK_HOME"] = "bar"
    mocker.patch.dict(os.environ, env)
    mocker.patch.object(os.path, "exists", return_value=True)
    mocker.patch("platform.system", return_value=platform)
    disc = get_discovery_directory()
    assert disc == os.path.join(expected, "avd", "running")


test_fallback = [
    ("Linux", {"XDG_RUNTIME_DIR": "foo"}),
    ("Darwin", {"HOME": "foo"}),
    ("Windows", {"LOCALAPPDATA": "foo"}),
]


@pytest.mark.parametrize("platform,env", test_fallback)
def test_get_discovery_directory_fallback(platform, env, mocker):
    env["ANDROID_EMULATOR_HOME"] = "bar"
    mocker.patch.dict(os.environ, env)
    mocker.patch.object(os, "getuid", return_value="bar")
    mocker.patch("platform.system", return_value=platform)
    disc = get_discovery_directory()
    assert disc == os.path.join("bar", "avd", "running")


@pytest.mark.parametrize("platform,env", test_fallback)
def test_get_discovery_directory_fallback(platform, env, mocker):
    env["ANDROID_SDK_HOME"] = "bar"
    mocker.patch.dict(os.environ, env)
    mocker.patch.object(os, "getuid", return_value="bar")
    mocker.patch("platform.system", return_value=platform)
    disc = get_discovery_directory()
    assert disc == os.path.join("bar", ".android", "avd", "running")

