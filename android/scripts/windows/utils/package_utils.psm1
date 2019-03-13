Import-Module $PSScriptRoot\common.psm1

class Package {
    # basename of the package
    [string]$Basename
    # full file path of the package
    [string]$FullName
    # patch file path, if any
    [string]$PatchFullName

    Package(
        [string]$basename,
        [string]$fullname,
        [string]$patchfile
    ) {
        $this.Basename = $basename
        $this.FullName = $fullname
        $this.PatchFullName = $patchfile
    }

    [string]ToString() {
        return ("basename={0} file={1} patches={2}" -f $this.Basename, $this.fullname, $this.patchfile)
    }
}

function Get-PrebuiltsDir {
    $aosp_dir = Get-AospRootDir
    return (Get-Item -Path "$aosp_dir\prebuilts").FullName
}

function Get-ArchiveDir {
    $prebuilts_dir = Get-PrebuiltsDir
    return (Get-Item -Path "$prebuilts_dir\android-emulator-build\archive").FullName
}

function Get-Package($pkg_basename) {
    # Attempts to find a package in the archives dir that starts with $pkg_basename
    # Returns the full path of the file
    $archive_dir = Get-ArchiveDir

    $files = Get-ChildItem -Path "$archive_dir\$pkg_basename*" -Exclude *-patches.*
    if (!$files) {
        Write-Host "Unable to locate pkg=[$pkg_basename]"
        return $null
    }

    # Just return the first result
    $fullname = $files[0].FullName
    # *.Basename doesn't give the correct result, so let's just split on the ".tar" part
    $basename = ($files[0].Basename -Split "\.tar")[0]
    $patch = $null

    # Try to find the patches file based on the basename
    $patch_file = Get-ChildItem -Path "$archive_dir\$basename-patches.*"
    if ($patch_file) {
        Write-Verbose "Found patch file [$patch_file]"
        $patch = $patch_file.FullName
    }
    
    $res = [Package]::new($basename, $fullname, $patch);
    return $res
}

function Initialize-BuildDir([Package]$pkg, [string]$buildDir) {
    # Generates the build directory where the src is unzipped and built.
    # if $buildDir is null or empty, a temp directory will be used.
    Write-Verbose "Building $pkg.basename"

    $basename = $pkg.Basename
    $global:temp_dir = "$env:TEMP\build-$basename"
    if ($buildDir) {
        $global:temp_dir = $buildDir
    } else {
        # delete the temp directory on exit
        Register-EngineEvent PowerShell.Exiting –Action {
            Write-Host "Deleting [$global:temp_dir]"
            if (Test-Path -Path "$global:temp_dir") {
                #Remove-Item "$global:temp_dir" -Force
            }
        }
    }
    Write-Host "BuildDir=[$global:temp_dir]"
    New-Item -Path "$global:temp_dir" -ItemType directory | out-null

    # TODO: support more than just .tar format
    $tarFile = $pkg.FullName
    Expand-Tar -tarFile "$tarFile" -dest "$global:temp_dir"
     
}

function Expand-Tar($tarFile, $dest) {
#    if (-not (Get-Command Expand-7Zip -ErrorAction Ignore)) {
#        Install-Package -Scope CurrentUser -Force 7Zip4PowerShell
#    }
    # you'll need to install 7Zip4PowerShell:
    # > Install-Module 7Zip4PowerShell -Force

    Write-Host "Expanding [$tarFile] to [$dest]"
    Expand-7Zip $tarFile $dest
}
