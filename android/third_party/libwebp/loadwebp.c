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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "third_party/chromium_headless/libwebp/webp/decode.h"

#if 0
#define LOG(x...) fprintf(stderr,"WebP decode error: " x)
#else
#define LOG(x...) do {} while (0)
#endif

////////////////////////////////////////////////////////////
//
// readWebP
//
// Decode a WebP-compressed image from memory, producing RGBA
//
// Input:
//     |base|  Address of the WebP image in memory
//     |size|  Number of bytes in the WebP image
//
// Output:
//     |pWidth|   The width of the decoded image
//     |pHeight|  The height of the decoded image
//
// Return value: If successful, the address of the
//               malloc'd buffer that holds the
//               decoded RGBA data
//               If unsuccessful, 0

void* readWebP(const unsigned char*  base, size_t  size,
               unsigned *pWidth, unsigned *pHeight)
{
    WebPDecoderConfig webpConfig;
    VP8StatusCode     decodeStatus;

    //////////////////////////////

    *pWidth  = 0;
    *pHeight = 0;

    // Configure the decoder for RGBA output
    memset(&webpConfig, 0, sizeof(webpConfig));
    webpConfig.output.colorspace = MODE_RGBA;

    // Decode to RGBA
    decodeStatus = WebPDecode(base, size, &webpConfig);
    if (decodeStatus != VP8_STATUS_OK) {
        LOG("WebPDecode() failed\n");
        return 0;
    }

    // Success

    *pWidth  = webpConfig.output.width;
    *pHeight = webpConfig.output.height;
    return (void*)webpConfig.output.u.RGBA.rgba;
}

////////////////////////////////////////////////////////////
//
// loadWebP
//
// Decode a WebP-compressed image from a file, producing RGBA
//
// Input:
//     |fileName|  Path and name of the WebP file
//
// Output:
//     |pWidth|   The width of the decoded image
//     |pHeight|  The height of the decode image
//
// Return value: If successful, the address of the
//               malloc'd buffer that holds the
//               decoded RGBA data
//               If unsuccessful, 0

#define HEADER_SIZE 12

void* loadWebP(const char *fileName, unsigned *pWidth, unsigned *pHeight) {
    int               bytesRead;
    int               fileSize;
    struct stat       fileStatus;
    FILE*             fp;
    void*             rgbaAddr;
    uint8_t*          webpBytes;
    uint8_t           webpHeader[HEADER_SIZE];

    //////////////////////////////

    *pWidth  = 0;
    *pHeight = 0;

    // Get the file size
    if (stat(fileName, &fileStatus) != 0) {
        LOG("Could not status file: %s\n", fileName);
        return 0;
    }
    fileSize = fileStatus.st_size;
    if (fileSize < HEADER_SIZE) {
        LOG("WebP file is way too small: %s\n", fileName);
        return 0;
    }

    // Read the first few bytes into memory
    fp = android_fopen(fileName, "rb");
    if(fp == 0) {
        LOG("Failed to open file: %s\n", fileName);
        return 0;
    }
    bytesRead = fread(webpHeader, 1, HEADER_SIZE, fp);
    if (bytesRead != HEADER_SIZE) {
        LOG("Failed to read header of file: %s\n", fileName);
        fclose(fp);
        return 0;
    }
    // Verify that it is probably WebP
    if (webpHeader[ 0] != 'R' ||
        webpHeader[ 1] != 'I' ||
        webpHeader[ 2] != 'F' ||
        webpHeader[ 3] != 'F' ||
        webpHeader[ 8] != 'W' ||
        webpHeader[ 9] != 'E' ||
        webpHeader[10] != 'B' ||
        webpHeader[11] != 'P'   )
    {
        LOG("Not a WebP file: %s\n", fileName);
        fclose(fp);
        return 0;
    }
    rewind(fp);

    // Read the entire file into memory
    webpBytes = malloc(fileSize);
    if (webpBytes == NULL) {
        LOG("malloc(%d) failed\n", fileSize);
        fclose(fp);
        return 0;
    }

    bytesRead = fread(webpBytes, 1, fileSize, fp);
    fclose(fp);
    if (bytesRead != fileSize) {
        LOG("Failed to read contents of file %s\n", fileName);
        free(webpBytes);
        return 0;
    }

    rgbaAddr = readWebP(webpBytes, fileSize, pWidth, pHeight);

    // Free the file data. Keep the pixel data.
    free(webpBytes);

    return rgbaAddr;
}
