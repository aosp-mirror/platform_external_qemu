/*
 * Copyright (C) 2011 The Android Open Source Project
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


#include <stdlib.h>
#include "android/utils/system.h"
#include "bmp-reader.h"

BmpInfo* bmp_read(FILE* fp){
    BmpInfo* bi;
    ANEW0(bi);
    ANEW0(bi->bfh);
    ANEW0(bi->dh);
    // Reading file header.
    fread(bi->bfh, sizeof(BMP_file_header), 1, fp);
    fread(bi->dh, sizeof(DIB_header), 1, fp);
    // Go to the image's pixel data.
    fseek(fp, bi->bfh->offset, SEEK_SET);
    char* temp;
    AARRAY_NEW(temp, 4);
    // The rows are stacked upside-down.
    Pixel **pixels;
    AARRAY_NEW(pixels, bi->dh->biHeight);
    int i;
    for (i = bi->dh->biHeight - 1; i >= 0; --i) {
         // Allocate memory for the row.
         AARRAY_NEW(pixels[i], bi->dh->biWidth);
         // Read row.
         int j;
         for (j=0; j<bi->dh->biWidth; ++j) {
             // Read pixel
             fread(&(pixels[i][j].blue), sizeof(pixels[i][j].red), 1, fp);
             fread(&(pixels[i][j].green), sizeof(pixels[i][j].red), 1, fp);
             fread(&(pixels[i][j].red), sizeof(pixels[i][j].red), 1, fp);
         }
         // Padd.
         unsigned int toPad = (bi->dh->biWidth*3) % 4;
         toPad = (4 - toPad)%4;
         fread(temp, 1, toPad, fp);
    }
    AFREE(temp);
    bi->pixels=pixels;
    int pixel_num = bi->dh->biHeight * bi->dh->biWidth * 4; // worst case. go for the overkill
    AARRAY_NEW(bi->fb, pixel_num);
    char* fb_temp = bi->fb;
    for (i = 0; i<bi->dh->biHeight; ++i) {
         int j;
         for (j = 0; j< bi->dh->biWidth; ++j) {
              fb_temp[0] = pixels[i][j].red;
              fb_temp[1] = pixels[i][j].green;
              fb_temp[2] = pixels[i][j].blue;
              fb_temp+=3;
         }
         // Align pointers to 16 bit;
         if (((uintptr_t)fb_temp & 1) != 0) fb_temp = (char*)fb_temp + 1;
    }
    return bi;
}

void bmp_close(BmpInfo* bi) {
    int i;
    for (i = 0; i<bi->dh->biHeight; ++i) {
        AFREE(bi->pixels[i]);
    }
    AFREE(bi->pixels);
}
