@echo off
setlocal

set AVD_NAME=%1
if "%AVD_NAME%"=="" (
  echo Usage: %~nx0 avd_name
  exit /b
)

if "%AVD_DATA_ROOT%"=="" (
  set AVD_DATA_ROOT=%USERPROFILE%\.android\avd
)

set AVD_CONFIG_INI=%AVD_DATA_ROOT%\%AVD_NAME%.avd\config.ini
if not exist %AVD_CONFIG_INI% (
  echo AVD config file %AVD_CONFIG_INI% does not exist
  echo Failed
  exit /b
)

:: Sanity check
findstr hw.arc %AVD_CONFIG_INI%
if not ERRORLEVEL 1 (
  echo AVD %AVD_NAME% is already a Chrome OS AVD.
  echo Failed
  exit /b
)
findstr hw.cpu.arch=x86 %AVD_CONFIG_INI%
if ERRORLEVEL 1 (
  echo AVD %AVD_NAME% is not an x86 image
  echo Failed
  exit /b
)
findstr hw.cpu.arch=x86_64 %AVD_CONFIG_INI%
if not ERRORLEVEL 1 (
  echo AVD %AVD_NAME% is an x86_64 image
  echo Failed
  exit /b
)
findstr /r image.sysdir.1=system-images\\chromeos-m.*\\x86 %AVD_CONFIG_INI%
if ERRORLEVEL 1 (
  echo AVD %AVD_NAME% is not a Chrome OS image
  echo Failed
  exit /b
)

echo Updating %AVD_CONFIG_INI%
:: Set Chrome OS mode
copy %AVD_CONFIG_INI% %AVD_CONFIG_INI%.old
:: Remove old cpu/gpu setting from config.ini
findstr -v hw.cpu.arch %AVD_CONFIG_INI%.old|findstr -v hw.gpu > %AVD_CONFIG_INI%
echo hw.arc=yes >> %AVD_CONFIG_INI%
:: If the ABI is x86, AVDManager assumes CPU arch is x86 too. Chrome OS requires
:: x86_64 for the CPU arch, even if the Android ABI is x86.
echo hw.cpu.arch=x86_64 >> %AVD_CONFIG_INI%
:: Disable GPU acceleration, Chrome OS uses software rendering atm.
echo hw.gpu.enabled=no >> %AVD_CONFIG_INI%
echo hw.gpu.mode=off >> %AVD_CONFIG_INI%
echo Success!
