:: Runs all *.Tests.ps1 files
:: If you don't have Pester installed, just run the following in a powershell:
:: > Install-Module Pester
::
:: Do run just one Test file (e.g. common.Tests.ps1):
:: > Invoke-Pester 'common.Tests.ps1'
::
:: To run all *.Tests.ps1 in a dir:
:: > Invoke-Pester 'C:\path\to\dir'

powershell -Command "Invoke-Pester '%~dp0'"
