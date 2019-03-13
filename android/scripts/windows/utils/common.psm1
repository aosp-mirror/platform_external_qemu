function Get-AospRootDir {
    # Assume we are in external\qemu\android\scripts\windows\utils
    return (Get-Item -Path "$PSScriptRoot\..\..\..\..\..\..").FullName
}
