@echo off
pip3 install patch absl-py --user
python3 %~dp0\build-breakpad-msvc.py %*