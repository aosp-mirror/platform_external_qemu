Import-Module $PSScriptRoot\utils\package_utils.psm1

# Test all utils\common.psm1 functions
$aosp_dir = Get-AospRootDir
Write-Host "AOSP_DIR=$aosp_dir"

# Test all utils\package_utils.psm1 functions
$prebuilts_dir = Get-PrebuiltsDir
Write-Host "PREBUILTS_DIR=$prebuilts_dir"
