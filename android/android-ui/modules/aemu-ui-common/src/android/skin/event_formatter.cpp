
// Copyright (C) 2024 The Android Open Source Project
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
#include "android/skin/event_formatter.h"
#include <iostream>
#include <sstream>
#include <string>
#include "android/skin/event.h"
#include "android/utils/system.h"

// Implement << operator for SkinEvent
std::ostream& operator<<(std::ostream& os, const SkinEvent& event) {
    switch (event.type) {
        case kEventKeyDown:
            os << "KeyDown: keycode=" << event.u.key.keycode
               << ", mod=" << event.u.key.mod;
            break;
        case kEventKeyUp:
            os << "KeyUp: keycode=" << event.u.key.keycode
               << ", mod=" << event.u.key.mod;
            break;
        case kEventGeneric:
            os << "Generic: type=" << event.u.generic_event.type
               << ", code=" << event.u.generic_event.code
               << ", value=" << event.u.generic_event.value;
            break;
        case kEventTextInput:
            os << "TextInput: keycode=" << event.u.text.keycode
               << ", mod=" << event.u.text.mod << ", text=" << event.u.text.text
               << ", down=" << event.u.text.down;
            break;
        case kEventMouseButtonDown:
            os << "MouseButtonDown: display_id=" << event.u.mouse.display_id
               << ", x=" << event.u.mouse.x << ", y=" << event.u.mouse.y
               << ", button=" << event.u.mouse.button;
            break;
        case kEventMouseButtonUp:
            os << "MouseButtonUp: display_id=" << event.u.mouse.display_id
               << ", x=" << event.u.mouse.x << ", y=" << event.u.mouse.y
               << ", button=" << event.u.mouse.button;
            break;
        case kEventMouseMotion:
            os << "MouseMotion: display_id=" << event.u.mouse.display_id
               << ", x=" << event.u.mouse.x << ", y=" << event.u.mouse.y
               << ", xrel=" << event.u.mouse.xrel
               << ", yrel=" << event.u.mouse.yrel
               << ", button=" << event.u.mouse.button;
            break;
        case kEventMouseWheel:
            os << "MouseWheel: display_id=" << event.u.wheel.display_id
               << ", x_delta=" << event.u.wheel.x_delta
               << ", y_delta=" << event.u.wheel.y_delta;
            break;
        case kEventQuit:
            os << "Quit";
            break;
        case kEventScrollBarChanged:
            os << "ScrollBarChanged: x=" << event.u.scroll.x
               << ", y=" << event.u.scroll.y << ", xmax=" << event.u.scroll.xmax
               << ", ymax=" << event.u.scroll.ymax;
            break;
        case kEventRotaryInput:
            os << "RotaryInput: delta=" << event.u.rotary_input.delta;
            break;
        case kEventSetScale:
            os << "SetScale: scale=" << event.u.window.scale
               << ", x=" << event.u.window.x << ", y=" << event.u.window.y;
            break;
        case kEventSetZoom:
            os << "SetZoom: scale=" << event.u.window.scale
               << ", x=" << event.u.window.x << ", y=" << event.u.window.y;
            break;
        case kEventForceRedraw:
            os << "ForceRedraw";
            break;
        case kEventWindowMoved:
            os << "WindowMoved: x=" << event.u.window.x
               << ", y=" << event.u.window.y;
            break;
        case kEventLayoutRotate:
            os << "LayoutRotate: rotation=" << event.u.layout_rotation.rotation;
            break;
        case kEventScreenChanged:
            os << "ScreenChanged: x=" << event.u.screen.x
               << ", y=" << event.u.screen.y << ", w=" << event.u.screen.w
               << ", h=" << event.u.screen.h << ", dpr=" << event.u.screen.dpr;
            break;
        case kEventZoomedWindowResized:
            os << "ZoomedWindowResized";
            break;
        case kEventToggleTrackball:
            os << "ToggleTrackball";
            break;
        case kEventWindowChanged:
            os << "WindowChanged";
            break;
        case kEventSetDisplayRegion:
            os << "SetDisplayRegion: xOffset=" << event.u.display_region.xOffset
               << ", yOffset=" << event.u.display_region.yOffset
               << ", width=" << event.u.display_region.width
               << ", height=" << event.u.display_region.height;
            break;
        case kEventSetDisplayRegionAndUpdate:
            os << "SetDisplayRegionAndUpdate: xOffset="
               << event.u.display_region.xOffset
               << ", yOffset=" << event.u.display_region.yOffset
               << ", width=" << event.u.display_region.width
               << ", height=" << event.u.display_region.height;
            break;
        case kEventSetNoSkin:
            os << "SetNoSkin";
            break;
        case kEventRestoreSkin:
            os << "RestoreSkin";
            break;
        case kEventMouseStartTracking:
            os << "MouseStartTracking";
            break;
        case kEventMouseStopTracking:
            os << "MouseStopTracking";
            break;
        case kEventPenPress:
            os << "PenPress: tracking_id=" << event.u.pen.tracking_id
               << ", x=" << event.u.pen.x << ", y=" << event.u.pen.y
               << ", pressure=" << event.u.pen.pressure
               << ", orientation=" << event.u.pen.orientation;
            break;
        case kEventPenRelease:
            os << "PenRelease: tracking_id=" << event.u.pen.tracking_id
               << ", x=" << event.u.pen.x << ", y=" << event.u.pen.y
               << ", pressure=" << event.u.pen.pressure
               << ", orientation=" << event.u.pen.orientation;
            break;
        case kEventPenMove:
            os << "PenMove: tracking_id=" << event.u.pen.tracking_id
               << ", x=" << event.u.pen.x << ", y=" << event.u.pen.y
               << ", pressure=" << event.u.pen.pressure
               << ", orientation=" << event.u.pen.orientation;
            break;
        case kEventTouchBegin:
            os << "TouchBegin: display_id="
               << event.u.multi_touch_point.display_id
               << ", id=" << event.u.multi_touch_point.id
               << ", x=" << event.u.multi_touch_point.x
               << ", y=" << event.u.multi_touch_point.y
               << ", pressure=" << event.u.multi_touch_point.pressure
               << ", orientation=" << event.u.multi_touch_point.orientation;
            break;
        case kEventTouchEnd:
            os << "TouchEnd: display_id="
               << event.u.multi_touch_point.display_id
               << ", id=" << event.u.multi_touch_point.id
               << ", x=" << event.u.multi_touch_point.x
               << ", y=" << event.u.multi_touch_point.y
               << ", pressure=" << event.u.multi_touch_point.pressure
               << ", orientation=" << event.u.multi_touch_point.orientation;
            break;
        case kEventTouchUpdate:
            os << "TouchUpdate: display_id="
               << event.u.multi_touch_point.display_id
               << ", id=" << event.u.multi_touch_point.id
               << ", x=" << event.u.multi_touch_point.x
               << ", y=" << event.u.multi_touch_point.y
               << ", pressure=" << event.u.multi_touch_point.pressure
               << ", orientation=" << event.u.multi_touch_point.orientation;
            break;
        case kEventSetDisplayActiveConfig:
            os << "SetDisplayActiveConfig: configId="
               << event.u.display_active_config;
            break;
        case kEventAddDisplay:
            os << "AddDisplay: id=" << event.u.add_display.id
               << ", width=" << event.u.add_display.width
               << ", height=" << event.u.add_display.height;
            break;
        case kEventRemoveDisplay:
            os << "RemoveDisplay: id=" << event.u.remove_display.id;
            break;
        case kEventSetFoldedSkin:
            os << "SetFoldedSkin";
            break;
        default:
            os << "Unknown event type: " << event.type;
            break;
    }
    return os;
}


extern "C" char* format_skin_event(SkinEvent* event) {
    std::stringstream ss;
    ss << *event;
    std::string str = ss.str();
    return ASTRDUP(str.c_str());
}