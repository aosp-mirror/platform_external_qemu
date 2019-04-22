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

class VulkanBaseProtoDefs(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.codegen = CodeGen()

        self.knownStructs = {}

        self.apis = []

    def onBegin(self,):
        VulkanWrapperGenerator.onBegin(self)

    def onGenType(self, typeXml, name, alias):
        VulkanWrapperGenerator.onGenType(self, typeXml, name, alias)

        if name in self.knownStructs:
            return

        category = self.typeInfo.categoryOf(name)

        if category in ["struct", "union"] and not alias:
            self.knownStructs[name] = 1

            structInfo = self.typeInfo.structs[name]

            def messageGenerator(cgen):
                cgen.line("message %s" % name)
                cgen.beginBlock()
                for (i, member) in enumerate(structInfo.members):
                    protoInfo = VulkanTypeProtobufInfo(self.typeInfo, structInfo, member)
                    self.codegen.line("// Original field: %s %s %s. stringarray? %d string? %d hasLenInfo? %d" %
                        (protoInfo.origTypeCategory, member.typeName, member.paramName, \
                         protoInfo.isRepeatedString, protoInfo.isString, protoInfo.lengthInfo != None))

                    protoFieldTypeModifier = ""

                    if protoInfo.isRepeatedString:
                        protoFieldTypeModifier = "repeated"
                    elif protoInfo.isString:
                        protoFieldTypeModifier = "optional"
                    elif protoInfo.isExtensionStruct:
                        protoFieldTypeModifier = "optional"
                    elif protoInfo.lengthInfo == None:
                        protoFieldTypeModifier = "optional"
                    else:
                        protoFieldTypeModifier = "repeated"

                    protoFieldType = ""

                    if protoInfo.isRepeatedString:
                        protoFieldType = "string"
                    elif protoInfo.isString:
                        protoFieldType = "string"
                    elif protoInfo.isExtensionStruct:
                        protoFieldType = "VkExtensionStruct"
                    elif protoInfo.needsMessage:
                        protoFieldType = "%s" % (member.typeName)
                    else:
                        protoFieldType = "%s" % (protoInfo.protobufType)

                    protoFieldName = member.paramName

                    protoTagNumber = i + 1

                    self.codegen.stmt("%s %s %s = %d" %
                        (protoFieldTypeModifier, protoFieldType, protoFieldName, protoTagNumber))

                cgen.endBlock()
                return cgen.swapCode()

            self.module.append(messageGenerator(self.codegen))

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

        vulkanApi = self.typeInfo.apis[name]

        self.apis.append(name)

        def messageGenerator(cgen):
            cgen.line("message %s" % name)
            cgen.beginBlock()
            for (i, param) in enumerate(vulkanApi.parameters):
                protoInfo = VulkanTypeProtobufInfo(self.typeInfo, vulkanApi, param)
                self.codegen.line("// Original field: %s %s %s. stringarray? %d string? %d hasLenInfo? %d" %
                    (protoInfo.origTypeCategory, param.typeName, param.paramName, \
                     protoInfo.isRepeatedString, protoInfo.isString, protoInfo.lengthInfo != None))

                protoFieldTypeModifier = ""

                if protoInfo.isRepeatedString:
                    protoFieldTypeModifier = "repeated"
                elif protoInfo.isString:
                    protoFieldTypeModifier = "optional"
                elif protoInfo.isExtensionStruct:
                    protoFieldTypeModifier = "optional"
                elif protoInfo.lengthInfo == None:
                    protoFieldTypeModifier = "optional"
                else:
                    protoFieldTypeModifier = "repeated"

                protoFieldType = ""

                if protoInfo.isRepeatedString:
                    protoFieldType = "string"
                elif protoInfo.isString:
                    protoFieldType = "string"
                elif protoInfo.isExtensionStruct:
                    protoFieldType = "VkExtensionStruct"
                elif protoInfo.needsMessage:
                    protoFieldType = "%s" % (param.typeName)
                else:
                    protoFieldType = "%s" % (protoInfo.protobufType)

                protoFieldName = param.paramName

                protoTagNumber = i + 1

                self.codegen.stmt("%s %s %s = %d" %
                    (protoFieldTypeModifier, protoFieldType, protoFieldName, protoTagNumber))

            cgen.endBlock()
            return cgen.swapCode()

        self.module.append(messageGenerator(self.codegen))

    def onEnd(self,):
        VulkanWrapperGenerator.onEnd(self)

        self.codegen.line("message VkExtensionStruct")
        self.codegen.beginBlock()

        self.codegen.line("oneof struct")
        self.codegen.beginBlock()

        def onStructExtension(i, ext, cgen):
            cgen.stmt("%s %s = %d" % (ext.name, "extension_" + ext.name, i + 1))

        self.emitForEachStructExtensionGeneral( \
            self.codegen, onStructExtension)

        self.codegen.endBlock()
        self.codegen.endBlock()

        # Generate a proto that captures all Vulkan APIs.

        self.codegen.line("message VkApiCall")

        self.codegen.beginBlock()

        self.codegen.line("oneof api")
        self.codegen.beginBlock()

        for (i, api) in enumerate(self.apis):
            self.codegen.stmt("%s api_%s = %d" % (api, api, i + 1))

        self.codegen.endBlock()
        self.codegen.endBlock()

        self.module.append(self.codegen.swapCode())
