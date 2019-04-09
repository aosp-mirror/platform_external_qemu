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

PROTO_CONVERSION_POOL_VAR_NAME = "pool"
PROTO_CONVERSION_HANDLEMAP_VAR_NAME = "handleMapping"
PROTO_CONVERSION_INPUT_VAR_NAME = "input"
PROTO_CONVERSION_OUTPUT_VAR_NAME = "output"

TO_PROTO_PREFIX = "to_proto_"
FROM_PROTO_PREFIX = "from_proto_"

class VulkanStructToProtoCodegen(VulkanTypeIterator):
    def __init__(self, cgen, handlemapVarName, inputVarName, outputVarName):
        self.cgen = cgen
        self.prefix = TO_PROTO_PREFIX
        self.typeInfo = None
        self.structInfo = None

        self.handlemapVarName = handlemapVarName
        self.inputVarName = inputVarName
        self.outputVarName = outputVarName

        self.exprAccessor = lambda t: self.cgen.generalAccess(t, parentVarName = self.inputVarName, asPtr = True)
        self.exprPrimitiveValueAccessor = lambda t: self.cgen.generalAccess(t, self.inputVarName, asPtr = False)
        self.lenAccessor = lambda t: self.cgen.generalLengthAccess(t, parentVarName = self.inputVarName)

    def genPrimitiveProtobufSetCall(self, vulkanType, access):
        protoInfo = VulkanTypeProtobufInfo(self.typeInfo, self.structInfo, vulkanType)

        ptrCast = ""
        if vulkanType.pointerIndirectionLevels > 0:
            ptrCast = "(uintptr_t)"

        self.cgen.stmt("%s->set_%s((%s)%s%s)" % (self.outputVarName, vulkanType.paramName.lower(), protoInfo.protobufCType, ptrCast, access))

    def genHandleMappingCall(self, vulkanType, access, lenAccess):
        if lenAccess is None:
            lenAccess = "1"

        handle64Var = self.cgen.var()

        if lenAccess != "1":
            self.cgen.beginIf(lenAccess)
            self.cgen.stmt("std::vector<uint64_t> %s(%s)" % (handle64Var, lenAccess))
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
            self.cgen.stmt("%s->add_%s(%s[i])" % (self.outputVarName, vulkanType.paramName.lower(), handle64Var))
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
            outputAccess = "%s->add_%s()" % (self.outputVarName, vulkanType.paramName.lower())
            self.cgen.beginFor(forInit, forCond, forIncr)
        else:
            outputAccess = "%s->mutable_%s()" % (self.outputVarName, vulkanType.paramName.lower())

        self.cgen.funcCall(None, self.prefix + vulkanType.typeName, \
            [self.handlemapVarName, access, outputAccess])

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
        protoInfo = VulkanTypeProtobufInfo(self.typeInfo, self.structInfo, vulkanType)

        if protoInfo.isString:
            self.cgen.stmt("%s->set_%s(%s)" % \
                (self.outputVarName, vulkanType.paramName.lower(), access))
        elif vulkanType.isHandleType():
            self.genHandleMappingCall(vulkanType, access, lenAccess)
        else:
            self.cgen.beginFor("uint32_t i = 0", "i < (uint32_t)%s" % lenAccess, "++i")

            casted = access

            if vulkanType.typeName == "void":
                casted = "((const char*)(%s))" % casted

            self.cgen.stmt("%s->add_%s(%s[i])" % \
                (self.outputVarName, vulkanType.paramName.lower(), casted))

            self.cgen.endFor()

    def onStructExtension(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        outputAccess = "%s->mutable_%s()" % (self.outputVarName, vulkanType.paramName.lower())

        self.cgen.beginIf(access)

        if vulkanType.typeName in ["VkBaseOutStructure", "VkBaseInStructure"]:
            self.cgen.funcCall(None, self.prefix + vulkanType.typeName, \
                [self.handlemapVarName, access, outputAccess])
        else:
            self.cgen.funcCall(None, self.prefix + "extension_struct", \
                [self.handlemapVarName, access, outputAccess])

        self.cgen.endIf()

    def onPointer(self, vulkanType):
        access = self.exprAccessor(vulkanType)

        lenAccess = self.lenAccessor(vulkanType)

        if vulkanType.isHandleType():
            self.genHandleMappingCall(vulkanType, access, lenAccess)
        else:
            if lenAccess is not None:
                self.cgen.beginFor("uint32_t i = 0", "i < (uint32_t)%s" % lenAccess, "++i")
                casted = access

                if vulkanType.typeName == "void":
                    casted = "((const char*)(%s))" % casted

                self.cgen.stmt("%s->add_%s(%s[i])" % \
                    (self.outputVarName, vulkanType.paramName.lower(), casted))
                self.cgen.endFor()
            else:
                self.genPrimitiveProtobufSetCall(vulkanType, access)

    def onValue(self, vulkanType):
        if vulkanType.isHandleType():
            access = self.exprAccessor(vulkanType)
            self.genHandleMappingCall(
                vulkanType.getForAddressAccess(), access, "1")
        else:
            access = self.exprPrimitiveValueAccessor(vulkanType)
            self.genPrimitiveProtobufSetCall(vulkanType, access)

class VulkanProtoToStructCodegen(VulkanTypeIterator):
    def __init__(self, cgen, poolVarName, handlemapVarName, inputVarName, outputVarName):
        self.cgen = cgen
        self.prefix = FROM_PROTO_PREFIX
        self.typeInfo = None
        self.structInfo = None

        self.poolVarName = poolVarName
        self.handlemapVarName = handlemapVarName
        self.inputVarName = inputVarName
        self.outputVarName = outputVarName

        self.exprAccessor = lambda t: self.cgen.generalAccess(t, parentVarName = self.outputVarName, asPtr = True)
        self.exprPrimitiveValueAccessor = lambda t: self.cgen.generalAccess(t, self.outputVarName, asPtr = False)
        self.lenAccessor = lambda t: self.cgen.generalLengthAccess(t, parentVarName = self.outputVarName)

    def genPrimitiveProtobufReadAndSetCall(self, vulkanType, access):
        protoInfo = VulkanTypeProtobufInfo(self.typeInfo, self.structInfo, vulkanType)

        cast = "(%s%s)" % (vulkanType.typeName, "".join(["*"] * vulkanType.pointerIndirectionLevels))
        if vulkanType.pointerIndirectionLevels > 0:
            cast = "%s(uintptr_t)" % cast

        self.cgen.stmt("%s = %s%s->%s()" % (access, cast, self.inputVarName, vulkanType.paramName.lower()))

    def genHandleMappingCall(self, vulkanType, access, lenAccess):
        if lenAccess is None:
            lenAccess = "1"

        if lenAccess != "1":
            self.cgen.beginIf(lenAccess)

        self.cgen.beginFor("uint32_t i = 0", "i < %s" % lenAccess, "++i")
        self.cgen.stmt("uint64_t current = %s->%s(%s)" % (self.inputVarName, vulkanType.paramName.lower(), "" if lenAccess == "1" else "i"))
        self.cgen.stmt("%s->mapHandles_u64_%s(%s, (%s*)%s, %s)" % ( \
            self.handlemapVarName, vulkanType.typeName, "&current", \
            vulkanType.typeName,
            ("&%s[i]" % access) if lenAccess != "1" else ("&%s" % access), "1"))
        self.cgen.endFor()

        if lenAccess != "1":
            self.cgen.endIf()

    def onCheck(self, vulkanType):
        self.checked = True
        self.cgen.line("// WARNING PTR CHECK")

        protoInfo = VulkanTypeProtobufInfo(self.typeInfo, self.structInfo, vulkanType)

        if protoInfo.lengthInfo is not None:
            self.cgen.beginIf("%s->%s_size()" % (self.inputVarName, vulkanType.paramName.lower()))
        else:
            self.cgen.beginIf("%s->has_%s()" % (self.inputVarName, vulkanType.paramName.lower()))

    def endCheck(self, vulkanType):
        if self.checked:
            self.cgen.endIf()
            self.checked = False

    def doAllocSpace(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)
        sizeof = self.cgen.sizeofExpr( \
                     vulkanType.getForValueAccess())
        if lenAccess:
            bytesExpr = "%s * %s" % (lenAccess, sizeof)
        else:
            bytesExpr = sizeof

        self.cgen.stmt( \
            "*(void**)&%s = %s->alloc(%s)" % \
                (access, self.poolVarName, bytesExpr))

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
            protoAccess = "%s->mutable_%s(i)" % (self.inputVarName, vulkanType.paramName.lower())
            self.cgen.beginFor(forInit, forCond, forIncr)
        else:
            protoAccess = "%s->mutable_%s()" % (self.inputVarName, vulkanType.paramName.lower())

        castedAccess = "(%s*)(%s)" % (vulkanType.typeName, access)
        self.cgen.funcCall(None, self.prefix + vulkanType.typeName, \
            [self.poolVarName, self.handlemapVarName, protoAccess, castedAccess])

        if lenAccess is not None:
            self.cgen.endFor()

    def onString(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        self.cgen.stmt( \
            "*(const char**)(&%s) = %s->%s().c_str()" % (access, self.inputVarName, vulkanType.paramName.lower()))

    def onStringArray(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

        self.cgen.beginIf(lenAccess)

        self.cgen.stmt( \
            "*(void**)&%s = %s->alloc(%s * sizeof(char*))" % \
                (access, self.poolVarName, lenAccess))

        self.cgen.endIf()

        self.cgen.beginFor("uint32_t i = 0", "i < (uint32_t)%s" % lenAccess, "++i")
        bytesExpr = "(strlen(%s[i]) + 1)" % access
        self.cgen.stmt( \
            "*(const char**)(&%s[i]) = %s->%s(i).c_str()" % \
                (access, self.inputVarName, vulkanType.paramName.lower()))

        self.cgen.endFor()

    def onStaticArr(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)
        protoInfo = VulkanTypeProtobufInfo(self.typeInfo, self.structInfo, vulkanType)

        if protoInfo.isString:
            access = self.exprAccessor(vulkanType)
            lenAccess = self.lenAccessor(vulkanType)
            sizeof = self.cgen.sizeofExpr( \
                         vulkanType.getForValueAccess())
            bytesExpr = lenAccess
            self.cgen.stmt( \
                "memset(%s, 0x0, %s)" % (access, bytesExpr))
            self.cgen.stmt( \
                "strncpy(%s, %s->%s().c_str(), strlen(%s->%s().c_str()))" % (access, self.inputVarName, vulkanType.paramName.lower(), self.inputVarName, vulkanType.paramName.lower()))
        elif vulkanType.isHandleType():
            self.genHandleMappingCall(vulkanType, access, lenAccess)
        else:
            self.cgen.beginFor("uint32_t i = 0", "i < (uint32_t)%s" % lenAccess, "++i")

            cast = "(%s)" % (vulkanType.typeName)
            if vulkanType.typeName == "void":
                cast = "(void*)"

            self.cgen.stmt("%s[i] = %s%s->%s(i)" % \
                (access, cast, self.inputVarName, vulkanType.paramName.lower()))

            self.cgen.endFor()

    def onStructExtension(self, vulkanType):
        protoAccess = "%s->mutable_%s()" % (self.inputVarName, vulkanType.paramName.lower())
        access = self.exprAccessor(vulkanType)

        self.cgen.beginIf("%s->has_%s()" % (self.inputVarName, vulkanType.paramName.lower()))

        if vulkanType.typeName in ["VkBaseOutStructure", "VkBaseInStructure"]:
            self.cgen.stmt( \
                "*(void**)&%s = %s->alloc(sizeof(%s))" % \
                    (access, self.poolVarName, vulkanType.typeName))
            self.cgen.funcCall(None, self.prefix + vulkanType.typeName, \
                [self.poolVarName, self.handlemapVarName, protoAccess, "(%s*)(%s)" % (vulkanType.typeName, access)])
        else:
            self.cgen.funcCall( \
                "VkStructureType structType",
                "extension_proto_to_struct_type", [protoAccess])
            self.cgen.stmt( \
                "*(void**)&%s = %s->alloc(goldfish_vk_extension_struct_size((const void*)(&structType)))" % \
                    (access, self.poolVarName))
            self.cgen.stmt( \
                "*(VkStructureType*)%s = structType" % access)
            self.cgen.funcCall(None, self.prefix + "extension_struct", \
                [self.poolVarName, self.handlemapVarName, protoAccess, "(void*)(%s)" % (access)])
        self.cgen.endIf()

    def onPointer(self, vulkanType):
        access = self.exprAccessor(vulkanType)

        lenAccess = self.lenAccessor(vulkanType)

        if vulkanType.isHandleType():
            self.genHandleMappingCall(vulkanType, access, lenAccess)
        else:
            if lenAccess is not None:
                self.cgen.beginFor("uint32_t i = 0", "i < (uint32_t)%s" % lenAccess, "++i")
                cast = "(%s%s)" % (vulkanType.typeName, "".join(["*"] * (vulkanType.pointerIndirectionLevels - 1)))
                if vulkanType.typeName == "void":
                    cast = "(char%s)" % "".join(["*"] * (vulkanType.pointerIndirectionLevels - 1))

                self.cgen.stmt("*((%s%s)(&%s) + i) = %s%s->%s(i)" % \
                    (vulkanType.typeName if vulkanType.typeName != "void" else "char",
                     "".join(["*"] * (vulkanType.pointerIndirectionLevels)), \
                     access, cast, self.inputVarName, vulkanType.paramName.lower()))
                self.cgen.endFor()
            else:
                self.genPrimitiveProtobufReadAndSetCall(vulkanType, access)

    def onValue(self, vulkanType):
        if vulkanType.isHandleType():
            access = self.exprPrimitiveValueAccessor(vulkanType)
            self.genHandleMappingCall(
                vulkanType.getForAddressAccess(), access, "1")
        else:
            access = self.exprPrimitiveValueAccessor(vulkanType)
            self.genPrimitiveProtobufReadAndSetCall(vulkanType, access)

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

        self.fromProtoCodegen = \
            VulkanProtoToStructCodegen( \
                None,
                PROTO_CONVERSION_POOL_VAR_NAME,
                PROTO_CONVERSION_HANDLEMAP_VAR_NAME,
                PROTO_CONVERSION_INPUT_VAR_NAME,
                PROTO_CONVERSION_OUTPUT_VAR_NAME)

        self.knownStructs = {}

    def onBegin(self,):
        VulkanWrapperGenerator.onBegin(self)

        self.extensionProtoToStructTypeFuncPrototype = \
            VulkanAPI("extension_proto_to_struct_type",
                      makeVulkanTypeSimple(False, "VkStructureType", 0),
                      [makeVulkanTypeSimple(True, "goldfish_vk_proto::VkExtensionStruct", 1, PROTO_CONVERSION_INPUT_VAR_NAME)])

        self.extensionToProtoFuncPrototype = \
            VulkanAPI(TO_PROTO_PREFIX + "extension_struct",
                      makeVulkanTypeSimple(False, "void", 0),
                      [ \
                          makeVulkanTypeSimple(False, "VulkanHandleMapping", 1, PROTO_CONVERSION_HANDLEMAP_VAR_NAME),
                          STRUCT_EXTENSION_PARAM,
                          makeVulkanTypeSimple(False, "void", 1, PROTO_CONVERSION_OUTPUT_VAR_NAME)])

        self.extensionFromProtoFuncPrototype = \
            VulkanAPI(FROM_PROTO_PREFIX + "extension_struct",
                      makeVulkanTypeSimple(False, "void", 0),
                      [ \
                          makeVulkanTypeSimple(False, "Pool", 1, PROTO_CONVERSION_POOL_VAR_NAME),
                          makeVulkanTypeSimple(False, "VulkanHandleMapping", 1, PROTO_CONVERSION_HANDLEMAP_VAR_NAME),
                          makeVulkanTypeSimple(False, "void", 1, PROTO_CONVERSION_INPUT_VAR_NAME),
                          STRUCT_EXTENSION_PARAM_FOR_WRITE])

        self.module.appendImpl(self.cgenImpl.makeFuncDecl(self.extensionProtoToStructTypeFuncPrototype))
        self.module.appendImpl(self.cgenImpl.makeFuncDecl(self.extensionToProtoFuncPrototype))
        self.module.appendImpl(self.cgenImpl.makeFuncDecl(self.extensionFromProtoFuncPrototype))


    def onGenType(self, typeXml, name, alias):
        VulkanWrapperGenerator.onGenType(self, typeXml, name, alias)

        if name in self.knownStructs:
            return

        category = self.typeInfo.categoryOf(name)

        if category in ["struct", "union"] and not alias:
            self.knownStructs[name] = 1

            structInfo = self.typeInfo.structs[name]

            # TO protobuf
            toProtoApiPrototype = \
                VulkanAPI(TO_PROTO_PREFIX + name,
                          makeVulkanTypeSimple(False, "void", 0),
                          [ \
                              makeVulkanTypeSimple(False, "VulkanHandleMapping", 1, PROTO_CONVERSION_HANDLEMAP_VAR_NAME),
                              makeVulkanTypeSimple(True, name, 1, PROTO_CONVERSION_INPUT_VAR_NAME),
                              makeVulkanTypeSimple(False, "goldfish_vk_proto::%s" % name, 1, PROTO_CONVERSION_OUTPUT_VAR_NAME)])

            def toProtoFuncDef(cgen):
                self.toProtoCodegen.cgen = cgen
                self.toProtoCodegen.typeInfo = self.typeInfo
                self.toProtoCodegen.structInfo = structInfo

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

            # FROM protobuf
            fromProtoApiPrototype = \
                VulkanAPI(FROM_PROTO_PREFIX + name,
                          makeVulkanTypeSimple(False, "void", 0),
                          [ \
                              makeVulkanTypeSimple(False, "Pool", 1, PROTO_CONVERSION_POOL_VAR_NAME),
                              makeVulkanTypeSimple(False, "VulkanHandleMapping", 1, PROTO_CONVERSION_HANDLEMAP_VAR_NAME),
                              makeVulkanTypeSimple(False, "goldfish_vk_proto::%s" % name, 1, PROTO_CONVERSION_INPUT_VAR_NAME),
                              makeVulkanTypeSimple(False, name, 1, PROTO_CONVERSION_OUTPUT_VAR_NAME)])

            def fromProtoFuncDef(cgen):
                self.fromProtoCodegen.cgen = cgen
                self.fromProtoCodegen.typeInfo = self.typeInfo
                self.fromProtoCodegen.structInfo = structInfo

                if category == "struct":
                    cgen.stmt("memset(output, 0x0, sizeof(%s))" % (structInfo.name))
                    for member in structInfo.members:
                        iterateVulkanType(self.typeInfo, member, self.fromProtoCodegen)

                if category == "union":
                    iterateVulkanType(self.typeInfo, structInfo.members[0], self.fromProtoCodegen)

            self.module.appendHeader(self.cgenHeader.makeFuncDecl(fromProtoApiPrototype))
            self.module.appendImpl( \
                self.cgenImpl.makeFuncImpl( \
                    fromProtoApiPrototype,
                    fromProtoFuncDef))

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

    def onEnd(self,):
        VulkanWrapperGenerator.onEnd(self)

        def forEachExtensionGetStructTypeFromProto(i, ext, cgen):
            cgen.line("if (%s->has_extension_%s()) { return %s; }" % \
                (PROTO_CONVERSION_INPUT_VAR_NAME, \
                 ext.name.lower(), ext.structEnumExpr))

        def forEachExtensionConvertToProtobuf(ext, castedAccess, cgen):
            castedProtobufAccess = \
                "reinterpret_cast<goldfish_vk_proto::VkExtensionStruct*>(%s)->mutable_extension_%s()" % \
                    (PROTO_CONVERSION_OUTPUT_VAR_NAME, ext.name.lower())

            cgen.funcCall(None, TO_PROTO_PREFIX + ext.name,
                          [PROTO_CONVERSION_HANDLEMAP_VAR_NAME, \
                           castedAccess, castedProtobufAccess])

        def forEachExtensionConvertFromProtobuf(ext, castedAccess, cgen):
            castedProtobufAccess = \
                "reinterpret_cast<goldfish_vk_proto::VkExtensionStruct*>(%s)->mutable_extension_%s()" % \
                    (PROTO_CONVERSION_INPUT_VAR_NAME, ext.name.lower())
            cgen.funcCall(None, FROM_PROTO_PREFIX + ext.name,
                          [PROTO_CONVERSION_POOL_VAR_NAME, \
                           PROTO_CONVERSION_HANDLEMAP_VAR_NAME, \
                           castedProtobufAccess, castedAccess])

        self.module.appendImpl(
            self.cgenImpl.makeFuncImpl(
                self.extensionProtoToStructTypeFuncPrototype,
                lambda cgen: self.emitForEachStructExtensionGeneral(
                    cgen,
                    forEachExtensionGetStructTypeFromProto,
                    doFeatureIfdefs=True)))

        self.module.appendImpl(
            self.cgenImpl.makeFuncImpl(
                self.extensionToProtoFuncPrototype,
                lambda cgen: self.emitForEachStructExtension(
                    cgen,
                    makeVulkanTypeSimple(False, "void", 0),
                    STRUCT_EXTENSION_PARAM,
                    forEachExtensionConvertToProtobuf)))

        self.module.appendImpl(
            self.cgenImpl.makeFuncImpl(
                self.extensionFromProtoFuncPrototype,
                lambda cgen: self.emitForEachStructExtension(
                    cgen,
                    makeVulkanTypeSimple(False, "void", 0),
                    STRUCT_EXTENSION_PARAM_FOR_WRITE,
                    forEachExtensionConvertFromProtobuf)))
