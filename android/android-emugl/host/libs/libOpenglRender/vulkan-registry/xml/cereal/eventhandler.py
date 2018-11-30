from .common.codegen import CodeGen, VulkanWrapperGenerator
from .common.vulkantypes import VulkanAPI, makeVulkanTypeSimple

eventhandler_decl_preamble = """
class VkEncoder;
class VkEventHandler {
public:
    VkEventHandler();
    virtual ~VkEventHandler();
"""

eventhandler_decl_postamble = """
};
"""

eventhandler_impl_preamble = """
VkEventHandler::VkEventHandler() { }
VkEventHandler::~VkEventHandler() { }
"""

PRE_POST_CONTEXT_TYPE = makeVulkanTypeSimple(False, "VkEncoder", 1, "encoder")
GET_PRE_POST_INPUT_RESULT_TYPE = lambda t : t.withModifiedName("input_result")

def extra_params_for_api(api):
    extraParams = [PRE_POST_CONTEXT_TYPE]

    if api.getRetTypeExpr() != "void":
        extraParams.append(GET_PRE_POST_INPUT_RESULT_TYPE(api.retType))

    return extraParams

def get_pre_api(api):
    return \
        VulkanAPI( \
            "pre_" + api.name,
            api.retType,
            extra_params_for_api(api) + api.parameters)

def get_post_api(api):
    return \
        VulkanAPI( \
            "post_" + api.name,
            api.retType,
            extra_params_for_api(api) + api.parameters)

def emit_prototype(api, cgen):
    cgen.stmt("virtual " + cgen.makeFuncProto(api))

def emit_stub_impl(api, cgen):
    cgen.line( \
        cgen.makeFuncProto(api.withModifiedName("VkEventHandler::" + api.name),
                           useParamName=False))
    cgen.beginBlock()

    if api.getRetTypeExpr() != "void":
        cgen.stmt("%s %s = (%s)0" % \
            (api.getRetTypeExpr(), api.getRetVarExpr(),
             api.getRetTypeExpr()))
        cgen.stmt("return %s" % api.getRetVarExpr())

    cgen.endBlock()

class VulkanEventHandler(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)
        self.typeInfo = typeInfo
        self.cgenHeader = CodeGen()
        self.cgenImpl = CodeGen()

        self.cgenHeader.incrIndent()

    def onBegin(self,):
        self.module.appendHeader(eventhandler_decl_preamble)
        self.module.appendImpl(eventhandler_impl_preamble)

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

        api = self.typeInfo.apis[name]

        preApi = get_pre_api(api)
        postApi = get_post_api(api)

        emit_prototype(preApi, self.cgenHeader)
        emit_prototype(postApi, self.cgenHeader)
        self.module.appendHeader(self.cgenHeader.swapCode())

        emit_stub_impl(preApi, self.cgenImpl)
        emit_stub_impl(postApi, self.cgenImpl)

        self.module.appendImpl(self.cgenImpl.swapCode())

    def onEnd(self,):
        self.module.appendHeader(eventhandler_decl_postamble)
        self.cgenHeader.decrIndent()
