# Copyright (C) 2019 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
PYTHON=python
PY_VERSION := $(shell $(PYTHON) -c "import sys;sys.stdout.write(str(sys.version_info[0]))")
MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR :=  $(abspath $(MAKEFILE_PATH)/..)
AEMU_ROOT := $(abspath $(CURRENT_DIR)/../../../python)
OUTPUT_DIR :=  $(if $(DIST),$(DIST),$(CURRENT_DIR))
WHEEL := $(OUTPUT_DIR)/aemu_grpc-0.0.1-py$(PY_VERSION)-none-any.whl

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(WHEEL): $(OUTPUT_DIR)
	make -C $(AEMU_ROOT) DIST=$(WHEEL_DIR)
		$(PYTHON) -m pip install -q --upgrade $(WHEEL)

build: $(OUTPUT_DIR)
	$(PYTHON) setup.py -q bdist_wheel --dist-dir $(OUTPUT_DIR)

.DEFAULT_GOAL := build

