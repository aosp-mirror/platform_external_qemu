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

from .vulkantypes import VulkanType, VulkanCompoundType, VulkanAPI

from copy import deepcopy

import os
import sys


# Class capturing a .cpp file and a .h file (a "C++ module")
class Module(object):

    def __init__(self, directory, basename):
        self.directory = directory
        self.basename = basename

        self.headerPreamble = ""
        self.implPreamble = ""

        self.headerPostamble = ""
        self.implPostamble = ""

        self.headerFileHandle = ""
        self.implFileHandle = ""

    def getMakefileSrcEntry(self):
        dirName = self.directory
        baseName = self.basename
        joined = os.path.join(dirName, baseName)
        return "    " + joined + ".cpp \\\n"

    def begin(self, globalDir):
        # Create subdirectory, if needed
        absDir = os.path.join(globalDir, self.directory)

        filename = os.path.join(absDir, self.basename)

        fpHeader = open(filename + ".h", "w", encoding="utf-8")
        fpImpl = open(filename + ".cpp", "w", encoding="utf-8")

        self.headerFileHandle = fpHeader
        self.implFileHandle = fpImpl

        self.headerFileHandle.write(self.headerPreamble)
        self.implFileHandle.write(self.implPreamble)

    def appendHeader(self, toAppend):
        self.headerFileHandle.write(toAppend)

    def appendImpl(self, toAppend):
        self.implFileHandle.write(toAppend)

    def end(self):
        self.headerFileHandle.write(self.headerPostamble)
        self.implFileHandle.write(self.implPostamble)

        self.headerFileHandle.close()
        self.implFileHandle.close()


class CodeGen(object):

    def __init__(self,):
        self.code = ""
        self.indentLevel = 0

    def swapCode(self,):
        res = "%s" % self.code
        self.code = ""
        return res

    def indent(self,):
        return "".join("    " * self.indentLevel)

    def beginBlock(self,):
        self.code += self.indent() + "{\n"
        self.indentLevel += 1

    def endBlock(self,):
        self.indentLevel -= 1
        self.code += self.indent() + "}\n"

    def beginIf(self, cond):
        self.code += self.indent() + "if (" + cond + ")\n"
        self.beginBlock()

    def beginElse(self, cond = None):
        if cond is not None:
            self.code += \
                self.indent() + \
                "else if (" + cond + ")\n"
        else:
            self.code += self.indent() + "else\n"
        self.beginBlock()

    def endElse(self):
        self.endBlock()

    def endIf(self):
        self.endBlock()

    def beginWhile(self, cond):
        self.code += self.indent() + "while (" + cond + ")\n"
        self.beginBlock()

    def endWhile(self):
        self.endBlock()

    def beginFor(self, initial, condition, increment):
        self.code += \
            self.indent() + "for (" + \
            "; ".join([initial, condition, increment]) + \
            ")\n"
        self.beginBlock()

    def endFor(self):
        self.endBlock()

    def stmt(self, code):
        self.code += self.indent() + code + ";\n"

    def line(self, code):
        self.code += self.indent() + code + "\n"

    def funcCall(self, lhs, funcName, parameters):
        res = self.indent()

        if lhs is not None:
            res += lhs + " = "

        res += funcName + "(%s);\n" % (", ".join(parameters))

        self.code += res

    # Given a VulkanType object, generate a C type declaration
    # with optional parameter name:
    # [const] [typename][*][const*] [paramName]
    def makeCTypeDecl(self, vulkanType, useParamName=True):
        constness = "const " if vulkanType.isConst else ""
        typeName = vulkanType.typeName

        if vulkanType.pointerIndirectionLevels == 0:
            ptrSpec = ""
        elif vulkanType.isPointerToConstPointer:
            ptrSpec = "* const*" if vulkanType.isConst else "**"
            if vulkanType.pointerIndirectionLevels > 2:
                ptrSpec += "*" * (vulkanType.pointerIndirectionLevels - 2)
        else:
            ptrSpec = "*" * vulkanType.pointerIndirectionLevels

        if useParamName and (vulkanType.paramName is not None):
            paramStr = (" " + vulkanType.paramName)
        else:
            paramStr = ""

        return "%s%s%s%s" % (constness, typeName, ptrSpec, paramStr)

    # Given a VulkanAPI object, generate the C function protype:
    # <returntype> <funcname>(<parameters>)
    def makeFuncProto(self, vulkanApi):

        protoBegin = "%s %s" % (self.makeCTypeDecl(
            vulkanApi.retType, useParamName=False), vulkanApi.name)
        protoParams = "(\n    %s)" % (",\n    ".join(
            list(map(self.makeCTypeDecl, vulkanApi.parameters))))

        return protoBegin + protoParams

    def makeFuncDecl(self, vulkanApi):
        return self.makeFuncProto(vulkanApi) + ";\n\n"

    def makeFuncImpl(self, vulkanApi, codegenFunc):
        self.swapCode()

        self.line(self.makeFuncProto(vulkanApi))
        self.beginBlock()
        codegenFunc(self)
        self.endBlock()

        return self.swapCode() + "\n"

    def makeStructAccess(self,
                         vulkanType,
                         structVarName,
                         asPtr=True,
                         structAsPtr=True,
                         accessIndex=None):

        deref = "->" if structAsPtr else "."

        indexExpr = (" + %s" % accessIndex) if accessIndex else ""

        addrOfExpr = "" if vulkanType.accessibleAsPointer() or (
            not asPtr) else "&"

        return "%s%s%s%s%s" % (addrOfExpr, structVarName, deref,
                               vulkanType.paramName, indexExpr)

    def makeRawLengthAccess(self, vulkanType):
        lenExpr = vulkanType.getLengthExpression()

        if not lenExpr:
            return None

        if lenExpr == "null-terminated":
            return "strlen(%s)" % vulkanType.paramName

        return lenExpr

    def makeLengthAccessFromStruct(self,
                                   structInfo,
                                   vulkanType,
                                   structVarName,
                                   asPtr=True):
        # Handle special cases first
        # Mostly when latexmath is involved
        def handleSpecialCases(structInfo, vulkanType, structVarName, asPtr):
            cases = [
                {
                    "structName": "VkShaderModuleCreateInfo",
                    "field": "pCode",
                    "lenExprMember": "codeSize",
                    "postprocess": lambda expr: "(%s / 4)" % expr
                },
                {
                    "structName": "VkPipelineMultisampleStateCreateInfo",
                    "field": "pSampleMask",
                    "lenExprMember": "rasterizationSamples",
                    "postprocess": lambda expr: "(((%s) + 31) / 32)" % expr
                },
            ]

            for c in cases:
                if (structInfo.name, vulkanType.paramName) == (c["structName"],
                                                               c["field"]):
                    deref = "->" if asPtr else "."
                    expr = "%s%s%s" % (structVarName, deref, c["lenExprMember"])
                    return c["postprocess"](expr)

            return None

        specialCaseAccess = \
            handleSpecialCases( \
                structInfo, vulkanType, structVarName, asPtr)

        if specialCaseAccess is not None:
            return specialCaseAccess

        lenExpr = vulkanType.getLengthExpression()

        if not lenExpr:
            return None

        deref = "->" if asPtr else "."

        if lenExpr == "null-terminated":
            return "strlen(%s%s%s)" % (structVarName, deref,
                                       vulkanType.paramName)

        if not structInfo.getMember(lenExpr):
            return self.makeRawLengthAccess(vulkanType)

        return "%s%s%s" % (structVarName, deref, lenExpr)

    def makeLengthAccessFromApi(self, api, vulkanType):
        # Handle special cases first
        # Mostly when :: is involved
        def handleSpecialCases(vulkanType):
            lenExpr = vulkanType.getLengthExpression()

            if lenExpr is None:
                return None

            if "::" in lenExpr:
                structVarName, memberVarName = lenExpr.split("::")
                return "%s->%s" % (structVarName, memberVarName)
            return None

        specialCaseAccess = handleSpecialCases(vulkanType)

        if specialCaseAccess is not None:
            return specialCaseAccess

        lenExpr = vulkanType.getLengthExpression()

        if not lenExpr:
            return None

        lenExprInfo = api.getParameter(lenExpr)

        if not lenExprInfo:
            return self.makeRawLengthAccess(vulkanType)

        if lenExpr == "null-terminated":
            return "strlen(%s)" % vulkanType.paramName()
        else:
            deref = "*" if lenExprInfo.pointerIndirectionLevels > 0 else ""
            return "(%s(%s))" % (deref, lenExpr)

    def accessParameter(self, param, asPtr=True):
        if asPtr:
            if param.pointerIndirectionLevels > 0:
                return param.paramName
            else:
                return "&%s" % param.paramName
        else:
            return param.paramName

    def sizeofExpr(self, vulkanType):
        return "sizeof(%s)" % (
            self.makeCTypeDecl(vulkanType, useParamName=False))

    def generalAccess(self,
                      vulkanType,
                      parentVarName="parent",
                      asPtr=True,
                      structAsPtr=True):
        if vulkanType.parent is None:
            return self.accessParameter(vulkanType, asPtr=asPtr)

        if isinstance(vulkanType.parent, VulkanCompoundType):
            return self.makeStructAccess(
                vulkanType, parentVarName, asPtr=asPtr, structAsPtr=structAsPtr)

        if isinstance(vulkanType.parent, VulkanAPI):
            return self.accessParameter(vulkanType, asPtr=asPtr)

        raise

    def generalLengthAccess(self, vulkanType, parentVarName="parent"):
        if vulkanType.parent is None:
            return self.makeRawLengthAccess(vulkanType)

        if isinstance(vulkanType.parent, VulkanCompoundType):
            return self.makeLengthAccessFromStruct(
                vulkanType.parent, vulkanType, parentVarName, asPtr=True)

        if isinstance(vulkanType.parent, VulkanAPI):
            return self.makeLengthAccessFromApi(vulkanType.parent, vulkanType)

        raise


# Class to wrap a Vulkan API call.
#
# The user gives a generic callback, |codegenDef|,
# that takes a CodeGen object and a VulkanAPI object as arguments.
# codegenDef uses CodeGen along with the VulkanAPI object
# to generate the function body.
class VulkanAPIWrapper(object):

    def __init__(self,
                 customApiPrefix,
                 extraParameters=None,
                 returnTypeOverride=None,
                 codegenDef=None):
        self.customApiPrefix = customApiPrefix
        self.extraParameters = extraParameters
        self.returnTypeOverride = returnTypeOverride

        self.codegen = CodeGen()

        self.definitionFunc = codegenDef

        # Private function
        def makeApiFunc(self, typeInfo, apiName):
            customApi = deepcopy(typeInfo.apis[apiName])
            customApi.name = self.customApiPrefix + customApi.name
            if self.extraParameters is not None:
                if isinstance(self.extraParameters, list):
                    customApi.parameters = \
                        self.extraParameters + customApi.parameters
                else:
                    raise

            if self.returnTypeOverride is not None:
                customApi.retType = self.returnTypeOverride
            return customApi

        self.makeApi = makeApiFunc

    def setCodegenDef(self, codegenDefFunc):
        self.definitionFunc = codegenDefFunc

    def makeDecl(self, typeInfo, apiName):
        return self.codegen.makeFuncProto(
            self.makeApi(self, typeInfo, apiName)) + ";\n\n"

    def makeDefinition(self, typeInfo, apiName, isStatic=False):
        vulkanApi = self.makeApi(self, typeInfo, apiName)

        self.codegen.swapCode()
        self.codegen.beginBlock()

        if self.definitionFunc is None:
            print("ERROR: No definition found for (%s, %s)" %
                  (vulkanApi.name, self.customApiPrefix))
            sys.exit(1)

        self.definitionFunc(self.codegen, vulkanApi)

        self.codegen.endBlock()

        return ("static " if isStatic else "") + self.codegen.makeFuncProto(
            vulkanApi) + "\n" + self.codegen.swapCode() + "\n"

# Base class for wrapping all Vulkan API objects.  These work with Vulkan
# Registry generators and have gen* triggers.  They tend to contain
# VulkanAPIWrapper objects to make it easier to generate the code.
class VulkanWrapperGenerator(object):

    def __init__(self, module, typeInfo):
        self.module = module
        self.typeInfo = typeInfo

    def onGenType(self, typeInfo, name, alias):
        pass

    def onGenStruct(self, typeInfo, typeName, alias):
        pass

    def onGenGroup(self, groupinfo, groupName, alias=None):
        pass

    def onGenEnum(self, enuminfo, name, alias):
        pass

    def onGenCmd(self, cmdinfo, name, alias):
        pass
