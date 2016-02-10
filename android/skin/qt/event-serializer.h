// Copyright 2015 The Android Open Source Project
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

#include "android/skin/qt/ui-event-recorder.h"

#include <sstream>
#include <type_traits>

template <class ContainerType>
typename std::enable_if<
    std::is_convertible<typename ContainerType::value_type,
                        EventRecord>::value,
    std::string>::type
serializeEvents(const ContainerType& container) {
    std::ostringstream str;
    static int event_enum_index =
        QEvent::staticMetaObject.indexOfEnumerator("Type");
    for (int i = 0; i < container.size(); ++i) {
        const auto& event = container[i].event;
        str << container[i].target_name << " "
            << QEvent::staticMetaObject.enumerator(event_enum_index).valueToKey(event->type()) << " ";
        switch (event->type()) {
        case QEvent::Enter:
            {
                QEnterEvent* enter_event = dynamic_cast<QEnterEvent*>(event.get());
                str << enter_event->localPos().x() << " " << enter_event->localPos().y() << " "
                    << enter_event->windowPos().x() << " " << enter_event->windowPos().y() << " "
                    << enter_event->screenPos().x() << " " << enter_event->screenPos().y() << " ";
                break;
            }
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            {
                QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event.get());
                str << mouse_event->localPos().x() << " " << mouse_event->localPos().y() << " "
                    << mouse_event->windowPos().x() << " " << mouse_event->windowPos().y() << " "
                    << mouse_event->screenPos().x() << " " << mouse_event->screenPos().y() << " "
                    << mouse_event->buttons() << " "
                    << mouse_event->modifiers() << " ";
                break;
            }
        case QEvent::Resize:
            {
                QResizeEvent* resize_event = dynamic_cast<QResizeEvent*>(event.get());
                str << resize_event->size().width() << " " << resize_event->size().height() << " "
                    << resize_event->oldSize().width() << " " << resize_event->oldSize().height() << " ";
                break;
            }
        case QEvent::Close:
        case QEvent::Leave:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Hide:
            break;
        default:
            str << "<unsupported by serializer>";
        }
        str << std::endl;
    }
    return str.str();
}

