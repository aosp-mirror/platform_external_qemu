@echo off
AT > NUL
IF NOT %ERRORLEVEL% EQU 0 (
    ECHO Need Administrator privileges to set clang symlink. Exiting...
    PING 127.0.0.1 > NUL 2>&1
    EXIT /B 1
)
echo Make sure you are in a visual studio environment, with ninja available!

SET CLANG_VER=clang-r346389
SET CLANG_ROOT=%~dp0\..\..\..\..\prebuilts\clang\host\windows-x86\%CLANG_VER%

rem Discover absolute directory, and setup clang symlink
pushd %CLANG_ROOT%
set ABS_CLANG_PATH=%CD%
mklink clang-cl.exe clang.exe
popd

rem Configure clang
cmake -H. -G Ninja -Bbuild -DCMAKE_C_COMPILER:PATH="%ABS_CLANG_PATH%\bin\clang-cl.exe" -DCMAKE_CXX_COMPILER:PATH="%ABS_CLANG_PATH%\bin\clang-cl.exe"
ninja -C build