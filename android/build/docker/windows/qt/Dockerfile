# Copyright (C) 2023 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# This Dockerfile creates an image that has an environment to build Qt 6.5.3 on Windows.
# From testing, Qt6 will build if the container is given 24G of ram, and 16 CPU cores.
# > docker run -dt 24g --cpus 16 <image id>
#
# Note: The build failed when I gave the container only 16G ram with 16 cores. Also, mounting the
# emulator source directory doesn't seem to work, as running Qt's init-repository command fails with
# some git error.
#
# To test building Qt6 in this container on windows, these are the commands I used:
#
# - Create the container:
# > docker run -dt --memory 24g --cpus 16 <image id>
#
# - Copy emulator source directory into container:
# > docker stop <container_id>
# > docker cp <path\to\emu> <container_id>:C:\emu
#
# - Exec cmd.exe in the running container:
# > docker exec -it <container_id> cmd.exe
#
# - In the container, load VS build environment:
# > "%PROGRAMFILES(X86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" amd64
#
# - Run prebuilt build script:
# > cd C:\emu & python tools\buildSrc\servers\build_tools.py --out_dir C:\out-prebuilts --build-id 12345678 --dist_dir C:\emu-dist\12345678 --prebuilts

FROM mcr.microsoft.com/windows/servercore:ltsc2022

SHELL ["powershell.exe", "-ExecutionPolicy", "Bypass", "-Command", "$ErrorActionPreference = 'Stop'; $ProgressPreference='SilentlyContinue'; $VerbosePreference='Continue';"]

# Enable Long Paths for Win32 File/Folder APIs.
RUN New-ItemProperty -Path HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem \
        -Name LongPathsEnabled -Value 1 -PropertyType DWORD -Force

# Install Chocolatey
RUN [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# Install git, GnuWin32 gperf, bison, flex
RUN choco install --yes gperf winflexbison3 git
# rename win_bison.exe and win_flex.exe to bison.exe, flex.exe
RUN Move-Item $env:PROGRAMDATA\chocolatey\bin\win_bison.exe $env:PROGRAMDATA\chocolatey\bin\bison.exe
RUN Move-Item $env:PROGRAMDATA\chocolatey\bin\win_flex.exe $env:PROGRAMDATA\chocolatey\bin\flex.exe

# Install Strawberry Perl v5.38.0.1
RUN Invoke-WebRequest -Uri "https://github.com/StrawberryPerl/Perl-Dist-Strawberry/releases/download/SP_5380_5361/strawberry-perl-5.38.0.1-64bit.msi" -OutFile "perl-installer.msi"; \
    Start-Process msiexec -ArgumentList '/i perl-installer.msi /quiet AllUsers=1' -Wait; \
    Remove-Item perl-installer.msi

# Install Node.js v20.10.0
RUN Invoke-WebRequest -Uri "https://nodejs.org/dist/v20.10.0/node-v20.10.0-x64.msi" -OutFile "nodejs-installer.msi"; \
    Start-Process msiexec -ArgumentList '/i nodejs-installer.msi /quiet AllUsers=1' -Wait; \
    Remove-Item nodejs-installer.msi

# Install Python 3.10.11
RUN Invoke-WebRequest -Uri "https://www.python.org/ftp/python/3.10.11/python-3.10.11-amd64.exe" -OutFile "python-installer.exe"; \
    Start-Process python-installer.exe -ArgumentList '/quiet InstallAllUsers=1 PrependPath=1' -Wait; \
    Remove-Item python-installer.exe

# Install python html5lib
RUN pip.exe install html5lib

# Restore the default Windows shell for correct batch processing.
SHELL ["cmd", "/S", "/C"]

RUN \
    # Download the Build Tools bootstrapper.
    curl -SL --output vs_buildtools.exe https://aka.ms/vs/17/release/vs_buildtools.exe \
    \
    # Install Build Tools with the Microsoft.VisualStudio.Workload.AzureBuildTools workload, excluding workloads and components with known issues.
    && (start /w vs_buildtools.exe --quiet --wait --norestart --nocache \
        --installPath "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools" \
        --add Microsoft.VisualStudio.Workload.VCTools \
        --add Microsoft.VisualStudio.Component.Windows11SDK.22621 \
        --add Microsoft.VisualStudio.Component.VC.Llvm.Clang \
        --add Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset \
        --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \
        --add Microsoft.VisualStudio.Component.VC.ATLMFC \
        || IF "%ERRORLEVEL%"=="3010" EXIT 0) \
    \
    # Cleanup
    && del /q vs_buildtools.exe

ENTRYPOINT ["%PROGRAMFILES(X86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat", "amd64"]