set PYTHON=./../../../../../prebuilts/python/windows/x86/python3
%PYTHON% -m venv
start venv\Scripts\activate.bat

rem let's write out the pip.conf used to pull local packages
echo "[global]" > venv/pip.conf
echo "timeout=1" >> venv/pip.conf
echo "%~dp0/repo/simple" >> venv/pip.conf

rem install all the requirements
pip install --pre  --extra-index-url repo/simple -e .\[test\]