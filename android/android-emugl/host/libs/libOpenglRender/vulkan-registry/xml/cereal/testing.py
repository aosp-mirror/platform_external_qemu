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

from .wrapperdefs import *

class VulkanEqualityCodegen(object):
    def __init__(self, cgen, inputVars, onFailCompareVar, prefix):
        self.cgen = cgen
        self.inputVars = inputVars
        self.onFailCompareVar = onFailCompareVar
        self.prefix = prefix

        self.exprAccessorLhs = lambda t: self.cgen.generalAccess(t, parentVarName = self.inputVars[0], asPtr = True)
        self.exprAccessorRhs = lambda t: self.cgen.generalAccess(t, parentVarName = self.inputVars[1])

        self.exprAccessorValueLhs = lambda t: self.cgen.generalAccess(t, parentVarName = self.inputVars[0], asPtr = False)
        self.exprAccessorValueRhs = lambda t: self.cgen.generalAccess(t, parentVarName = self.inputVars[1], asPtr = False)

        self.lenAccessorLhs = lambda t: self.cgen.generalLengthAccess(t, parentVarName = self.inputVars[0])
        self.lenAccessorRhs = lambda t: self.cgen.generalLengthAccess(t, parentVarName = self.inputVars[1])

    def getTypeForCompare(self, vulkanType):
        res = deepcopy(vulkanType)

        if not vulkanType.accessibleAsPointer():
            res = res.getForAddressAccess()

        if vulkanType.staticArrExpr:
            res = res.getForAddressAccess()

        return res

    def makeCastExpr(self, vulkanType):
        return "(%s)" % (self.cgen.makeCTypeDecl(vulkanType, useParamName = False))

    def makeEqualExpr(self, lhs, rhs):
        return "(%s) == (%s)" % (lhs, rhs)

    def makeEqualBufExpr(self, lhs, rhs, size):
        return "(memcmp(%s, %s, %s) == 0)" % (lhs, rhs, size)

    def makeBothNotNullExpr(self, lhs, rhs):
        return "(%s) && (%s)" % (lhs, rhs)

    def makeBothNullExpr(self, lhs, rhs):
        return "!(%s) && !(%s)" % (lhs, rhs)

    def compareWithConsequence(self, compareExpr, vulkanType, errMsg = ""):
        self.cgen.stmt("if (!(%s)) %s(\"%s (Error: %s)\")" % (compareExpr, self.onFailCompareVar, self.exprAccessorValueLhs(vulkanType), errMsg))

    def onCheck(self, vulkanType):

        self.checked = True

        accessLhs = self.exprAccessorLhs(vulkanType)
        accessRhs = self.exprAccessorRhs(vulkanType)

        self.compareWithConsequence( \
            "(%s) || (%s)" % (self.makeBothNullExpr(accessLhs, accessRhs), self.makeBothNotNullExpr(accessLhs, accessRhs)),
            vulkanType, "Mismatch in optional field")

        skipStreamInternal = vulkanType.typeName == "void"

        if skipStreamInternal:
            return

        self.cgen.beginIf(accessLhs)

    def endCheck(self, vulkanType):

        skipStreamInternal = vulkanType.typeName == "void"
        if skipStreamInternal:
            return

        if self.checked:
            self.cgen.endIf()
            self.checked = False

    def onCompoundType(self, vulkanType):
        accessLhs = self.exprAccessorLhs(vulkanType)
        accessRhs = self.exprAccessorRhs(vulkanType)

        lenAccessLhs = self.lenAccessorLhs(vulkanType)
        lenAccessRhs = self.lenAccessorRhs(vulkanType)

        if lenAccessLhs is not None:
            self.compareWithConsequence( \
                self.makeEqualExpr(lenAccessLhs, lenAccessRhs),
                vulkanType, "Lengths not equal")

            loopVar = "i"
            accessLhs = "%s + %s" % (accessLhs, loopVar)
            accessRhs = "%s + %s" % (accessRhs, loopVar)
            forInit = "uint32_t %s = 0" % loopVar
            forCond = "%s < (uint32_t)%s" % (loopVar, lenAccessLhs)
            forIncr = "++%s" % loopVar
            self.cgen.beginFor(forInit, forCond, forIncr)

        accessWithCast = "%s(%s)" % (self.makeCastExpr(self.getTypeForCompare(vulkanType)), accessLhs)

        self.cgen.funcCall(None, self.prefix + vulkanType.typeName, [accessLhs, accessRhs, self.onFailCompareVar])

        if lenAccessLhs is not None:
            self.cgen.endFor()

    def onString(self, vulkanType):
        pass

        # TODO
        # access = self.exprAccessor(vulkanType)

        # if self.direction == "write":
        #     self.cgen.stmt("%s->putString(%s)" % (self.streamVarName, access))
        # else:
        #     self.cgen.stmt("%s->getString()" % (self.streamVarName))

    def onStringArray(self, vulkanType):
        pass

        # TODO
        # access = self.exprAccessor(vulkanType)
        # lenAccess = self.lenAccessor(vulkanType)

        # if self.direction == "write":
        #     self.cgen.stmt("%s->putStringArray(%s, %s)" % (self.streamVarName, access, lenAccess))
        # else:
        #     self.cgen.stmt("%s->getStringArray()" % (self.streamVarName))

    def onStaticArr(self, vulkanType):
        accessLhs = self.exprAccessorLhs(vulkanType)
        accessRhs = self.exprAccessorRhs(vulkanType)

        lenAccessLhs = self.lenAccessorLhs(vulkanType)

        finalLenExpr = "%s * %s" % (lenAccessLhs, self.cgen.sizeofExpr(vulkanType))

        self.compareWithConsequence(self.makeEqualBufExpr(accessLhs, accessRhs, finalLenExpr), vulkanType, "Unequal static array")

    def onPointer(self, vulkanType):
        skipStreamInternal = vulkanType.typeName == "void"
        if skipStreamInternal:
            return

        accessLhs = self.exprAccessorLhs(vulkanType)
        accessRhs = self.exprAccessorRhs(vulkanType)

        lenAccessLhs = self.lenAccessorLhs(vulkanType)
        lenAccessRhs = self.lenAccessorRhs(vulkanType)

        if lenAccessLhs is not None:
            self.compareWithConsequence( \
                self.makeEqualExpr(lenAccessLhs, lenAccessRhs),
                vulkanType, "Lengths not equal")

            finalLenExpr = "%s * %s" % (lenAccessLhs, self.cgen.sizeofExpr(vulkanType.getForValueAccess()))
        else:
            finalLenExpr = self.cgen.sizeofExpr(vulkanType.getForValueAccess())

        self.compareWithConsequence(self.makeEqualBufExpr(accessLhs, accessRhs, finalLenExpr), vulkanType, "Unequal dyn array")

    def onValue(self, vulkanType):
        accessLhs = self.exprAccessorValueLhs(vulkanType)
        accessRhs = self.exprAccessorValueRhs(vulkanType)
        self.compareWithConsequence(self.makeEqualExpr(accessLhs, accessRhs), vulkanType, "Value not equal")


class VulkanTesting(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.codegen = CodeGen()

        self.equalityCodegen = \
            VulkanEqualityCodegen(
                None,
                EQUALITY_VAR_NAMES,
                EQUALITY_ON_FAIL_VAR,
                API_PREFIX_EQUALITY)

        self.knownDefs = {}


    def onGenType(self, typeXml, name, alias):
        VulkanWrapperGenerator.onGenType(self, typeXml, name, alias)

        if name in self.knownDefs:
            return

        category = self.typeInfo.categoryOf(name)

        if category in ["struct", "union"] and not alias:

            structInfo = self.typeInfo.structs[name]

            comparePrototype = \
                VulkanAPI(API_PREFIX_EQUALITY + name,
                          EQUALITY_RET_TYPE,
                          list(map(lambda v: makeVulkanTypeSimple(True, name, 1, v), EQUALITY_VAR_NAMES)) + [EQUALITY_ON_FAIL_VAR_TYPE])

            def structCompareDef(cgen):
                self.equalityCodegen.cgen = cgen
                for member in structInfo.members:
                    iterateVulkanType(self.typeInfo, member, self.equalityCodegen)

            self.module.appendHeader(self.codegen.makeFuncDecl(comparePrototype))
            self.module.appendImpl(self.codegen.makeFuncImpl(comparePrototype, structCompareDef))

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

        # TODO: Figure something out for API testing
