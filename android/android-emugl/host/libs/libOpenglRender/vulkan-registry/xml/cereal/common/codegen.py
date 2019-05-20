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

from .vulkantypes import VulkanType, VulkanTypeInfo, VulkanCompoundType, VulkanAPI

from copy import copy

import os
import sys

# Class capturing a .cpp file and a .h file (a "C++ module")
class Module(object):

    def __init__(self, directory, basename, customAbsDir = None, suppress = False):
        self.directory = directory
        self.basename = basename

        self.headerPreamble = ""
        self.implPreamble = ""

        self.headerPostamble = ""
        self.implPostamble = ""

        self.headerFileHandle = ""
        self.implFileHandle = ""

        self.customAbsDir = customAbsDir

        self.suppress = suppress

    def getMakefileSrcEntry(self):
        if self.customAbsDir:
            return self.basename + ".cpp \\\n"
        dirName = self.directory
        baseName = self.basename
        joined = os.path.join(dirName, baseName)
        return "    " + joined + ".cpp \\\n"

    def getCMakeSrcEntry(self):
        if self.customAbsDir:
            return self.basename + ".cpp "
        dirName = self.directory
        baseName = self.basename
        joined = os.path.join(dirName, baseName)
        return "    " + joined + ".cpp "

    def begin(self, globalDir):
        if self.suppress:
            return

        # Create subdirectory, if needed
        if self.customAbsDir:
            absDir = self.customAbsDir
        else:
            absDir = os.path.join(globalDir, self.directory)

        filename = os.path.join(absDir, self.basename)

        fpHeader = open(filename + ".h", "w", encoding="utf-8")
        fpImpl = open(filename + ".cpp", "w", encoding="utf-8")

        self.headerFileHandle = fpHeader
        self.implFileHandle = fpImpl

        self.headerFileHandle.write(self.headerPreamble)
        self.implFileHandle.write(self.implPreamble)

    def appendHeader(self, toAppend):
        if self.suppress:
            return

        self.headerFileHandle.write(toAppend)

    def appendImpl(self, toAppend):
        if self.suppress:
            return

        self.implFileHandle.write(toAppend)

    def end(self):
        if self.suppress:
            return

        self.headerFileHandle.write(self.headerPostamble)
        self.implFileHandle.write(self.implPostamble)

        self.headerFileHandle.close()
        self.implFileHandle.close()

# Class capturing a .proto protobuf definition file
class Proto(object):

    def __init__(self, directory, basename, customAbsDir = None, suppress = False):
        self.directory = directory
        self.basename = basename
        self.customAbsDir = customAbsDir

        self.preamble = ""
        self.postamble = ""

        self.suppress = suppress

    def getMakefileSrcEntry(self):
        if self.customAbsDir:
            return self.basename + ".proto \\\n"
        dirName = self.directory
        baseName = self.basename
        joined = os.path.join(dirName, baseName)
        return "    " + joined + ".proto \\\n"

    def getCMakeSrcEntry(self):
        if self.customAbsDir:
            return self.basename + ".proto "

        dirName = self.directory
        baseName = self.basename
        joined = os.path.join(dirName, baseName)
        return "    " + joined + ".proto "

    def begin(self, globalDir):
        if self.suppress:
            return

        # Create subdirectory, if needed
        if self.customAbsDir:
            absDir = self.customAbsDir
        else:
            absDir = os.path.join(globalDir, self.directory)

        filename = os.path.join(absDir, self.basename)

        fpProto = open(filename + ".proto", "w", encoding="utf-8")
        self.protoFileHandle = fpProto
        self.protoFileHandle.write(self.preamble)

    def append(self, toAppend):
        if self.suppress:
            return

        self.protoFileHandle.write(toAppend)

    def end(self):
        if self.suppress:
            return

        self.protoFileHandle.write(self.postamble)
        self.protoFileHandle.close()

class CodeGen(object):

    def __init__(self,):
        self.code = ""
        self.indentLevel = 0
        self.gensymCounter = 0

    def var(self, prefix="cgen_var"):
        res = "%s_%d" % (prefix, self.gensymCounter)
        self.gensymCounter += 1
        return res

    def swapCode(self,):
        res = "%s" % self.code
        self.code = ""
        return res

    def indent(self,extra=0):
        return "".join("    " * (self.indentLevel + extra))

    def incrIndent(self,):
        self.indentLevel += 1

    def decrIndent(self,):
        if self.indentLevel > 0:
            self.indentLevel -= 1

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

    def beginLoop(self, loopVarType, loopVar, loopInit, loopBound):
        self.beginFor(
            "%s %s = %s" % (loopVarType, loopVar, loopInit),
            "%s < %s" % (loopVar, loopBound),
            "++%s" % (loopVar))

    def endLoop(self):
        self.endBlock()

    def stmt(self, code):
        self.code += self.indent() + code + ";\n"

    def line(self, code):
        self.code += self.indent() + code + "\n"

    def leftline(self, code):
        self.code += code + "\n"

    def makeCallExpr(self, funcName, parameters):
        return funcName + "(%s)" % (", ".join(parameters))

    def funcCall(self, lhs, funcName, parameters):
        res = self.indent()

        if lhs is not None:
            res += lhs + " = "

        res += self.makeCallExpr(funcName, parameters) + ";\n"
        self.code += res

    def funcCallRet(self, _lhs, funcName, parameters):
        res = self.indent()
        res += "return " + self.makeCallExpr(funcName, parameters) + ";\n"
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

    def makeRichCTypeDecl(self, vulkanType, useParamName=True):
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

        if vulkanType.staticArrExpr:
            staticArrInfo = "[%s]" % vulkanType.staticArrExpr
        else:
            staticArrInfo = ""

        return "%s%s%s%s%s" % (constness, typeName, ptrSpec, paramStr, staticArrInfo)

    # Given a VulkanAPI object, generate the C function protype:
    # <returntype> <funcname>(<parameters>)
    def makeFuncProto(self, vulkanApi, useParamName=True):

        protoBegin = "%s %s" % (self.makeCTypeDecl(
            vulkanApi.retType, useParamName=False), vulkanApi.name)

        def getFuncArgDecl(param):
            if param.staticArrExpr:
                return self.makeCTypeDecl(param, useParamName=useParamName) + ("[%s]" % param.staticArrExpr)
            else:
                return self.makeCTypeDecl(param, useParamName=useParamName)

        protoParams = "(\n    %s)" % ((",\n%s" % self.indent(1)).join(
            list(map(
                getFuncArgDecl,
                vulkanApi.parameters))))

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

    def emitFuncImpl(self, vulkanApi, codegenFunc):
        self.line(self.makeFuncProto(vulkanApi))
        self.beginBlock()
        codegenFunc(self)
        self.endBlock()

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
                      parentVarName=None,
                      asPtr=True,
                      structAsPtr=True):
        if vulkanType.parent is None:
            if parentVarName is None:
                return self.accessParameter(vulkanType, asPtr=asPtr)
            else:
                return self.accessParameter(vulkanType.withModifiedName(parentVarName), asPtr=asPtr)

        if isinstance(vulkanType.parent, VulkanCompoundType):
            return self.makeStructAccess(
                vulkanType, parentVarName, asPtr=asPtr, structAsPtr=structAsPtr)

        if isinstance(vulkanType.parent, VulkanAPI):
            if parentVarName is None:
                return self.accessParameter(vulkanType, asPtr=asPtr)
            else:
                return self.accessParameter(vulkanType.withModifiedName(parentVarName), asPtr=asPtr)

        os.abort("Could not find a way to access Vulkan type %s" %
                 vulkanType.name)


    def generalLengthAccess(self, vulkanType, parentVarName="parent"):
        if vulkanType.parent is None:
            return self.makeRawLengthAccess(vulkanType)

        if isinstance(vulkanType.parent, VulkanCompoundType):
            return self.makeLengthAccessFromStruct(
                vulkanType.parent, vulkanType, parentVarName, asPtr=True)

        if isinstance(vulkanType.parent, VulkanAPI):
            return self.makeLengthAccessFromApi(vulkanType.parent, vulkanType)

        os.abort("Could not find a way to access length of Vulkan type %s" %
                 vulkanType.name)

    def vkApiCall(self, api, customPrefix="", customParameters=None, retVarDecl=True):
        callLhs = None

        retTypeName = api.getRetTypeExpr()
        retVar = None

        if retTypeName != "void":
            retVar = api.getRetVarExpr()
            if retVarDecl:
                self.stmt("%s %s = (%s)0" % (retTypeName, retVar, retTypeName))
            callLhs = retVar

        if customParameters is None:
            self.funcCall(
            callLhs, customPrefix + api.name, [p.paramName for p in api.parameters])
        else:
            self.funcCall(
                callLhs, customPrefix + api.name, customParameters)

        return (retTypeName, retVar)

    def makeCheckVkSuccess(self, expr):
        return "((%s) == VK_SUCCESS)" % expr

    def makeReinterpretCast(self, varName, typeName, const=True):
        return "reinterpret_cast<%s%s*>(%s)" % \
               ("const " if const else "", typeName, varName)

    def validPrimitive(self, typeInfo, typeName):
        size = typeInfo.getPrimitiveEncodingSize(typeName)
        return size != None

    def makePrimitiveStreamMethod(self, typeInfo, typeName, direction="write"):
        if not self.validPrimitive(typeInfo, typeName):
            return None

        size = typeInfo.getPrimitiveEncodingSize(typeName)
        prefix = "put" if direction == "write" else "get"
        suffix = None
        if size == 1:
            suffix = "Byte"
        elif size == 2:
            suffix = "Be16"
        elif size == 4:
            suffix = "Be32"
        elif size == 8:
            suffix = "Be64"

        if suffix:
            return prefix + suffix

        return None

    def streamPrimitive(self, typeInfo, streamVar, accessExpr, accessType, direction="write"):
        accessTypeName = accessType.typeName

        if accessType.pointerIndirectionLevels == 0 and not self.validPrimitive(typeInfo, accessTypeName):
            print("Tried to stream a non-primitive type: %s" % accessTypeName)
            os.abort()

        needPtrCast = False

        if accessType.pointerIndirectionLevels > 0:
            streamSize = 8
            streamStorageVarType = "uint64_t"
            needPtrCast = True
            streamMethod = "putBe64" if direction == "write" else "getBe64"
        else:
            streamSize = typeInfo.getPrimitiveEncodingSize(accessTypeName)
            if streamSize == 1:
                streamStorageVarType = "uint8_t"
            elif streamSize == 2:
                streamStorageVarType = "uint16_t"
            elif streamSize == 4:
                streamStorageVarType = "uint32_t"
            elif streamSize == 8:
                streamStorageVarType = "uint64_t"
            streamMethod = self.makePrimitiveStreamMethod(
                typeInfo, accessTypeName, direction=direction)

        streamStorageVar = self.var()

        accessCast = self.makeRichCTypeDecl(accessType, useParamName=False)

        ptrCast = "(uintptr_t)" if needPtrCast else ""

        if direction == "read":
            self.stmt("%s = (%s)%s%s->%s()" % \
                (accessExpr,
                 accessCast,
                 ptrCast,
                 streamVar,
                 streamMethod))
        else:
            self.stmt("%s %s = (%s)%s%s" %
                      (streamStorageVarType, streamStorageVar,
                       streamStorageVarType, ptrCast, accessExpr))
            self.stmt("%s->%s(%s)" % (streamVar, streamMethod, streamStorageVar))


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
            customApi = copy(typeInfo.apis[apiName])
            customApi.name = self.customApiPrefix + customApi.name
            if self.extraParameters is not None:
                if isinstance(self.extraParameters, list):
                    customApi.parameters = \
                        self.extraParameters + customApi.parameters
                else:
                    os.abort(
                        "Type of extra parameters to custom API not valid. Expected list, got %s" % type(
                            self.extraParameters))

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
        self.extensionStructTypes = []

    def onBegin(self):
        pass

    def onEnd(self):
        pass

    def onBeginFeature(self, featureName):
        pass

    def onEndFeature(self):
        pass

    def onGenType(self, typeInfo, name, alias):
        category = self.typeInfo.categoryOf(name)
        if category in ["struct", "union"] and not alias:
            structInfo = self.typeInfo.structs[name]
            if structInfo.structExtendsExpr:
                self.extensionStructTypes.append(structInfo)
        pass

    def onGenStruct(self, typeInfo, name, alias):
        pass

    def onGenGroup(self, groupinfo, groupName, alias=None):
        pass

    def onGenEnum(self, enuminfo, name, alias):
        pass

    def onGenCmd(self, cmdinfo, name, alias):
        pass

    def emitForEachStructExtension(self, cgen, retType, triggerVar, forEachFunc, autoBreak=True):
        def readStructType(structTypeName, structVarName, cgen):
            cgen.stmt("uint32_t %s = (uint32_t)%s(%s)" % \
                (structTypeName, "goldfish_vk_struct_type", structVarName))

        def castAsStruct(varName, typeName, const=True):
            return "reinterpret_cast<%s%s*>(%s)" % \
                   ("const " if const else "", typeName, varName)

        def doDefaultReturn(cgen):
            if retType.typeName == "void":
                cgen.stmt("return")
            else:
                cgen.stmt("return (%s)0" % retType.typeName)

        cgen.beginIf("!%s" % triggerVar.paramName)
        doDefaultReturn(cgen)
        cgen.endIf()

        readStructType("structType", triggerVar.paramName, cgen)

        cgen.line("switch(structType)")
        cgen.beginBlock()

        currFeature = None

        for ext in self.extensionStructTypes:
            if not currFeature:
                cgen.leftline("#ifdef %s" % ext.feature)
                currFeature = ext.feature

            if currFeature and ext.feature != currFeature:
                cgen.leftline("#endif")
                cgen.leftline("#ifdef %s" % ext.feature)
                currFeature = ext.feature

            enum = ext.structEnumExpr
            cgen.line("case %s:" % enum)
            cgen.beginBlock()

            castedAccess = castAsStruct(
                triggerVar.paramName, ext.name, const=triggerVar.isConst)
            forEachFunc(ext, castedAccess, cgen)

            if autoBreak:
                cgen.stmt("break")
            cgen.endBlock()

        if currFeature:
            cgen.leftline("#endif")

        cgen.line("default:")
        cgen.beginBlock()
        doDefaultReturn(cgen)
        cgen.endBlock()

        cgen.endBlock()

    def emitForEachStructExtensionGeneral(self, cgen, forEachFunc, doFeatureIfdefs=False):
        currFeature = None

        for (i, ext) in enumerate(self.extensionStructTypes):
            if doFeatureIfdefs:
                if not currFeature:
                    cgen.leftline("#ifdef %s" % ext.feature)
                    currFeature = ext.feature

                if currFeature and ext.feature != currFeature:
                    cgen.leftline("#endif")
                    cgen.leftline("#ifdef %s" % ext.feature)
                    currFeature = ext.feature

            forEachFunc(i, ext, cgen)

        if doFeatureIfdefs:
            if currFeature:
                cgen.leftline("#endif")
