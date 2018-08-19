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

# Classes and methods for parsing information about Vulkan types.

generatorInstance = None

class VulkanType(object):
    def __init__(self, tag):
        global generatorInstance

        self.gen = generatorInstance

        self.typeCategory = ""
        self.typeName = ""

        self.paramName = None

        self.lenExpr = None
        self.isOptional = False

        self.isConst = False

        self.staticArrExpr = "" # "" means not static array
        self.staticArrCount = 0 # 0 means this static array size is not specified as a numeric literal
        self.pointerIndirectionLevels = 0 # 0 means not pointer

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
            self.staticArrExpr = nametag.tail[1:-1]
            self.staticArrCount = int(self.staticArrExpr)

        # Determine const

        beforeTypePart = noneStr(tag.text)

        if "const" in beforeTypePart:
            self.isConst = True

        # Calculate type and pointer info

        for elem in tag:
            if elem.tag == "type":
                duringTypePart = noneStr(elem.text)
                afterTypePart = noneStr(elem.tail)
                # Now we know enough to fill some stuff in
                self.typeCategory = "TODO"
                self.typeName = duringTypePart
               
                for c in afterTypePart:
                    if c == "*":
                        self.pointerIndirectionLevels += 1

                # If void*, treat like it's not a pointer
                if duringTypePart == 'void':
                    self.pointerIndirectionLevels -= 1

        # Calculate optionality

        if tag.attrib.get('optional') is not None:
            self.isOptional = True

        def isHandleTypeDispatchable(handlename):
            handle = self.gen.registry.tree.find("types/type/[name='" + handlename + "'][@category='handle']")
            if handle is not None and handle.find('type').text == 'VK_DEFINE_HANDLE':
                return True
            else:
                return False

        # Get the category
        def getTypeCategory(typename):
            types = self.gen.registry.tree.findall("types/type")
            for elem in types:
                if (elem.find("name") is not None and elem.find('name').text == typename) or elem.attrib.get('name') == typename:
                    return elem.attrib.get('category')

        self.typeCategory = getTypeCategory(self.typeName)

    def __str__(self,):
        return "(vulkantype %s %s %s paramName %s len %s optional? %s staticArrExpr %s %s)" % (self.typeCategory, self.typeName + ("*" * self.pointerIndirectionLevels) , "const" if self.isConst else "nonconst", self.paramName, self.lenExpr, self.isOptional, self.staticArrExpr, self.staticArrCount)

