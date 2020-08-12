# Copyright 2020 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# TODO: add support to parse prebuilts\android-emulator-build\archive\PACKAGES.TXT

Function Get-AndroidRootDirectory {
    $scriptDir = $MyInvocation.PSScriptRoot
    $androidRootDir = $scriptDir -Split "\\external\\qemu"
    return $androidRootDir[0]
}

Function Get-PrebuiltsDirectory {
    return "$(Get-AndroidRootDirectory)\prebuilts\android-emulator-build"
}

Function Get-ArchiveDirectory {
    return "$(Get-PrebuiltsDirectory)\archive"
 }

 Function Get-BuildDirectory
 {
    return $env:TEMP
 }

 Function Decompress-TarArchive
 {
    Param($Source, $Dest)
    New-Item -ItemType directory -Path $Dest > $null
    # Recent versions of Windows has tar.exe built-in.
    tar -zxf $Source -C $Dest
 }

 Function Get-VisualStudioDirectory
 {
    Param($Version)
    return "C:\Program Files (x86)\Microsoft Visual Studio\$version"
 }