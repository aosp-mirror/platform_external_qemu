# Copyright (c) 2018 Google Inc.
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

from .codegen import *
from .vulkantypes import *

# Contains definitions for various Vulkan API wrappers.
# These work with generators and have gen* triggers.
# They tend to contain VulkanAPIWrapper objects to make it easier
# to generate the code.

# Base class
class VulkanWrapperGenerator(object):
    def __init__(self, module, typeInfo):
        self.module = module
        self.typeInfo = typeInfo

    def onGenType(self, typeInfo, name, alias):
        pass
    def onGenStruct(self, typeInfo, typeName, alias):
        pass
    def onGenGroup(self, groupinfo, groupName, alias = None):
        pass
    def onGenEnum(self, enuminfo, name, alias):
        pass
    def onGenCmd(self, cmdinfo, name, alias):
        pass

# Global information shared between wrapper generators

API_PREFIX_MARSHAL = "marshal_"
API_PREFIX_ENCODE = "encode_"
API_PREFIX_VALIDATE = "validate_"
API_PREFIX_FRONTEND = "goldfish_frontend_"

VULKAN_STREAM_TYPE = "VulkanStream"
VULKAN_STREAM_VAR_NAME = "vkStream"

VALIDATE_RESULT_TYPE = "VkResult"
VALIDATE_VAR_NAME = "validateResult"
VALIDATE_GOOD_RESULT = "VK_SUCCESS"

PARAMETERS_MARSHAL = [makeVulkanTypeSimple(False, VULKAN_STREAM_TYPE, 1, VULKAN_STREAM_VAR_NAME)]
PARAMETERS_ENCODE = [makeVulkanTypeSimple(False, VULKAN_STREAM_TYPE, 1, VULKAN_STREAM_VAR_NAME)]
PARAMETERS_VALIDATE = [makeVulkanTypeSimple(False, VALIDATE_RESULT_TYPE, 1, VALIDATE_VAR_NAME)]

VOID_TYPE = makeVulkanTypeSimple(False, "void", 0)
STREAM_RET_TYPE = makeVulkanTypeSimple(False, "void", 0)

MARSHAL_INPUT_VAR_NAME = "forMarshaling"

# General function to iterate over a vulkan type and generate code that processes
# each of its sub-components, if any.
def iterateVulkanType(typeInfo, cgen, vulkanType, accessExprFunc, lengthExprFunc, compoundIter, simpleIter):
    if not vulkanType.isArrayOfStrings():
        if vulkanType.isPointerToConstPointer:
            cgen.line("// TODO: Handle %s (ptr2constptr)" % cgen.makeCTypeDecl(vulkanType))
            return

        if vulkanType.pointerIndirectionLevels > 1:
            cgen.line("// TODO: multiIndirection: handle %s" % cgen.makeCTypeDecl(vulkanType))
            return

    lenExpr = vulkanType.getLengthExpression();

    if typeInfo.isCompoundType(vulkanType.typeName):
        paramAccess = accessExprFunc(vulkanType)

        if lenExpr:
            lenAccess = lengthExprFunc(vulkanType)
            cgen.beginFor("uint32_t i = 0", "i < (uint32_t)%s" % lenAccess, "++i")
            compoundIter(cgen, vulkanType, paramAccess, loopVar = "i")
            cgen.endFor()
        else:
            compoundIter(cgen, vulkanType, paramAccess, loopVar = None)

    else:
        paramAccess = accessExprFunc(vulkanType)
        lenAccess = lengthExprFunc(vulkanType)
        simpleIter(cgen, vulkanType, paramAccess, lenAccess)

# Class for iterating Vulkan types with VulkanStream writing or reading
class VulkanStreamCodegen(object):
    def __init__(self, direction = "write"):
        self.direction = direction
        self.processString = "putString" if self.direction == "write" else "getString"
        self.processStringArray = "putStringArray" if self.direction == "write" else "getStringArray"
        self.processOther = "write" if self.direction == "write" else "read"

    def compoundIter(self, cgen, vulkanType, toProcessExpr, loopVar = None):
        if loopVar:
            finalExpr = "%s + i" % toProcessExpr
        else:
            finalExpr = toProcessExpr

        cgen.funcCall(None, API_PREFIX_MARSHAL + vulkanType.typeName, [VULKAN_STREAM_VAR_NAME, finalExpr])

    def simpleIter(self, cgen, vulkanType, toProcessExpr, lenExpr):
        if vulkanType.isString():
            cgen.stmt("%s->%s(%s)" % (VULKAN_STREAM_VAR_NAME, self.processString, toProcessExpr))
        elif vulkanType.isArrayOfStrings():
            cgen.stmt("%s->%s(%s, %s)" % (VULKAN_STREAM_VAR_NAME, self.processStringArray, toProcessExpr, lenExpr))
        elif lenExpr:
            finalLenExpr = "%s * %s" % (lenExpr, cgen.sizeofExpr(vulkanType))
            cgen.stmt("%s->%s(%s, %s)" % (VULKAN_STREAM_VAR_NAME, self.processOther, toProcessExpr, finalLenExpr))
        else:
            finalLenExpr = cgen.sizeofExpr(vulkanType)
            cgen.stmt("%s->%s(%s, %s)" % (VULKAN_STREAM_VAR_NAME, self.processOther, toProcessExpr, finalLenExpr))

    def processSimple(self, cgen, vulkanType):
        finalLenExpr = cgen.sizeofExpr(vulkanType)
        cgen.stmt("%s->%s(%s, %s)" % (VULKAN_STREAM_VAR_NAME, self.processOther, vulkanType.paramName, finalLenExpr))

# Marshaling
class VulkanMarshaling(VulkanWrapperGenerator):

    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.streamWriteCodegen = VulkanStreamCodegen("write")
        self.streamReadCodegen = VulkanStreamCodegen("read")

        def apiMarshalingDef(cgen, api):
            for param in api.parameters:

                if param.paramName == VULKAN_STREAM_VAR_NAME:
                    continue

                if param.possiblyOutput():
                    cgen.line("// TODO: Output: %s" % cgen.makeCTypeDecl(param))

                    continue

                iterateVulkanType(typeInfo,
                                  cgen,
                                  param,
                                  lambda t : cgen.accessParameterAsPtr(t),
                                  lambda t: cgen.makeLengthAccessFromApi(api, t),
                                  self.streamWriteCodegen.compoundIter,
                                  self.streamWriteCodegen.simpleIter)
                                  
            if api.retType.isVoidWithNoSize():
                pass
            else:
                result_var_name = "%s_%s_return" % (api.name, api.retType.typeName)
                cgen.stmt("%s %s" % (cgen.makeCTypeDecl(api.retType, useParamName = False), result_var_name))
                cgen.stmt("%s->read(&%s, %s)" % (VULKAN_STREAM_VAR_NAME, result_var_name, cgen.sizeofExpr(api.retType)))
                cgen.stmt("return %s" % result_var_name)

        self.marshalWrapper = \
            VulkanAPIWrapper(
                API_PREFIX_MARSHAL,
                PARAMETERS_MARSHAL,
                None,
                apiMarshalingDef)

        self.knownDefs = {}


    def onGenType(self, typeXml, name, alias):
        VulkanWrapperGenerator.onGenType(self, typeXml, name, alias)

        if name in self.knownDefs:
            return

        category = self.typeInfo.categoryOf(name)

        if category in ["struct", "union"] and not alias:

            prototypeParameters = \
                PARAMETERS_MARSHAL + \
                [makeVulkanTypeSimple(True, name, 1, MARSHAL_INPUT_VAR_NAME)]

            prototype = VulkanAPI(API_PREFIX_MARSHAL + name,
                                  STREAM_RET_TYPE,
                                  prototypeParameters)

            structInfo = self.typeInfo.structs[name]

            def structMarshalingDef(cgen):

                self.streamWriteCodegen.processSimple(cgen, 
                    makeVulkanTypeSimple(False, structInfo.name, 1, MARSHAL_INPUT_VAR_NAME))

                cgen.stmt("if (!%s) return" % MARSHAL_INPUT_VAR_NAME)

                for member in structInfo.members:

                    iterateVulkanType(self.typeInfo,
                                      cgen,
                                      member,
                                      lambda t : cgen.makeStructAccess(member, MARSHAL_INPUT_VAR_NAME, asPtr = True),
                                      lambda t : cgen.makeLengthAccessFromStruct(structInfo, member, MARSHAL_INPUT_VAR_NAME, asPtr = True),
                                      self.streamWriteCodegen.compoundIter,
                                      self.streamWriteCodegen.simpleIter)

            self.module.appendHeader(self.marshalWrapper.codegen.makeFuncDecl(prototype))
            self.module.appendImpl(self.marshalWrapper.codegen.makeFuncImpl(prototype, structMarshalingDef))

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)
        self.module.appendHeader(self.marshalWrapper.makeDecl(self.typeInfo, name))
        self.module.appendImpl(self.marshalWrapper.makeDefinition(self.typeInfo, name))

# Encoding
class VulkanEncoding(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        def encodeDefFunc(codegen, api):
            if api.retType.typeName != "void":
                codegen.stmt("return (%s)0" % api.retType.typeName)

        self.encodeWrapper = \
            VulkanAPIWrapper(
                API_PREFIX_ENCODE,
                PARAMETERS_ENCODE,
                None,
                encodeDefFunc)

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)
        self.module.appendHeader(self.encodeWrapper.makeDecl(self.typeInfo, name))
        self.module.appendImpl(self.encodeWrapper.makeDefinition(self.typeInfo, name))

# Frontend
class VulkanFrontend(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        def validateDefFunc(codegen, api):
            pass

        self.validateWrapper = \
            VulkanAPIWrapper(
                API_PREFIX_VALIDATE,
                PARAMETERS_VALIDATE,
                VOID_TYPE,
                validateDefFunc)

        def frontendDefFunc(codegen, api):
            retTypeName = api.retType.typeName

            codegen.stmt("%s %s = %s" % (VALIDATE_RESULT_TYPE, VALIDATE_VAR_NAME, VALIDATE_GOOD_RESULT))
            codegen.funcCall(None, API_PREFIX_VALIDATE + api.origName, ["&%s" % VALIDATE_VAR_NAME] + list(map(lambda p: p.paramName, api.parameters)))

            codegen.beginIf("%s != %s" % (VALIDATE_VAR_NAME, VALIDATE_GOOD_RESULT))
            if retTypeName == VALIDATE_RESULT_TYPE:
                codegen.stmt("return %s" % VALIDATE_VAR_NAME)
            elif retTypeName != "void":
                codegen.stmt("return (%s)0" % retTypeName)
            else:
                codegen.stmt("return")
            codegen.endIf()

            codegen.stmt("// VULKAN_STREAM_GET()")
            codegen.stmt("%s* %s = nullptr" % (VULKAN_STREAM_TYPE, VULKAN_STREAM_VAR_NAME))

            retLhs = None
            if retTypeName != 'void':
                retLhs = retTypeName + " res"

            codegen.funcCall(retLhs, API_PREFIX_ENCODE + api.origName, [VULKAN_STREAM_VAR_NAME] + list(map(lambda p: p.paramName, api.parameters)))

            if retTypeName != 'void':
                codegen.stmt("return res")

        self.frontendWrapper = \
            VulkanAPIWrapper(
                API_PREFIX_FRONTEND,
                [],
                None,
                frontendDefFunc)

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)
        self.module.appendHeader(self.frontendWrapper.makeDecl(self.typeInfo, name))
        self.module.appendImpl(self.validateWrapper.makeDefinition(self.typeInfo, name, isStatic = True))
        self.module.appendImpl(self.frontendWrapper.makeDefinition(self.typeInfo, name))
