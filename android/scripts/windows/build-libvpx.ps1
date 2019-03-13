#Import-Module $PSScriptRoot\utils\package_utils.psm1
using module '.\utils\package_utils.psm1'

[Package]$libvpx_pkg = Get-Package "libvpx-v1.8.0"
Write-Host "Found libvpx [$libvpx_pkg]"

Initialize-BuildDir($libvpx_pkg)
