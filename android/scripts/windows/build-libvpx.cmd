@echo off

REM Copyright 2020 - The Android Open Source Project
REM
REM Licensed under the Apache License, Version 2.0 (the',  help='License');
REM you may not use this file except in compliance with the License.
REM You may obtain a copy of the License at
REM
REM     http://www.apache.org/licenses/LICENSE-2.0
REM
REM Unless required by applicable law or agreed to in writing, software
REM distributed under the License is distributed on an',  help='AS IS' BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM See the License for the specific language governing permissions and
REM limitations under the License.

REM Initialize developer environment
REM TODO: check for VS2017

CMD /C "call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat" x64 & powershell -file "%~dp0\build-libvpx.ps1""
