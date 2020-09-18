#  Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# -*- coding: utf-8 -*-
import sys
import grpc
from google.protobuf import empty_pb2
from aemu.proto.ui_controller_service_pb2 import ThemingStyle

from aemu.discovery.emulator_discovery import get_default_emulator

# Create a client
stub = get_default_emulator().get_ui_controller_service()

_EMPTY_ = empty_pb2.Empty()

def setUiTheme(theme):
	stub.setUiTheme(theme)

def main(args):
	print("Set Emulator's theme to DARK")
	theme = ThemingStyle(style="DARK")
	setUiTheme(theme)
	print("Set Emulator's theme to LIGHT")
	theme = ThemingStyle(style="LIGHT")
	setUiTheme(theme)
	print("Show extended window")
	stub.showExtendedControls(_EMPTY_)
	print("Close extended window")
	stub.closeExtendedControls(_EMPTY_)

if __name__ == "__main__":
	main(sys.argv)

