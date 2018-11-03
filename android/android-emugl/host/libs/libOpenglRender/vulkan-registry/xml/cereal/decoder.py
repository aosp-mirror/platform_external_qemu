from .common.codegen import CodeGen, VulkanWrapperGenerator, VulkanAPIWrapper
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType
from .marshaling import VulkanMarshalingCodegen

from .wrapperdefs import VULKAN_STREAM_VAR_NAME
from .wrapperdefs import UNMARSHAL_INPUT_VAR_NAME
from .wrapperdefs import API_PREFIX_UNMARSHAL

decoder_decl_preamble = """

class IOStream;

class VkDecoder {
public:
    VkDecoder();
    ~VkDecoder();
    size_t decode(void* buf, size_t bufsize, IOStream* stream);
private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};
"""

decoder_impl_preamble ="""
using emugl::vkDispatch;

using namespace goldfish_vk;

class VkDecoder::Impl {
public:
    Impl() : m_vk(vkDispatch()) { }
    VulkanStream* stream() { return &m_vkStream; }

    size_t decode(void* buf, size_t bufsize, IOStream* stream);

private:
    VulkanDispatch* m_vk;
    VulkanStream m_vkStream { nullptr };
};

VkDecoder::VkDecoder() :
    mImpl(new VkDecoder::Impl()) { }

VkDecoder::~VkDecoder() = default;

size_t VkDecoder::decode(void* buf, size_t bufsize, IOStream* stream) {
    return mImpl->decode(buf, bufsize, stream);
}

// VkDecoder::Impl::decode to follow
"""

class VulkanDecoder(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)
        self.typeInfo = typeInfo
        self.cgen = CodeGen()

        self.readCodegen = \
            VulkanMarshalingCodegen(
                None,
                VULKAN_STREAM_VAR_NAME,
                UNMARSHAL_INPUT_VAR_NAME,
                API_PREFIX_UNMARSHAL,
                direction = "read",
                dynAlloc = True)

        self.apiOutputCodegenForWrite = \
            VulkanMarshalingCodegen(
                None,
                VULKAN_STREAM_VAR_NAME,
                UNMARSHAL_INPUT_VAR_NAME,
                API_PREFIX_UNMARSHAL,
                direction = "write",
                forApiOutput = True,
                dynAlloc = True)

    def onBegin(self,):
        self.module.appendHeader(decoder_decl_preamble)
        self.module.appendImpl(decoder_impl_preamble)

        self.module.appendImpl(
            "size_t VkDecoder::Impl::decode(void* buf, size_t len, IOStream* stream)\n")

        self.cgen.beginBlock() # function body

        self.cgen.stmt("if (len < 8) return 0;")
        self.cgen.stmt("unsigned char *ptr = (unsigned char *)buf")
        self.cgen.stmt("const unsigned char* const end = (const unsigned char*)buf + len")

        self.cgen.line("while (end - ptr >= 8)")
        self.cgen.beginBlock() # while loop

        self.cgen.stmt("uint32_t opcode = *(uint32_t *)ptr")
        self.cgen.stmt("int32_t packetLen = *(int32_t *)(ptr + 4)")
        self.cgen.stmt("if (end - ptr < packetLen) return ptr - (unsigned char*)buf")

        self.cgen.stmt("m_vkStream.setStream(stream)")
        self.cgen.stmt("VulkanStream* %s = &m_vkStream" % VULKAN_STREAM_VAR_NAME)

        self.cgen.line("switch (opcode)")
        self.cgen.beginBlock() # switch stmt

        self.module.appendImpl(self.cgen.swapCode())

    def onGenCmd(self, cmdinfo, name, alias):
        cgen = self.cgen

        cgen.line("case OP_%s:" % name)
        cgen.beginBlock()

        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)
        api = self.typeInfo.apis[name]

        # need to actually write out the encoding size as well here.

        for param in api.parameters:
            cgen.stmt("%s %s" % (param.typeName, param.paramName))

        cgen.funcCall (
            "%s ret" % api.retType.typeName if api.retType.typeName != "void" else None,
            "m_vk->" + api.name,
            [p.paramName for p in api.parameters])

        if api.retType.isVoidWithNoSize():
            pass
        else:
            cgen.stmt("%s->write(&%s, %s)" % (VULKAN_STREAM_VAR_NAME,
                                              "ret",
                                              cgen.sizeofExpr(api.retType)))

        cgen.endBlock()
        self.module.appendImpl(self.cgen.swapCode())

    def onEnd(self,):
        self.cgen.endBlock() # while loop
        self.cgen.endBlock() # switch stmt

        self.cgen.stmt("return 0;")
        self.cgen.endBlock() # function body
        self.module.appendImpl(self.cgen.swapCode())
