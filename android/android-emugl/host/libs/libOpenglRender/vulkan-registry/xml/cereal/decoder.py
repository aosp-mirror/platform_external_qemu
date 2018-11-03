from .common.codegen import CodeGen, VulkanWrapperGenerator, VulkanAPIWrapper
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType

from .marshaling import VulkanMarshalingCodegen

from .wrapperdefs import API_PREFIX_MARSHAL
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
    VulkanMemReadingStream* readStream() { return &m_vkMemReadingStream; }

    size_t decode(void* buf, size_t bufsize, IOStream* stream);

private:
    VulkanDispatch* m_vk;
    VulkanStream m_vkStream { nullptr };
    VulkanMemReadingStream m_vkMemReadingStream { nullptr };
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

    def onBegin(self,):
        self.module.appendHeader(decoder_decl_preamble)
        self.module.appendImpl(decoder_impl_preamble)

        self.module.appendImpl(
            "size_t VkDecoder::Impl::decode(void* buf, size_t len, IOStream* ioStream)\n")

        self.cgen.beginBlock() # function body

        self.cgen.stmt("if (len < 8) return 0;")
        self.cgen.stmt("unsigned char *ptr = (unsigned char *)buf")
        self.cgen.stmt("const unsigned char* const end = (const unsigned char*)buf + len")

        self.cgen.line("while (end - ptr >= 8)")
        self.cgen.beginBlock() # while loop

        self.cgen.stmt("uint32_t opcode = *(uint32_t *)ptr")
        self.cgen.stmt("int32_t packetLen = *(int32_t *)(ptr + 4)")
        self.cgen.stmt("if (end - ptr < packetLen) return ptr - (unsigned char*)buf")

        self.cgen.stmt("stream()->setStream(ioStream)")
        self.cgen.stmt("VulkanStream* vkStream = stream()")
        self.cgen.stmt("VulkanMemReadingStream* vkReadStream = readStream()")

        self.cgen.line("switch (opcode)")
        self.cgen.beginBlock() # switch stmt

        self.module.appendImpl(self.cgen.swapCode())

    def onGenCmd(self, cmdinfo, name, alias):
        cgen = self.cgen

        def streamParamsFromPacketBuffer(params):
            for p in params:
                cgen.stmt(cgen.makeRichCTypeDecl(p.getForNonConstAccess()))
            cgen.stmt("vkReadStream->setBuf((uint8_t*)(ptr + 8))")
            for p in params:
                unmarshal = VulkanMarshalingCodegen(
                    None,
                    "vkReadStream",
                    p.paramName,
                    API_PREFIX_UNMARSHAL,
                    direction = "read",
                    dynAlloc = True)
                unmarshal.cgen = cgen
                iterateVulkanType(self.typeInfo, p, unmarshal)


        api = self.typeInfo.apis[name]

        cgen.line("case OP_%s:" % name)
        cgen.beginBlock()

        paramsToRead = [] 
        paramsToWrite = [] 

        for param in api.parameters:
            paramsToRead.append(param)
            if param.possiblyOutput():
                paramsToWrite.append(param)

        streamParamsFromPacketBuffer(paramsToRead)

        cgen.stmt("break")
        cgen.endBlock()
        self.module.appendImpl(self.cgen.swapCode())

    def onEnd(self,):
        self.cgen.endBlock() # while loop
        self.cgen.endBlock() # switch stmt

        self.cgen.stmt("return 0;")
        self.cgen.endBlock() # function body
        self.module.appendImpl(self.cgen.swapCode())
