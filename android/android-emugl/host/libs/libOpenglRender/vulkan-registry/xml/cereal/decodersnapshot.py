from .common.codegen import CodeGen, VulkanWrapperGenerator, VulkanAPIWrapper
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType, DISPATCHABLE_HANDLE_TYPES, NON_DISPATCHABLE_HANDLE_TYPES

from .marshaling import VulkanMarshalingCodegen
from .transform import TransformCodegen, genTransformsForVulkanType

from .wrapperdefs import API_PREFIX_MARSHAL
from .wrapperdefs import API_PREFIX_UNMARSHAL
from .wrapperdefs import VULKAN_STREAM_TYPE

from copy import copy

decoder_snapshot_decl_preamble = """

namespace android {
namespace base {
class Pool;
} // namespace base {
} // namespace android {

class VkDecoderSnapshot {
public:
    VkDecoderSnapshot();
    ~VkDecoderSnapshot();
"""

decoder_snapshot_decl_postamble = """
private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};
"""

decoder_snapshot_impl_preamble ="""

using namespace goldfish_vk;

class VkDecoderSnapshot::Impl {
public:
    Impl() { }
"""

decoder_snapshot_impl_postamble = """
};

VkDecoderSnapshot::VkDecoderSnapshot() :
    mImpl(new VkDecoderSnapshot::Impl()) { }

VkDecoderSnapshot::~VkDecoderSnapshot() = default;
"""

def emit_impl(typeInfo, api, cgen):
    cgen.line("// TODO: Implement")

def emit_passthrough_to_impl(typeInfo, api, cgen):
    cgen.vkApiCall(api, customPrefix = "mImpl->")

class VulkanDecoderSnapshot(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.typeInfo = typeInfo

        self.cgenHeader = CodeGen()
        self.cgenHeader.incrIndent()

        self.cgenImpl = CodeGen()

        self.currentFeature = None

        self.feature_apis = []

    def onBegin(self,):
        self.module.appendHeader(decoder_snapshot_decl_preamble)
        self.module.appendImpl(decoder_snapshot_impl_preamble)

    def onBeginFeature(self, featureName):
        VulkanWrapperGenerator.onBeginFeature(self, featureName)
        self.currentFeature = featureName

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

        api = self.typeInfo.apis[name]


        additionalParams = \
            [makeVulkanTypeSimple(False, "android::base::Pool", 1, "pool"),]

        if api.retType.typeName != "void":
            additionalParams.append( \
                makeVulkanTypeSimple(False, api.retType.typeName, 0, "input_result"))

        apiForSnapshot = \
            api.withCustomParameters( \
                additionalParams + \
                api.parameters).withCustomReturnType( \
                    makeVulkanTypeSimple(False, "void", 0, "void"))

        self.feature_apis.append((self.currentFeature, apiForSnapshot))

        self.cgenHeader.stmt(self.cgenHeader.makeFuncProto(apiForSnapshot))
        self.module.appendHeader(self.cgenHeader.swapCode())

        self.cgenImpl.emitFuncImpl( \
            apiForSnapshot, lambda cgen: emit_impl(self.typeInfo, apiForSnapshot, cgen))
        self.module.appendImpl(self.cgenImpl.swapCode())

    def onEnd(self,):
        self.module.appendHeader(decoder_snapshot_decl_postamble)
        self.module.appendImpl(decoder_snapshot_impl_postamble)
        self.cgenHeader.decrIndent()

        for feature, api in self.feature_apis:
            if feature is not None:
                self.cgenImpl.line("#ifdef %s" % feature)

            apiImplShell = \
                api.withModifiedName("VkDecoderSnapshot::" + api.name)

            self.cgenImpl.emitFuncImpl( \
                apiImplShell, lambda cgen: emit_passthrough_to_impl(self.typeInfo, api, cgen))

            if feature is not None:
                self.cgenImpl.line("#endif")

        self.module.appendImpl(self.cgenImpl.swapCode())

