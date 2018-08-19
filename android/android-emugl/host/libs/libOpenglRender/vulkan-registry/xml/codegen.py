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

from vulkantypes import *

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

    def beginElse(self, cond):
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
        self.code += self.indent() + "for (" + "; ".join([initial, condition, increment]) + ")\n"
        self.beginBlock()

    def endFor(self, initial, condition, increment):
        self.endBlock()

    def stmt(self, code):
        self.code += self.indent() + code + ";\n"

    def line(self, code):
        self.code += self.indent() + code + "\n"

    # Given a VulkanType object, generate a C type declaration with optional parameter name:
    # [const] [typename][*][const*] [paramName]
    def makeCTypeDecl(self, vulkanType, useParamName = True):
        constness = "const " if vulkanType.isConst else ""
        typeName = vulkanType.typeName

        if vulkanType.pointerIndirectionLevels == 0:
            ptrSpec = ""
        elif vulkanType.isPointerToConstPointer:
            ptrSpec = "* const*"
        else:
            ptrSpec = "*" * vulkanType.pointerIndirectionLevels

        paramStr = (" " + vulkanType.paramName) if useParamName and (vulkanType.paramName is not None) else ""

        return "%s%s%s%s" % (constness, typeName, ptrSpec, paramStr)

    # Given a VulkanAPI object, generate the C function protype:
    # <returntype> <funcname>(<comma-separated parameters each with <type> <name> >)
    def makeFuncProto(self, vulkanApi):

        protoBegin = "%s %s" % (self.makeCTypeDecl(vulkanApi.retType, useParamName = False), vulkanApi.name)
        protoParams = "(\n    %s)" % (",\n    ".join(list(map(self.makeCTypeDecl, vulkanApi.parameters))))

        return protoBegin + protoParams + "\n"

# TODO: next: Class for generating Vulkan API wrappers.
