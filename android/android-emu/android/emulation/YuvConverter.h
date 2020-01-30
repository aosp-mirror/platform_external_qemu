// Copyright (C) 2019 The Android Open Source Project
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

namespace android {
namespace emulation {

template <typename T>
class YuvConverter {
public:
    YuvConverter(int nWidth, int nHeight) : nWidth(nWidth), nHeight(nHeight) {
        pQuad = new T[nWidth * nHeight / 4];
    }
    ~YuvConverter() { delete[] pQuad; }
    void PlanarToUVInterleaved(T* pFrame, int nPitch = 0) {
        if (nPitch == 0) {
            nPitch = nWidth;
        }
        T* puv = pFrame + nPitch * nHeight;
        if (nPitch == nWidth) {
            memcpy(pQuad, puv, nWidth * nHeight / 4 * sizeof(T));
        } else {
            for (int i = 0; i < nHeight / 2; i++) {
                memcpy(pQuad + nWidth / 2 * i, puv + nPitch / 2 * i,
                       nWidth / 2 * sizeof(T));
            }
        }
        T* pv = puv + (nPitch / 2) * (nHeight / 2);
        for (int y = 0; y < nHeight / 2; y++) {
            for (int x = 0; x < nWidth / 2; x++) {
                puv[y * nPitch + x * 2] = pQuad[y * nWidth / 2 + x];
                puv[y * nPitch + x * 2 + 1] = pv[y * nPitch / 2 + x];
            }
        }
    }
    void UVInterleavedToPlanar(T* pFrame, int nPitch = 0) {
        if (nPitch == 0) {
            nPitch = nWidth;
        }
        T *puv = pFrame + nPitch * nHeight, *pu = puv,
          *pv = puv + nPitch * nHeight / 4;
        for (int y = 0; y < nHeight / 2; y++) {
            for (int x = 0; x < nWidth / 2; x++) {
                pu[y * nPitch / 2 + x] = puv[y * nPitch + x * 2];
                pQuad[y * nWidth / 2 + x] = puv[y * nPitch + x * 2 + 1];
            }
        }
        if (nPitch == nWidth) {
            memcpy(pv, pQuad, nWidth * nHeight / 4 * sizeof(T));
        } else {
            for (int i = 0; i < nHeight / 2; i++) {
                memcpy(pv + nPitch / 2 * i, pQuad + nWidth / 2 * i,
                       nWidth / 2 * sizeof(T));
            }
        }
    }

private:
    T* pQuad;
    int nWidth, nHeight;
};

}  // namespace emulation
}  // namespace android
