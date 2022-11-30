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
import time
from aemu.process.command import Command, CommandFailedException
from aemu.process.log_handler import LogHandler

@pytest.mark.skipif(platform.system() == "Windows", reason="false binary does not exist in windows")
def test_failing_run_throws():
    with pytest.raises(CommandFailedException):
        Command(["false"]).run()


@pytest.mark.skipif(platform.system() == "Windows", reason="false binary does not exist in windows")
def test_running_true_succeeds():
    Command(["true"]).run()

def test_log_filter_gets_called():
    called = False

    def log_transform(x):
        nonlocal called
        called = True

    handler = LogHandler(log_transform)

    Command(["echo", "Hello"]).with_log_handler(handler).run()

    # There are other threads doing the logging. give them a chance.
    time.sleep(0.2)
    assert called
