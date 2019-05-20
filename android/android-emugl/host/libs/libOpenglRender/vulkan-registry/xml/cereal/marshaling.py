# Copyright (c) 2018 The Android Open Source Project
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

from copy import copy

from .common.codegen import CodeGen, VulkanAPIWrapper
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType, VulkanTypeIterator

from .wrapperdefs import VulkanWrapperGenerator
from .wrapperdefs import VULKAN_STREAM_VAR_NAME
from .wrapperdefs import STREAM_RET_TYPE
from .wrapperdefs import MARSHAL_INPUT_VAR_NAME
from .wrapperdefs import UNMARSHAL_INPUT_VAR_NAME
from .wrapperdefs import PARAMETERS_MARSHALING
from .wrapperdefs import PARAMETERS_MARSHALING_GUEST
from .wrapperdefs import STRUCT_EXTENSION_PARAM, STRUCT_EXTENSION_PARAM_FOR_WRITE, EXTENSION_SIZE_API_NAME
from .wrapperdefs import API_PREFIX_MARSHAL
from .wrapperdefs import API_PREFIX_UNMARSHAL

class VulkanMarshalingCodegen(VulkanTypeIterator):

    def __init__(self,
                 cgen,
                 streamVarName,
                 inputVarName,
                 marshalPrefix,
                 direction = "write",
                 forApiOutput = False,
                 dynAlloc = False,
                 mapHandles = True):
        self.cgen = cgen
        self.direction = direction
        self.processSimple = "write" if self.direction == "write" else "read"
        self.forApiOutput = forApiOutput

        self.checked = False

        self.streamVarName = streamVarName
        self.inputVarName = inputVarName
        self.marshalPrefix = marshalPrefix

        self.exprAccessor = lambda t: self.cgen.generalAccess(t, parentVarName = self.inputVarName, asPtr = True)
        self.exprPrimitiveValueAccessor = lambda t: self.cgen.generalAccess(t, parentVarName = self.inputVarName, asPtr = False)
        self.lenAccessor = lambda t: self.cgen.generalLengthAccess(t, parentVarName = self.inputVarName)

        self.dynAlloc = dynAlloc
        self.mapHandles = mapHandles

    def getTypeForStreaming(self, vulkanType):
        res = copy(vulkanType)

        if not vulkanType.accessibleAsPointer():
            res = res.getForAddressAccess()

        if vulkanType.staticArrExpr:
            res = res.getForAddressAccess()

        if self.direction == "write":
            return res
        else:
            return res.getForNonConstAccess()

    def makeCastExpr(self, vulkanType):
        return "(%s)" % (
            self.cgen.makeCTypeDecl(vulkanType, useParamName=False))

    def genStreamCall(self, vulkanType, toStreamExpr, sizeExpr):
        varname = self.streamVarName
        func = self.processSimple
        cast = self.makeCastExpr(self.getTypeForStreaming(vulkanType))

        self.cgen.stmt(
            "%s->%s(%s%s, %s)" % (varname, func, cast, toStreamExpr, sizeExpr))

    def genPrimitiveStreamCall(self, vulkanType, access):
        varname = self.streamVarName

        self.cgen.streamPrimitive(
            self.typeInfo,
            varname,
            access,
            vulkanType,
            direction=self.direction)

    def genHandleMappingCall(self, vulkanType, access, lenAccess):
        if lenAccess is None:
            lenAccess = "1"
            handle64Bytes = "8"
        else:
            handle64Bytes = "%s * 8" % lenAccess

        handle64Var = self.cgen.var()
        if lenAccess != "1":
            self.cgen.beginIf(lenAccess)
            self.cgen.stmt("uint64_t* %s" % handle64Var)
            self.cgen.stmt(
                "%s->alloc((void**)&%s, %s * 8)" % \
                (self.streamVarName, handle64Var, lenAccess))
            handle64VarAccess = handle64Var
            handle64VarType = \
                makeVulkanTypeSimple(False, "uint64_t", 1, paramName=handle64Var)
        else:
            self.cgen.stmt("uint64_t %s" % handle64Var)
            handle64VarAccess = "&%s" % handle64Var
            handle64VarType = \
                makeVulkanTypeSimple(False, "uint64_t", 0, paramName=handle64Var)

        if self.direction == "write":
            self.cgen.stmt(
                "%s->handleMapping()->mapHandles_%s_u64(%s, %s, %s)" %
                (self.streamVarName, vulkanType.typeName,
                access,
                handle64VarAccess, lenAccess))
            self.genStreamCall(handle64VarType, handle64VarAccess, handle64Bytes)
        else:
            self.genStreamCall(handle64VarType, handle64VarAccess, handle64Bytes)
            self.cgen.stmt(
                "%s->handleMapping()->mapHandles_u64_%s(%s, %s%s, %s)" %
                (self.streamVarName, vulkanType.typeName,
                handle64VarAccess,
                self.makeCastExpr(vulkanType.getForNonConstAccess()), access,
                lenAccess))

        if lenAccess != "1":
            self.cgen.endIf()

    def doAllocSpace(self, vulkanType):
        if self.dynAlloc and self.direction == "read":
            access = self.exprAccessor(vulkanType)
            lenAccess = self.lenAccessor(vulkanType)
            sizeof = self.cgen.sizeofExpr( \
                         vulkanType.getForValueAccess())
            if lenAccess:
                bytesExpr = "%s * %s" % (lenAccess, sizeof)
            else:
                bytesExpr = sizeof

            self.cgen.stmt( \
                "%s->alloc((void**)&%s, %s)" %
                    (self.streamVarName,
                     access, bytesExpr))

    def onCheck(self, vulkanType):

        if self.forApiOutput:
            return

        self.checked = True

        access = self.exprAccessor(vulkanType)

        needConsistencyCheck = False

        self.cgen.line("// WARNING PTR CHECK")
        if (self.dynAlloc and self.direction == "read") or self.direction == "write":
            checkAccess = self.exprAccessor(vulkanType)
            addrExpr = "&" + checkAccess
            sizeExpr = self.cgen.sizeofExpr(vulkanType)
        else:
            checkName = "check_%s" % vulkanType.paramName
            self.cgen.stmt("%s %s" % (
                self.cgen.makeCTypeDecl(vulkanType, useParamName = False), checkName))
            checkAccess = checkName
            addrExpr = "&" + checkAccess
            sizeExpr = self.cgen.sizeofExpr(vulkanType)
            needConsistencyCheck = True

        self.genPrimitiveStreamCall(
            vulkanType,
            checkAccess)

        self.cgen.beginIf(access)

        if needConsistencyCheck:
            self.cgen.beginIf("!(%s)" % checkName)
            self.cgen.stmt(
                "fprintf(stderr, \"fatal: %s inconsistent between guest and host\\n\")" % (access))
            self.cgen.endIf()

    def endCheck(self, vulkanType):

        if self.checked:
            self.cgen.endIf()
            self.checked = False

    def onCompoundType(self, vulkanType):

        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

        if vulkanType.pointerIndirectionLevels > 0:
            self.doAllocSpace(vulkanType)

        if lenAccess is not None:
            loopVar = "i"
            access = "%s + %s" % (access, loopVar)
            forInit = "uint32_t %s = 0" % loopVar
            forCond = "%s < (uint32_t)%s" % (loopVar, lenAccess)
            forIncr = "++%s" % loopVar
            self.cgen.beginFor(forInit, forCond, forIncr)

        accessWithCast = "%s(%s)" % (self.makeCastExpr(
            self.getTypeForStreaming(vulkanType)), access)

        self.cgen.funcCall(None, self.marshalPrefix + vulkanType.typeName,
                           [self.streamVarName, accessWithCast])

        if lenAccess is not None:
            self.cgen.endFor()

    def onString(self, vulkanType):

        access = self.exprAccessor(vulkanType)

        if self.direction == "write":
            self.cgen.stmt("%s->putString(%s)" % (self.streamVarName, access))
        else:
            castExpr = \
                self.makeCastExpr( \
                    self.getTypeForStreaming( \
                        vulkanType.getForAddressAccess()))

            self.cgen.stmt( \
                "%s->loadStringInPlace(%s&%s)" % (self.streamVarName, castExpr, access))

    def onStringArray(self, vulkanType):

        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

        if self.direction == "write":
            self.cgen.stmt("saveStringArray(%s, %s, %s)" % (self.streamVarName,
                                                            access, lenAccess))
        else:
            castExpr = \
                self.makeCastExpr( \
                    self.getTypeForStreaming( \
                        vulkanType.getForAddressAccess()))

            self.cgen.stmt("%s->loadStringArrayInPlace(%s&%s)" % (self.streamVarName, castExpr, access))

    def onStaticArr(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)
        finalLenExpr = "%s * %s" % (lenAccess, self.cgen.sizeofExpr(vulkanType))
        self.genStreamCall(vulkanType, access, finalLenExpr)

    def onStructExtension(self, vulkanType):
        access = self.exprAccessor(vulkanType)

        sizeVar = "%s_size" % vulkanType.paramName

        if self.direction == "write":
            self.cgen.stmt("size_t %s = %s(%s)" % (sizeVar, EXTENSION_SIZE_API_NAME, access))
            self.cgen.stmt("%s->putBe32(%s)" % \
                (self.streamVarName, sizeVar))
        else:
            self.cgen.stmt("size_t %s" % sizeVar)
            self.cgen.stmt("%s = %s->getBe32()" % \
                (sizeVar, self.streamVarName))

        if self.direction == "read" and self.dynAlloc:
            self.cgen.stmt("%s = nullptr" % access)

        self.cgen.beginIf(sizeVar)

        if self.direction == "read":
            if self.dynAlloc:
                self.cgen.stmt( \
                    "%s->alloc((void**)&%s, %s_size)" %
                    (self.streamVarName, access, vulkanType.paramName))
            castedAccessExpr = "(%s)(%s)" % ("void*", access)
        else:
            castedAccessExpr = access

        if self.dynAlloc or self.direction == "write":
            self.genStreamCall(vulkanType, access, "sizeof(VkStructureType)")
        else:
            self.cgen.stmt("uint64_t pNext_placeholder")
            placeholderAccess = "(&pNext_placeholder)"
            self.genStreamCall(vulkanType, placeholderAccess, "sizeof(VkStructureType)")


        self.cgen.funcCall(None, self.marshalPrefix + "extension_struct",
                           [self.streamVarName, castedAccessExpr])

        self.cgen.endIf()

    def onPointer(self, vulkanType):
        access = self.exprAccessor(vulkanType)

        lenAccess = self.lenAccessor(vulkanType)

        self.doAllocSpace(vulkanType)

        if vulkanType.isHandleType() and self.mapHandles:
            self.genHandleMappingCall(vulkanType, access, lenAccess)
        else:
            if self.typeInfo.isNonAbiPortableType(vulkanType.typeName):
                if lenAccess is not None:
                    self.cgen.beginFor("uint32_t i = 0", "i < (uint32_t)%s" % lenAccess, "++i")
                    self.genPrimitiveStreamCall(vulkanType.getForValueAccess(), "%s[i]" % access)
                    self.cgen.endFor()
                else:
                    self.genPrimitiveStreamCall(vulkanType.getForValueAccess(), "(*%s)" % access)
            else:
                if lenAccess is not None:
                    finalLenExpr = "%s * %s" % (
                        lenAccess, self.cgen.sizeofExpr(vulkanType.getForValueAccess()))
                else:
                    finalLenExpr = "%s" % (
                        self.cgen.sizeofExpr(vulkanType.getForValueAccess()))
                self.genStreamCall(vulkanType, access, finalLenExpr)

    def onValue(self, vulkanType):
        if vulkanType.isHandleType() and self.mapHandles:
            access = self.exprAccessor(vulkanType)
            self.genHandleMappingCall(
                vulkanType.getForAddressAccess(), access, "1")
        elif self.typeInfo.isNonAbiPortableType(vulkanType.typeName):
            access = self.exprPrimitiveValueAccessor(vulkanType)
            self.genPrimitiveStreamCall(vulkanType, access)
        else:
            access = self.exprAccessor(vulkanType)
            self.genStreamCall(vulkanType, access, self.cgen.sizeofExpr(vulkanType))

class VulkanMarshaling(VulkanWrapperGenerator):

    def __init__(self, module, typeInfo, variant="host"):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.cgenHeader = CodeGen()
        self.cgenImpl = CodeGen()

        self.variant = variant

        self.currentFeature = None
        self.apiOpcodes = {}

        if self.variant == "guest":
            self.marshalingParams = PARAMETERS_MARSHALING_GUEST
        else:
            self.marshalingParams = PARAMETERS_MARSHALING

        self.writeCodegen = \
            VulkanMarshalingCodegen(
                None,
                VULKAN_STREAM_VAR_NAME,
                MARSHAL_INPUT_VAR_NAME,
                API_PREFIX_MARSHAL,
                direction = "write")

        self.readCodegen = \
            VulkanMarshalingCodegen(
                None,
                VULKAN_STREAM_VAR_NAME,
                UNMARSHAL_INPUT_VAR_NAME,
                API_PREFIX_UNMARSHAL,
                direction = "read",
                dynAlloc = self.variant != "guest")

        self.knownDefs = {}

        # Begin Vulkan API opcodes from something high
        # that is not going to interfere with renderControl
        # opcodes
        self.currentOpcode = 20000

        self.extensionMarshalPrototype = \
            VulkanAPI(API_PREFIX_MARSHAL + "extension_struct",
                      STREAM_RET_TYPE,
                      self.marshalingParams +
                      [STRUCT_EXTENSION_PARAM])

        self.extensionUnmarshalPrototype = \
            VulkanAPI(API_PREFIX_UNMARSHAL + "extension_struct",
                      STREAM_RET_TYPE,
                      self.marshalingParams +
                      [STRUCT_EXTENSION_PARAM_FOR_WRITE])

    def onBegin(self,):
        VulkanWrapperGenerator.onBegin(self)
        self.module.appendImpl(self.cgenImpl.makeFuncDecl(self.extensionMarshalPrototype))
        self.module.appendImpl(self.cgenImpl.makeFuncDecl(self.extensionUnmarshalPrototype))

    def onBeginFeature(self, featureName):
        VulkanWrapperGenerator.onBeginFeature(self, featureName)
        self.currentFeature = featureName

    def onGenType(self, typeXml, name, alias):
        VulkanWrapperGenerator.onGenType(self, typeXml, name, alias)

        if name in self.knownDefs:
            return

        category = self.typeInfo.categoryOf(name)

        if category in ["struct", "union"] and not alias:

            structInfo = self.typeInfo.structs[name]

            marshalParams = self.marshalingParams + \
                [makeVulkanTypeSimple(True, name, 1, MARSHAL_INPUT_VAR_NAME)]
            marshalPrototype = \
                VulkanAPI(API_PREFIX_MARSHAL + name,
                          STREAM_RET_TYPE,
                          marshalParams)

            def structMarshalingDef(cgen):
                self.writeCodegen.cgen = cgen
                if category == "struct":
                    for member in structInfo.members:
                        iterateVulkanType(self.typeInfo, member, self.writeCodegen)
                if category == "union":
                    iterateVulkanType(self.typeInfo, structInfo.members[0], self.writeCodegen)

            self.module.appendHeader(
                self.cgenHeader.makeFuncDecl(marshalPrototype))
            self.module.appendImpl(
                self.cgenImpl.makeFuncImpl(
                    marshalPrototype, structMarshalingDef))

            unmarshalPrototype = \
                VulkanAPI(API_PREFIX_UNMARSHAL + name,
                          STREAM_RET_TYPE,
                          self.marshalingParams + [makeVulkanTypeSimple(False, name, 1, UNMARSHAL_INPUT_VAR_NAME)])

            def structUnmarshalingDef(cgen):
                self.readCodegen.cgen = cgen
                if category == "struct":
                    for member in structInfo.members:
                        iterateVulkanType(self.typeInfo, member, self.readCodegen)
                if category == "union":
                    iterateVulkanType(self.typeInfo, structInfo.members[0], self.readCodegen)

            self.module.appendHeader(
                self.cgenHeader.makeFuncDecl(unmarshalPrototype))
            self.module.appendImpl(
                self.cgenImpl.makeFuncImpl(
                    unmarshalPrototype, structUnmarshalingDef))

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)
        self.module.appendHeader(
            "#define OP_%s %d\n" % (name, self.currentOpcode))
        self.apiOpcodes[name] = (self.currentOpcode, self.currentFeature)
        self.currentOpcode += 1

    def onEnd(self,):
        VulkanWrapperGenerator.onEnd(self)

        def forEachExtensionMarshal(ext, castedAccess, cgen):
            cgen.funcCall(None, API_PREFIX_MARSHAL + ext.name,
                          [VULKAN_STREAM_VAR_NAME, castedAccess])

        def forEachExtensionUnmarshal(ext, castedAccess, cgen):
            cgen.funcCall(None, API_PREFIX_UNMARSHAL + ext.name,
                          [VULKAN_STREAM_VAR_NAME, castedAccess])

        self.module.appendImpl(
            self.cgenImpl.makeFuncImpl(
                self.extensionMarshalPrototype,
                lambda cgen: self.emitForEachStructExtension(
                    cgen,
                    STREAM_RET_TYPE,
                    STRUCT_EXTENSION_PARAM,
                    forEachExtensionMarshal)))

        self.module.appendImpl(
            self.cgenImpl.makeFuncImpl(
                self.extensionUnmarshalPrototype,
                lambda cgen: self.emitForEachStructExtension(
                    cgen,
                    STREAM_RET_TYPE,
                    STRUCT_EXTENSION_PARAM_FOR_WRITE,
                    forEachExtensionUnmarshal)))

        opcode2stringPrototype = \
            VulkanAPI("api_opcode_to_string",
                          makeVulkanTypeSimple(True, "char", 1, "none"),
                          [ makeVulkanTypeSimple(True, "uint32_t", 0, "opcode") ])

        self.module.appendHeader(
            self.cgenHeader.makeFuncDecl(opcode2stringPrototype))

        def emitOpcode2StringImpl(apiOpcodes, cgen):
            cgen.line("switch(opcode)")
            cgen.beginBlock()

            currFeature = None

            for (name, (opcodeNum, feature)) in sorted(apiOpcodes.items(), key = lambda x : x[1][0]):
                if not currFeature:
                    cgen.leftline("#ifdef %s" % feature)
                    currFeature = feature

                if currFeature and feature != currFeature:
                    cgen.leftline("#endif")
                    cgen.leftline("#ifdef %s" % feature)
                    currFeature = feature

                cgen.line("case OP_%s:" % name)
                cgen.beginBlock()
                cgen.stmt("return \"OP_%s\"" % name)
                cgen.endBlock()

            if currFeature:
                cgen.leftline("#endif")

            cgen.line("default:")
            cgen.beginBlock()
            cgen.stmt("return \"OP_UNKNOWN_API_CALL\"")
            cgen.endBlock()

            cgen.endBlock()

        self.module.appendImpl(
            self.cgenImpl.makeFuncImpl(
                opcode2stringPrototype,
                lambda cgen: emitOpcode2StringImpl(self.apiOpcodes, cgen)))
