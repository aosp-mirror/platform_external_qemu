@echo off

set action=install

:parse_input:
    if [%1] == [] (
        goto %action%
    )
    goto invalid_param

:install
    pnputil -i -a .\Android_USB_Assistant.Inf
    exit /b 0

:invalid_param
    echo invalid parameter for %1
    exit /b 1

