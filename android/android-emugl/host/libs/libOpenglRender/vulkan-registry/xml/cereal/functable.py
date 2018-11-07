from .common.codegen import CodeGen, VulkanWrapperGenerator
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType

class VulkanFuncTable(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)
        self.typeInfo = typeInfo
        self.cgen = CodeGen()
        self.entries = []
        self.entryFeatures = []
        self.feature = None

    def onBegin(self,):
        pass

    def onBeginFeature(self, featureName):
        self.feature = featureName

    def onGenCmd(self, cmdinfo, name, alias):
        typeInfo = self.typeInfo
        cgen = self.cgen
        api = typeInfo.apis[name]
        self.entries.append(api)
        self.entryFeatures.append(self.feature)

        api_entry = api.withModifiedName("entry_" + api.name)

        cgen.line("static " + self.cgen.makeFuncProto(api_entry))
        cgen.beginBlock()

        cgen.stmt("auto vkEnc = HostConnection::get()->vkEncoder()")

        callLhs = None
        retTypeName = api.getRetTypeExpr()
        if retTypeName != "void":
            retVar = api.getRetVarExpr()
            cgen.stmt("%s %s = (%s)0" % (retTypeName, retVar, retTypeName))
            callLhs = retVar

        cgen.funcCall(
            callLhs, "vkEnc->" + api.name, [p.paramName for p in api.parameters])

        if retTypeName != "void":
            cgen.stmt("return %s" % retVar)

        cgen.endBlock()

        self.module.appendImpl(cgen.swapCode())

    def onEnd(self,):
        getProcAddressDecl = "void* goldfish_vulkan_get_proc_address(const char* name)"
        self.module.appendHeader(getProcAddressDecl + ";\n")
        self.module.appendImpl(getProcAddressDecl)
        self.cgen.beginBlock()

        prevFeature = None
        for e, f in zip(self.entries, self.entryFeatures):
            featureEndif = prevFeature is not None and (f != prevFeature)
            featureif = not featureEndif and (f != prevFeature)

            if featureEndif:
                self.cgen.leftline("#endif")
                self.cgen.leftline("#ifdef %s" % f)
            
            if featureif:
                self.cgen.leftline("#ifdef %s" % f)

            self.cgen.beginIf("!strcmp(name, \"%s\")" % e.name)
            self.cgen.stmt("return (void*)%s" % ("entry_" + e.name))
            self.cgen.endIf()
            prevFeature = f

        self.cgen.leftline("#endif")

        self.cgen.stmt("return nullptr")
        self.cgen.endBlock()
        self.module.appendImpl(self.cgen.swapCode())