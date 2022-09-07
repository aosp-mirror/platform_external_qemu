
// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace std::chrono_literals;

int main(int argc, char** argv) {
    std::chrono::milliseconds sleep = 500ms;
    int exit = 0;
    std::string msg, err;
    if (argc > 0 && argv[1] != nullptr) {
        sleep = std::chrono::milliseconds(std::stoi(argv[1]));
    }

    if (argc > 1 && argv[2] != nullptr) {
        exit = std::stoi(argv[2]);
    }

    if (argc > 2 && argv[3] != nullptr) {
        msg = argv[3];
    }

    if (argc > 3 && argv[4] != nullptr) {
        err = argv[4];
    }

    if (!msg.empty()) {
        std::cout << msg;
        std::flush(std::cout);
    }

    if (!err.empty()) {
        std::cerr << err;
        std::flush(std::cerr);
    }

    std::this_thread::sleep_for(sleep);

    return exit;
}
