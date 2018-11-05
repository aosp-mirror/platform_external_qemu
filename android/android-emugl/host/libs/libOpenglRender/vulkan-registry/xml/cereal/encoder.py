from .common.codegen import CodeGen, VulkanWrapperGenerator
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType

from .marshaling import VulkanMarshalingCodegen

from .wrapperdefs import API_PREFIX_MARSHAL
from .wrapperdefs import API_PREFIX_UNMARSHAL

encoder_decl_preamble = """
class VkEncoder {
public:
    VkEncoder(IOStream* stream);
    ~VkEncoder();
"""

encoder_decl_postamble = """
private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};
"""

encoder_impl_preamble ="""

using goldfish_vk::VulkanCountingStream;
using goldfish_vk::VulkanStream;

class VkEncoder::Impl {
public:
    Impl(IOStream* stream) : m_stream(stream) { }
    VulkanCountingStream* countingStream() { return &m_countingStream; }
    VulkanStream* stream() { return &m_stream; }
private:
    VulkanCountingStream m_countingStream;
    VulkanStream m_stream;
};

VkEncoder::VkEncoder(IOStream *stream) :
    mImpl(new VkEncoder::Impl(stream)) { }
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
            def streamParams(params, direction, streamVar):
                if direction == "write":
                    apiPrefix = API_PREFIX_MARSHAL
                else:
                    apiPrefix = API_PREFIX_UNMARSHAL

                for p in params:
                    codegen = \
                        VulkanMarshalingCodegen(
                            None,
                            streamVar,
                            p.paramName,
                            apiPrefix,
                            direction = direction)
                    codegen.cgen = self.cgenImpl
                    iterateVulkanType(self.typeInfo, p, codegen)

            cgen.stmt("auto stream = mImpl->stream()")
            cgen.stmt("auto countingStream = mImpl->countingStream()")
            cgen.stmt("countingStream->rewind()")

            cgen.beginBlock()

            paramsToWrite = []
            paramsToRead = []

            for param in api.parameters:
                paramsToWrite.append(param)
                if param.possiblyOutput():
                    paramsToRead.append(param)

            streamParams(paramsToWrite, "write", "countingStream")

            cgen.endBlock()

            cgen.stmt("uint32_t packetSize_%s = 4 + 4 + (uint32_t)countingStream->bytesWritten()" % api.name)
            cgen.stmt("countingStream->rewind()")

            cgen.stmt("uint32_t opcode_%s = OP_%s" % (api.name, api.name))
            cgen.stmt("stream->write(&opcode_%s, sizeof(uint32_t))" % api.name)
            cgen.stmt("stream->write(&packetSize_%s, sizeof(uint32_t))" % api.name)

            streamParams(paramsToWrite, "write", "stream")
            streamParams(paramsToRead, "read", "stream")

            if api.retType.typeName == "void":
                return

            retType = api.retType.typeName
            retVar = "%s_%s_return" % (api.name, retType)
            cgen.stmt("%s %s = (%s)0" % (retType, retVar, retType))
            cgen.stmt("stream->read(&%s, %s)" % (retVar, cgen.sizeofExpr(api.retType)))
            cgen.stmt("return %s" % retVar)


        self.module.appendHeader(self.cgenHeader.swapCode())
        self.module.appendImpl(self.cgenImpl.makeFuncImpl(apiImpl, makeImpl))

    def onEnd(self,):
        self.module.appendHeader(encoder_decl_postamble)
        self.cgenHeader.decrIndent()
