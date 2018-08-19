#!/usr/bin/python3 -i
#
# Copyright (c) 2013-2018 The Khronos Group Inc.
# Copyright (c) 2013-2018 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os, re, sys
from generator import *

import cereal

from copy import deepcopy

# CerealGenerator - generates complete set of encoder and decoder source
# while being agnostic to the stream implementation

# ---- methods overriding base class ----
# beginFile(genOpts)
# endFile()
# beginFeature(interface, emit)
# endFeature()
# genType(typeinfo,name)
# genStruct(typeinfo,name)
# genGroup(groupinfo,name)
# genEnum(enuminfo, name)
# genCmd(cmdinfo)
class CerealGenerator(OutputGenerator):

    """Generate serialization code"""
    def __init__(self, errFile = sys.stderr,
                       warnFile = sys.stderr,
                       diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)

        self.typeInfo = cereal.VulkanTypeInfo()

        self.modules = {}
        self.moduleList = []

        self.wrappers = []

        self.codegen = cereal.CodeGen()

        self.cereal_Android_mk_header = """
LOCAL_PATH := $(call my-dir)

# For Vulkan libraries

cereal_C_INCLUDES := \\
    $(LOCAL_PATH) \\
    $(EMUGL_PATH)/host/include/vulkan \\

cereal_STATIC_LIBRARIES := \\
    android-emu \\
    android-emu-base \\
"""

        # Define our generated modules and wrappers here

        self.addModule("guest", "goldfish_vk_frontend")
        self.addModule("guest", "goldfish_vk_encoder")

        self.addModule("common", "goldfish_vk_marshaling")

        self.addWrapper(cereal.VulkanMarshaling(self.modules["goldfish_vk_marshaling"], self.typeInfo))

        self.cereal_Android_mk_body = """
$(call emugl-begin-static-library,lib$(BUILD_TARGET_SUFFIX)OpenglRender_vulkan_cereal_guest)

LOCAL_C_INCLUDES += $(cereal_C_INCLUDES)

LOCAL_STATIC_LIBRARIES += $(cereal_STATIC_LIBRARIES)

LOCAL_SRC_FILES := \\
"""
        def addSrcEntry(m):
            self.cereal_Android_mk_body += m.getMakefileSrcEntry()

        self.forEachModule(addSrcEntry)

        self.cereal_Android_mk_body += """
$(call emugl-end-module)
"""

        self.modules["goldfish_vk_frontend"].headerPreamble = """
#include <vulkan.h>
"""

        self.modules["goldfish_vk_frontend"].implPreamble = """
#include "guest/goldfish_vk_frontend.h"

#include "guest/goldfish_vk_encoder.h"
"""

        self.modules["goldfish_vk_encoder"].implPreamble = """
#include "guest/goldfish_vk_encoder.h"

#include "common/goldfish_vk_marshaling.h"
"""

    def addModule(self, directory, basename):
        self.moduleList.append(basename)
        self.modules[basename] = cereal.Module(directory, basename)

    def addWrapper(self, wrapper):
        self.wrappers.append(wrapper)

    def forEachModule(self, func):
        for moduleName in self.moduleList:
            func(self.modules[moduleName])

    def forEachWrapper(self, func):
        for wrapper in self.wrappers:
            func(wrapper)

## Marshaling definitions#######################################################

    def makeMarshalingProto(self, cmdInfoElem, name):
        marshalingApi = deepcopy(self.typeInfo.apis[name])
        marshalingApi.retType = cereal.makeVulkanTypeSimple(False, "void", 0)
        marshalingApi.parameters.insert(0, cereal.makeVulkanTypeSimple(False, "void", 1, "vkStream"))
        return self.codegen.makeFuncProto(marshalingApi)

    def makeMarshalingDef(self, cmdInfoElem, name):
        params = cmdInfoElem.findall('param')

        funcPrototype = \
            self.makeMarshalingProto(cmdInfoElem, name)

        self.codegen.swapCode();

        self.codegen.beginBlock();

        retType = cmdInfoElem.find('proto').find('type').text

        if retType != "void":
            self.codegen.stmt("return (%s)0" % retType)

        self.codegen.endBlock();
        return funcPrototype + self.codegen.swapCode()

    def makeMarshalingCall(self, cmdInfoElem, name):
        params = cmdInfoElem.findall('param')

        args = ["vkStream"]
        for p in params:
            ptype = p.find('type')
            pname = p.find('name')
            args.append(pname.text)

        callName = "encode_" + name

        return callName + "(" + ", ".join(args) + ")"

## Encoder definitions##########################################################

    def makeEncodeProto(self, cmdInfoElem, name):
        encodeApi = deepcopy(self.typeInfo.apis[name])
        encodeApi.name = "encode_" + encodeApi.name
        encodeApi.parameters.insert(0, cereal.makeVulkanTypeSimple(False, "void", 1, "vkStream"))
        return self.codegen.makeFuncProto(encodeApi)

    def makeEncoderDef(self, cmdInfoElem, name):
        params = cmdInfoElem.findall('param')

        funcPrototype = \
            self.makeEncodeProto(cmdInfoElem, name)

        self.codegen.swapCode();

        self.codegen.beginBlock();

        retType = cmdInfoElem.find('proto').find('type').text

        if retType != "void":
            self.codegen.stmt("return (%s)0" % retType)

        self.codegen.endBlock();
        return funcPrototype + self.codegen.swapCode()

    def makeEncoderCall(self, cmdInfoElem, name):
        params = cmdInfoElem.findall('param')

        args = ["vkStream"]
        for p in params:
            ptype = p.find('type')
            pname = p.find('name')
            args.append(pname.text)

        callName = "encode_" + name

        return callName + "(" + ", ".join(args) + ")"


## Validate definitions#########################################################

    def makeValidateProto(self, cmdInfoElem, name):
        validateApi = deepcopy(self.typeInfo.apis[name])
        validateApi.name = "validate_" + validateApi.name
        validateApi.retType = cereal.makeVulkanTypeSimple(False, "void", 0)
        validateApi.parameters.insert(0, cereal.makeVulkanTypeSimple(False, "VkResult", 1, "validateResult"))
        return self.codegen.makeFuncProto(validateApi)

    def makeValidateDef(self, cmdInfoElem, name):
        params = cmdInfoElem.findall('param')

        funcPrototype = \
            "static " + \
            self.makeValidateProto(cmdInfoElem, name)

        self.codegen.swapCode();

        self.codegen.beginBlock();

        self.codegen.endBlock();
        return funcPrototype + self.codegen.swapCode()

    def makeValidateCall(self, cmdInfoElem, name):
        params = cmdInfoElem.findall('param')

        args = ["&validateResult"]
        for p in params:
            ptype = p.find('type')
            pname = p.find('name')
            args.append(pname.text)

        callName = "validate_" + name

        return callName + "(" + ", ".join(args) + ")"

## Frontend definitions#########################################################

    def makeFrontendProto(self, cmdInfoElem, name):
        frontendApi = deepcopy(self.typeInfo.apis[name])
        frontendApi.name = "goldfish_frontend_" + frontendApi.name
        return self.codegen.makeFuncProto(frontendApi)

    def makeFrontendDef(self, cmdInfoElem, name):

        prototype = self.makeFrontendProto(cmdInfoElem, name)

        self.codegen.swapCode()

        self.codegen.beginBlock()

        self.codegen.stmt("VkResult validateResult = VK_SUCCESS")

        retType = cmdInfoElem.find('proto').find('type').text

        self.codegen.stmt(self.makeValidateCall(cmdInfoElem, name))

        self.codegen.beginIf("validateResult != VK_SUCCESS")
        if retType == 'VkResult':
            self.codegen.stmt("return validateResult")
        elif retType != "void":
            self.codegen.stmt("return (%s)0" % retType)
        else:
            self.codegen.stmt("return")
        self.codegen.endIf()

        self.codegen.stmt("// VULKAN_STREAM_GET()")
        self.codegen.stmt("void* vkStream = 0")

        if retType != 'void':
            self.codegen.stmt(retType + " res = " + self.makeEncoderCall(cmdInfoElem, name))
            self.codegen.stmt("return res")
        else:
            self.codegen.stmt(self.makeEncoderCall(cmdInfoElem, name))

        self.codegen.endBlock()

        implBody = self.codegen.swapCode()
        res = prototype + implBody

        return res

## Overrides################3###################################################

    def beginFile(self, genOpts):
        OutputGenerator.beginFile(self, genOpts)

        write(self.cereal_Android_mk_header, file = self.outFile)
        write(self.cereal_Android_mk_body, file = self.outFile)

        self.forEachModule(lambda m: m.begin(self.genOpts.directory))

    def endFile(self):
        OutputGenerator.endFile(self)

        self.forEachModule(lambda m: m.end())

    def beginFeature(self, interface, emit):
        # Start processing in superclass
        OutputGenerator.beginFeature(self, interface, emit)

        self.forEachModule(lambda m: m.appendHeader("#ifdef %s\n" % self.featureName))
        self.forEachModule(lambda m: m.appendImpl("#ifdef %s\n" % self.featureName))

    def endFeature(self):
        # Finish processing in superclass
        OutputGenerator.endFeature(self)

        self.forEachModule(lambda m: m.appendHeader("#endif\n"))
        self.forEachModule(lambda m: m.appendImpl("#endif\n"))

    def genType(self, typeinfo, name, alias):
        OutputGenerator.genType(self, typeinfo, name, alias)
        self.typeInfo.onGenType(typeinfo, name, alias)

    def genStruct(self, typeinfo, typeName, alias):
        OutputGenerator.genStruct(self, typeinfo, typeName, alias)
        self.typeInfo.onGenStruct(typeinfo, typeName, alias)

    def genGroup(self, groupinfo, groupName, alias = None):
        OutputGenerator.genGroup(self, groupinfo, groupName, alias)
        self.typeInfo.onGenGroup(groupinfo, groupName, alias)

    def genEnum(self, enuminfo, name, alias):
        OutputGenerator.genEnum(self, enuminfo, name, alias)
        self.typeInfo.onGenEnum(enuminfo, name, alias)

    def genCmd(self, cmdinfo, name, alias):
        OutputGenerator.genCmd(self, cmdinfo, name, alias)
        self.typeInfo.onGenCmd(cmdinfo, name, alias)

        frontendDecl = self.makeFrontendProto(cmdinfo.elem, name) + ";\n"

        validateDef = self.makeValidateDef(cmdinfo.elem, name)
        frontendDef = self.makeFrontendDef(cmdinfo.elem, name)

        self.modules["goldfish_vk_frontend"].appendHeader(frontendDecl)
        self.modules["goldfish_vk_frontend"].appendImpl(validateDef)
        self.modules["goldfish_vk_frontend"].appendImpl(frontendDef)

        encoderDecl = self.makeEncodeProto(cmdinfo.elem, name) + ";\n"
        encoderDef = self.makeEncoderDef(cmdinfo.elem, name)

        self.modules["goldfish_vk_encoder"].appendHeader(encoderDecl)
        self.modules["goldfish_vk_encoder"].appendImpl(encoderDef)

        self.forEachWrapper(lambda w: w.onGenCmd(cmdinfo, name, alias))

        # marshalingDecl = self.makeMarshalingProto(cmdinfo.elem, name) + ";\n"
        # marshalingDef = self.makeMarshalingDef(cmdinfo.elem, name) + ";\n"

        # self.modules["goldfish_vk_marshaling"].appendHeader(marshalingDecl)
        # self.modules["goldfish_vk_marshaling"].appendImpl(marshalingDef)

