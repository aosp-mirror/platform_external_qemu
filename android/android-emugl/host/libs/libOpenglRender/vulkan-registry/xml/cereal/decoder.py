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

READ_STREAM = "vkReadStream"
WRITE_STREAM = "vkStream"

def emit_param_decl_for_reading(param, cgen):
    cgen.stmt(cgen.makeRichCTypeDecl(param.getForNonConstAccess()))

def emit_unmarshal(typeInfo, param, cgen):
    iterateVulkanType(typeInfo, param, VulkanMarshalingCodegen(
        cgen,
        READ_STREAM,
        param.paramName,
        API_PREFIX_UNMARSHAL,
        direction="read",
        dynAlloc=True))

def emit_marshal(typeInfo, param, cgen):
    iterateVulkanType(typeInfo, param, VulkanMarshalingCodegen(
        cgen,
        WRITE_STREAM,
        param.paramName,
        API_PREFIX_MARSHAL,
        direction="write"))

def emit_decode_parameters(typeInfo, api, cgen):
    paramsToRead = []

    for param in api.parameters:
        paramsToRead.append(param)

    for p in paramsToRead:
        emit_param_decl_for_reading(p, cgen)
    for p in paramsToRead:
        emit_unmarshal(typeInfo, p, cgen)

def emit_dispatch_call(api, cgen):
    return cgen.vkApiCall(api, customPrefix="m_vk->")

def emit_decode_parameters_writeback(typeInfo, api, cgen):
    paramsToWrite = []

    for param in api.parameters:
        if param.possiblyOutput():
            paramsToWrite.append(param)

    for p in paramsToWrite:
        emit_marshal(typeInfo, p, cgen)

def emit_decode_return_writeback(api, cgen):
    retTypeName = api.getRetTypeExpr()
    if retTypeName != "void":
        retVar = api.getRetVarExpr()
        cgen.stmt("%s->write(&%s, %s)" %
            (WRITE_STREAM, retVar, cgen.sizeofExpr(api.retType)))

def emit_decode_finish(cgen):
    cgen.stmt("%s->commitWrite()" % WRITE_STREAM)

## Custom decoding definitions##################################################
def emit_default_decoding(typeInfo, api, cgen):
    emit_decode_parameters(typeInfo, api, cgen)
    emit_dispatch_call(api, cgen)
    emit_decode_parameters_writeback(typeInfo, api, cgen)
    emit_decode_return_writeback(api, cgen)
    emit_decode_finish(cgen)

def emit_default_decoding_except_finish(typeInfo, api, cgen):
    emit_decode_parameters(typeInfo, api, cgen)
    emit_dispatch_call(api, cgen)
    emit_decode_parameters_writeback(typeInfo, api, cgen)
    emit_decode_return_writeback(api, cgen)

def decode_vkMapMemory(typeInfo, api, cgen):
    emit_default_decoding_except_finish(typeInfo, api, cgen)

    mapSuccess = cgen.makeVkSuccessCheck(api.getRetVarExpr())
    writebackCond = "(ppData && size > 0)"
    cgen.stmt("bool mapSuccess = %s" % mapSuccess)
    cgen.stmt("bool needWriteback = mapSuccess && %s" % writebackCond)

    cgen.beginIf("needWriteback")
    cgen.stmt("%s->write(*ppData, size)" % (WRITE_STREAM))
    cgen.endIf()

    cgen.beginIf("mapSuccess")
    unmapApi = typeInfo.apis["vkUnmapMemory"]
    cgen.vkApiCall(unmapApi, customPrefix="m_vk->")
    cgen.endIf()

    emit_decode_finish(cgen)

def decode_vkUnmapMemory(typeInfo, api, cgen):
    # From the spec, we do not need to do anything to sync memory on unmap.
    # The spec wording says the explicit flush and invalidate *must*
    # be used to synchronize non-coherent accesses.

    # vkFlushMappedMemoryRanges must be used to guarantee that host writes to
    # non-coherent memory are visible to the device. It must be called after
    # the host writes to non-coherent memory have completed and before command
    # buffers that will read or write any of those memory locations are
    # submitted to a queue.

    # vkInvalidateMappedMemoryRanges must be used to guarantee that device
    # writes to non-coherent memory are visible to the host. It must be called
    # after command buffers that execute and flush (via memory barriers) the
    # device writes have completed, and before the host will read or write any
    # of those locations. If a range of non-coherent memory is written by the
    # host and then invalidated without first being flushed, its contents are
    # undefined.

    # So, the default procedure suffices:    
    emit_default_decoding(typeInfo, api, cgen)

    # But we're explicitly capturing this as a custom decode/encode anyway to
    # make this comment more prominent.

def decode_vkFlushMappedMemoryRanges(typeInfo, api, cgen):
    emit_decode_parameters(typeInfo, api, cgen)

    # Read the result from the guest before emitting the call on host.
    cgen.beginFor("uint32_t i", "i < memoryRangeCount", "++i")
    cgen.stmt("VkMemoryMapFlags flags = 0")
    cgen.stmt("VkDeviceMemory memory = pMemoryRanges[i].memory")
    cgen.stmt("VkDeviceSize offset = pMemoryRanges[i].offset")
    cgen.stmt("VkDeviceSize size = pMemoryRanges[i].size")
    cgen.stmt("void** ppData")
    cgen.vkApiCall(typeInfo.apis["vkMapMemory"], customPrefix="m_vk->")
    cgen.stmt("%s->read(*ppData, size)" % READ_STREAM)
    cgen.stmt("VkMappedMemoryRange hostFlush = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 0, memory, offset, size }")
    cgen.vkApiCall(typeInfo.apis["vkFlushMappedMemoryRanges"], customPrefix="m_vk->",
                   customParams=["device", "1", "&hostFlush"])
    unmapApi = typeInfo.apis["vkUnmapMemory"]
    cgen.vkApiCall(unmapApi, customPrefix="m_vk->")
    cgen.endFor()

    emit_decode_finish(cgen)

def decode_vkInvalidateMappedMemoryRanges(typeInfo, api, cgen):
    emit_decode_parameters(typeInfo, api, cgen)

    # Read the result from the host before transmitting the data to the guest.
    cgen.beginFor("uint32_t i", "i < memoryRangeCount", "++i")
    cgen.stmt("VkMemoryMapFlags flags = 0")
    cgen.stmt("VkDeviceMemory memory = pMemoryRanges[i].memory")
    cgen.stmt("VkDeviceSize offset = pMemoryRanges[i].offset")
    cgen.stmt("VkDeviceSize size = pMemoryRanges[i].size")
    cgen.stmt("void** ppData")
    _, _ = cgen.vkApiCall(typeInfo.apis["vkMapMemory"], customPrefix="m_vk->")
    cgen.stmt("VkMappedMemoryRange hostInvalidate = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 0, memory, offset, size }")
    cgen.vkApiCall(typeInfo.apis["vkInvalidateMappedMemoryRanges"], customPrefix="m_vk->",
                   customParams=["device", "1", "&hostInvalidate"])
    cgen.stmt("%s->write(*ppData, size)" % READ_STREAM)
    unmapApi = typeInfo.apis["vkUnmapMemory"]
    cgen.vkApiCall(unmapApi, customPrefix="m_vk->")
    cgen.endFor()

    emit_decode_finish(cgen)

custom_decodes = {
    "vkMapMemory" : decode_vkMapMemory,
    "vkUnmapMemory" : decode_vkUnmapMemory,
    "vkFlushMappedMemoryRanges" : decode_vkFlushMappedMemoryRanges,
    "vkInvalidateMappedMemoryRanges": decode_vkInvalidateMappedMemoryRanges,
}

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
        self.cgen.stmt("VulkanStream* %s = stream()" % WRITE_STREAM)
        self.cgen.stmt("VulkanMemReadingStream* %s = readStream()" % READ_STREAM)
        self.cgen.stmt("%s->setBuf((uint8_t*)(ptr + 8))" % READ_STREAM)

        self.cgen.line("switch (opcode)")
        self.cgen.beginBlock() # switch stmt

        self.module.appendImpl(self.cgen.swapCode())

        self.custom_decode_funcs = []

        self.feature = None

    def onBeginFeature(self, featureName):
        self.feature = featureName

    def onGenCmd(self, cmdinfo, name, alias):
        typeInfo = self.typeInfo
        cgen = self.cgen
        api = typeInfo.apis[name]

        if api.name in custom_decodes.keys():
            self.custom_decode_funcs.append(
                (self.feature, api.name,
                 lambda: custom_decodes[api.name](typeInfo, api, cgen)))
        else:
            cgen.line("case OP_%s:" % name)
            cgen.beginBlock()

            emit_decode_parameters(typeInfo, api, cgen)
            emit_dispatch_call(api, cgen)
            emit_decode_parameters_writeback(typeInfo, api, cgen)
            emit_decode_return_writeback(api, cgen)
            emit_decode_finish(cgen)

            cgen.stmt("break")
            cgen.endBlock()
            self.module.appendImpl(self.cgen.swapCode())

    def onEnd(self,):
        cgen = self.cgen

        for feature, name, func in self.custom_decode_funcs:
            cgen.leftline("#ifdef %s" % feature)
            cgen.line("case OP_%s:" % name)
            cgen.beginBlock()

            func()

            cgen.stmt("break")
            cgen.endBlock()
            cgen.leftline("#endif")

        cgen.line("default:")
        cgen.beginBlock()
        cgen.stmt("return ptr - (unsigned char *)buf")
        cgen.endBlock()

        cgen.endBlock() # switch stmt

        cgen.stmt("ptr += packetLen")
        cgen.endBlock() # while loop

        cgen.stmt("return ptr - (unsigned char*)buf;")
        cgen.endBlock() # function body

        self.module.appendImpl(self.cgen.swapCode())
