/* Copyright (C) 2016 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#include <windows.h>
#include "android/skin/qt/windows-native-window.h"

int numHeldMouseButtons() {

    // We care about the left and right mouse buttons because these functions
    // query the physical mouse state, but those buttons can be re-mapped in
    // the system virtually.
    int numHeld = 0;
    if (GetKeyState(VK_LBUTTON) & 0x80) {
        numHeld++;
    }
    if (GetKeyState(VK_RBUTTON) & 0x80) {
        numHeld++;
    }
    return numHeld;
}

bool takeScreenshot(unsigned char* pixelData, size_t width, size_t height) {
    bool error = false;

    // Hack for DWM: There is no way to wait for DWM animations to finish, so we
    // just have to wait for a while before issuing screenshot if window was
    // just made visible.
    /*{
        static const double WAIT_WINDOW_VISIBLE_MS = 0.5;  // Half a second for
    the animation double timeSinceVisible                    =
    mSetVisibleTimer->getElapsedTime();

        if (timeSinceVisible < WAIT_WINDOW_VISIBLE_MS)
        {
            Sleep(static_cast<DWORD>((WAIT_WINDOW_VISIBLE_MS - timeSinceVisible)
    * 1000));
        }
    }*/

    HDC screenDC = nullptr;
    // HDC windowDC      = nullptr;
    HDC tmpDC = nullptr;
    HBITMAP tmpBitmap = nullptr;

    if (!error) {
        screenDC = GetDC(HWND_DESKTOP);
        error = screenDC == nullptr;
    }

    /*if (!error)
    {
        windowDC = GetDC(mNativeWindow);
        error    = windowDC == nullptr;
    }*/

    if (!error) {
        tmpDC = CreateCompatibleDC(screenDC);
        error = tmpDC == nullptr;
    }

    if (!error) {
        tmpBitmap = CreateCompatibleBitmap(screenDC, width, height);
        error = tmpBitmap == nullptr;
    }

    POINT topLeft = {0, 0};
    // if (!error) {
    //    error = (MapWindowPoints(mNativeWindow, HWND_DESKTOP, &topLeft, 1) ==
    //             0);
    //}

    if (!error) {
        error = SelectObject(tmpDC, tmpBitmap) == nullptr;
    }

    if (!error) {
        error = BitBlt(tmpDC, 0, 0, width, height, screenDC, topLeft.x,
                       topLeft.y, SRCCOPY) == 0;
    }

    if (!error) {
        BITMAPINFOHEADER bitmapInfo;
        bitmapInfo.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.biWidth = width;
        bitmapInfo.biHeight = -height;
        bitmapInfo.biPlanes = 1;
        bitmapInfo.biBitCount = 32;
        bitmapInfo.biCompression = BI_RGB;
        bitmapInfo.biSizeImage = 0;
        bitmapInfo.biXPelsPerMeter = 0;
        bitmapInfo.biYPelsPerMeter = 0;
        bitmapInfo.biClrUsed = 0;
        bitmapInfo.biClrImportant = 0;
        int getBitsResult = GetDIBits(
                screenDC, tmpBitmap, 0, height, pixelData,
                reinterpret_cast<BITMAPINFO*>(&bitmapInfo), DIB_RGB_COLORS);
        error = (getBitsResult == 0);
    }

    if (tmpBitmap != nullptr) {
        DeleteObject(tmpBitmap);
    }
    if (tmpDC != nullptr) {
        DeleteDC(tmpDC);
    }
    if (screenDC != nullptr) {
        ReleaseDC(nullptr, screenDC);
    }
    /*if (windowDC != nullptr)
    {
        ReleaseDC(mNativeWindow, windowDC);
    }*/

    return !error;
}
