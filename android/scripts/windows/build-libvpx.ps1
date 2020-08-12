# Copyright 2020 - The Android Open Source Project
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

$scriptRoot = Split-Path -Path $MyInvocation.InvocationName -Parent

. "$scriptRoot\utils\package-utils.ps1"

$packageName = "libvpx-v1.8.0.tar.gz"
$projectName = $($packageName -Split ".tar")[0]
Write-Output "Building $projectName for windows-msvc-x86_64 target"

Write-Output ".. Checking for Visual Studio 2017 installation"
$vs2017 = Get-VisualStudioDirectory -Version 2017
if (-Not (Test-Path $vs2017)) {
    Write-Error "Visual Studio 2017 is not installed. Please install VS2017."
    Exit 1
 }

 # Use Windows Subsystem for linux to generate VS solution files
Write-Output ".. Checking for msbuild.exe"
if ((Get-Command "MSBuild.exe" -ErrorAction SilentlyContinue) -eq $null) {
    Write-Error "MSBuild.exe not found. Did you run build-libvpx.cmd? Or run in developer cmd prompt."
    Exit 1
}

# Use Windows Subsystem for linux to generate VS solution files
Write-Output ".. Checking for Windows subsystem for Linux (wsl.exe)"
if ((Get-Command "wsl.exe" -ErrorAction SilentlyContinue) -eq $null) {
    Write-Error "Windows subsystem for linux (wsl.exe) not installed"
    Exit 1
}

$package = "$(Get-ArchiveDirectory)\$packageName"
$buildDir = "$(Get-BuildDirectory)\build-$projectName"
Write-Verbose "Build directory: [$buildDir]"

Write-Output ".. Extracting $package to $buildDir"
Decompress-TarArchive -Source $package -Dest $buildDir

Write-Output "..Generating VS solution files from configure script"

#trap {
#    Write-Output "Something failed while building libvpx"
#    Exit 1
#}
Function runConfigure {
    trap {
        "Something failed while running configure script"
        break
    }
    cd $buildDir
    $configOpts = (
     "--target=x86_64-win64-vs15 " +
     "--enable-static-msvcrt " +
     "--enable-static " +
     "--enable-vp9-highbitdepth " +
     "--disable-tools " +
     "--disable-unit-tests " +
     "--disable-docs " +
     "--disable-examples")
    $configOpts
    wsl -e sh -c "./configure $configOpts"
    wsl -e sh -c make V=1
}

runConfigure
# TODO: Move into package-utils.ps1
Write-Output "Build complete. Deleting $buildDir"
#Remove-Item -Recurse $buildDir