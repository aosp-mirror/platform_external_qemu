// Copyright (C) 2020 The Android Open Source Project
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
#include <stdio.h>                        // for printf
#include <stdlib.h>                       // for exit, EXIT_FAILURE, NULL
#include <iostream>                       // for operator<<, endl, basic_ost...
#include <string>                         // for string
#include <vector>                         // for vector

#ifdef _MSC_VER
#include "msvc-posix.h"
#include "msvc-getopt.h"
#else
#include <getopt.h>                       // for no_argument, getopt_long
#endif

#include <google/protobuf/text_format.h>  // for TextFormat

#include "android/avd/info.h"             // for avdInfo_new
#include "android/base/StringView.h"      // for StringView
#include "android/globals.h"              // for android_avdInfo, android_av...
#include "android/snapshot/Snapshot.h"    // for Snapshot
#include "snapshot.pb.h"                  // for Snapshot
#include "snapshot_service.pb.h"          // for SnapshotDetails, SnapshotList

using android::emulation::control::SnapshotDetails;
using android::emulation::control::SnapshotList;
using android::snapshot::Snapshot;

static struct option long_options[] = {{"valid-only", no_argument, 0, 'l'},
                                       {"invalid-only", no_argument, 0, 'i'},
                                       {"no-size", no_argument, 0, 's'},
                                       {"human", no_argument, 0, 'u'},
                                       {"help", no_argument, 0, 'h'},
                                       {0, 0, 0, 0}};

static void printUsage() {
    printf("usage: qsn [--valid-only] [--invalid-only] [--no-size] [--help] "
           "avd\n"
           "\n"
           "Lists available snapshots for the given avd in protobuf text "
           "format. The protobuf specification can be found\n"
           "in $ANDROID_SDK_ROOT/emulator/lib/snapshot_service.proto\n"
           "\n"
           "positional arguments:\n"
           "  avd             Name of the avd to query.\n"
           "\n"
           "optional arguments:\n"
           "  --valid-only    only valid snapshots are returned\n"
           "  --invalid-only  only invalid snapshots are returned\n"
           "  --no-size       size on disk is not included in the output\n"
           "  --help          show this help message and exit\n");
}

bool FLAG_valid_only = false, FLAG_invalid_only = false, FLAG_size = true;
std::string FLAG_avd = "";

static void parseArgs(int argc, char** argv) {
    int long_index = 0;
    int opt = 0;
    while ((opt = getopt_long(argc, argv, "", long_options, &long_index)) !=
           -1) {
        switch (opt) {
            case 'l':
                FLAG_valid_only = true;
                break;
            case 'i':
                FLAG_invalid_only = true;
                break;
            case 's':
                FLAG_size = false;
                break;
            case 'h':
            default:
                printUsage();
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        FLAG_avd = argv[optind];
    } else {
        printUsage();
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    parseArgs(argc, argv);

    // Initialize the avd
    android_avdInfo = avdInfo_new(FLAG_avd.c_str(), android_avdParams);
    if (android_avdInfo == NULL) {
        std::cerr << "Could not find virtual device named: " << FLAG_avd
                  << std::endl;
        exit(1);
    }

    // And list the snapshots.
    SnapshotList list;
    for (auto snapshot : Snapshot::getExistingSnapshots()) {
        auto protobuf = snapshot.getGeneralInfo();

        bool keep = (FLAG_valid_only && snapshot.checkOfflineValid()) ||
                    (FLAG_invalid_only && !snapshot.checkOfflineValid()) ||
                    (!FLAG_valid_only && !FLAG_invalid_only);
        if (protobuf && keep) {
            auto details = list.add_snapshots();
            details->set_snapshot_id(snapshot.name());
            if (FLAG_size)
                details->set_size(snapshot.folderSize());
            // We only need to check for snapshot validity once.
            // Invariant: FLAG_valid_only -> snapshot.checkOfflineValid()
            if (FLAG_valid_only || snapshot.checkOfflineValid()) {
                details->set_status(SnapshotDetails::Compatible);
            } else {
                details->set_status(SnapshotDetails::Incompatible);
            }

            *details->mutable_details() = *protobuf;
        }
    }

    std::string asText;
    google::protobuf::TextFormat::PrintToString(list, &asText);

    std::cout << asText << std::endl;
}
