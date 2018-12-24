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
        VulkanCompoundType, VulkanAPI, makeVulkanTypeSimple, vulkanTypeNeedsTransform, vulkanTypeGetNeededTransformTypes, VulkanTypeIterator, iterateVulkanType, vulkanTypeforEachSubType

from .wrapperdefs import VulkanWrapperGenerator
from .wrapperdefs import STRUCT_EXTENSION_PARAM, STRUCT_EXTENSION_PARAM_FOR_WRITE

def deviceMemoryTransform(resourceTrackerVarName, structOrApiInfo, getExpr, getLen, cgen):
    paramIndices = \
        structOrApiInfo.deviceMemoryInfoParameterIndices

    for _, info in paramIndices.items():
        orderedKeys = [
            "handle",
            "offset",
            "size",
            "typeIndex",
            "typeBits",]

        accesses = {
            "handle" : "nullptr",
            "offset" : "nullptr",
            "size" : "nullptr",
            "typeIndex" : "nullptr",
            "typeBits" : "nullptr",
        }
    
        lenAccesses = {
            "handle" : "1",
            "offset" : "1",
            "size" : "1",
            "typeIndex" : "1",
            "typeBits" : "1",
        }

        def doParam(i, vulkanType):
            access = getExpr(vulkanType)
            lenAccess = getLen(vulkanType)
  
            for k in orderedKeys:
                if i == info.__dict__[k]:
                    accesses[k] = access
                    if lenAccess is not None:
                        lenAccesses[k] = lenAccess
    
        vulkanTypeforEachSubType(structOrApiInfo, doParam)

        callParams = ", ".join( \
            ["%s, %s" % (accesses[k], lenAccesses[k]) \
                for k in orderedKeys])

        cgen.stmt("%s->deviceMemoryTransform(%s)" % \
            (resourceTrackerVarName, callParams))

def genTransformsForVulkanType(resourceTrackerVarName, structOrApiInfo, getExpr, getLen, cgen):
    for transform in vulkanTypeGetNeededTransformTypes(structOrApiInfo):
        if transform == "devicememory":
            deviceMemoryTransform( \
                resourceTrackerVarName,
                structOrApiInfo,
                getExpr, getLen, cgen)

class TransformCodegen(VulkanTypeIterator):
    def __init__(self, cgen, inputVar, resourceTrackerVarName, prefix):
        self.cgen = cgen
        self.inputVar = inputVar
        self.prefix = prefix
        self.resourceTrackerVarName = resourceTrackerVarName

        def makeAccess(varName, asPtr = True):
            return lambda t: self.cgen.generalAccess(t, parentVarName = varName, asPtr = asPtr)

        def makeLengthAccess(varName):
            return lambda t: self.cgen.generalLengthAccess(t, parentVarName = varName)

        self.exprAccessor = makeAccess(self.inputVar)
        self.exprAccessorValue = makeAccess(self.inputVar, asPtr = False)
        self.lenAccessor = makeLengthAccess(self.inputVar)

        self.checked = False

    def makeCastExpr(self, vulkanType):
        return "(%s)" % (
            self.cgen.makeCTypeDecl(vulkanType, useParamName=False))

    def asNonConstCast(self, access, vulkanType):
        if vulkanType.staticArrExpr:
            casted = "%s(%s)" % (self.makeCastExpr(vulkanType.getForAddressAccess().getForNonConstAccess()), access)
        elif vulkanType.accessibleAsPointer():
            casted = "%s(%s)" % (self.makeCastExpr(vulkanType.getForNonConstAccess()), access)
        else:
            casted = "%s(%s)" % (self.makeCastExpr(vulkanType.getForAddressAccess().getForNonConstAccess()), access)
        return casted

    def onCheck(self, vulkanType):
        pass

    def endCheck(self, vulkanType):
        pass

    def onCompoundType(self, vulkanType):

        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

        isPtr = vulkanType.pointerIndirectionLevels > 0

        if isPtr:
            self.cgen.beginIf(access)

        if lenAccess is not None:

            loopVar = "i"
            access = "%s + %s" % (access, loopVar)
            forInit = "uint32_t %s = 0" % loopVar
            forCond = "%s < (uint32_t)%s" % (loopVar, lenAccess)
            forIncr = "++%s" % loopVar

            self.cgen.beginFor(forInit, forCond, forIncr)

        accessCasted = self.asNonConstCast(access, vulkanType)
        self.cgen.funcCall(None, self.prefix + vulkanType.typeName,
                           [self.resourceTrackerVarName, accessCasted])

        if lenAccess is not None:
            self.cgen.endFor()

        if isPtr:
            self.cgen.endIf()

    def onString(self, vulkanType):
        pass

    def onStringArray(self, vulkanType):
        pass

    def onStaticArr(self, vulkanType):
        pass

    def onStructExtension(self, vulkanType):
        access = self.exprAccessor(vulkanType)

        castedAccessExpr = "(%s)(%s)" % ("void*", access)
        self.cgen.beginIf(access)
        self.cgen.funcCall(None, self.prefix + "extension_struct",
                           [self.resourceTrackerVarName, castedAccessExpr])
        self.cgen.endIf()

    def onPointer(self, vulkanType):
        pass

    def onValue(self, vulkanType):
        pass


class VulkanTransform(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.codegen = CodeGen()

        self.transformPrefix = "transform_"
        self.toTransformVar = "toTransform"
        self.resourceTrackerVarName = "resourceTracker"
        self.transformParam = \
            makeVulkanTypeSimple(False, "ResourceTracker", 1,
                                 self.resourceTrackerVarName)
        self.voidType = makeVulkanTypeSimple(False, "void", 0)

        self.extensionTransformPrototype = \
            VulkanAPI(self.transformPrefix + "extension_struct",
                      self.voidType,
                      [self.transformParam, STRUCT_EXTENSION_PARAM_FOR_WRITE])

        self.knownStructs = {}
        self.needsTransform = set([])

    def onBegin(self,):
        VulkanWrapperGenerator.onBegin(self)
        self.module.appendImpl(self.codegen.makeFuncDecl(
            self.extensionTransformPrototype))

    def onGenType(self, typeXml, name, alias):
        VulkanWrapperGenerator.onGenType(self, typeXml, name, alias)

        if name in self.knownStructs:
            return

        category = self.typeInfo.categoryOf(name)

        if category in ["struct", "union"] and not alias:
            structInfo = self.typeInfo.structs[name]
            self.knownStructs[name] = structInfo

            api = VulkanAPI( \
                self.transformPrefix + name,
                self.voidType,
                [self.transformParam] + \
                [makeVulkanTypeSimple( \
                    False, name, 1, self.toTransformVar)])

            transformer = TransformCodegen(
                None,
                self.toTransformVar,
                self.resourceTrackerVarName,
                self.transformPrefix)

            def funcDefGenerator(cgen):
                transformer.cgen = cgen
                for p in api.parameters:
                    cgen.stmt("(void)%s" % p.paramName)

                genTransformsForVulkanType(
                    self.resourceTrackerVarName,
                    structInfo,
                    transformer.exprAccessor,
                    transformer.lenAccessor,
                    cgen)

                for member in structInfo.members:
                    iterateVulkanType(
                        self.typeInfo, member,
                        transformer)

            self.module.appendHeader(
                self.codegen.makeFuncDecl(api))
            self.module.appendImpl(
                self.codegen.makeFuncImpl(api, funcDefGenerator))


    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

    def onEnd(self,):
        VulkanWrapperGenerator.onEnd(self)

        def forEachExtensionTransform(ext, castedAccess, cgen):
            cgen.funcCall(None, self.transformPrefix + ext.name,
                          [self.resourceTrackerVarName, castedAccess])

        self.module.appendImpl(
            self.codegen.makeFuncImpl(
                self.extensionTransformPrototype,
                lambda cgen: self.emitForEachStructExtension(
                    cgen,
                    self.voidType,
                    STRUCT_EXTENSION_PARAM_FOR_WRITE,
                    forEachExtensionTransform)))
