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

from .common.codegen import CodeGen
from .common.vulkantypes import \
        VulkanCompoundType, VulkanAPI, makeVulkanTypeSimple, vulkanTypeNeedsTransform, vulkanTypeGetNeededTransformTypes, VulkanTypeIterator, iterateVulkanType, vulkanTypeforEachSubType, TRANSFORMED_TYPES, VulkanTypeProtobufInfo

from .wrapperdefs import VulkanWrapperGenerator
from .wrapperdefs import STRUCT_EXTENSION_PARAM, STRUCT_EXTENSION_PARAM_FOR_WRITE

PROTO_CONVERSION_HANDLEMAP_VAR_NAME = "handleMapping"
PROTO_CONVERSION_INPUT_VAR_NAME = "input"
PROTO_CONVERSION_OUTPUT_VAR_NAME = "output"

TO_PROTO_PREFIX = "to_proto_"
FROM_PROTO_PREFIX = "from_proto_"

class VulkanStructToProtoCodegen(VulkanTypeIterator):
    def __init__(self, cgen, handlemapVarName, inputVarName, outputVarName):
        self.cgen = cgen
        self.prefix = TO_PROTO_PREFIX

        self.handlemapVarName = handlemapVarName
        self.inputVarName = inputVarName
        self.outputVarName = outputVarName

        self.exprAccessor = lambda t: self.cgen.generalAccess(t, parentVarName = self.inputVarName, asPtr = True)
        self.exprPrimitiveValueAccessor = lambda t: self.cgen.generalAccess(t, self.inputVarName, asPtr = False)
        self.lenAccessor = lambda t: self.cgen.generalLengthAccess(t, parentVarName = self.inputVarName)

    def genPrimitiveProtobufSetCall(self, vulkanType, access):
        "%s->set_%s(%s)" % (self.outputVarName, vulkanType.typeName, access)

    def genHandleMappingCall(self, vulkanType, access, lenAccess):
        if lenAccess is None:
            lenAccess = "1"
            handle64Bytes = "8"
        else:
            handle64Bytes = "%s * 8" % lenAccess

        handle64Var = self.cgen.var()

        if lenAccess != "1":
            self.cgen.beginIf(lenAccess)
            self.cgen.stmt("std::vector<uint64_t> %s" % handle64Var)
            handle64VarAccess = "%s.data()" % handle64Var
        else:
            self.cgen.stmt("uint64_t %s" % handle64Var)
            handle64VarAccess = "&%s" % handle64Var

        self.cgen.stmt(
            "%s->mapHandles_%s_u64(%s, %s, %s)" %
            (self.handlemapVarName, vulkanType.typeName,
             access,
             handle64VarAccess, lenAccess))

        if lenAccess != "1":
            self.cgen.beginFor("uint32_t i = 0", "i < %s" % lenAccess, "++i")
            self.cgen.stmt("*(%s->add_%s()) = %s[i]" % (self.outputVarName, vulkanType.paramName.lower(), handle64Var))
            self.cgen.endFor()
        else:
            self.cgen.stmt("%s->set_%s(%s)" % (self.outputVarName, vulkanType.paramName.lower(), handle64Var))

        if lenAccess != "1":
            self.cgen.endIf()

    def onCheck(self, vulkanType):
        self.checked = True
        access = self.exprAccessor(vulkanType)
        self.cgen.line("// WARNING PTR CHECK")
        self.cgen.beginIf(access)

    def endCheck(self, vulkanType):
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

        self.cgen.funcCall(None, self.prefix + vulkanType.typeName, \
            [self.handlemapVarName, self.inputVarName, self.outputVarName])

        if lenAccess is not None:
            self.cgen.endFor()

    def onString(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        self.cgen.stmt("%s->set_%s(%s)" % \
            (self.outputVarName, vulkanType.paramName.lower(), access))

    def onStringArray(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

        self.cgen.beginFor("uint32_t i = 0", "i < (uint32_t)%s" % lenAccess, "++i")
        self.cgen.stmt("*(%s->add_%s()) = %s[i]" % \
            (self.outputVarName, vulkanType.paramName.lower(), access))
        self.cgen.endFor()

    def onStaticArr(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

        self.cgen.beginFor("uint32_t i = 0", "i < (uint32_t)%s" % lenAccess, "++i")
        self.cgen.stmt("*(%s->add_%s()) = %s[i]" % \
            (self.outputVarName, vulkanType.paramName.lower(), access))
        self.cgen.endFor()

    def onStructExtension(self, vulkanType):
        self.cgen.line("// TODO extension struct")

    def onPointer(self, vulkanType):
        access = self.exprAccessor(vulkanType)

        lenAccess = self.lenAccessor(vulkanType)

        if vulkanType.isHandleType():
            self.genHandleMappingCall(vulkanType, access, lenAccess)
        else:
            if lenAccess is not None:
                self.cgen.beginFor("uint32_t i = 0", "i < (uint32_t)%s" % lenAccess, "++i")
                self.cgen.stmt("*(%s->add_%s()) = %s[i]" % \
                    (self.outputVarName, vulkanType.paramName.lower(), access))
                self.cgen.endFor()
            else:
                self.genPrimitiveProtobufSetCall(vulkanType, access)

    def onValue(self, vulkanType):
        if vulkanType.isHandleType():
            access = self.exprAccessor(vulkanType)
            self.genHandleMappingCall(
                vulkanType.getForAddressAccess(), access, "1")
        else:
            access = self.exprAccessor(vulkanType)
            self.genPrimitiveProtobufSetCall(vulkanType, access)

class VulkanBaseProtoConversion(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.cgenHeader = CodeGen()
        self.cgenImpl = CodeGen()

        self.toProtoCodegen = \
            VulkanStructToProtoCodegen( \
                None,
                PROTO_CONVERSION_HANDLEMAP_VAR_NAME,
                PROTO_CONVERSION_INPUT_VAR_NAME,
                PROTO_CONVERSION_OUTPUT_VAR_NAME)

        self.knownStructs = {}

    def onBegin(self,):
        VulkanWrapperGenerator.onBegin(self)

    def onGenType(self, typeXml, name, alias):
        VulkanWrapperGenerator.onGenType(self, typeXml, name, alias)

        if name in self.knownStructs:
            return

        category = self.typeInfo.categoryOf(name)

        if category in ["struct", "union"] and not alias:
            self.knownStructs[name] = 1

            structInfo = self.typeInfo.structs[name]

            toProtoApiParams = [ \
                makeVulkanTypeSimple(False, "VulkanHandleMapping", 1, PROTO_CONVERSION_HANDLEMAP_VAR_NAME),
                makeVulkanTypeSimple(True, name, 1, PROTO_CONVERSION_INPUT_VAR_NAME),
                makeVulkanTypeSimple(False, "goldfish_vk_proto::%s" % name, 1, PROTO_CONVERSION_OUTPUT_VAR_NAME)]

            toProtoApiPrototype = \
                VulkanAPI(TO_PROTO_PREFIX + name,
                          makeVulkanTypeSimple(False, "void", 0),
                          toProtoApiParams)

            def toProtoFuncDef(cgen):
                self.toProtoCodegen.cgen = cgen
                if category == "struct":
                    for member in structInfo.members:
                        iterateVulkanType(self.typeInfo, member, self.toProtoCodegen)

                if category == "union":
                    iterateVulkanType(self.typeInfo, structInfo.members[0], self.toProtoCodegen)

            self.module.appendHeader(self.cgenHeader.makeFuncDecl(toProtoApiPrototype))
            self.module.appendImpl( \
                self.cgenImpl.makeFuncImpl( \
                    toProtoApiPrototype,
                    toProtoFuncDef))

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

    def onEnd(self,):
        VulkanWrapperGenerator.onEnd(self)
