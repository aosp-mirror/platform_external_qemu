/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HW_EMULATOR_CAMERA_BMP_READER_H
#define HW_EMULATOR_CAMERA_BMP_READER_H

#include <stdio.h>
//private:

#pragma pack(2)
typedef struct {
  char file_type[2];
  int file_size;
  char reserved1[2];
  char reserved2[2];
  int offset;
} BMP_file_header;

/* Bitmap information header
*/
typedef struct {
  unsigned int biSize;
  unsigned int biWidth;
  unsigned int biHeight;
  unsigned short biPlanes;
  unsigned short biBitCount;
  unsigned int biCompression;
  unsigned int biSizeImage;
  unsigned int biXPelsPerMeter;
  unsigned int biYPelsPerMeter;
  unsigned int biClrUsed;
  unsigned int biClrImportant;
} DIB_header;

#pragma pack()

typedef struct {
  char blue;
  char green;
  char red;
} Pixel;

typedef struct {
  BMP_file_header *bfh;
  DIB_header *dh;
  Pixel **pixels;
  char* fb;
} BmpInfo;

//public:
extern BmpInfo* bmp_read(FILE* fp);
extern void bmp_close(BmpInfo* bi);

#endif /* HW_EMULATOR_CAMERA_BMP_READER_H */
