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
private:
};

VkDecoderSnapshot::VkDecoderSnapshot() :
    mImpl(new VkDecoderSnapshot::Impl()) { }

VkDecoderSnapshot::~VkDecoderSnapshot() = default;
"""

class VulkanDecoderSnapshot(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.typeInfo = typeInfo

        self.cgenHeader = CodeGen()
        self.cgenHeader.incrIndent()

        self.cgenImpl = CodeGen()

    def onBegin(self,):
        self.module.appendHeader(decoder_snapshot_decl_preamble)
        self.module.appendImpl(decoder_snapshot_impl_preamble)

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

        # api = self.typeInfo.apis[name]
        # self.cgenHeader.stmt(
        #     self.cgenHeader.makeFuncProto(api))

        # apiImpl = api.withModifiedName("VkEncoder::" + api.name)

        # self.module.appendHeader(self.cgenHeader.swapCode())

        # if api.name in custom_encodes.keys():
        #     self.module.appendImpl(self.cgenImpl.makeFuncImpl(
        #         apiImpl, lambda cgen: custom_encodes[api.name](self.typeInfo, api, cgen)))
        # else:
        #     self.module.appendImpl(self.cgenImpl.makeFuncImpl(apiImpl,
        #         lambda cgen: emit_default_encoding(self.typeInfo, api, cgen)))

    def onEnd(self,):
        self.module.appendHeader(decoder_snapshot_decl_postamble)
        self.cgenHeader.decrIndent()
