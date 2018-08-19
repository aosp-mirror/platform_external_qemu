from .wrapperdefs import *

class VulkanMarshalingCodegen(object):
    def __init__(self, cgen, streamVarName, inputVarName, marshalPrefix, direction = "write", forApiOutput = False):
        self.cgen = cgen
        self.direction = direction
        self.processSimple = "write" if self.direction == "write" else "read"
        self.forApiOutput = forApiOutput

        self.checked = False

        self.streamVarName = streamVarName
        self.inputVarName = inputVarName
        self.marshalPrefix = marshalPrefix

        self.exprAccessor = lambda t: self.cgen.generalAccess(t, parentVarName = self.inputVarName, asPtr = True)
        self.lenAccessor = lambda t: self.cgen.generalLengthAccess(t, parentVarName = self.inputVarName)

    def getTypeForStreaming(self, vulkanType):
        res = deepcopy(vulkanType)

        if not vulkanType.accessibleAsPointer():
            res = res.getForAddressAccess()

        if vulkanType.staticArrExpr:
            res = res.getForAddressAccess()

        if self.direction == "write":
            return res
        else:
            return res.getForNonConstAccess()

    def makeCastExpr(self, vulkanType):
        return "(%s)" % (self.cgen.makeCTypeDecl(vulkanType, useParamName = False))

    def genStreamCall(self, vulkanType, toStreamExpr, sizeExpr):
        varname = self.streamVarName
        func = self.processSimple
        cast = self.makeCastExpr(self.getTypeForStreaming(vulkanType))

        self.cgen.stmt("%s->%s(%s%s, %s)" % (varname, func, cast, toStreamExpr, sizeExpr))

    def onCheck(self, vulkanType):

        if self.forApiOutput:
            return

        self.checked = True

        access = self.exprAccessor(vulkanType)
        addrExpr = "&" + access
        sizeExpr = self.cgen.sizeofExpr(vulkanType)
        self.genStreamCall(vulkanType.getForAddressAccess(), addrExpr, sizeExpr)

        skipStreamInternal = vulkanType.typeName == "void"

        if skipStreamInternal:
            return

        self.cgen.beginIf(access)

    def endCheck(self, vulkanType):

        skipStreamInternal = vulkanType.typeName == "void"
        if skipStreamInternal:
            return

        if self.checked:
            self.cgen.endIf()
            self.checked = False

    def onCompoundType(self, vulkanType):

        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

        if lenAccess is not None:
            loopVar = "i"
            access = "%s + %s" % (access, loopVar)
            forInit = "uint32_t %s = 0" % loopVar
            forCond = "%s < (uint32_t)%s" % (loopVar, lenAccess)
            forIncr = "++%s" % loopVar
            self.cgen.beginFor(forInit, forCond, forIncr)

        accessWithCast = "%s(%s)" % (self.makeCastExpr(self.getTypeForStreaming(vulkanType)), access)

        self.cgen.funcCall(None, self.marshalPrefix + vulkanType.typeName, [self.streamVarName, accessWithCast])

        if lenAccess is not None:
            self.cgen.endFor()

    def onString(self, vulkanType):

        access = self.exprAccessor(vulkanType)

        if self.direction == "write":
            self.cgen.stmt("%s->putString(%s)" % (self.streamVarName, access))
        else:
            self.cgen.stmt("%s->getString()" % (self.streamVarName))
        
    def onStringArray(self, vulkanType):

        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

        if self.direction == "write":
            self.cgen.stmt("%s->putStringArray(%s, %s)" % (self.streamVarName, access, lenAccess))
        else:
            self.cgen.stmt("%s->getStringArray()" % (self.streamVarName))

    def onStaticArr(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)
        finalLenExpr = "%s * %s" % (lenAccess, self.cgen.sizeofExpr(vulkanType))
        self.genStreamCall(vulkanType, access, finalLenExpr)

    def onPointer(self, vulkanType):
        skipStreamInternal = vulkanType.typeName == "void"
        if skipStreamInternal:
            return

        access = self.exprAccessor(vulkanType)
        lenAccess = self.lenAccessor(vulkanType)

        if lenAccess is not None:
            finalLenExpr = "%s * %s" % (lenAccess, self.cgen.sizeofExpr(vulkanType.getForValueAccess()))
        else:
            finalLenExpr = self.cgen.sizeofExpr(vulkanType.getForValueAccess())

        self.genStreamCall(vulkanType, access, finalLenExpr)

    def onValue(self, vulkanType):
        access = self.exprAccessor(vulkanType)
        self.genStreamCall(vulkanType, access, self.cgen.sizeofExpr(vulkanType))

class VulkanMarshaling(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.writeCodegen = \
            VulkanMarshalingCodegen(
                None,
                VULKAN_STREAM_VAR_NAME,
                MARSHAL_INPUT_VAR_NAME,
                API_PREFIX_MARSHAL,
                direction = "write")

        self.apiOutputCodegenForRead = \
            VulkanMarshalingCodegen(
                None,
                VULKAN_STREAM_VAR_NAME,
                MARSHAL_INPUT_VAR_NAME,
                API_PREFIX_MARSHAL,
                direction = "read",
                forApiOutput = True)

        self.readCodegen = \
            VulkanMarshalingCodegen(
                None,
                VULKAN_STREAM_VAR_NAME,
                UNMARSHAL_INPUT_VAR_NAME,
                API_PREFIX_UNMARSHAL,
                direction = "read")

        self.apiOutputCodegenForWrite = \
            VulkanMarshalingCodegen(
                None,
                VULKAN_STREAM_VAR_NAME,
                UNMARSHAL_INPUT_VAR_NAME,
                API_PREFIX_UNMARSHAL,
                direction = "write",
                forApiOutput = True)

        def apiMarshalingDef(cgen, api):
            self.apiOutputCodegenForRead.cgen = cgen
            self.writeCodegen.cgen = cgen

            for param in api.parameters:
                if param.paramName == VULKAN_STREAM_VAR_NAME:
                    continue
                if param.possiblyOutput():
                    iterateVulkanType(typeInfo, param, self.apiOutputCodegenForRead)
                else:
                    iterateVulkanType(typeInfo, param, self.writeCodegen)
                                  
            if api.retType.isVoidWithNoSize():
                pass
            else:
                result_var_name = "%s_%s_return" % (api.name, api.retType.typeName)
                cgen.stmt("%s %s" % (cgen.makeCTypeDecl(api.retType, useParamName = False), result_var_name))
                cgen.stmt("%s->read(&%s, %s)" % (VULKAN_STREAM_VAR_NAME, result_var_name, cgen.sizeofExpr(api.retType)))
                cgen.stmt("return %s" % result_var_name)

        self.marshalWrapper = \
            VulkanAPIWrapper(
                API_PREFIX_MARSHAL,
                PARAMETERS_MARSHALING,
                None,
                apiMarshalingDef)

        def apiUnmarshalingDef(cgen, api):
            self.apiOutputCodegenForWrite.cgen = cgen
            self.readCodegen.cgen = cgen

            for param in api.parameters:
                if param.paramName == VULKAN_STREAM_VAR_NAME:
                    continue
                if param.possiblyOutput():
                    iterateVulkanType(typeInfo, param, self.apiOutputCodegenForWrite)
                else:
                    iterateVulkanType(typeInfo, param, self.readCodegen)
                                  
            if api.retType.isVoidWithNoSize():
                pass
            else:
                result_var_name = "%s_%s_return" % (api.name, api.retType.typeName)
                cgen.stmt("%s %s" % (cgen.makeCTypeDecl(api.retType, useParamName = False), result_var_name))
                cgen.stmt("%s->write(&%s, %s)" % (VULKAN_STREAM_VAR_NAME, result_var_name, cgen.sizeofExpr(api.retType)))
                cgen.stmt("return %s" % result_var_name)

        self.unmarshalWrapper = \
            VulkanAPIWrapper(
                API_PREFIX_UNMARSHAL,
                PARAMETERS_MARSHALING,
                None,
                apiUnmarshalingDef)

        self.knownDefs = {}


    def onGenType(self, typeXml, name, alias):
        VulkanWrapperGenerator.onGenType(self, typeXml, name, alias)

        if name in self.knownDefs:
            return

        category = self.typeInfo.categoryOf(name)

        if category in ["struct", "union"] and not alias:

            structInfo = self.typeInfo.structs[name]

            marshalPrototype = \
                VulkanAPI(API_PREFIX_MARSHAL + name,
                          STREAM_RET_TYPE,
                          PARAMETERS_MARSHALING + [makeVulkanTypeSimple(True, name, 1, MARSHAL_INPUT_VAR_NAME)])

            def structMarshalingDef(cgen):
                self.writeCodegen.cgen = cgen
                for member in structInfo.members:
                    iterateVulkanType(self.typeInfo, member, self.writeCodegen)

            self.module.appendHeader(self.marshalWrapper.codegen.makeFuncDecl(marshalPrototype))
            self.module.appendImpl(self.marshalWrapper.codegen.makeFuncImpl(marshalPrototype, structMarshalingDef))

            unmarshalPrototype = \
                VulkanAPI(API_PREFIX_UNMARSHAL + name,
                          STREAM_RET_TYPE,
                          PARAMETERS_MARSHALING + [makeVulkanTypeSimple(False, name, 1, UNMARSHAL_INPUT_VAR_NAME)])

            def structUnmarshalingDef(cgen):
                self.readCodegen.cgen = cgen
                for member in structInfo.members:
                    iterateVulkanType(self.typeInfo, member, self.readCodegen)

            self.module.appendHeader(self.unmarshalWrapper.codegen.makeFuncDecl(unmarshalPrototype))
            self.module.appendImpl(self.unmarshalWrapper.codegen.makeFuncImpl(unmarshalPrototype, structUnmarshalingDef))

            comparePrototype = \
                VulkanAPI(API_PREFIX_COMPARE + name,
                          COMPARE_RET_TYPE,
                          list(map(lambda v: makeVulkanTypeSimple(True, name, 1, v), COMPARE_VAR_NAMES)) + [COMPARE_ON_FAIL_VAR_TYPE])

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)
        self.module.appendHeader(self.marshalWrapper.makeDecl(self.typeInfo, name))
        self.module.appendImpl(self.marshalWrapper.makeDefinition(self.typeInfo, name))
        self.module.appendHeader(self.unmarshalWrapper.makeDecl(self.typeInfo, name))
        self.module.appendImpl(self.unmarshalWrapper.makeDefinition(self.typeInfo, name))
