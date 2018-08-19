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

from generator import *

# Holds information about a Vulkan type instance (i.e., not a type definition).
# Type instances are used as struct field definitions or function parameters.
class VulkanType(object):
    def __init__(self, tag):
        self.typeName = ""

        self.paramName = None

        self.lenExpr = None
        self.isOptional = False

        self.isConst = False

        self.staticArrExpr = "" # "" means not static array
        self.staticArrCount = 0 # 0 means this static array size is not specified as a numeric literal
        self.pointerIndirectionLevels = 0 # 0 means not pointer
        self.isPointerToConstPointer = False

        # Process the length expression

        if tag.attrib.get('len') is not None:
            lengths = tag.attrib.get('len').split(',')
            self.lenExpr = lengths[0]

        # Calculate static array expression

        nametag = tag.find('name')
        enumtag = tag.find('enum')

        if enumtag is not None:
            self.staticArrExpr = enumtag.text
        elif nametag is not None:
            self.staticArrExpr = noneStr(nametag.tail)[1:-1]
            if self.staticArrExpr != "":
                self.staticArrCount = int(self.staticArrExpr)

        # Determine const

        beforeTypePart = noneStr(tag.text)

        if "const" in beforeTypePart:
            self.isConst = True

        # Calculate type and pointer info

        for elem in tag:
            if elem.tag == "name":
                self.paramName = elem.text
            if elem.tag == "type":
                duringTypePart = noneStr(elem.text)
                afterTypePart = noneStr(elem.tail)
                # Now we know enough to fill some stuff in
                self.typeName = duringTypePart
              
                # This only handles pointerIndirectionLevels == 2
                # along with optional constant pointer for the inner part.
                for c in afterTypePart:
                    if c == "*":
                        self.pointerIndirectionLevels += 1
                if "const" in afterTypePart and self.pointerIndirectionLevels == 2:
                    self.isPointerToConstPointer = True

                # If void*, treat like it's not a pointer
                if duringTypePart == 'void':
                    self.pointerIndirectionLevels -= 1

        # Calculate optionality

        if tag.attrib.get('optional') is not None:
            self.isOptional = True

    def __str__(self,):
        return "(vulkantype %s %s paramName %s len %s optional? %s staticArrExpr %s %s)" % (self.typeName + ("*" * self.pointerIndirectionLevels) + ("ptr2constptr" if self.isPointerToConstPointer else ""), "const" if self.isConst else "nonconst", self.paramName, self.lenExpr, self.isOptional, self.staticArrExpr, self.staticArrCount)

class VulkanStruct(object):
    def __init__(self, name, members):
        self.name = name
        self.members = members

class VulkanAPI(object):
    def __init__(self, name, retType, parameters):
        self.name = name
        self.retType = retType
        self.parameters = parameters

# Parses everything about Vulkan types into a Python readable format.
# Meant to be used with generators:
# 
class VulkanTypeInfo(object):
    def __init__(self,):
        self.typeCategories = {}

    def onGenType(self, typeinfo, name, alias):
        typeElem = typeinfo.elem
        category = typeElem.get('category')
        self.typeCategories[name] = category
        if category == 'struct' or category == 'union':
            self.onGenStruct(typeinfo, name, alias)

    def onGenStruct(self, typeinfo, typeName, alias):
        pass
        # if not alias:
        #     for member in typeElem.findall('.//member'):
        #         self.forEachModule(lambda m: m.appendHeader("// genStructOrUnionField: %s" % self.makeCParamDecl(member, 0)))
        #         testObj = vt.VulkanType(member)
        #         self.forEachModule(lambda m: m.appendHeader("/*\n"))
        #         self.forEachModule(lambda m: m.appendHeader("%s\n" % str(testObj)))
        #         self.forEachModule(lambda m: m.appendHeader("*/\n"))


    def onGenGroup(self, groupinfo, groupName, alias = None):
        pass
        # OutputGenerator.onGenGroup(self, groupinfo, groupName, alias)
        # self.forEachModule(lambda m: m.appendHeader("// onGenGroup for %s" % groupName))

    def onGenEnum(self, enuminfo, name, alias):
        pass
        # self.forEachModule(lambda m: m.appendHeader("// onGenEnum for %s" % name))

    def onGenCmd(self, cmdinfo, name, alias):
        pass
        # frontendDecl = self.makeFrontendProto(cmdinfo.elem, name) + ";\n"

        # validateDef = self.makeValidateDef(cmdinfo.elem, name)
        # frontendDef = self.makeFrontendDef(cmdinfo.elem, name)

        # self.modules["goldfish_vk_frontend"].appendHeader(frontendDecl)
        # self.modules["goldfish_vk_frontend"].appendImpl(validateDef)
        # self.modules["goldfish_vk_frontend"].appendImpl(frontendDef)

        # encoderDecl = self.makeEncodeProto(cmdinfo.elem, name) + ";\n"
        # encoderDef = self.makeEncoderDef(cmdinfo.elem, name)

        # self.modules["goldfish_vk_encoder"].appendHeader(encoderDecl)
        # self.modules["goldfish_vk_encoder"].appendImpl(encoderDef)

        # marshalingDecl = self.makeMarshalingProto(cmdinfo.elem, name) + ";\n"
        # marshalingDef = self.makeMarshalingDef(cmdinfo.elem, name) + ";\n"

        # self.modules["goldfish_vk_marshaling"].appendHeader(marshalingDecl)
        # self.modules["goldfish_vk_marshaling"].appendImpl(marshalingDef)

