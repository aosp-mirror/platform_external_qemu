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
#include "GLESv1Dispatch.h"

#include <stdio.h>
#include <stdlib.h>

#include "emugl/common/shared_library.h"

GLESv1Dispatch s_gles1;

static emugl::SharedLibrary *s_gles1_lib = NULL;

//
// This function is called only once during initialiation before
// any thread has been created - hence it should NOT be thread safe.
//

#define DEFAULT_GLES_CM_LIB EMUGL_LIBNAME("GLES_CM_translator")

bool init_gles1_dispatch()
{
    const char *libName = getenv("ANDROID_GLESv1_LIB");
    if (!libName) libName = DEFAULT_GLES_CM_LIB;

    s_gles1_lib = emugl::SharedLibrary::open(libName);
    if (!s_gles1_lib) return false;

    s_gles1.glAlphaFunc = (glAlphaFunc_t) s_gles1_lib->findSymbol("glAlphaFunc");
    s_gles1.glClearColor = (glClearColor_t) s_gles1_lib->findSymbol("glClearColor");
    s_gles1.glClearDepthf = (glClearDepthf_t) s_gles1_lib->findSymbol("glClearDepthf");
    s_gles1.glClipPlanef = (glClipPlanef_t) s_gles1_lib->findSymbol("glClipPlanef");
    s_gles1.glColor4f = (glColor4f_t) s_gles1_lib->findSymbol("glColor4f");
    s_gles1.glDepthRangef = (glDepthRangef_t) s_gles1_lib->findSymbol("glDepthRangef");
    s_gles1.glFogf = (glFogf_t) s_gles1_lib->findSymbol("glFogf");
    s_gles1.glFogfv = (glFogfv_t) s_gles1_lib->findSymbol("glFogfv");
    s_gles1.glFrustumf = (glFrustumf_t) s_gles1_lib->findSymbol("glFrustumf");
    s_gles1.glGetClipPlanef = (glGetClipPlanef_t) s_gles1_lib->findSymbol("glGetClipPlanef");
    s_gles1.glGetFloatv = (glGetFloatv_t) s_gles1_lib->findSymbol("glGetFloatv");
    s_gles1.glGetLightfv = (glGetLightfv_t) s_gles1_lib->findSymbol("glGetLightfv");
    s_gles1.glGetMaterialfv = (glGetMaterialfv_t) s_gles1_lib->findSymbol("glGetMaterialfv");
    s_gles1.glGetTexEnvfv = (glGetTexEnvfv_t) s_gles1_lib->findSymbol("glGetTexEnvfv");
    s_gles1.glGetTexParameterfv = (glGetTexParameterfv_t) s_gles1_lib->findSymbol("glGetTexParameterfv");
    s_gles1.glLightModelf = (glLightModelf_t) s_gles1_lib->findSymbol("glLightModelf");
    s_gles1.glLightModelfv = (glLightModelfv_t) s_gles1_lib->findSymbol("glLightModelfv");
    s_gles1.glLightf = (glLightf_t) s_gles1_lib->findSymbol("glLightf");
    s_gles1.glLightfv = (glLightfv_t) s_gles1_lib->findSymbol("glLightfv");
    s_gles1.glLineWidth = (glLineWidth_t) s_gles1_lib->findSymbol("glLineWidth");
    s_gles1.glLoadMatrixf = (glLoadMatrixf_t) s_gles1_lib->findSymbol("glLoadMatrixf");
    s_gles1.glMaterialf = (glMaterialf_t) s_gles1_lib->findSymbol("glMaterialf");
    s_gles1.glMaterialfv = (glMaterialfv_t) s_gles1_lib->findSymbol("glMaterialfv");
    s_gles1.glMultMatrixf = (glMultMatrixf_t) s_gles1_lib->findSymbol("glMultMatrixf");
    s_gles1.glMultiTexCoord4f = (glMultiTexCoord4f_t) s_gles1_lib->findSymbol("glMultiTexCoord4f");
    s_gles1.glNormal3f = (glNormal3f_t) s_gles1_lib->findSymbol("glNormal3f");
    s_gles1.glOrthof = (glOrthof_t) s_gles1_lib->findSymbol("glOrthof");
    s_gles1.glPointParameterf = (glPointParameterf_t) s_gles1_lib->findSymbol("glPointParameterf");
    s_gles1.glPointParameterfv = (glPointParameterfv_t) s_gles1_lib->findSymbol("glPointParameterfv");
    s_gles1.glPointSize = (glPointSize_t) s_gles1_lib->findSymbol("glPointSize");
    s_gles1.glPolygonOffset = (glPolygonOffset_t) s_gles1_lib->findSymbol("glPolygonOffset");
    s_gles1.glRotatef = (glRotatef_t) s_gles1_lib->findSymbol("glRotatef");
    s_gles1.glScalef = (glScalef_t) s_gles1_lib->findSymbol("glScalef");
    s_gles1.glTexEnvf = (glTexEnvf_t) s_gles1_lib->findSymbol("glTexEnvf");
    s_gles1.glTexEnvfv = (glTexEnvfv_t) s_gles1_lib->findSymbol("glTexEnvfv");
    s_gles1.glTexParameterf = (glTexParameterf_t) s_gles1_lib->findSymbol("glTexParameterf");
    s_gles1.glTexParameterfv = (glTexParameterfv_t) s_gles1_lib->findSymbol("glTexParameterfv");
    s_gles1.glTranslatef = (glTranslatef_t) s_gles1_lib->findSymbol("glTranslatef");
    s_gles1.glActiveTexture = (glActiveTexture_t) s_gles1_lib->findSymbol("glActiveTexture");
    s_gles1.glAlphaFuncx = (glAlphaFuncx_t) s_gles1_lib->findSymbol("glAlphaFuncx");
    s_gles1.glBindBuffer = (glBindBuffer_t) s_gles1_lib->findSymbol("glBindBuffer");
    s_gles1.glBindTexture = (glBindTexture_t) s_gles1_lib->findSymbol("glBindTexture");
    s_gles1.glBlendFunc = (glBlendFunc_t) s_gles1_lib->findSymbol("glBlendFunc");
    s_gles1.glBufferData = (glBufferData_t) s_gles1_lib->findSymbol("glBufferData");
    s_gles1.glBufferSubData = (glBufferSubData_t) s_gles1_lib->findSymbol("glBufferSubData");
    s_gles1.glClear = (glClear_t) s_gles1_lib->findSymbol("glClear");
    s_gles1.glClearColorx = (glClearColorx_t) s_gles1_lib->findSymbol("glClearColorx");
    s_gles1.glClearDepthx = (glClearDepthx_t) s_gles1_lib->findSymbol("glClearDepthx");
    s_gles1.glClearStencil = (glClearStencil_t) s_gles1_lib->findSymbol("glClearStencil");
    s_gles1.glClientActiveTexture = (glClientActiveTexture_t) s_gles1_lib->findSymbol("glClientActiveTexture");
    s_gles1.glClipPlanex = (glClipPlanex_t) s_gles1_lib->findSymbol("glClipPlanex");
    s_gles1.glColor4ub = (glColor4ub_t) s_gles1_lib->findSymbol("glColor4ub");
    s_gles1.glColor4x = (glColor4x_t) s_gles1_lib->findSymbol("glColor4x");
    s_gles1.glColorMask = (glColorMask_t) s_gles1_lib->findSymbol("glColorMask");
    s_gles1.glColorPointer = (glColorPointer_t) s_gles1_lib->findSymbol("glColorPointer");
    s_gles1.glCompressedTexImage2D = (glCompressedTexImage2D_t) s_gles1_lib->findSymbol("glCompressedTexImage2D");
    s_gles1.glCompressedTexSubImage2D = (glCompressedTexSubImage2D_t) s_gles1_lib->findSymbol("glCompressedTexSubImage2D");
    s_gles1.glCopyTexImage2D = (glCopyTexImage2D_t) s_gles1_lib->findSymbol("glCopyTexImage2D");
    s_gles1.glCopyTexSubImage2D = (glCopyTexSubImage2D_t) s_gles1_lib->findSymbol("glCopyTexSubImage2D");
    s_gles1.glCullFace = (glCullFace_t) s_gles1_lib->findSymbol("glCullFace");
    s_gles1.glDeleteBuffers = (glDeleteBuffers_t) s_gles1_lib->findSymbol("glDeleteBuffers");
    s_gles1.glDeleteTextures = (glDeleteTextures_t) s_gles1_lib->findSymbol("glDeleteTextures");
    s_gles1.glDepthFunc = (glDepthFunc_t) s_gles1_lib->findSymbol("glDepthFunc");
    s_gles1.glDepthMask = (glDepthMask_t) s_gles1_lib->findSymbol("glDepthMask");
    s_gles1.glDepthRangex = (glDepthRangex_t) s_gles1_lib->findSymbol("glDepthRangex");
    s_gles1.glDisable = (glDisable_t) s_gles1_lib->findSymbol("glDisable");
    s_gles1.glDisableClientState = (glDisableClientState_t) s_gles1_lib->findSymbol("glDisableClientState");
    s_gles1.glDrawArrays = (glDrawArrays_t) s_gles1_lib->findSymbol("glDrawArrays");
    s_gles1.glDrawElements = (glDrawElements_t) s_gles1_lib->findSymbol("glDrawElements");
    s_gles1.glEnable = (glEnable_t) s_gles1_lib->findSymbol("glEnable");
    s_gles1.glEnableClientState = (glEnableClientState_t) s_gles1_lib->findSymbol("glEnableClientState");
    s_gles1.glFinish = (glFinish_t) s_gles1_lib->findSymbol("glFinish");
    s_gles1.glFlush = (glFlush_t) s_gles1_lib->findSymbol("glFlush");
    s_gles1.glFogx = (glFogx_t) s_gles1_lib->findSymbol("glFogx");
    s_gles1.glFogxv = (glFogxv_t) s_gles1_lib->findSymbol("glFogxv");
    s_gles1.glFrontFace = (glFrontFace_t) s_gles1_lib->findSymbol("glFrontFace");
    s_gles1.glFrustumx = (glFrustumx_t) s_gles1_lib->findSymbol("glFrustumx");
    s_gles1.glGetBooleanv = (glGetBooleanv_t) s_gles1_lib->findSymbol("glGetBooleanv");
    s_gles1.glGetBufferParameteriv = (glGetBufferParameteriv_t) s_gles1_lib->findSymbol("glGetBufferParameteriv");
    s_gles1.glGetClipPlanex = (glGetClipPlanex_t) s_gles1_lib->findSymbol("glGetClipPlanex");
    s_gles1.glGenBuffers = (glGenBuffers_t) s_gles1_lib->findSymbol("glGenBuffers");
    s_gles1.glGenTextures = (glGenTextures_t) s_gles1_lib->findSymbol("glGenTextures");
    s_gles1.glGetError = (glGetError_t) s_gles1_lib->findSymbol("glGetError");
    s_gles1.glGetFixedv = (glGetFixedv_t) s_gles1_lib->findSymbol("glGetFixedv");
    s_gles1.glGetIntegerv = (glGetIntegerv_t) s_gles1_lib->findSymbol("glGetIntegerv");
    s_gles1.glGetLightxv = (glGetLightxv_t) s_gles1_lib->findSymbol("glGetLightxv");
    s_gles1.glGetMaterialxv = (glGetMaterialxv_t) s_gles1_lib->findSymbol("glGetMaterialxv");
    s_gles1.glGetPointerv = (glGetPointerv_t) s_gles1_lib->findSymbol("glGetPointerv");
    s_gles1.glGetString = (glGetString_t) s_gles1_lib->findSymbol("glGetString");
    s_gles1.glGetTexEnviv = (glGetTexEnviv_t) s_gles1_lib->findSymbol("glGetTexEnviv");
    s_gles1.glGetTexEnvxv = (glGetTexEnvxv_t) s_gles1_lib->findSymbol("glGetTexEnvxv");
    s_gles1.glGetTexParameteriv = (glGetTexParameteriv_t) s_gles1_lib->findSymbol("glGetTexParameteriv");
    s_gles1.glGetTexParameterxv = (glGetTexParameterxv_t) s_gles1_lib->findSymbol("glGetTexParameterxv");
    s_gles1.glHint = (glHint_t) s_gles1_lib->findSymbol("glHint");
    s_gles1.glIsBuffer = (glIsBuffer_t) s_gles1_lib->findSymbol("glIsBuffer");
    s_gles1.glIsEnabled = (glIsEnabled_t) s_gles1_lib->findSymbol("glIsEnabled");
    s_gles1.glIsTexture = (glIsTexture_t) s_gles1_lib->findSymbol("glIsTexture");
    s_gles1.glLightModelx = (glLightModelx_t) s_gles1_lib->findSymbol("glLightModelx");
    s_gles1.glLightModelxv = (glLightModelxv_t) s_gles1_lib->findSymbol("glLightModelxv");
    s_gles1.glLightx = (glLightx_t) s_gles1_lib->findSymbol("glLightx");
    s_gles1.glLightxv = (glLightxv_t) s_gles1_lib->findSymbol("glLightxv");
    s_gles1.glLineWidthx = (glLineWidthx_t) s_gles1_lib->findSymbol("glLineWidthx");
    s_gles1.glLoadIdentity = (glLoadIdentity_t) s_gles1_lib->findSymbol("glLoadIdentity");
    s_gles1.glLoadMatrixx = (glLoadMatrixx_t) s_gles1_lib->findSymbol("glLoadMatrixx");
    s_gles1.glLogicOp = (glLogicOp_t) s_gles1_lib->findSymbol("glLogicOp");
    s_gles1.glMaterialx = (glMaterialx_t) s_gles1_lib->findSymbol("glMaterialx");
    s_gles1.glMaterialxv = (glMaterialxv_t) s_gles1_lib->findSymbol("glMaterialxv");
    s_gles1.glMatrixMode = (glMatrixMode_t) s_gles1_lib->findSymbol("glMatrixMode");
    s_gles1.glMultMatrixx = (glMultMatrixx_t) s_gles1_lib->findSymbol("glMultMatrixx");
    s_gles1.glMultiTexCoord4x = (glMultiTexCoord4x_t) s_gles1_lib->findSymbol("glMultiTexCoord4x");
    s_gles1.glNormal3x = (glNormal3x_t) s_gles1_lib->findSymbol("glNormal3x");
    s_gles1.glNormalPointer = (glNormalPointer_t) s_gles1_lib->findSymbol("glNormalPointer");
    s_gles1.glOrthox = (glOrthox_t) s_gles1_lib->findSymbol("glOrthox");
    s_gles1.glPixelStorei = (glPixelStorei_t) s_gles1_lib->findSymbol("glPixelStorei");
    s_gles1.glPointParameterx = (glPointParameterx_t) s_gles1_lib->findSymbol("glPointParameterx");
    s_gles1.glPointParameterxv = (glPointParameterxv_t) s_gles1_lib->findSymbol("glPointParameterxv");
    s_gles1.glPointSizex = (glPointSizex_t) s_gles1_lib->findSymbol("glPointSizex");
    s_gles1.glPolygonOffsetx = (glPolygonOffsetx_t) s_gles1_lib->findSymbol("glPolygonOffsetx");
    s_gles1.glPopMatrix = (glPopMatrix_t) s_gles1_lib->findSymbol("glPopMatrix");
    s_gles1.glPushMatrix = (glPushMatrix_t) s_gles1_lib->findSymbol("glPushMatrix");
    s_gles1.glReadPixels = (glReadPixels_t) s_gles1_lib->findSymbol("glReadPixels");
    s_gles1.glRotatex = (glRotatex_t) s_gles1_lib->findSymbol("glRotatex");
    s_gles1.glSampleCoverage = (glSampleCoverage_t) s_gles1_lib->findSymbol("glSampleCoverage");
    s_gles1.glSampleCoveragex = (glSampleCoveragex_t) s_gles1_lib->findSymbol("glSampleCoveragex");
    s_gles1.glScalex = (glScalex_t) s_gles1_lib->findSymbol("glScalex");
    s_gles1.glScissor = (glScissor_t) s_gles1_lib->findSymbol("glScissor");
    s_gles1.glShadeModel = (glShadeModel_t) s_gles1_lib->findSymbol("glShadeModel");
    s_gles1.glStencilFunc = (glStencilFunc_t) s_gles1_lib->findSymbol("glStencilFunc");
    s_gles1.glStencilMask = (glStencilMask_t) s_gles1_lib->findSymbol("glStencilMask");
    s_gles1.glStencilOp = (glStencilOp_t) s_gles1_lib->findSymbol("glStencilOp");
    s_gles1.glTexCoordPointer = (glTexCoordPointer_t) s_gles1_lib->findSymbol("glTexCoordPointer");
    s_gles1.glTexEnvi = (glTexEnvi_t) s_gles1_lib->findSymbol("glTexEnvi");
    s_gles1.glTexEnvx = (glTexEnvx_t) s_gles1_lib->findSymbol("glTexEnvx");
    s_gles1.glTexEnviv = (glTexEnviv_t) s_gles1_lib->findSymbol("glTexEnviv");
    s_gles1.glTexEnvxv = (glTexEnvxv_t) s_gles1_lib->findSymbol("glTexEnvxv");
    s_gles1.glTexImage2D = (glTexImage2D_t) s_gles1_lib->findSymbol("glTexImage2D");
    s_gles1.glTexParameteri = (glTexParameteri_t) s_gles1_lib->findSymbol("glTexParameteri");
    s_gles1.glTexParameterx = (glTexParameterx_t) s_gles1_lib->findSymbol("glTexParameterx");
    s_gles1.glTexParameteriv = (glTexParameteriv_t) s_gles1_lib->findSymbol("glTexParameteriv");
    s_gles1.glTexParameterxv = (glTexParameterxv_t) s_gles1_lib->findSymbol("glTexParameterxv");
    s_gles1.glTexSubImage2D = (glTexSubImage2D_t) s_gles1_lib->findSymbol("glTexSubImage2D");
    s_gles1.glTranslatex = (glTranslatex_t) s_gles1_lib->findSymbol("glTranslatex");
    s_gles1.glVertexPointer = (glVertexPointer_t) s_gles1_lib->findSymbol("glVertexPointer");
    s_gles1.glViewport = (glViewport_t) s_gles1_lib->findSymbol("glViewport");
    s_gles1.glPointSizePointerOES = (glPointSizePointerOES_t) s_gles1_lib->findSymbol("glPointSizePointerOES");
    s_gles1.glBlendEquationSeparateOES = (glBlendEquationSeparateOES_t) s_gles1_lib->findSymbol("glBlendEquationSeparateOES");
    s_gles1.glBlendFuncSeparateOES = (glBlendFuncSeparateOES_t) s_gles1_lib->findSymbol("glBlendFuncSeparateOES");
    s_gles1.glBlendEquationOES = (glBlendEquationOES_t) s_gles1_lib->findSymbol("glBlendEquationOES");
    s_gles1.glDrawTexsOES = (glDrawTexsOES_t) s_gles1_lib->findSymbol("glDrawTexsOES");
    s_gles1.glDrawTexiOES = (glDrawTexiOES_t) s_gles1_lib->findSymbol("glDrawTexiOES");
    s_gles1.glDrawTexxOES = (glDrawTexxOES_t) s_gles1_lib->findSymbol("glDrawTexxOES");
    s_gles1.glDrawTexsvOES = (glDrawTexsvOES_t) s_gles1_lib->findSymbol("glDrawTexsvOES");
    s_gles1.glDrawTexivOES = (glDrawTexivOES_t) s_gles1_lib->findSymbol("glDrawTexivOES");
    s_gles1.glDrawTexxvOES = (glDrawTexxvOES_t) s_gles1_lib->findSymbol("glDrawTexxvOES");
    s_gles1.glDrawTexfOES = (glDrawTexfOES_t) s_gles1_lib->findSymbol("glDrawTexfOES");
    s_gles1.glDrawTexfvOES = (glDrawTexfvOES_t) s_gles1_lib->findSymbol("glDrawTexfvOES");
    s_gles1.glEGLImageTargetTexture2DOES = (glEGLImageTargetTexture2DOES_t) s_gles1_lib->findSymbol("glEGLImageTargetTexture2DOES");
    s_gles1.glEGLImageTargetRenderbufferStorageOES = (glEGLImageTargetRenderbufferStorageOES_t) s_gles1_lib->findSymbol("glEGLImageTargetRenderbufferStorageOES");
    s_gles1.glAlphaFuncxOES = (glAlphaFuncxOES_t) s_gles1_lib->findSymbol("glAlphaFuncxOES");
    s_gles1.glClearColorxOES = (glClearColorxOES_t) s_gles1_lib->findSymbol("glClearColorxOES");
    s_gles1.glClearDepthxOES = (glClearDepthxOES_t) s_gles1_lib->findSymbol("glClearDepthxOES");
    s_gles1.glClipPlanexOES = (glClipPlanexOES_t) s_gles1_lib->findSymbol("glClipPlanexOES");
    s_gles1.glColor4xOES = (glColor4xOES_t) s_gles1_lib->findSymbol("glColor4xOES");
    s_gles1.glDepthRangexOES = (glDepthRangexOES_t) s_gles1_lib->findSymbol("glDepthRangexOES");
    s_gles1.glFogxOES = (glFogxOES_t) s_gles1_lib->findSymbol("glFogxOES");
    s_gles1.glFogxvOES = (glFogxvOES_t) s_gles1_lib->findSymbol("glFogxvOES");
    s_gles1.glFrustumxOES = (glFrustumxOES_t) s_gles1_lib->findSymbol("glFrustumxOES");
    s_gles1.glGetClipPlanexOES = (glGetClipPlanexOES_t) s_gles1_lib->findSymbol("glGetClipPlanexOES");
    s_gles1.glGetFixedvOES = (glGetFixedvOES_t) s_gles1_lib->findSymbol("glGetFixedvOES");
    s_gles1.glGetLightxvOES = (glGetLightxvOES_t) s_gles1_lib->findSymbol("glGetLightxvOES");
    s_gles1.glGetMaterialxvOES = (glGetMaterialxvOES_t) s_gles1_lib->findSymbol("glGetMaterialxvOES");
    s_gles1.glGetTexEnvxvOES = (glGetTexEnvxvOES_t) s_gles1_lib->findSymbol("glGetTexEnvxvOES");
    s_gles1.glGetTexParameterxvOES = (glGetTexParameterxvOES_t) s_gles1_lib->findSymbol("glGetTexParameterxvOES");
    s_gles1.glLightModelxOES = (glLightModelxOES_t) s_gles1_lib->findSymbol("glLightModelxOES");
    s_gles1.glLightModelxvOES = (glLightModelxvOES_t) s_gles1_lib->findSymbol("glLightModelxvOES");
    s_gles1.glLightxOES = (glLightxOES_t) s_gles1_lib->findSymbol("glLightxOES");
    s_gles1.glLightxvOES = (glLightxvOES_t) s_gles1_lib->findSymbol("glLightxvOES");
    s_gles1.glLineWidthxOES = (glLineWidthxOES_t) s_gles1_lib->findSymbol("glLineWidthxOES");
    s_gles1.glLoadMatrixxOES = (glLoadMatrixxOES_t) s_gles1_lib->findSymbol("glLoadMatrixxOES");
    s_gles1.glMaterialxOES = (glMaterialxOES_t) s_gles1_lib->findSymbol("glMaterialxOES");
    s_gles1.glMaterialxvOES = (glMaterialxvOES_t) s_gles1_lib->findSymbol("glMaterialxvOES");
    s_gles1.glMultMatrixxOES = (glMultMatrixxOES_t) s_gles1_lib->findSymbol("glMultMatrixxOES");
    s_gles1.glMultiTexCoord4xOES = (glMultiTexCoord4xOES_t) s_gles1_lib->findSymbol("glMultiTexCoord4xOES");
    s_gles1.glNormal3xOES = (glNormal3xOES_t) s_gles1_lib->findSymbol("glNormal3xOES");
    s_gles1.glOrthoxOES = (glOrthoxOES_t) s_gles1_lib->findSymbol("glOrthoxOES");
    s_gles1.glPointParameterxOES = (glPointParameterxOES_t) s_gles1_lib->findSymbol("glPointParameterxOES");
    s_gles1.glPointParameterxvOES = (glPointParameterxvOES_t) s_gles1_lib->findSymbol("glPointParameterxvOES");
    s_gles1.glPointSizexOES = (glPointSizexOES_t) s_gles1_lib->findSymbol("glPointSizexOES");
    s_gles1.glPolygonOffsetxOES = (glPolygonOffsetxOES_t) s_gles1_lib->findSymbol("glPolygonOffsetxOES");
    s_gles1.glRotatexOES = (glRotatexOES_t) s_gles1_lib->findSymbol("glRotatexOES");
    s_gles1.glSampleCoveragexOES = (glSampleCoveragexOES_t) s_gles1_lib->findSymbol("glSampleCoveragexOES");
    s_gles1.glScalexOES = (glScalexOES_t) s_gles1_lib->findSymbol("glScalexOES");
    s_gles1.glTexEnvxOES = (glTexEnvxOES_t) s_gles1_lib->findSymbol("glTexEnvxOES");
    s_gles1.glTexEnvxvOES = (glTexEnvxvOES_t) s_gles1_lib->findSymbol("glTexEnvxvOES");
    s_gles1.glTexParameterxOES = (glTexParameterxOES_t) s_gles1_lib->findSymbol("glTexParameterxOES");
    s_gles1.glTexParameterxvOES = (glTexParameterxvOES_t) s_gles1_lib->findSymbol("glTexParameterxvOES");
    s_gles1.glTranslatexOES = (glTranslatexOES_t) s_gles1_lib->findSymbol("glTranslatexOES");
    s_gles1.glIsRenderbufferOES = (glIsRenderbufferOES_t) s_gles1_lib->findSymbol("glIsRenderbufferOES");
    s_gles1.glBindRenderbufferOES = (glBindRenderbufferOES_t) s_gles1_lib->findSymbol("glBindRenderbufferOES");
    s_gles1.glDeleteRenderbuffersOES = (glDeleteRenderbuffersOES_t) s_gles1_lib->findSymbol("glDeleteRenderbuffersOES");
    s_gles1.glGenRenderbuffersOES = (glGenRenderbuffersOES_t) s_gles1_lib->findSymbol("glGenRenderbuffersOES");
    s_gles1.glRenderbufferStorageOES = (glRenderbufferStorageOES_t) s_gles1_lib->findSymbol("glRenderbufferStorageOES");
    s_gles1.glGetRenderbufferParameterivOES = (glGetRenderbufferParameterivOES_t) s_gles1_lib->findSymbol("glGetRenderbufferParameterivOES");
    s_gles1.glIsFramebufferOES = (glIsFramebufferOES_t) s_gles1_lib->findSymbol("glIsFramebufferOES");
    s_gles1.glBindFramebufferOES = (glBindFramebufferOES_t) s_gles1_lib->findSymbol("glBindFramebufferOES");
    s_gles1.glDeleteFramebuffersOES = (glDeleteFramebuffersOES_t) s_gles1_lib->findSymbol("glDeleteFramebuffersOES");
    s_gles1.glGenFramebuffersOES = (glGenFramebuffersOES_t) s_gles1_lib->findSymbol("glGenFramebuffersOES");
    s_gles1.glCheckFramebufferStatusOES = (glCheckFramebufferStatusOES_t) s_gles1_lib->findSymbol("glCheckFramebufferStatusOES");
    s_gles1.glFramebufferRenderbufferOES = (glFramebufferRenderbufferOES_t) s_gles1_lib->findSymbol("glFramebufferRenderbufferOES");
    s_gles1.glFramebufferTexture2DOES = (glFramebufferTexture2DOES_t) s_gles1_lib->findSymbol("glFramebufferTexture2DOES");
    s_gles1.glGetFramebufferAttachmentParameterivOES = (glGetFramebufferAttachmentParameterivOES_t) s_gles1_lib->findSymbol("glGetFramebufferAttachmentParameterivOES");
    s_gles1.glGenerateMipmapOES = (glGenerateMipmapOES_t) s_gles1_lib->findSymbol("glGenerateMipmapOES");
    s_gles1.glMapBufferOES = (glMapBufferOES_t) s_gles1_lib->findSymbol("glMapBufferOES");
    s_gles1.glUnmapBufferOES = (glUnmapBufferOES_t) s_gles1_lib->findSymbol("glUnmapBufferOES");
    s_gles1.glGetBufferPointervOES = (glGetBufferPointervOES_t) s_gles1_lib->findSymbol("glGetBufferPointervOES");
    s_gles1.glCurrentPaletteMatrixOES = (glCurrentPaletteMatrixOES_t) s_gles1_lib->findSymbol("glCurrentPaletteMatrixOES");
    s_gles1.glLoadPaletteFromModelViewMatrixOES = (glLoadPaletteFromModelViewMatrixOES_t) s_gles1_lib->findSymbol("glLoadPaletteFromModelViewMatrixOES");
    s_gles1.glMatrixIndexPointerOES = (glMatrixIndexPointerOES_t) s_gles1_lib->findSymbol("glMatrixIndexPointerOES");
    s_gles1.glWeightPointerOES = (glWeightPointerOES_t) s_gles1_lib->findSymbol("glWeightPointerOES");
    s_gles1.glQueryMatrixxOES = (glQueryMatrixxOES_t) s_gles1_lib->findSymbol("glQueryMatrixxOES");
    s_gles1.glDepthRangefOES = (glDepthRangefOES_t) s_gles1_lib->findSymbol("glDepthRangefOES");
    s_gles1.glFrustumfOES = (glFrustumfOES_t) s_gles1_lib->findSymbol("glFrustumfOES");
    s_gles1.glOrthofOES = (glOrthofOES_t) s_gles1_lib->findSymbol("glOrthofOES");
    s_gles1.glClipPlanefOES = (glClipPlanefOES_t) s_gles1_lib->findSymbol("glClipPlanefOES");
    s_gles1.glGetClipPlanefOES = (glGetClipPlanefOES_t) s_gles1_lib->findSymbol("glGetClipPlanefOES");
    s_gles1.glClearDepthfOES = (glClearDepthfOES_t) s_gles1_lib->findSymbol("glClearDepthfOES");
    s_gles1.glTexGenfOES = (glTexGenfOES_t) s_gles1_lib->findSymbol("glTexGenfOES");
    s_gles1.glTexGenfvOES = (glTexGenfvOES_t) s_gles1_lib->findSymbol("glTexGenfvOES");
    s_gles1.glTexGeniOES = (glTexGeniOES_t) s_gles1_lib->findSymbol("glTexGeniOES");
    s_gles1.glTexGenivOES = (glTexGenivOES_t) s_gles1_lib->findSymbol("glTexGenivOES");
    s_gles1.glTexGenxOES = (glTexGenxOES_t) s_gles1_lib->findSymbol("glTexGenxOES");
    s_gles1.glTexGenxvOES = (glTexGenxvOES_t) s_gles1_lib->findSymbol("glTexGenxvOES");
    s_gles1.glGetTexGenfvOES = (glGetTexGenfvOES_t) s_gles1_lib->findSymbol("glGetTexGenfvOES");
    s_gles1.glGetTexGenivOES = (glGetTexGenivOES_t) s_gles1_lib->findSymbol("glGetTexGenivOES");
    s_gles1.glGetTexGenxvOES = (glGetTexGenxvOES_t) s_gles1_lib->findSymbol("glGetTexGenxvOES");
    s_gles1.glBindVertexArrayOES = (glBindVertexArrayOES_t) s_gles1_lib->findSymbol("glBindVertexArrayOES");
    s_gles1.glDeleteVertexArraysOES = (glDeleteVertexArraysOES_t) s_gles1_lib->findSymbol("glDeleteVertexArraysOES");
    s_gles1.glGenVertexArraysOES = (glGenVertexArraysOES_t) s_gles1_lib->findSymbol("glGenVertexArraysOES");
    s_gles1.glIsVertexArrayOES = (glIsVertexArrayOES_t) s_gles1_lib->findSymbol("glIsVertexArrayOES");
    s_gles1.glDiscardFramebufferEXT = (glDiscardFramebufferEXT_t) s_gles1_lib->findSymbol("glDiscardFramebufferEXT");
    s_gles1.glMultiDrawArraysEXT = (glMultiDrawArraysEXT_t) s_gles1_lib->findSymbol("glMultiDrawArraysEXT");
    s_gles1.glMultiDrawElementsEXT = (glMultiDrawElementsEXT_t) s_gles1_lib->findSymbol("glMultiDrawElementsEXT");
    s_gles1.glClipPlanefIMG = (glClipPlanefIMG_t) s_gles1_lib->findSymbol("glClipPlanefIMG");
    s_gles1.glClipPlanexIMG = (glClipPlanexIMG_t) s_gles1_lib->findSymbol("glClipPlanexIMG");
    s_gles1.glRenderbufferStorageMultisampleIMG = (glRenderbufferStorageMultisampleIMG_t) s_gles1_lib->findSymbol("glRenderbufferStorageMultisampleIMG");
    s_gles1.glFramebufferTexture2DMultisampleIMG = (glFramebufferTexture2DMultisampleIMG_t) s_gles1_lib->findSymbol("glFramebufferTexture2DMultisampleIMG");
    s_gles1.glDeleteFencesNV = (glDeleteFencesNV_t) s_gles1_lib->findSymbol("glDeleteFencesNV");
    s_gles1.glGenFencesNV = (glGenFencesNV_t) s_gles1_lib->findSymbol("glGenFencesNV");
    s_gles1.glIsFenceNV = (glIsFenceNV_t) s_gles1_lib->findSymbol("glIsFenceNV");
    s_gles1.glTestFenceNV = (glTestFenceNV_t) s_gles1_lib->findSymbol("glTestFenceNV");
    s_gles1.glGetFenceivNV = (glGetFenceivNV_t) s_gles1_lib->findSymbol("glGetFenceivNV");
    s_gles1.glFinishFenceNV = (glFinishFenceNV_t) s_gles1_lib->findSymbol("glFinishFenceNV");
    s_gles1.glSetFenceNV = (glSetFenceNV_t) s_gles1_lib->findSymbol("glSetFenceNV");
    s_gles1.glGetDriverControlsQCOM = (glGetDriverControlsQCOM_t) s_gles1_lib->findSymbol("glGetDriverControlsQCOM");
    s_gles1.glGetDriverControlStringQCOM = (glGetDriverControlStringQCOM_t) s_gles1_lib->findSymbol("glGetDriverControlStringQCOM");
    s_gles1.glEnableDriverControlQCOM = (glEnableDriverControlQCOM_t) s_gles1_lib->findSymbol("glEnableDriverControlQCOM");
    s_gles1.glDisableDriverControlQCOM = (glDisableDriverControlQCOM_t) s_gles1_lib->findSymbol("glDisableDriverControlQCOM");
    s_gles1.glExtGetTexturesQCOM = (glExtGetTexturesQCOM_t) s_gles1_lib->findSymbol("glExtGetTexturesQCOM");
    s_gles1.glExtGetBuffersQCOM = (glExtGetBuffersQCOM_t) s_gles1_lib->findSymbol("glExtGetBuffersQCOM");
    s_gles1.glExtGetRenderbuffersQCOM = (glExtGetRenderbuffersQCOM_t) s_gles1_lib->findSymbol("glExtGetRenderbuffersQCOM");
    s_gles1.glExtGetFramebuffersQCOM = (glExtGetFramebuffersQCOM_t) s_gles1_lib->findSymbol("glExtGetFramebuffersQCOM");
    s_gles1.glExtGetTexLevelParameterivQCOM = (glExtGetTexLevelParameterivQCOM_t) s_gles1_lib->findSymbol("glExtGetTexLevelParameterivQCOM");
    s_gles1.glExtTexObjectStateOverrideiQCOM = (glExtTexObjectStateOverrideiQCOM_t) s_gles1_lib->findSymbol("glExtTexObjectStateOverrideiQCOM");
    s_gles1.glExtGetTexSubImageQCOM = (glExtGetTexSubImageQCOM_t) s_gles1_lib->findSymbol("glExtGetTexSubImageQCOM");
    s_gles1.glExtGetBufferPointervQCOM = (glExtGetBufferPointervQCOM_t) s_gles1_lib->findSymbol("glExtGetBufferPointervQCOM");
    s_gles1.glExtGetShadersQCOM = (glExtGetShadersQCOM_t) s_gles1_lib->findSymbol("glExtGetShadersQCOM");
    s_gles1.glExtGetProgramsQCOM = (glExtGetProgramsQCOM_t) s_gles1_lib->findSymbol("glExtGetProgramsQCOM");
    s_gles1.glExtIsProgramBinaryQCOM = (glExtIsProgramBinaryQCOM_t) s_gles1_lib->findSymbol("glExtIsProgramBinaryQCOM");
    s_gles1.glExtGetProgramBinarySourceQCOM = (glExtGetProgramBinarySourceQCOM_t) s_gles1_lib->findSymbol("glExtGetProgramBinarySourceQCOM");
    s_gles1.glStartTilingQCOM = (glStartTilingQCOM_t) s_gles1_lib->findSymbol("glStartTilingQCOM");
    s_gles1.glEndTilingQCOM = (glEndTilingQCOM_t) s_gles1_lib->findSymbol("glEndTilingQCOM");

    return true;
}

//
// This function is called only during initialiation before
// any thread has been created - hence it should NOT be thread safe.
//
void *gles1_dispatch_get_proc_func(const char *name, void *userData)
{
    if (!s_gles1_lib) {
        return NULL;
    }
    return (void *)s_gles1_lib->findSymbol(name);
}
