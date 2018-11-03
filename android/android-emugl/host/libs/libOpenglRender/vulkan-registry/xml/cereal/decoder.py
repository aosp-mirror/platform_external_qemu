from .common.codegen import CodeGen, VulkanWrapperGenerator

decoder_decl_preamble = """
class VkDecoder {
public:
    VkDecoder(IOStream* stream);
    ~VkDecoder();
    size_t decode(void* buf, size_t bufsize, IOStream* stream);
"""

decoder_decl_postamble = """
private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};
"""

decoder_impl_preamble ="""
using goldfish_vk::VulkanStream;

class VkDecoder::Impl {
public:
    Impl(IOStream* stream) : m_stream(stream) { }
    VulkanStream* stream() { return &m_stream; }
private:
    VulkanStream m_stream;
};

VkDecoder::VkDecoder(IOStream *stream)
    mImpl(new VkDecoder::Impl(stream)) { }
"""

class VulkanEncoder(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)
        self.typeInfo = typeInfo

        self.cgenHeader = CodeGen()
        self.cgenHeader.incrIndent()

        self.cgenImpl = CodeGen()

    def onBegin(self,):
        self.module.appendHeader(encoder_decl_preamble)
        self.module.appendImpl(encoder_impl_preamble)

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

        api = self.typeInfo.apis[name]
        self.cgenHeader.stmt(self.cgenHeader.makeFuncProto(api))

        apiImpl = api.withModifiedName("VkEncoder::" + api.name)

        def makeImpl(cgen):
            cgen.stmt("auto stream = mImpl->stream()")
            (cgen.funcCall if api.retType.typeName == "void" else cgen.funcCallRet)(
                None, "marshal_" + api.name, [
                    "stream"] + [p.paramName for p in api.parameters])

        self.module.appendHeader(self.cgenHeader.swapCode())
        self.module.appendImpl(self.cgenImpl.makeFuncImpl(apiImpl, makeImpl))

    def onEnd(self,):
        self.module.appendHeader(encoder_decl_postamble)
        self.cgenHeader.decrIndent()
