$scriptRoot = Split-Path $MyInvocation.MyCommand.Path
Import-Module $scriptRoot\common.psm1

Describe 'common.psm1 Tests' {
  Context 'Get-AospRootDir tests' {
    It 'Get-AospRootDir returns valid path' {
      $aosp_dir = Get-AospRootDir
      Test-Path -Path $aosp_dir
    }
  }
}
