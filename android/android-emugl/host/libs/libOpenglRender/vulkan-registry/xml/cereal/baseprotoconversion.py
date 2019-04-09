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

class VulkanStructToProtoCodegen(VulkanTypeIterator):
    def __init__(self, cgen, inputVarName, outputVarPointer):
        self.cgen = cgen

        self.exprAccessor = lambda t: self.cgen.generalAccess(t, parentVarName = self.inputVarName, asPtr = True)
        self.exprPrimitiveValueAccessor = lambda t: self.cgen.generalAccess(t, self.inputVarName, asPtr = False)
        self.lenAccessor = lambda t: self.cgen.generalLengthAccess(t, parentVarName = self.inputVarName)

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

        if vulkanType.pointerIndirectionLevels > 0:
            pass

        if lenAccess is not None:
            pass

        if lenAccess is not None:
            pass

    def onString(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        pass

    def onStringArray(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

    def onStaticArr(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)
        finalLenExpr = "%s * %s" % (lenAccess, self.cgen.sizeofExpr(vulkanType))
        # self.genStreamCall(vulkanType, access, finalLenExpr)

    def onStructExtension(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        sizeVar = "%s_size" % vulkanType.paramName
        #    self.cgen.stmt("size_t %s = %s(%s)" % (sizeVar, EXTENSION_SIZE_API_NAME, access))
        #    self.cgen.stmt("%s->putBe32(%s)" % \
        #        (self.streamVarName, sizeVar))
        self.cgen.beginIf(sizeVar)
        # castedAccessExpr = access
        # self.genStreamCall(vulkanType, access, "sizeof(VkStructureType)")
        self.cgen.funcCall(None, self.marshalPrefix + "extension_struct",
                           [self.streamVarName, castedAccessExpr])
        self.cgen.endIf()

    def onPointer(self, vulkanType):
        access = self.exprAccessor(vulkanType)

        lenAccess = self.lenAccessor(vulkanType)

        self.doAllocSpace(vulkanType)

        if vulkanType.isHandleType():
            self.genHandleMappingCall(vulkanType, access, lenAccess)
        else:
            if self.typeInfo.isNonAbiPortableType(vulkanType.typeName):
                if lenAccess is not None:
                    self.cgen.beginFor("uint32_t i = 0", "i < (uint32_t)%s" % lenAccess, "++i")
                    # self.genPrimitiveStreamCall(vulkanType.getForValueAccess(), "%s[i]" % access)
                    self.cgen.endFor()
                else:
                    pass
                    # self.genPrimitiveStreamCall(vulkanType.getForValueAccess(), "(*%s)" % access)
            else:
                if lenAccess is not None:
                    finalLenExpr = "%s * %s" % (
                        lenAccess, self.cgen.sizeofExpr(vulkanType.getForValueAccess()))
                else:
                    finalLenExpr = "%s" % (
                        self.cgen.sizeofExpr(vulkanType.getForValueAccess()))
                # self.genStreamCall(vulkanType, access, finalLenExpr)

    def onValue(self, vulkanType):
        if vulkanType.isHandleType() and self.mapHandles:
            access = self.exprAccessor(vulkanType)
            self.genHandleMappingCall(
                vulkanType.getForAddressAccess(), access, "1")
        elif self.typeInfo.isNonAbiPortableType(vulkanType.typeName):
            access = self.exprPrimitiveValueAccessor(vulkanType)
            # self.genPrimitiveStreamCall(vulkanType, access)
        else:
            access = self.exprAccessor(vulkanType)
            # self.genStreamCall(vulkanType, access, self.cgen.sizeofExpr(vulkanType))

class VulkanBaseProtoConversion(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.codegen = CodeGen()

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

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

    def onEnd(self,):
        VulkanWrapperGenerator.onEnd(self)
