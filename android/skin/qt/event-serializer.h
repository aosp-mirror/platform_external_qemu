// Copyright 2016 The Android Open Source Project
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

#pragma once

#include "android/base/TypeUtils.h"
#include "android/skin/qt/ui-event-recorder.h"

#include <QEvent>
#include <QMetaEnum>

#include <sstream>

using android::base::enable_if_convertible;

void serializeEventToStream(std::ostream&, const QEvent*);

template <class ContainerType>
std::string serializeEvents(
        const ContainerType& container,
        enable_if_convertible<typename ContainerType::value_type, EventRecord> =
                nullptr) {
    static int event_enum_index =
        QEvent::staticMetaObject.indexOfEnumerator("Type");
    std::ostringstream str;
    for (int i = 0; i < container.size(); ++i) {
        const auto& event = container[i].event;
        if (event) {
            str << container[i].target_name << ":"
                << QEvent::staticMetaObject.enumerator(event_enum_index)
                            .valueToKey(event->type())
                << ":" << container[i].dt << ":";
            serializeEventToStream(str, event.get());
        }
    }
    return str.str();
}

std::unique_ptr<QEvent> deserializeEventFromStream(std::istringstream& iss,
                                                   int event_type);

bool end_of_stream(std::istringstream& recordstream);

template <class ContainerType>
void deserializeEvents(std::istringstream& recordstream,
                       ContainerType& container,
                       enable_if_convertible<typename ContainerType::value_type,
                                             EventRecord> = nullptr) {
    while (true) {
        std::string target_name;
        std::getline(recordstream, target_name, ':');
        if (target_name.empty()) {
            if (end_of_stream(recordstream)) {
                break;
            }
            // malformatted line, missing info to deserialize
            continue;
        }

        std::string str_event_type;
        std::getline(recordstream, str_event_type, ':');
        if (str_event_type.empty()) {
            if (end_of_stream(recordstream)) {
                break;
            }
            // malformatted line, missing info to deserialize
            continue;
        }
        int event_enum_index =
                QEvent::staticMetaObject.indexOfEnumerator("Type");
        int event_type = QEvent::staticMetaObject.enumerator(event_enum_index)
                                 .keyToValue(str_event_type.c_str());

        std::string str_dt;
        std::getline(recordstream, str_dt, ':');
        if (str_dt.empty()) {
            if (end_of_stream(recordstream)) {
                break;
            }
            // malformatted line, missing info to deserialize
            continue;
        }
        int dt = stoi(str_dt);

        std::string str_event;
        std::getline(recordstream, str_event);
        if (str_event.empty()) {
            if (end_of_stream(recordstream)) {
                break;
            }
            // malformatted line, missing info to deserialize
            continue;
        }
        std::istringstream iss(str_event);
        EventRecord record{deserializeEventFromStream(iss, event_type),
                           target_name, dt};

        // skip unsupported events (deserializer returned null)
        if (record.event != nullptr)
            container.push_back(std::move(record));
    }
}
