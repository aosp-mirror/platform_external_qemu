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
# Type instances are used as struct field definitions or function parameters,
# to be later fed to code generation.
# VulkanType instances can be constructed in two ways:
# 1. From an XML tag with <type> / <param> tags in vk.xml, using makeVulkanTypeFromXMLTag
# 2. User-defined instances with makeVulkanTypeSimple.
class VulkanType(object):
    def __init__(self):
        pass

    def __str__(self,):
        return "(vulkantype %s %s paramName %s len %s optional? %s staticArrExpr %s %s)" % (self.typeName + ("*" * self.pointerIndirectionLevels) + ("ptr2constptr" if self.isPointerToConstPointer else ""), "const" if self.isConst else "nonconst", self.paramName, self.lenExpr, self.isOptional, self.staticArrExpr, self.staticArrCount)

    # Utility functions to make codegen life easier.
    # This method derives the correct "count" expression
    def getLengthExpression(self,):
        if self.staticArrExpr != "":
            return self.staticArrExpr
        if self.lenExpr:
            return self.lenExpr
        return None

    # Can we just pass this to functions expecting T*
    def accessibleAsPointer(self,):
        if self.staticArrExpr != "":
            return True
        if self.pointerIndirectionLevels > 0:
            return True
        return False

    # Rough attempt to infer where a type could be an output.
    # Good for inferring which things need to be marshaled in
    # versus marshaled out for Vulkan API calls
    def possiblyOutput(self,):
        return self.pointerIndirectionLevels > 0 and (not self.isConst)


def makeVulkanTypeFromXMLTag(tag):
    res = VulkanType()
    res.typeName = ""

    res.paramName = None

    res.lenExpr = None
    res.isOptional = False

    res.isConst = False

    res.staticArrExpr = "" # "" means not static array
    res.staticArrCount = 0 # 0 means this static array size is not specified as a numeric literal
    res.pointerIndirectionLevels = 0 # 0 means not pointer
    res.isPointerToConstPointer = False

    # Process the length expression

    if tag.attrib.get("len") is not None:
        lengths = tag.attrib.get("len").split(",")
        res.lenExpr = lengths[0]

    # Calculate static array expression

    nametag = tag.find("name")
    enumtag = tag.find("enum")

    if enumtag is not None:
        res.staticArrExpr = enumtag.text
    elif nametag is not None:
        res.staticArrExpr = noneStr(nametag.tail)[1:-1]
        if res.staticArrExpr != "":
            res.staticArrCount = int(res.staticArrExpr)

    # Determine const

    beforeTypePart = noneStr(tag.text)

    if "const" in beforeTypePart:
        res.isConst = True

    # Calculate type and pointer info

    for elem in tag:
        if elem.tag == "name":
            res.paramName = elem.text
        if elem.tag == "type":
            duringTypePart = noneStr(elem.text)
            afterTypePart = noneStr(elem.tail)
            # Now we know enough to fill some stuff in
            res.typeName = duringTypePart

            # This only handles pointerIndirectionLevels == 2
            # along with optional constant pointer for the inner part.
            for c in afterTypePart:
                if c == "*":
                    res.pointerIndirectionLevels += 1
            if "const" in afterTypePart and res.pointerIndirectionLevels == 2:
                res.isPointerToConstPointer = True

            # If void*, treat like it's not a pointer
            # if duringTypePart == "void":
                # res.pointerIndirectionLevels -= 1

    # Calculate optionality
    if tag.attrib.get("optional") is not None:
        res.isOptional = True

    return res

def makeVulkanTypeSimple(isConst, typeName, ptrIndirectionLevels, paramName = None):
    res = VulkanType()

    res.typeName = typeName
    res.isConst = isConst
    res.pointerIndirectionLevels = ptrIndirectionLevels
    res.isPointerToConstPointer = False
    res.paramName = paramName

    # Default to no length expression, not optional, not a static array
    res.lenExpr = None
    res.isOptional = False

    res.staticArrExpr = "" # "" means not static array
    res.staticArrCount = 0 # 0 means this static array size is not specified as a numeric literal

    return res

# Classes for describing aggregate types (unions, structs) and API calls.
class VulkanCompoundType(object):
    def __init__(self, name, members, isUnion = False):
        self.isUnion = isUnion
        self.name = name
        self.members = members

    def getMember(self, memberName):
        for m in self.members:
            if m.paramName == memberName:
                return m
        return None

class VulkanAPI(object):
    def __init__(self, name, retType, parameters):
        self.name = name
        self.origName = name
        self.retType = retType
        self.parameters = parameters

    def getParameter(self, parameterName):
        for p in self.parameters:
            if p.paramName == parameterName:
                return p
        return None

# Parses everything about Vulkan types into a Python readable format.
class VulkanTypeInfo(object):
    def __init__(self,):
        self.typeCategories = {}

        self.structs = {}

        self.enums = {}

        self.apis = {}

    def categoryOf(self, name):
        return self.typeCategories[name]

    def isCompoundType(self, name):
        if name in self.typeCategories:
            return self.typeCategories[name] in ["struct", "union"]
        else:
            return False

    def onGenType(self, typeinfo, name, alias):
        category = typeinfo.elem.get("category")
        self.typeCategories[name] = category

        if category in ["struct", "union"]:
            self.onGenStruct(typeinfo, name, alias)

    def onGenStruct(self, typeinfo, typeName, alias):
        if not alias:
            members = []
            for member in typeinfo.elem.findall(".//member"):
                members.append(makeVulkanTypeFromXMLTag(member))

            self.structs[typeName] = \
                VulkanCompoundType(typeName, members, isUnion = self.categoryOf(typeName) == "union")

    def onGenGroup(self, groupinfo, groupName, alias = None):
        self.enums[groupName] = 1

    def onGenEnum(self, enuminfo, name, alias):
        self.enums[name] = 1

    def onGenCmd(self, cmdinfo, name, alias):
        self.typeCategories[name] = "api"

        proto = cmdinfo.elem.find("proto")
        params = cmdinfo.elem.findall("param")

        self.apis[name] = \
            VulkanAPI(
                name,
                makeVulkanTypeFromXMLTag(proto),
                list(map(makeVulkanTypeFromXMLTag, params)))
