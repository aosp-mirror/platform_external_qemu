#include "android/loadpng.h"
#include "android/utils/file_io.h"
#include <png.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
#define LOG(x...) fprintf(stderr,"error: " x)
#else
#define LOG(x...) do {} while (0)
#endif

void *loadpng(const char *fn, unsigned *_width, unsigned *_height)
{
    FILE *fp = 0;
    unsigned char header[8];
    unsigned char *data = 0;
    unsigned char **rowptrs = 0;
    png_structp p = 0;
    png_infop pi = 0;

    png_uint_32 width, height;
    int bitdepth, colortype, imethod, cmethod, fmethod, i;

    p = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if(p == 0) {
        LOG("%s: failed to allocate png read struct\n", fn);
        return 0;
    }

    pi = png_create_info_struct(p);
    if(pi == 0) {
        LOG("%s: failed to allocate png info struct\n", fn);
        goto oops;
    }

    fp = android_fopen(fn, "rb");
    if(fp == 0) {
        LOG("%s: failed to open file\n", fn);
        return 0;
    }

    if(fread(header, 8, 1, fp) != 1) {
        LOG("%s: failed to read header\n", fn);
        goto oops;
    }

    if(png_sig_cmp(header, 0, 8)) {
        LOG("%s: header is not a PNG header\n", fn);
        goto oops;
    }

    if(setjmp(png_jmpbuf(p))) {
        LOG("%s: png library error\n", fn);
    oops:
        png_destroy_read_struct(&p, &pi, 0);
        if(fp != 0) fclose(fp);
        if(data != 0) free(data);
        if(rowptrs != 0) free(rowptrs);
        return 0;
    }

    png_init_io(p, fp);
    png_set_sig_bytes(p, 8);

    png_read_info(p, pi);

    png_get_IHDR(p, pi, &width, &height, &bitdepth, &colortype,
                 &imethod, &cmethod, &fmethod);

    switch(colortype){
    case PNG_COLOR_TYPE_PALETTE:
        png_set_palette_to_rgb(p);
        break;

    case PNG_COLOR_TYPE_RGB:
        if(png_get_valid(p, pi, PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(p);
        } else {
            png_set_filler(p, 0xff, PNG_FILLER_AFTER);
        }
        break;

    case PNG_COLOR_TYPE_RGB_ALPHA:
        break;

    case PNG_COLOR_TYPE_GRAY:
        if(bitdepth < 8) {
            png_set_expand_gray_1_2_4_to_8(p);
        }

    default:
        LOG("%s: unsupported (grayscale?) color type\n");
        goto oops;
    }

    if(bitdepth == 16) {
        png_set_strip_16(p);
    }

    data = (unsigned char*) malloc((width * 4) * height);
    rowptrs = (unsigned char **) malloc(sizeof(unsigned char*) * height);

    if((data == 0) || (rowptrs == 0)){
        LOG("could not allocate data buffer\n");
        goto oops;
    }

    for(i = 0; i < height; i++) {
        rowptrs[i] = data + ((width * 4) * i);
    }

    png_read_image(p, rowptrs);

    png_destroy_read_struct(&p, &pi, 0);
    fclose(fp);
    if(rowptrs != 0) free(rowptrs);

    *_width = width;
    *_height = height;

    return (void*) data;
}

int write_png_user_function(png_structp p,
                            png_infop pi,
                             unsigned int nChannels,
                             unsigned int width,
                             unsigned int height,
                             SkinRotation rotation,
                             void* pixels) {
    if (nChannels != 3 && nChannels != 4) {
        fprintf(stderr, "write_png only support 3 or 4 channel images\n");
        return 0;
    }
    if (!pixels) {
        fprintf(stderr, "Pixels are null.\n");
        return 0;
    }

    bool isPortrait =
            rotation == SKIN_ROTATION_0 || rotation == SKIN_ROTATION_180;
    unsigned int rows = isPortrait ? height : width;
    unsigned int cols = isPortrait ? width : height;

    if (setjmp(png_jmpbuf(p))) {
        LOG("Failed to set IHDR header\n");
        return 0;
    }
    png_set_IHDR(p, pi, cols, rows, 8,
                 nChannels == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(p, pi);

    if (setjmp(png_jmpbuf(p))) {
        LOG("Failed to write a row.\n");
        return 0;
    }
    if (rotation == SKIN_ROTATION_0) {
        unsigned int i = 0;
        for (i = 0; i < height; i++) {
            png_write_row(p, pixels + i * nChannels * width);
        }
    } else {
        char* rowBuffer = malloc(nChannels * cols);
        char* src = NULL;
        // Increment in src after scanning one pixel or finishing one row
        int srcDelta0 = 0;
        int srcDelta1 = 0;
        switch (rotation) {
            case SKIN_ROTATION_0:
                srcDelta0 = 0;
                srcDelta1 = 0;
                src = pixels;
                break;
            case SKIN_ROTATION_90:
                srcDelta0 = -nChannels - nChannels * width;
                srcDelta1 = nChannels + nChannels * width * height;
                src = pixels + nChannels * width * (height - 1);
                break;
            case SKIN_ROTATION_180:
                srcDelta0 = -nChannels * 2;
                srcDelta1 = 0;
                src = pixels + nChannels * (width * height - 1);
                break;
            case SKIN_ROTATION_270:
                srcDelta0 = -nChannels + nChannels * width;
                srcDelta1 = -nChannels - nChannels * width * height;
                src = pixels + nChannels * (width - 1);
                break;
        }
        unsigned int i;
        unsigned int j;
        unsigned int c;
        for (i = 0; i < rows; i++) {
            char* dst = rowBuffer;
            for (j = 0; j < cols; j++) {
                for (c = 0; c < nChannels; c++) {
                    *(dst++) = *(src++);
                }
                src += srcDelta0;
            }
            png_write_row(p, (void*)rowBuffer);
            src += srcDelta1;
        }
        free(rowBuffer);
    }
    if (setjmp(png_jmpbuf(p))) {
        LOG("Failed to write end header\n");
        return 0;
    }
    png_write_end(p, NULL);
    return 1;
}

void savepng(const char* fn,
             unsigned int nChannels,
             unsigned int width,
             unsigned int height,
             SkinRotation rotation,
             void* pixels) {
    if (nChannels != 3 && nChannels != 4) {
        fprintf(stderr, "savepng only support 3 or 4 channel images\n");
    }

    FILE* fp = android_fopen(fn, "wb");
    if (!fp) {
        LOG("Unable to write to file %s.\n", fn);
        return;
    }
    png_structp p =
            png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    png_set_write_fn(p, 0, 0, 0);
    if (setjmp(png_jmpbuf(p))) {
        LOG("Unable to initialize png lib.\n");
        return;
    }
    png_init_io(p, fp);


    png_infop pi = png_create_info_struct(p);
    write_png_user_function(p, pi, nChannels, width, height, rotation, pixels);
    png_destroy_write_struct(&p, &pi);

    fclose(fp);
}

static void putUint32_t(unsigned char* dst, uint32_t data) {
    dst[0] = (unsigned char)data;
    dst[1] = (unsigned char)(data >> 8);
    dst[2] = (unsigned char)(data >> 16);
    dst[3] = (unsigned char)(data >> 24);
}

static void putUint16_t(unsigned char* dst, uint16_t data) {
    dst[0] = (unsigned char)data;
    dst[1] = (unsigned char)(data >> 8);
}


void savebmp(const char* fn, unsigned int nChannels, unsigned int width,
        unsigned int height, void* pixels) {
    if (nChannels != 1 && nChannels != 3 && nChannels != 4) {
        fprintf(stderr, "savebmp only support 1, 3 or 4 channel images\n");
        return;
    }
    FILE *fp = android_fopen(fn, "wb");
    if (!fp) {
        LOG("Unable to write to file %s.\n", fn);
        return;
    }
    uint32_t w = (uint32_t)width;
    uint32_t h = (uint32_t)height;
    const uint32_t dibHeaderSize = 40;
    const uint32_t offset = 14 + dibHeaderSize;
    uint32_t imgSize = nChannels * w * h;
    uint32_t fileSize = offset + imgSize;
    uint16_t nClrPln = 1;
    uint16_t nBpp = (uint16_t)nChannels * 8;
    uint32_t comp = 0;
    uint32_t reso = 0;
    uint32_t nClrPalette = 0;
    uint32_t nImptClr = 0;
    unsigned char header[offset];
    header[0] = 'B';
    header[1] = 'M';
    putUint32_t(header + 2, fileSize);
    // bit 6~10 are reserved
    putUint32_t(header + 10, offset);
    // bitmap information header
    putUint32_t(header + 14, dibHeaderSize);
    putUint32_t(header + 18, w);
    putUint32_t(header + 22, h);
    putUint16_t(header + 26, nClrPln);
    putUint16_t(header + 28, nBpp);
    putUint32_t(header + 30, comp);
    putUint32_t(header + 34, imgSize);
    putUint32_t(header + 38, reso);
    putUint32_t(header + 42, reso);
    putUint32_t(header + 46, nClrPalette);
    putUint32_t(header + 50, nImptClr);
    fwrite(header, 1, offset, fp);
    const unsigned char* src = pixels + nChannels * w * (h - 1);
    unsigned char padding[3] = {0, 0, 0};
    unsigned char* data = (unsigned char*)malloc(nChannels * w);
    int paddingBytes = (4 - nChannels * w % 4) % 4;
    int r = 0;
    int c = 0;
    for (r = 0; r < h; r++) {
        if (nChannels == 1) {
            fwrite(src, 1, w, fp);
        } else {
            for (c = 0; c < w; c++) { // flip rgb
                data[c * nChannels + 0] = src[c * nChannels + 2];
                data[c * nChannels + 1] = src[c * nChannels + 1];
                data[c * nChannels + 2] = src[c * nChannels + 0];
                if (nChannels == 4) {
                    data[c * nChannels + 3] = src[c * nChannels + 3];
                }
            }
            fwrite(data, 1, w * nChannels, fp);
        }
        fwrite(padding, 1, paddingBytes, fp);
        src -= w * nChannels;
    }
    free(data);
    fclose(fp);
}

typedef struct
{
    const unsigned char*  base;
    const unsigned char*  end;
    const unsigned char*  cursor;

} PngReader;

static void
png_reader_read_data( png_structp  png_ptr,
                      png_bytep   data,
                      png_size_t  length )
{
  PngReader* reader = png_get_io_ptr(png_ptr);
  png_size_t avail  = (png_size_t)(reader->end - reader->cursor);

  if (avail > length)
      avail = length;

  memcpy( data, reader->cursor, avail );
  reader->cursor += avail;
}


void *readpng(const unsigned char *base, size_t   size, unsigned *_width, unsigned *_height)
{
    PngReader  reader;
    unsigned char *data = 0;
    unsigned char **rowptrs = 0;
    png_structp p = 0;
    png_infop pi = 0;

    png_uint_32 width, height;
    int bitdepth, colortype, imethod, cmethod, fmethod, i;

    p = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if(p == 0) {
        LOG("%s: failed to allocate png read struct\n", fn);
        return 0;
    }

    pi = png_create_info_struct(p);
    if(pi == 0) {
        LOG("%s: failed to allocate png info struct\n", fn);
        goto oops;
    }

    reader.base   = base;
    reader.end    = base + size;
    reader.cursor = base;

    if(size < 8 || png_sig_cmp((unsigned char*)base, 0, 8)) {
        LOG("%s: header is not a PNG header\n", fn);
        goto oops;
    }

    reader.cursor += 8;

    if(setjmp(png_jmpbuf(p))) {
        LOG("%s: png library error\n", fn);
    oops:
        png_destroy_read_struct(&p, &pi, 0);
        if(data != 0) free(data);
        if(rowptrs != 0) free(rowptrs);
        return 0;
    }

    png_set_read_fn (p, &reader, png_reader_read_data);
    png_set_sig_bytes(p, 8);

    png_read_info(p, pi);

    png_get_IHDR(p, pi, &width, &height, &bitdepth, &colortype,
                 &imethod, &cmethod, &fmethod);

    switch(colortype){
    case PNG_COLOR_TYPE_PALETTE:
        png_set_palette_to_rgb(p);
        break;

    case PNG_COLOR_TYPE_RGB:
        if(png_get_valid(p, pi, PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(p);
        } else {
            png_set_filler(p, 0xff, PNG_FILLER_AFTER);
        }
        break;

    case PNG_COLOR_TYPE_RGB_ALPHA:
        break;

    case PNG_COLOR_TYPE_GRAY:
        if(bitdepth < 8) {
            png_set_expand_gray_1_2_4_to_8(p);
        }

    default:
        LOG("%s: unsupported (grayscale?) color type\n");
        goto oops;
    }

    if(bitdepth == 16) {
        png_set_strip_16(p);
    }

    data    = (unsigned char*) malloc((width * 4) * height);
    rowptrs = (unsigned char **) malloc(sizeof(unsigned char*) * height);

    if((data == 0) || (rowptrs == 0)){
        LOG("could not allocate data buffer\n");
        goto oops;
    }

    for(i = 0; i < height; i++) {
        rowptrs[i] = data + ((width * 4) * i);
    }

    png_read_image(p, rowptrs);

    png_destroy_read_struct(&p, &pi, 0);
    if(rowptrs != 0) free(rowptrs);

    *_width = width;
    *_height = height;

    return (void*) data;
}


#if 0
int main(int argc, char **argv)
{
    unsigned w,h;
    unsigned char *data;

    if(argc < 2) return 0;


    data = loadpng(argv[1], &w, &h);

    if(data != 0) {
        printf("w: %d  h: %d\n", w, h);
    }

    return 0;
}
#endif
