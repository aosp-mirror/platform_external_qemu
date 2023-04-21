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
import platform

from aemu.process.win_ps_list import WinPsList
from aemu.tasks.build_task import BuildTask


class KillNetsimdTask(BuildTask):
    """Kills any lingering netsimd tasks on windows."""

    def __init__(self) -> None:
        super().__init__()
        if platform.system() != "Windows":
            self.enable(False)

    def do_run(self) -> None:
        if platform.system() != "Windows":
            return

        wps = WinPsList()
        wps.kill_proc_by_name("netsimd.exe")
