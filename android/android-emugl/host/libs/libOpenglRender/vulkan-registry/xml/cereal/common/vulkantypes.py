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

from generator import noneStr

from copy import copy

# Holds information about core Vulkan objects
# and the API calls that are used to create/destroy each one.
class HandleInfo(object):
    def __init__(self, name, createApis, destroyApis):
        self.name = name
        self.createApis = createApis
        self.destroyApis = destroyApis

    def isCreateApi(self, apiName):
        return apiName == self.createApis or (apiName in self.createApis)

    def isDestroyApi(self, apiName):
        if self.destroyApis is None:
            return False
        return apiName == self.destroyApis or (apiName in self.destroyApis)

DISPATCHABLE_HANDLE_TYPES = [
    "VkInstance",
    "VkPhysicalDevice",
    "VkDevice",
    "VkQueue",
    "VkCommandBuffer",
]

NON_DISPATCHABLE_HANDLE_TYPES = [
    "VkDeviceMemory",
    "VkBuffer",
    "VkBufferView",
    "VkImage",
    "VkImageView",
    "VkShaderModule",
    "VkDescriptorPool",
    "VkDescriptorSetLayout",
    "VkDescriptorSet",
    "VkSampler",
    "VkPipelineLayout",
    "VkRenderPass",
    "VkFramebuffer",
    "VkPipelineCache",
    "VkCommandPool",
    "VkFence",
    "VkSemaphore",
    "VkEvent",
    "VkQueryPool",
    "VkSamplerYcbcrConversion",
    "VkDescriptorUpdateTemplate",
    "VkSurfaceKHR",
    "VkSwapchainKHR",
    "VkDisplayKHR",
    "VkDisplayModeKHR",
    "VkObjectTableNVX",
    "VkIndirectCommandsLayoutNVX",
    "VkValidationCacheEXT",
    "VkDebugReportCallbackEXT",
    "VkDebugUtilsMessengerEXT",
]

CUSTOM_HANDLE_CREATE_TYPES = [
    "VkQueue",
    "VkPipeline",
    "VkDeviceMemory",
    "VkDescriptorSet",
    "VkCommandBuffer",
]

HANDLE_TYPES = list(sorted(list(set(DISPATCHABLE_HANDLE_TYPES +
                                    NON_DISPATCHABLE_HANDLE_TYPES + CUSTOM_HANDLE_CREATE_TYPES))))

HANDLE_INFO = {}

for h in HANDLE_TYPES:
    if h in CUSTOM_HANDLE_CREATE_TYPES:
        if h == "VkQueue":
            HANDLE_INFO[h] = \
                HandleInfo(
                    "VkQueue",
                    "vkGetDeviceQueue", None)
        if h == "VkPipeline":
            HANDLE_INFO[h] = \
                HandleInfo(
                    "VkPipeline",
                    ["vkCreateGraphicsPipelines", "vkCreateComputePipelines"],
                    "vkDestroyPipeline")
        if h == "VkDeviceMemory":
            HANDLE_INFO[h] = \
                HandleInfo("VkDeviceMemory",
                           "vkAllocateMemory", "vkFreeMemory")
        if h == "VkDescriptorSet":
            HANDLE_INFO[h] = \
                HandleInfo("VkDescriptorSet", "vkAllocateDescriptorSets",
                           "vkFreeDescriptorSets")
        if h == "VkCommandBuffer":
            HANDLE_INFO[h] = \
                HandleInfo("VkCommandBuffer", "vkAllocateCommandBuffers",
                           "vkFreeCommandBuffers")
    else:
        HANDLE_INFO[h] = \
            HandleInfo(h, "vkCreate" + h[2:], "vkDestroy" + h[2:])

# Holds information about a Vulkan type instance (i.e., not a type definition).
# Type instances are used as struct field definitions or function parameters,
# to be later fed to code generation.
# VulkanType instances can be constructed in two ways:
# 1. From an XML tag with <type> / <param> tags in vk.xml,
#    using makeVulkanTypeFromXMLTag
# 2. User-defined instances with makeVulkanTypeSimple.
class VulkanType(object):

    def __init__(self):
        self.parent = None
        self.typeName = ""

        self.paramName = None

        self.lenExpr = None
        self.isOptional = False

        self.isConst = False

        self.staticArrExpr = ""  # "" means not static array
        # 0 means the array size is not a numeric literal
        self.staticArrCount = 0
        self.pointerIndirectionLevels = 0  # 0 means not pointer
        self.isPointerToConstPointer = False

    def __str__(self,):
        return ("(vulkantype %s %s paramName %s len %s optional? %s "
                "staticArrExpr %s %s)") % (
            self.typeName + ("*" * self.pointerIndirectionLevels) +
            ("ptr2constptr" if self.isPointerToConstPointer else ""), "const"
            if self.isConst else "nonconst", self.paramName, self.lenExpr,
            self.isOptional, self.staticArrExpr, self.staticArrCount)

    def isString(self,):
        return self.pointerIndirectionLevels == 1 and (self.typeName == "char")

    def isArrayOfStrings(self,):
        return self.isPointerToConstPointer and (self.typeName == "char")

    # Utility functions to make codegen life easier.
    # This method derives the correct "count" expression if possible.
    # Otherwise, returns None or "null-terminated" if a string.
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

    def isVoidWithNoSize(self,):
        return self.typeName == "void" and self.pointerIndirectionLevels == 0

    def getCopy(self,):
        return copy(self)

    def getTransformed(self, isConstChoice=None, ptrIndirectionChoice=None):
        res = self.getCopy()

        if isConstChoice is not None:
            res.isConst = isConstChoice
        if ptrIndirectionChoice is not None:
            res.pointerIndirectionLevels = ptrIndirectionChoice

        return res

    def getWithCustomName(self):
        return self.getTransformed(
            ptrIndirectionChoice=self.pointerIndirectionLevels + 1)

    def getForAddressAccess(self):
        return self.getTransformed(
            ptrIndirectionChoice=self.pointerIndirectionLevels + 1)

    def getForValueAccess(self):
        if self.typeName == "void" and self.pointerIndirectionLevels == 1:
            asUint8Type = self.getCopy()
            asUint8Type.typeName = "uint8_t"
            return asUint8Type.getForValueAccess()
        return self.getTransformed(
            ptrIndirectionChoice=self.pointerIndirectionLevels - 1)

    def getForNonConstAccess(self):
        return self.getTransformed(isConstChoice=False)

    def withModifiedName(self, newName):
        res = self.getCopy()
        res.paramName = newName
        return res

    def isNextPointer(self):
        if self.paramName == "pNext":
            return True
        return False

    # Only deals with 'core' handle types here.
    def isDispatchableHandleType(self):
        return self.typeName in DISPATCHABLE_HANDLE_TYPES

    def isNonDispatchableHandleType(self):
        return self.typeName in NON_DISPATCHABLE_HANDLE_TYPES

    def isHandleType(self):
        return self.isDispatchableHandleType() or \
               self.isNonDispatchableHandleType()

    def isCreatedBy(self, api):
        if self.typeName in HANDLE_INFO.keys():
            return HANDLE_INFO[self.typeName].isCreateApi(api.name)
        return False

    def isDestroyedBy(self, api):
        if self.typeName in HANDLE_INFO.keys():
            return HANDLE_INFO[self.typeName].isDestroyApi(api.name)
        return False

    def isSimpleValueType(self, typeInfo):
        if typeInfo.isCompoundType(self.typeName):
            return False
        if self.isString() or self.isArrayOfStrings():
            return False
        if self.staticArrExpr or self.pointerIndirectionLevels > 0:
            return False
        return True

def makeVulkanTypeFromXMLTag(tag):
    res = VulkanType()
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

    # Calculate optionality (based on validitygenerator.py)
    if tag.attrib.get("optional") is not None:
        res.isOptional = True

    # If no validity is being generated, it usually means that
    # validity is complex and not absolute, so let's say yes.
    if tag.attrib.get("noautovalidity") is not None:
        res.isOptional = True

    # If this is a structure extension, it is optional.
    if tag.attrib.get("structextends") is not None:
        res.isOptional = True

    # If this is a pNext pointer, it is optional.
    if res.paramName == "pNext":
        res.isOptional = True

    return res


def makeVulkanTypeSimple(isConst,
                         typeName,
                         ptrIndirectionLevels,
                         paramName=None):
    res = VulkanType()

    res.typeName = typeName
    res.isConst = isConst
    res.pointerIndirectionLevels = ptrIndirectionLevels
    res.isPointerToConstPointer = False
    res.paramName = paramName

    return res

# Classes for describing aggregate types (unions, structs) and API calls.
class VulkanCompoundType(object):

    def __init__(self, name, members, isUnion=False):
        self.isUnion = isUnion
        self.name = name
        self.members = members

        self.copy = None

    def initCopies(self):
        self.copy = self

        for m in self.members:
            m.parent = self.copy

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

        self.copy = None

    def initCopies(self):
        self.copy = self

        for m in self.parameters:
            m.parent = self.copy

    def getParameter(self, parameterName):
        for p in self.parameters:
            if p.paramName == parameterName:
                return p
        return None

    def withModifiedName(self, newName):
        res = VulkanAPI(newName, self.retType, self.parameters)
        return res

    def getRetVarExpr(self):
        if self.retType == "void":
            return None
        return "%s_%s_return" % (self.name, self.retType.typeName)

    def getRetTypeExpr(self):
        return self.retType.typeName

# Parses everything about Vulkan types into a Python readable format.
class VulkanTypeInfo(object):

    def __init__(self,):
        self.typeCategories = {}

        self.structs = {}

        self.enums = {}

        self.apis = {}

    def categoryOf(self, name):
        return self.typeCategories[name]

    def isHandleType(self, vulkantype):
        name = vulkantype.typeName
        if name in self.typeCategories:
            return self.typeCategories[name] == "handle"
        return False

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
                VulkanCompoundType( \
                    typeName,
                    members,
                    isUnion = self.categoryOf(typeName) == "union")
            self.structs[typeName].initCopies()

    def onGenGroup(self, _groupinfo, groupName, _alias=None):
        self.enums[groupName] = 1

    def onGenEnum(self, _enuminfo, name, _alias):
        self.enums[name] = 1

    def onGenCmd(self, cmdinfo, name, _alias):
        self.typeCategories[name] = "api"

        proto = cmdinfo.elem.find("proto")
        params = cmdinfo.elem.findall("param")

        self.apis[name] = \
            VulkanAPI(
                name,
                makeVulkanTypeFromXMLTag(proto),
                list(map(makeVulkanTypeFromXMLTag, params)))
        self.apis[name].initCopies()


# General function to iterate over a vulkan type and call code that processes
# each of its sub-components, if any.
def iterateVulkanType(typeInfo, vulkanType, forEachType):
    if not vulkanType.isArrayOfStrings():
        if vulkanType.isPointerToConstPointer:
            return False

    needCheck = \
        vulkanType.isOptional and \
        vulkanType.pointerIndirectionLevels > 0

    if typeInfo.isCompoundType(vulkanType.typeName):

        if needCheck:
            forEachType.onCheck(vulkanType)

        forEachType.onCompoundType(vulkanType)

        if needCheck:
            forEachType.endCheck(vulkanType)

    else:

        if vulkanType.isString():

            forEachType.onString(vulkanType)

        elif vulkanType.isArrayOfStrings():

            forEachType.onStringArray(vulkanType)

        elif vulkanType.staticArrExpr:

            forEachType.onStaticArr(vulkanType)

        elif vulkanType.pointerIndirectionLevels > 0:
            if needCheck:
                forEachType.onCheck(vulkanType)
            forEachType.onPointer(vulkanType)
            if needCheck:
                forEachType.endCheck(vulkanType)
        else:

            forEachType.onValue(vulkanType)

    return True
