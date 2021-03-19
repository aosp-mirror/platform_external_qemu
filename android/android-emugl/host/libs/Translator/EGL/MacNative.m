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
#include <stdio.h>
#include <Cocoa/Cocoa.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#include <Metal/Metal.h>
#include "MacPixelFormatsAttribs.h"

//
// EmuGLContext inherit from NSOpenGLContext
// and adds binding state for the context to know
// if it was last bounded to a pbuffer or a window.
// This is because after the context was bounded to
// a Pbuffer, before we bind it to a window we must
// release it form the pbuffer by calling the
// clearDrawable method. We do not want to call clearDrawable
// more than really needed since when it is called at a time
// that a window is bounded to the context it will clear the
// window content causing flickering effect.
// Thererfore we call clearDrawable only when we bind the context
// to a window and it was previously bound to a Pbuffer.
//
@interface EmuGLContext : NSOpenGLContext {
    @private
        int boundToPbuffer;
        int boundToWin;
}

- (id) initWithFormat:(NSOpenGLPixelFormat *)pixelFormat shareContext:(NSOpenGLContext *)share;
- (void) preBind:(int)forPbuffer;
@end

@implementation EmuGLContext
- (id) initWithFormat:(NSOpenGLPixelFormat *)pixelFormat shareContext:(NSOpenGLContext *)share
{
    self = [super initWithFormat:pixelFormat shareContext:share];
    if (self != nil) {
        boundToPbuffer = 0;
        boundToWin = 0;
    }
    return self;
}

- (void) preBind:(int)forPbuffer
{
    if ((!forPbuffer && boundToPbuffer)) {
        [self clearDrawable]; 
    }
    boundToPbuffer = forPbuffer;
    boundToWin = !boundToPbuffer;
}
@end

int getAttrListLength(const NSOpenGLPixelFormatAttribute* list) {
    int count = 0;
    while (list[count++] != 0);
    return count ? (count - 1) : 0;
}

static const NSOpenGLPixelFormatAttribute core32TestProfile[] = {
    NSOpenGLPFAAccelerated,
    NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAColorSize   ,32,
    NSOpenGLPFADepthSize   ,24,
    NSOpenGLPFAStencilSize ,8,
    0
};

static const NSOpenGLPixelFormatAttribute core41TestProfile[] = {
    NSOpenGLPFAAccelerated,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAColorSize   ,32,
    NSOpenGLPFADepthSize   ,24,
    NSOpenGLPFAStencilSize ,8,
    0
};

int setupCoreProfileNativeFormats() {

    NSOpenGLPixelFormat* core41Supported =
        [[NSOpenGLPixelFormat alloc] initWithAttributes: core41TestProfile];

    if (core41Supported) {
        setCoreProfileLevel(NSOpenGLProfileVersion4_1Core);
        [core41Supported release];
        return (int)NSOpenGLProfileVersion4_1Core;
    }

    NSOpenGLPixelFormat* core32Supported =
        [[NSOpenGLPixelFormat alloc] initWithAttributes: core32TestProfile];

    if (core32Supported) {
        setCoreProfileLevel(NSOpenGLProfileVersion3_2Core);
        [core32Supported release];
        return (int)NSOpenGLProfileVersion3_2Core;
    }

    return (int)NSOpenGLProfileVersionLegacy;
}

int getNumPixelFormats(){
    int size;
    const NSOpenGLPixelFormatAttribute* const* attrib_lists =
        getPixelFormatsAttributes(&size);
    return size;
}

void* finalizePixelFormat(bool coreProfile,
                          int attribsId) {
    int size;
    const NSOpenGLPixelFormatAttribute* const* attrib_lists =
        getPixelFormatsAttributes(&size);

    assert(attribsId < size);

    const NSOpenGLPixelFormatAttribute*  attrs =
        attrib_lists[attribsId];

    const NSOpenGLPixelFormatAttribute* selected_variant =
        coreProfile ?
        getCoreProfileAttributes() :
        getLegacyProfileAttributes();

    // Format it as |variant| |attribs|
    int variant_size =
        getAttrListLength(selected_variant);
    int attrib_size = getAttrListLength(attrs);
    int numAttribsTotal = attrib_size + variant_size + 1; // for trailing 0

    NSOpenGLPixelFormatAttribute* newAttrs =
        malloc(sizeof(NSOpenGLPixelFormatAttribute) * numAttribsTotal);

    int variant_part_bytes =
        sizeof(NSOpenGLPixelFormatAttribute) * variant_size;
    int attribs_part_bytes =
        sizeof(NSOpenGLPixelFormatAttribute) * attrib_size;

    memcpy(newAttrs, selected_variant, variant_part_bytes);
    memcpy((char*)newAttrs + variant_part_bytes,
           attrs, attribs_part_bytes);
    newAttrs[numAttribsTotal - 1] = 0;

    void* finalizedFormat =
        [[NSOpenGLPixelFormat alloc] initWithAttributes: newAttrs];

    free(newAttrs);

    return finalizedFormat;
}

static bool sIsKeyValueAttrib(NSOpenGLPixelFormatAttribute attrib) {
    switch (attrib) {
        // These are the ones that take a value, according to the current
        // NSOpenGLPixelFormat docs
        case NSOpenGLPFAOpenGLProfile:
        case NSOpenGLPFAAuxBuffers:
        case NSOpenGLPFAColorSize:
        case NSOpenGLPFAAlphaSize:
        case NSOpenGLPFADepthSize:
        case NSOpenGLPFAStencilSize:
        case NSOpenGLPFAAccumSize:
        case NSOpenGLPFARendererID:
        case NSOpenGLPFAScreenMask:
            return true;
        default:
            return false;
    }
}

int getPixelFormatAttrib(int i, int _query) {
    NSOpenGLPixelFormatAttribute query =
        (NSOpenGLPixelFormatAttribute)_query;
    int size;
    const NSOpenGLPixelFormatAttribute* const* attrib_lists = getPixelFormatsAttributes(&size);
    int attributes_num = i % size;
    const NSOpenGLPixelFormatAttribute* attribs = attrib_lists[attributes_num];
    int res = 0;
    while (*attribs) {
        if (sIsKeyValueAttrib(*attribs)) {
            if (query == *attribs) {
                return attribs[1];
            }
            attribs += 2;
        } else {
            // these are boolean attribs.
            // their mere presence signals
            // that the query should return true.
            if (query == *attribs) {
                return 1;
            }
            attribs++;
        }
    }
    // return 0 if key not found---takes care of all boolean attribs,
    // and we depend on returning alpha=0 to make the default
    // config for GLSurfaceView happy.
    return 0;
}

void* nsCreateContext(void* format,void* share){
    NSOpenGLPixelFormat* frmt = (NSOpenGLPixelFormat*)format;
    return [[EmuGLContext alloc] initWithFormat:frmt shareContext:share];
}

void* nsGetLowLevelContext(void* context) {
    EmuGLContext* ctx = (EmuGLContext *)context;
    return ctx;
}

void nsConvertVideoFrameToNV12Textures(void* context, void* iosurface, int* Ytex, int* UVtex) {
    //NSLog(@"calling nsConvertVideoFrameToNV12Textures");

    EmuGLContext* ctx = (EmuGLContext *)context;

    // https://developer.apple.com/forums/thread/27589
    // this is probably already current when this is called
    // [ctx makeCurrentContext];
    /*
CGLTexImageIOSurface2D(glContext,GL_TEXTURE_RECTANGLE,GL_R8,(GLsizei)textureSize.width,(GLsizei)textureSize.height,GL_RED,GL_UNSIGNED_BYTE,surfaceRef, 0);

// chroma texture, subsampled

CGLTexImageIOSurface2D(glContext,GL_TEXTURE_RECTANGLE, GL_RG8, (GLsizei)planeSize.width, (GLsizei)planeSize.height, GL_RG, GL_UNSIGNED_BYTE, surfaceRef, 1);
    */
    CGLContextObj cgl_ctx = ctx.CGLContextObj;
    
    glEnable(GL_TEXTURE_RECTANGLE);
   
    IOSurfaceRef* surface = (IOSurfaceRef*)iosurface;

    //NSLog(@"get w and h");
    GLsizei surface_w = (GLsizei)IOSurfaceGetWidth(surface);
    GLsizei surface_h = (GLsizei)IOSurfaceGetHeight(surface);
      
    //NSLog(@"create textures y");
    glGenTextures(1, Ytex);
    glBindTexture(GL_TEXTURE_RECTANGLE, *Ytex);
    //NSLog(@"fetch textures y");
    CGLError cglError =
        CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE,
                GL_R8, surface_w, surface_h, GL_RED, GL_UNSIGNED_BYTE, surface, 0);
   
    if (cglError != kCGLNoError) {
        fprintf(stderr, "create textures y error %d\n", cglError);
    }

    //NSLog(@"create textures uv");
    glGenTextures(1, UVtex);
    glBindTexture(GL_TEXTURE_RECTANGLE, *UVtex);
    cglError =
        CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE,
                GL_RG8, surface_w/2, surface_h/2, GL_RG, GL_UNSIGNED_BYTE, surface, 1);

    if (cglError != kCGLNoError) {
        fprintf(stderr, "create textures uv error %d\n", cglError);
    }
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    //NSLog(@"done creating textures uv");
}

CVPixelBufferRef nsCreateNV12PixelBuffer(int w, int h) {
        NSDictionary* cvBufferProperties = @{
            (__bridge NSString*)kCVPixelBufferOpenGLCompatibilityKey : @YES,
            (__bridge NSString*)kCVPixelBufferIOSurfaceOpenGLTextureCompatibilityKey: @YES,
        };
        CVPixelBufferRef _CVPixelBuffer;
        CVReturn cvret = CVPixelBufferCreate(kCFAllocatorDefault,
                                w, h,
                                kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
                                (__bridge CFDictionaryRef)cvBufferProperties,
                                &_CVPixelBuffer);

        return _CVPixelBuffer;
}

void nsCopyTexture(void* context, int from, int to, int width, int height) {
    //NSLog(@"calling nsCopyTexture");
    EmuGLContext* ctx = (EmuGLContext *)context;
    // assume ctx is alreldy current

      if (glGetError() != GL_NO_ERROR) {
          //NSLog(@"bad in blit 0");
      }
    int tex1 = from;
    int tex2 = to;
      GLuint g_fb=0;
      glGenFramebuffers( 1, &g_fb );
      glBindFramebuffer( GL_FRAMEBUFFER, g_fb );
      glBindTexture(GL_TEXTURE_RECTANGLE, tex1);
      glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
              GL_TEXTURE_RECTANGLE, tex1, 0);
      if (glGetError() != GL_NO_ERROR) {
          //NSLog(@"bad in blit 1");
      }
      glBindTexture(GL_TEXTURE_2D, tex2);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
              GL_TEXTURE_2D, tex2, 0);
      if (glGetError() != GL_NO_ERROR) {
          //NSLog(@"bad in blit 2");
      }
      glDrawBuffer(GL_COLOR_ATTACHMENT1);

      if (glGetError() != GL_NO_ERROR) {
          //NSLog(@"bad in blit 3");
      }
      glBlitFramebuffer(0,0,width, height, 0,0,width, height,
              GL_COLOR_BUFFER_BIT, GL_NEAREST);

      if (glGetError() != GL_NO_ERROR) {
          //NSLog(@"bad in blit4 ");
      }

      glBindFramebuffer( GL_FRAMEBUFFER, 0);
      glDeleteTextures(1, &from);
      glDeleteFramebuffers( 1, &g_fb );
}

void nsUpdateNV12TexturesFromIOSurface(void* context, void* iosurface, int Ytex1, int UVtex1) {
    EmuGLContext* ctx = (EmuGLContext *)context;
    CGLContextObj cgl_ctx = ctx.CGLContextObj;
    CVPixelBufferRef mDecodedFrame = (CVPixelBufferRef)iosurface;
    IOSurfaceRef* surface = CVPixelBufferGetIOSurface(mDecodedFrame);
    GLsizei surface_w = (GLsizei)IOSurfaceGetWidth(surface);
    GLsizei surface_h = (GLsizei)IOSurfaceGetHeight(surface);
    unsigned char * data =  NULL;//(unsigned char*)malloc(surface_w * surface_h);
    //NSLog(@"data[10] is %d", data[10]);

   

    CVPixelBufferLockBaseAddress(mDecodedFrame, kCVPixelBufferLock_ReadOnly);
    int planes = CVPixelBufferGetPlaneCount(mDecodedFrame);
    int linesize = CVPixelBufferGetBytesPerRowOfPlane(mDecodedFrame, 0);

    int planeWidth = CVPixelBufferGetWidthOfPlane(mDecodedFrame, 0);
    int planeHeight = CVPixelBufferGetHeightOfPlane(mDecodedFrame, 0);

    int Ytex, UVtex;
    glGenTextures(1, &Ytex);
    glBindTexture(GL_TEXTURE_RECTANGLE, Ytex);
    static CVPixelBufferRef mycopy = NULL;
    if (mycopy == NULL) {
        mycopy = nsCreateNV12PixelBuffer(surface_w, surface_h);
    }
    CVPixelBufferLockBaseAddress(mycopy, 0);
    void* planeData = CVPixelBufferGetBaseAddressOfPlane(mDecodedFrame, 0);
    data = CVPixelBufferGetBaseAddressOfPlane(mycopy, 0);
    memcpy(data, planeData, surface_w * surface_h);
    planeData = CVPixelBufferGetBaseAddressOfPlane(mDecodedFrame, 1);
    data = CVPixelBufferGetBaseAddressOfPlane(mycopy, 1);
    memcpy(data, planeData, surface_w * surface_h/2);
    CVPixelBufferUnlockBaseAddress(mycopy, 0);

    glActiveTexture(GL_TEXTURE0);
    if(1) {
        glBindTexture(GL_TEXTURE_2D, Ytex);
        CGLError cglError =
        CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE, GL_R8, surface_w, surface_h, GL_RED, GL_UNSIGNED_BYTE, surface, 0); 
        if (cglError != kCGLNoError) {
            fprintf(stderr, "create textures y error %d\n", cglError);
        }
    }

    glGenTextures(1, &UVtex);
    glBindTexture(GL_TEXTURE_RECTANGLE, UVtex);

    if(1) {
        glBindTexture(GL_TEXTURE_2D, UVtex);
        CGLError cglError = CGLTexImageIOSurface2D(cgl_ctx, GL_TEXTURE_RECTANGLE,
                GL_RG8, surface_w/2, surface_h/2, GL_RG, GL_UNSIGNED_BYTE, surface, 1); 
        if (cglError != kCGLNoError) {
            fprintf(stderr, "create textures uv error %d\n", cglError);
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    //free(data);
    CVPixelBufferUnlockBaseAddress(mDecodedFrame, kCVPixelBufferLock_ReadOnly);
    
    if (1) {
        nsCopyTexture(context, Ytex, Ytex1, surface_w, surface_h);
        nsCopyTexture(context, UVtex, UVtex1, surface_w/2, surface_h/2);
        return;
    }

}

void nsUpdateNV12TexturesFromIOSurfacev0(void* context, void* iosurface, int Ytex, int UVtex) {
    //NSLog(@"calling %s %d", __func__, __LINE__);

    EmuGLContext* ctx = (EmuGLContext *)context;
    CGLContextObj cgl_ctx = ctx.CGLContextObj;
    CVPixelBufferRef mDecodedFrame = (CVPixelBufferRef)iosurface;
    IOSurfaceRef* surface = CVPixelBufferGetIOSurface(mDecodedFrame);
    GLsizei surface_w = (GLsizei)IOSurfaceGetWidth(surface);
    GLsizei surface_h = (GLsizei)IOSurfaceGetHeight(surface);
    unsigned char * data =  NULL;//(unsigned char*)malloc(surface_w * surface_h);
    //NSLog(@"data[10] is %d", data[10]);

   

    CVPixelBufferLockBaseAddress(mDecodedFrame, kCVPixelBufferLock_ReadOnly);
    int planes = CVPixelBufferGetPlaneCount(mDecodedFrame);
     int linesize = CVPixelBufferGetBytesPerRowOfPlane(mDecodedFrame, 0);

    void* planeData = CVPixelBufferGetBaseAddressOfPlane(mDecodedFrame, 0);
                int planeWidth = CVPixelBufferGetWidthOfPlane(mDecodedFrame, 0);
            int planeHeight = CVPixelBufferGetHeightOfPlane(mDecodedFrame, 0);

    //NSLog(@"there are %d planes, 0th plandata is %p planw %d planeh %d linew %d", planes, planeData, planeWidth, planeHeight, linesize);
    //memcpy(data, planeData, surface_w * surface_h);
    data = planeData;
    //NSLog(@"copy done for plandata is %p", planeData);



    //NSLog(@"data[10] is %d later", data[10]);
    glActiveTexture(GL_TEXTURE0);
    if(1) {
    glBindTexture(GL_TEXTURE_2D, Ytex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, surface_w, surface_h, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,surface_w,surface_h,GL_RED,GL_UNSIGNED_BYTE,data);
    
   //   if (glGetError() != GL_NO_ERROR) { NSLog(@"bad in %s %d", __func__, __LINE__); }
    }

    if(1) {
    glBindTexture(GL_TEXTURE_2D, 0);
    planeData = CVPixelBufferGetBaseAddressOfPlane(mDecodedFrame, 1);
     linesize = CVPixelBufferGetBytesPerRowOfPlane(mDecodedFrame, 1);
    planeWidth = CVPixelBufferGetWidthOfPlane(mDecodedFrame, 1);
    planeHeight = CVPixelBufferGetHeightOfPlane(mDecodedFrame, 1);
    //NSLog(@"there are %d planes, 1th plandata is %p planw %d planeh %d linew %d", planes, planeData, planeWidth, planeHeight, linesize);
    //memcpy(data, planeData, surface_w * surface_h / 2);
    data = planeData;


    glBindTexture(GL_TEXTURE_2D, UVtex);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, surface_w/2, surface_h/2, 0, GL_RG, GL_UNSIGNED_BYTE, data);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,surface_w/2,surface_h/2,GL_RG,GL_UNSIGNED_BYTE,data);
//      if (glGetError() != GL_NO_ERROR) { NSLog(@"bad in %s %d", __func__, __LINE__); }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    //free(data);
    CVPixelBufferUnlockBaseAddress(mDecodedFrame, kCVPixelBufferLock_ReadOnly);

}

void  nsPBufferMakeCurrent(void* context,void* nativePBuffer,int level){
    EmuGLContext* ctx = (EmuGLContext *)context;
    NSOpenGLPixelBuffer* pbuff = (NSOpenGLPixelBuffer *)nativePBuffer;
    if(ctx == nil){
        [NSOpenGLContext clearCurrentContext];
    } else {
        if(pbuff != nil){
            [ctx preBind:1];
            [ctx setPixelBuffer:pbuff cubeMapFace:0 mipMapLevel:level currentVirtualScreen:0];
            [ctx makeCurrentContext];
        } else {
            // in this case, pbuffers deprecated and disabled.
            [ctx preBind:0];
            [ctx makeCurrentContext];
        }
    }
}

void nsWindowMakeCurrent(void* context,void* nativeWin){
    EmuGLContext* ctx = (EmuGLContext *)context;
    NSView* win = (NSView *)nativeWin;
    if(ctx == nil){
        [NSOpenGLContext clearCurrentContext];
    } else if (win != nil) {
        [ctx preBind:0];
        [ctx setView: win];
        [ctx makeCurrentContext];
    }
}

void nsSwapBuffers(){
    NSOpenGLContext* ctx = [NSOpenGLContext currentContext];
    if(ctx != nil){
        [ctx flushBuffer];
    }
}

void nsSwapInterval(int *interval){
    NSOpenGLContext* ctx = [NSOpenGLContext currentContext];
    if( ctx != nil){
        [ctx setValues:interval forParameter:NSOpenGLCPSwapInterval];
    }
}


void nsDestroyContext(void* context){
    EmuGLContext *ctx = (EmuGLContext*)context;
    if(ctx != nil){
        [ctx release];
    }
}


void* nsCreatePBuffer(GLenum target,GLenum format,int maxMip,int width,int height){
    return [[NSOpenGLPixelBuffer alloc] initWithTextureTarget:target
                                        textureInternalFormat:format
                                        textureMaxMipMapLevel:maxMip
                                        pixelsWide:width pixelsHigh:height];

}

void nsDestroyPBuffer(void* pbuffer){
    NSOpenGLPixelBuffer *pbuf = (NSOpenGLPixelBuffer*)pbuffer;
    if(pbuf != nil){
        [pbuf release];
    }
}

bool nsGetWinDims(void* win,unsigned int* width,unsigned int* height){
    NSView* view = (NSView*)win;
    if(view != nil){
        NSRect rect = [view bounds];
        *width  = rect.size.width;
        *height = rect.size.height;
        return true;
    }
    return false;
}

static bool nsCanUseBlitting() {
    NSArray<id<MTLDevice>> *_deviceList = MTLCopyAllDevices();
    NSMutableArray<id<MTLDevice>> *externalGPUs = [[NSMutableArray alloc] init];
    NSMutableArray<id<MTLDevice>> *integratedGPUs = [[NSMutableArray alloc] init];
    NSMutableArray<id<MTLDevice>> *discreteGPUs = [[NSMutableArray alloc] init];

    id<MTLDevice> defaultDev = MTLCreateSystemDefaultDevice();

    int count = 0;
    for (id <MTLDevice> device in _deviceList) {
        if (device.removable) {
            [externalGPUs addObject:device];
        } else if (device.lowPower) {
            ++count;
            [integratedGPUs addObject:device];
            if (device.registryID == defaultDev.registryID) {
                return true;
            }
        } else {
            ++count;
            [discreteGPUs addObject:device];
        }
    }

    // This is really hack based on my limited testing with Intel Mac and M1:
    // the conclusion so far:
    // For Intel Mac, when there are integrated GPU and descrete GPU (such as Radon),
    // if default GPU is integrated, then it is blitable; if the default is Radon,
    // then it is not blitable, and we have to use glTexSubImage2D to update textures;
    // if there is only one discreteGPU, we can still blit: e.g., M1 has one descrete GPU
    // and blit is fine
    if (count == 1) {
        return true;
    }
}

bool  nsCheckColor(void* win,int colorSize){
    NSView* view = (NSView*)win;
   if(view != nil){
       NSWindow* wnd = [view window];
       if(wnd != nil){
           NSWindowDepth limit = [wnd depthLimit];
           NSWindowDepth defaultLimit = [NSWindow defaultDepthLimit];

           int depth = (limit != 0) ? NSBitsPerPixelFromDepth(limit):
                                      NSBitsPerPixelFromDepth(defaultLimit);
           return depth >= colorSize;

       }
   }
   return false;

}
