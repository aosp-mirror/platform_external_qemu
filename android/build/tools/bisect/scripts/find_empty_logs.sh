# Script that will find the culprit of empty logfiles
emu-bisect -v -o --dest /tmp/verify --start 10955008 --end 10842786 --build_target emulator_test-linux_x64 --artifact testlogs/embedded_test_suite/embedded_test_suite.log echo "Checking $ARTIFACT_LOCATION"; [ -s ${ARTIFACT_LOCATION} ]
