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
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType

from .wrapperdefs import VulkanWrapperGenerator
from .wrapperdefs import STRUCT_EXTENSION_PARAM
from .wrapperdefs import STRUCT_EXTENSION_PARAM_FOR_WRITE
from .wrapperdefs import EXTENSION_SIZE_API_NAME
from .wrapperdefs import STRUCT_TYPE_API_NAME

class VulkanExtensionStructs(VulkanWrapperGenerator):

    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.codegen = CodeGen()

        self.structTypeRetType = \
            makeVulkanTypeSimple(False, "uint32_t", 0)
        self.structTypePrototype = \
            VulkanAPI(STRUCT_TYPE_API_NAME,
                      self.structTypeRetType,
                      [STRUCT_EXTENSION_PARAM])

        self.extensionStructSizeRetType = \
            makeVulkanTypeSimple(False, "size_t", 0)
        self.extensionStructSizePrototype = \
            VulkanAPI(EXTENSION_SIZE_API_NAME,
                      self.extensionStructSizeRetType,
                      [STRUCT_EXTENSION_PARAM])

    def onBegin(self,):
        VulkanWrapperGenerator.onBegin(self)
        self.module.appendHeader(self.codegen.makeFuncDecl(
            self.structTypePrototype))
        self.module.appendHeader(self.codegen.makeFuncDecl(
            self.extensionStructSizePrototype))

    def onGenType(self, typeXml, name, alias):
        VulkanWrapperGenerator.onGenType(self, typeXml, name, alias)

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

    def onEnd(self,):
        VulkanWrapperGenerator.onEnd(self)

        def castAsStruct(varName, typeName, const=True):
            return "reinterpret_cast<%s%s*>(%s)" % \
                   ("const " if const else "", typeName, varName)

        def structTypeImpl(cgen):
            cgen.stmt(
                "const uint32_t asStructType = *(%s)" %
                (castAsStruct(STRUCT_EXTENSION_PARAM.paramName, "uint32_t")))
            cgen.stmt("return asStructType")

        self.module.appendImpl(
            self.codegen.makeFuncImpl(
                self.structTypePrototype, structTypeImpl))

        def forEachExtensionReturnSize(ext, _, cgen):
            cgen.stmt("return sizeof(%s)" % ext.name)

        self.module.appendImpl(
            self.codegen.makeFuncImpl(
                self.extensionStructSizePrototype,
                lambda cgen: self.emitForEachStructExtension(
                    cgen,
                    self.extensionStructSizeRetType,
                    STRUCT_EXTENSION_PARAM,
                    forEachExtensionReturnSize, autoBreak=False)))