@echo off
REM Copyright 2023 - The Android Open Source Project
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

REM A script to build a qemu release using the Google AOSP toolchain

REM Check if the correct number of arguments is provided
if "%~2"=="" (
    echo Configure and build qemu for Windows
    echo Usage: %0 ^<destination^> ^<distribution^>
    exit /b 1
)

REM Get the directory of the script
for %%I in (%0) do set "script_dir=%%~dpI"
set "aosp_dir=%script_dir%\..\..\..\.."
REM Convert relative path to absolute path
for %%I in ("%aosp_dir%") do set "absolute_aosp=%%~fI"

REM Add compatiblity executables on the path
set PATH=%absolute_aosp%\external\qemu\google\compat\windows\bin;%PATH%

REM Assign parameters to variables
set "destination=%1"
set "distribution=%2"

echo -- Creating a virtual environment in %destination%
call python -m venv "%destination%\venv"

echo -- Activating the virtual environment
call %destination%\venv\Scripts\activate.bat

echo -- Installing the package from %script_dir% for building qemu
call pip install --no-cache-dir %script_dir%

echo -- Launching qemu configuration and starting a release
call amc setup --force --aosp "%absolute_aosp%"  "%destination%\qemu-bld"
call amc compile "%destination%\qemu-bld"
rem call amc test "%destination%\qemu-bld"
call amc release "%destination%\qemu-bld"  "%distribution%"