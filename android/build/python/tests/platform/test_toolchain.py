# Copyright 2022 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import pytest
import platform
from aemu.platform.toolchains import Toolchain
from collections import namedtuple


@pytest.fixture
def mac_m1(mocker):
    Uname = namedtuple("uname", "version system")
    mocker.patch.object(platform, "system", return_value="Darwin")
    mocker.patch.object(platform, "machine", return_value="arm64")
    mocker.patch.object(
        platform,
        "uname",
        return_value=Uname(
            "Darwin Kernel Version 21.6.0: Thu Sep 29 20:13:56 PDT 2022; root:xnu-8020.240.7~1/RELEASE_ARM64_T6000",
            "Darwin",
        ),
    )

@pytest.fixture
def mac_x86(mocker):
    Uname = namedtuple("uname", "version system")
    mocker.patch.object(platform, "system", return_value="Darwin")
    mocker.patch.object(platform, "machine", return_value="x86_64")
    mocker.patch.object(
        platform,
        "uname",
        return_value=Uname(
            "Darwin Kernel Version 21.6.0: Thu Sep 29 20:13:56 PDT 2022;",
            "Darwin",
        ),
    )



@pytest.fixture
def windows(mocker):
    Uname = namedtuple("uname", "version system")
    mocker.patch.object(platform, "system", return_value="Windows")
    mocker.patch.object(platform, "machine", return_value="AMD64")
    mocker.patch.object(
        platform,
        "uname",
        return_value=Uname(
            "10.0.220000",
            "Windows",
        ),
    )

@pytest.fixture
def linux(mocker):
    Uname = namedtuple("uname", "version system")
    mocker.patch.object(platform, "system", return_value="Linux")
    mocker.patch.object(platform, "machine", return_value="x86_64")
    mocker.patch.object(
        platform,
        "uname",
        return_value=Uname(
            "#1 SMP PREEMPT_DYNAMIC Debian 5.18.16-1rodete4 (2022-10-21)",
            "Linux",
        ),
    )

def test_infers_default():
    assert Toolchain("foo", None).target is not None


def test_infers_mac_arm(mac_m1):
    toolchain = Toolchain("foo", None)
    assert toolchain.target == "darwin-aarch64"
    assert not toolchain.is_crosscompile()

def test_infers_mac_x86(mac_x86):
    toolchain = Toolchain("foo", None)
    assert toolchain.target == "darwin-x86_64"
    assert not toolchain.is_crosscompile()


def test_infers_windows_x86(windows):
    toolchain = Toolchain("foo", None)
    assert toolchain.target == "windows-x86_64"
    assert not toolchain.is_crosscompile()


def test_infers_linux_x86(linux):
    toolchain = Toolchain("foo", None)
    assert toolchain.target == "linux-x86_64"
    assert not toolchain.is_crosscompile()

def test_is_crosscompile_x86_to_arm(linux):
    toolchain = Toolchain("foo", "linux-aarch64")
    assert toolchain.target == "linux-aarch64"
    assert toolchain.is_crosscompile()