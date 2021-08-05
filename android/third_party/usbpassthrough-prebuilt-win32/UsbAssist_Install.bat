@echo off

set action=install

:parse_input:
    if [%1] == [] (
        goto %action%
    )
    if [%1] == [-u] (
        set action=uninstall
        shift
        goto parse_input
    )
    if [%1] == [-v] (
        set action=checkinstall
        shift
        goto parse_input
    )

    REM Silently drop other parameters from haxm installer
    if [%1]==[-log] (
        shift
        shift
        goto parse_input
    )
    if [%1]==[-m] (
        shift
        shift
        goto parse_input
    )
    goto invalid_param
        

REM %ERRORLEVEL% seems not to be reliable and thus the extra work here
:install
    sc query UsbAssist > NUL 2>&1 || goto __install
    sc stop UsbAssist
    sc delete UsbAssist
:__install
    RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection DefaultInstall 132 .\UsbAssist.Inf
    sc start UsbAssist || exit /b 1
    exit /b 0

:uninstall
    sc query UsbAssist >null 2>&1 || exit /b 0
    sc stop UsbAssist
    sc delete UsbAssist || exit /b 1
    exit /b 0

:checkinstall
    sc query UsbAssist || exit /b 1
    exit /b 0

:invalid_param
    echo invalid parameter for %1
    exit /b 1

