/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "NativeSubWindow.h"

LRESULT CALLBACK subWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_PAINT) {
        SubWindowRepaintCallback repaint_callback =
            (SubWindowRepaintCallback)GetWindowLongPtr(hwnd, 0);
        void* repaint_callback_param =
            (void*)GetWindowLongPtr(hwnd, sizeof(SubWindowRepaintCallback));
        repaint_callback(repaint_callback_param);
        
    } 
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

EGLNativeWindowType createSubWindow(FBNativeWindowType p_window,
                                    int x, int y,int width, int height,
                                    SubWindowRepaintCallback repaint_callback,
                                    void* repaint_callback_param){
    static const char className[] = "subWin";

    WNDCLASS wc = {};
    if (!GetClassInfo(GetModuleHandle(NULL), className, &wc)) {
        wc.style =  CS_OWNDC | CS_HREDRAW | CS_VREDRAW;// redraw if size changes
        wc.lpfnWndProc = &subWindowProc;               // points to window procedure
        wc.cbWndExtra = sizeof(LONG_PTR) +
                        sizeof(SubWindowRepaintCallback) +
                        sizeof(void*) ;                // save extra window class memory, to store repaint callback
        wc.lpszClassName = className;                  // name of window class
        RegisterClass(&wc);
    }

    EGLNativeWindowType ret = CreateWindowEx(
                        WS_EX_NOPARENTNOTIFY,  // do not bother our parent window
                        className,
                        "sub",
                        WS_CHILD|WS_DISABLED,
                        x,y,width,height,
                        p_window,
                        NULL,
                        NULL,
                        NULL);
    SetWindowLongPtr(ret, 0, (LONG_PTR)repaint_callback);
    SetWindowLongPtr(
            ret,
            sizeof(SubWindowRepaintCallback),
            (LONG_PTR)repaint_callback_param);
    SetWindowLongPtr(
            ret,
            sizeof(SubWindowRepaintCallback) + sizeof(void*),
            1);
    ShowWindow(ret, SW_SHOW);
    return ret;
}

void destroySubWindow(EGLNativeWindowType win){
    PostMessage(win, WM_CLOSE, 0, 0);
}

int moveSubWindow(FBNativeWindowType p_parent_window,
                  EGLNativeWindowType p_sub_window,
                  int x,
                  int y,
                  int width,
                  int height) {
    BOOL ret = MoveWindow(p_sub_window,
                          x,
                          y,
                          width,
                          height,
                          TRUE);
    return ret;
}
