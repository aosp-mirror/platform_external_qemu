using module '.\package_utils.psm1'

Describe 'package_utils.psm1 Tests' {
  Context 'Get-PrebuiltsDir tests' {
    It 'Get-PrebuiltsDir returns valid path' {
      $dir = Get-PrebuiltsDir
      Test-Path -Path $dir
    }
  }

  Context 'Get-ArchiveDir tests' {
    It 'Get-ArchiveDir returns valid path' {
      $dir = Get-ArchiveDir
      Test-Path -Path $dir
    }
  }

  Context 'Get-Package tests' {
    It 'Get_Package returns null on unknown package' {
      $res = Get-Package -pkg_basename "not_a_pkg"
      $res | Should BeNullOrEmpty
    }

    It 'Get_Package for "glib" returns valid files + patches' {
      # Glib should have both src zip and patches zip
      [Package]$res = Get-Package -pkg_basename "glib"
      $res | Should not be $null
      Write-Verbose "res=$res"
      $res.Basename | Should not BeNullOrEmpty
      $res.Fullname | Should not BeNullOrEmpty
      Test-Path -Path $res.Fullname
      $res.PatchFullName | Should not BeNullOrEmpty
      Test-Path -Path $res.PatchFullName
    }

    It 'Get_Package for "libusb" returns valid file, but no patches' { 
      # libusb should not have a patches zip
      [Package]$res = Get-Package -pkg_basename "libusb"
      $res | Should not be $null
      Write-Verbose "res=$res"
      $res.Basename | Should not BeNullOrEmpty
      $res.Fullname | Should not BeNullOrEmpty
      Test-Path -Path $res.Fullname
      $res.PatchFullName | Should BeNullOrEmpty
    }
  }
}
