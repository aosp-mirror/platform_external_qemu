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
API_PREFIX_UNMARSHAL = "unmarshal_"

API_PREFIX_COMPARE = "compare_"
COMPARE_VAR_NAMES = ["a", "b"]
COMPARE_RET_TYPE = makeVulkanTypeSimple(False, "void", 0)

MARSHAL_INPUT_VAR_NAME = "forMarshaling"
UNMARSHAL_INPUT_VAR_NAME = "forUnmarshaling"

API_PREFIX_ENCODE = "encode_"
API_PREFIX_VALIDATE = "validate_"
API_PREFIX_FRONTEND = "goldfish_frontend_"

VULKAN_STREAM_TYPE = "VulkanStream"
VULKAN_STREAM_VAR_NAME = "vkStream"

VALIDATE_RESULT_TYPE = "VkResult"
VALIDATE_VAR_NAME = "validateResult"
VALIDATE_GOOD_RESULT = "VK_SUCCESS"

PARAMETERS_MARSHALING = [makeVulkanTypeSimple(False, VULKAN_STREAM_TYPE, 1, VULKAN_STREAM_VAR_NAME)]
PARAMETERS_ENCODE = [makeVulkanTypeSimple(False, VULKAN_STREAM_TYPE, 1, VULKAN_STREAM_VAR_NAME)]
PARAMETERS_VALIDATE = [makeVulkanTypeSimple(False, VALIDATE_RESULT_TYPE, 1, VALIDATE_VAR_NAME)]

VOID_TYPE = makeVulkanTypeSimple(False, "void", 0)
STREAM_RET_TYPE = makeVulkanTypeSimple(False, "void", 0)

# General function to iterate over a vulkan type and generate code that processes
# each of its sub-components, if any.
def iterateVulkanType(typeInfo, vulkanType, forEachType):
    if not vulkanType.isArrayOfStrings():
        if vulkanType.isPointerToConstPointer:
            # cgen.line("// TODO: Handle %s (ptr2constptr)" % cgen.makeCTypeDecl(vulkanType))
            return

    needCheck = vulkanType.isOptional and vulkanType.pointerIndirectionLevels > 0

    if typeInfo.isCompoundType(vulkanType.typeName):

        if needCheck:
            forEachType.onCheck(vulkanType)

        forEachType.onCompoundType(vulkanType)

        if needCheck:
            forEachType.endCheck(vulkanType)

    else:

        if vulkanType.isString():

            forEachType.onString(vulkanType)

        elif vulkanType.isArrayOfStrings():

            forEachType.onStringArray(vulkanType)

        elif vulkanType.staticArrExpr:

            forEachType.onStaticArr(vulkanType)

        elif vulkanType.pointerIndirectionLevels > 0:
            if needCheck:
                forEachType.onCheck(vulkanType)
            forEachType.onPointer(vulkanType)
            if needCheck:
                forEachType.endCheck(vulkanType)
        else:

            forEachType.onValue(vulkanType)

class VulkanMarshalingCodegen(object):
    def __init__(self, cgen, streamVarName, inputVarName, marshalPrefix, direction = "write", forApiOutput = False):
        self.cgen = cgen
        self.direction = direction
        self.processSimple = "write" if self.direction == "write" else "read"
        self.forApiOutput = forApiOutput

        self.checked = False

        self.streamVarName = streamVarName
        self.inputVarName = inputVarName
        self.marshalPrefix = marshalPrefix

        self.exprAccessor = lambda t: self.cgen.generalAccessAsPtr(t, parentVarName = self.inputVarName)
        self.lenAccessor = lambda t: self.cgen.generalLengthAccess(t, parentVarName = self.inputVarName)

    def getTypeForStreaming(self, vulkanType):
        res = deepcopy(vulkanType)

        if not vulkanType.accessibleAsPointer():
            res = res.getForAddressAccess()

        if vulkanType.staticArrExpr:
            res = res.getForAddressAccess()

        if self.direction == "write":
            return res
        else:
            return res.getForNonConstAccess()

    def makeCastExpr(self, vulkanType):
        return "(%s)" % (self.cgen.makeCTypeDecl(vulkanType, useParamName = False))

    def genStreamCall(self, vulkanType, toStreamExpr, sizeExpr):
        varname = self.streamVarName
        func = self.processSimple
        cast = self.makeCastExpr(self.getTypeForStreaming(vulkanType))

        self.cgen.stmt("%s->%s(%s%s, %s)" % (varname, func, cast, toStreamExpr, sizeExpr))

    def onCheck(self, vulkanType):

        if self.forApiOutput:
            return

        self.checked = True

        access = self.exprAccessor(vulkanType)
        addrExpr = "&" + access
        sizeExpr = self.cgen.sizeofExpr(vulkanType)
        self.genStreamCall(vulkanType.getForAddressAccess(), addrExpr, sizeExpr)

        skipStreamInternal = vulkanType.typeName == "void"

        if skipStreamInternal:
            return

        self.cgen.beginIf(access)

    def endCheck(self, vulkanType):

        skipStreamInternal = vulkanType.typeName == "void"
        if skipStreamInternal:
            return

        if self.checked:
            self.cgen.endIf()
            self.checked = False

    def onCompoundType(self, vulkanType):

        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

        if lenAccess is not None:
            loopVar = "i"
            access = "%s + %s" % (access, loopVar)
            forInit = "uint32_t %s = 0" % loopVar
            forCond = "%s < (uint32_t)%s" % (loopVar, lenAccess)
            forIncr = "++%s" % loopVar
            self.cgen.beginFor(forInit, forCond, forIncr)

        accessWithCast = "%s(%s)" % (self.makeCastExpr(self.getTypeForStreaming(vulkanType)), access)

        self.cgen.funcCall(None, self.marshalPrefix + vulkanType.typeName, [self.streamVarName, accessWithCast])

        if lenAccess is not None:
            self.cgen.endFor()

    def onString(self, vulkanType):

        access = self.exprAccessor(vulkanType)

        if self.direction == "write":
            self.cgen.stmt("%s->putString(%s)" % (self.streamVarName, access))
        else:
            self.cgen.stmt("%s->getString()" % (self.streamVarName))
        
    def onStringArray(self, vulkanType):

        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

        if self.direction == "write":
            self.cgen.stmt("%s->putStringArray(%s, %s)" % (self.streamVarName, access, lenAccess))
        else:
            self.cgen.stmt("%s->getStringArray()" % (self.streamVarName))

    def onStaticArr(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)
        finalLenExpr = "%s * %s" % (lenAccess, self.cgen.sizeofExpr(vulkanType))
        self.genStreamCall(vulkanType, access, finalLenExpr)

    def onPointer(self, vulkanType):
        skipStreamInternal = vulkanType.typeName == "void"
        if skipStreamInternal:
            return

        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

        if lenAccess is not None:
            finalLenExpr = "%s * %s" % (lenAccess, self.cgen.sizeofExpr(vulkanType))
        else:
            finalLenExpr = self.cgen.sizeofExpr(vulkanType)

        self.genStreamCall(vulkanType, access, finalLenExpr)

    def onValue(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        self.genStreamCall(vulkanType, access, self.cgen.sizeofExpr(vulkanType))

# Marshaling
class VulkanMarshaling(VulkanWrapperGenerator):

    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.writeCodegen = \
            VulkanMarshalingCodegen(
                None,
                VULKAN_STREAM_VAR_NAME,
                MARSHAL_INPUT_VAR_NAME,
                API_PREFIX_MARSHAL,
                direction = "write")

        self.apiOutputCodegenForRead = \
            VulkanMarshalingCodegen(
                None,
                VULKAN_STREAM_VAR_NAME,
                MARSHAL_INPUT_VAR_NAME,
                API_PREFIX_MARSHAL,
                direction = "read",
                forApiOutput = True)

        self.readCodegen = \
            VulkanMarshalingCodegen(
                None,
                VULKAN_STREAM_VAR_NAME,
                UNMARSHAL_INPUT_VAR_NAME,
                API_PREFIX_UNMARSHAL,
                direction = "read")

        self.apiOutputCodegenForWrite = \
            VulkanMarshalingCodegen(
                None,
                VULKAN_STREAM_VAR_NAME,
                UNMARSHAL_INPUT_VAR_NAME,
                API_PREFIX_UNMARSHAL,
                direction = "write",
                forApiOutput = True)

        def apiMarshalingDef(cgen, api):
            self.apiOutputCodegenForRead.cgen = cgen
            self.writeCodegen.cgen = cgen

            for param in api.parameters:
                if param.paramName == VULKAN_STREAM_VAR_NAME:
                    continue
                if param.possiblyOutput():
                    iterateVulkanType(typeInfo, param, self.apiOutputCodegenForRead)
                else:
                    iterateVulkanType(typeInfo, param, self.writeCodegen)
                                  
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
                PARAMETERS_MARSHALING,
                None,
                apiMarshalingDef)

        def apiUnmarshalingDef(cgen, api):
            self.apiOutputCodegenForWrite.cgen = cgen
            self.readCodegen.cgen = cgen

            for param in api.parameters:
                if param.paramName == VULKAN_STREAM_VAR_NAME:
                    continue
                if param.possiblyOutput():
                    iterateVulkanType(typeInfo, param, self.apiOutputCodegenForWrite)
                else:
                    iterateVulkanType(typeInfo, param, self.readCodegen)
                                  
            if api.retType.isVoidWithNoSize():
                pass
            else:
                result_var_name = "%s_%s_return" % (api.name, api.retType.typeName)
                cgen.stmt("%s %s" % (cgen.makeCTypeDecl(api.retType, useParamName = False), result_var_name))
                cgen.stmt("%s->write(&%s, %s)" % (VULKAN_STREAM_VAR_NAME, result_var_name, cgen.sizeofExpr(api.retType)))
                cgen.stmt("return %s" % result_var_name)

        self.unmarshalWrapper = \
            VulkanAPIWrapper(
                API_PREFIX_UNMARSHAL,
                PARAMETERS_MARSHALING,
                None,
                apiUnmarshalingDef)

        self.knownDefs = {}


    def onGenType(self, typeXml, name, alias):
        VulkanWrapperGenerator.onGenType(self, typeXml, name, alias)

        if name in self.knownDefs:
            return

        category = self.typeInfo.categoryOf(name)

        if category in ["struct", "union"] and not alias:

            structInfo = self.typeInfo.structs[name]

            marshalPrototype = \
                VulkanAPI(API_PREFIX_MARSHAL + name,
                          STREAM_RET_TYPE,
                          PARAMETERS_MARSHALING + [makeVulkanTypeSimple(True, name, 1, MARSHAL_INPUT_VAR_NAME)])

            def structMarshalingDef(cgen):
                self.writeCodegen.cgen = cgen
                for member in structInfo.members:
                    iterateVulkanType(self.typeInfo, member, self.writeCodegen)

            self.module.appendHeader(self.marshalWrapper.codegen.makeFuncDecl(marshalPrototype))
            self.module.appendImpl(self.marshalWrapper.codegen.makeFuncImpl(marshalPrototype, structMarshalingDef))

            unmarshalPrototype = \
                VulkanAPI(API_PREFIX_UNMARSHAL + name,
                          STREAM_RET_TYPE,
                          PARAMETERS_MARSHALING + [makeVulkanTypeSimple(False, name, 1, UNMARSHAL_INPUT_VAR_NAME)])

            def structUnmarshalingDef(cgen):
                self.readCodegen.cgen = cgen
                for member in structInfo.members:
                    iterateVulkanType(self.typeInfo, member, self.readCodegen)

            self.module.appendHeader(self.unmarshalWrapper.codegen.makeFuncDecl(unmarshalPrototype))
            self.module.appendImpl(self.unmarshalWrapper.codegen.makeFuncImpl(unmarshalPrototype, structUnmarshalingDef))


    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)
        self.module.appendHeader(self.marshalWrapper.makeDecl(self.typeInfo, name))
        self.module.appendImpl(self.marshalWrapper.makeDefinition(self.typeInfo, name))
        self.module.appendHeader(self.unmarshalWrapper.makeDecl(self.typeInfo, name))
        self.module.appendImpl(self.unmarshalWrapper.makeDefinition(self.typeInfo, name))

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
