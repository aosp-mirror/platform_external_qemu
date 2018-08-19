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

from .codegen import *
from .vulkantypes import *

# Contains definitions for various Vulkan API wrappers.
# These work with generators and have gen* triggers.
# They tend to contain VulkanAPIWrapper objects to make it easier
# to generate the code.

# Base class
class VulkanWrapperGenerator(object):
    def __init__(self, module, typeInfo):
        self.module = module
        self.typeInfo = typeInfo

    def onGenType(self, typeinfo, name, alias):
        pass
    def onGenStruct(self, typeinfo, typeName, alias):
        pass
    def onGenGroup(self, groupinfo, groupName, alias = None):
        pass
    def onGenEnum(self, enuminfo, name, alias):
        pass
    def onGenCmd(self, cmdinfo, name, alias):
        pass

# Marshaling
class VulkanMarshaling(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)
        
        def marshalDefFunc(codegen, api):
            if api.retType.typeName != "void":
                codegen.stmt("return (%s)0" % api.retType.typeName)

        self.marshalWrapper = \
            VulkanAPIWrapper(
                "marshal_",
                [makeVulkanTypeSimple(False, "void", 1, "vkStream")], 
                makeVulkanTypeSimple(False, "void", 0),
                marshalDefFunc)

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)
        self.module.appendHeader(self.marshalWrapper.makeDecl(self.typeInfo, name))
        self.module.appendImpl(self.marshalWrapper.makeDefinition(self.typeInfo, name))
