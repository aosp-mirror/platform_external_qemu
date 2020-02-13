@echo off

:: Install dependencies if needed
python -c "import absl" 2>NUL
if errorlevel 1 (
  python -m pip install absl-py
)

python -c "import requests" 2>NUL
if errorlevel 1 (
  python -m pip install requests
)

python -c "import enum" 2>NUL
if errorlevel 1 (
  python -m pip install enum34
)

python android\build\python\cmake.py  %*
goto :eof

:errorNoPython
echo Error^ No python interpreter available.

