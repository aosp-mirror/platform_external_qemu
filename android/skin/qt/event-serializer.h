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

#include "android/base/TypeTraits.h"
#include "android/skin/qt/ui-event-recorder.h"

#include <QEvent>
#include <QMetaEnum>

#include <sstream>

using android::base::enable_if_convertible;

void serializeEventToStream(std::ostream&, const QEvent*);

template <class ContainerType>
std::string serializeEvents(
        const ContainerType& container,
        enable_if_convertible<
            typename ContainerType::value_type,
            EventRecord> = nullptr) {
    static int event_enum_index =
        QEvent::staticMetaObject.indexOfEnumerator("Type");
    std::ostringstream str;
    for (int i = 0; i < container.size(); ++i) {
        const auto& event = container[i].event;
        if (event) {
            str << container[i].target_name << " "
                << QEvent::staticMetaObject.enumerator(event_enum_index).valueToKey(event->type()) << " ";
            serializeEventToStream(str, event.get());
        }
    }
    return str.str();
}

