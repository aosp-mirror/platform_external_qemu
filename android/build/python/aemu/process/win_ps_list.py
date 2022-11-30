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
import subprocess
import logging
import shutil
import re

from aemu.process.command import Command


class WmicNotFound(Exception):
    pass


class WinPsList:
    """A PSUtil library that can list, and kill processes."""

    def __init__(self):
        self.wmic = shutil.which("wmic")
        if not self.wmic:
            raise WmicNotFound("Unable to find the wmic executable")

    def list(self):
        """Lists all running process names with the given pid

        For example returns  { 33 : "chrome.exe", 4324: "netsimd.exe" }

        Returns:
            dict[int, str]: A map from pid to process name
        """
        process = subprocess.run(
            [self.wmic, "process", "get", "name,processid"],
            encoding="utf-8",
            check=False,
            capture_output=True,
            shell=True,
        )

        if process.returncode != 0:
            logging.warning(
                "Unable to query process list %s, %s", process.stdout, process.stderr
            )
            return {}

        pid_map = {}
        output = re.compile(r"(.*) (\d+)")
        for proc in process.stdout.splitlines():
            match = output.match(proc)
            if match:
                process_name = match.group(1).strip()
                process_id = int(match.group(2))
                pid_map[process_id] = process_name

        return pid_map

    def kill_proc_by_name(self, proc_name: str):
        """Kills the given process name if it exists

        If the process exists it will be forcefully terminated using
        taskkill /f /pid xxx

        Args:
            proc_name (str): The name of the process to be killed
        """
        procs = [k for k, v in self.list().items() if v == proc_name]
        for pid in procs:
            self.kill_pid(pid)

    def kill_pid(self, pid: int):
        """Kills the given pid.

        This will execute taskkill /f /pid pid, and can raise
        an exception if the pid does not exist.

        Args:
            pid (int): The process id to be terminated
        """
        # Forcefully terminate the pid
        Command(["taskkill", "/F", "/PID", pid]).run()
