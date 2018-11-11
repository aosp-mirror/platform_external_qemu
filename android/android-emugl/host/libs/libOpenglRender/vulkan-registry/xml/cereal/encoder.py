from .common.codegen import CodeGen, VulkanWrapperGenerator
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType, CUSTOM_CREATE_APIS

from .marshaling import VulkanMarshalingCodegen
from .handlemap import HandleMapCodegen
from .deepcopy import DeepcopyCodegen

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

using namespace goldfish_vk;

using android::aligned_buf_alloc;
using android::aligned_buf_free;
using android::base::Pool;

class VkEncoder::Impl {
public:
    Impl(IOStream* stream) : m_stream(stream) { }
    VulkanCountingStream* countingStream() { return &m_countingStream; }
    VulkanStream* stream() { return &m_stream; }
    Pool* pool() { return &m_pool; }
    ResourceTracker* resources() { return ResourceTracker::get(); }
private:
    VulkanCountingStream m_countingStream;
    VulkanStream m_stream;
    Pool m_pool { 8, 4096, 64 };
};

VkEncoder::VkEncoder(IOStream *stream) :
    mImpl(new VkEncoder::Impl(stream)) { }
"""

COUNTING_STREAM = "countingStream"
STREAM = "stream"
RESOURCES = "resources"
POOL = "pool"

# Common components of encoding a Vulkan API call
def emit_count_marshal(typeInfo, param, cgen):
    res = \
        iterateVulkanType(
            typeInfo, param,
            VulkanMarshalingCodegen( \
               cgen, COUNTING_STREAM, param.paramName,
               API_PREFIX_MARSHAL, direction="write"))
    if not res:
        cgen.stmt("(void)%s" % param.paramName)

def emit_marshal(typeInfo, param, cgen):
    res = \
        iterateVulkanType(
            typeInfo, param,
            VulkanMarshalingCodegen( \
               cgen, STREAM, param.paramName,
               API_PREFIX_MARSHAL, direction="write"))
    if not res:
        cgen.stmt("(void)%s" % param.paramName)

def emit_unmarshal(typeInfo, param, cgen):
    iterateVulkanType(
        typeInfo, param,
        VulkanMarshalingCodegen( \
           cgen, STREAM, param.paramName,
           API_PREFIX_UNMARSHAL, direction="read"))

def emit_deepcopy(typeInfo, param, cgen):
    res = \
        iterateVulkanType(typeInfo, param, DeepcopyCodegen(
            cgen, [param.paramName, "local_" + param.paramName], "pool", "deepcopy_"))
    if not res:
        cgen.stmt("(void)%s" % param.paramName)

def emit_handlemap_unwrap(typeInfo, param, cgen):
    iterateVulkanType(typeInfo, param, HandleMapCodegen(
        cgen, None, "resources->unwrapMapping()", "handlemap_",
        lambda vtype: typeInfo.isHandleType(vtype)
    ))

def emit_handlemap_create(typeInfo, param, cgen):
    iterateVulkanType(typeInfo, param, HandleMapCodegen(
        cgen, None, "resources->createMapping()", "handlemap_",
        lambda vtype: typeInfo.isHandleType(vtype)
    ))

def emit_custom_create(typeInfo, api, cgen):
    if api.name in CUSTOM_CREATE_APIS:
        cgen.funcCall(
            None,
            "goldfish_" + api.name,
            [p.paramName for p in api.parameters])

def emit_handlemap_destroy(typeInfo, param, cgen):
    iterateVulkanType(typeInfo, param, HandleMapCodegen(
        cgen, None, "resources->destroyMapping()", "handlemap_",
        lambda vtype: typeInfo.isHandleType(vtype)
    ))

class EncodingParameters(object):
    def __init__(self, api):
        self.localCopied = []
        self.toWrite = []
        self.toRead = []
        self.toCreate = []
        self.toDestroy = []

        for param in api.parameters:
            if param.possiblyOutput():
                self.toWrite.append(param)
                self.toRead.append(param)
                if param.isCreatedBy(api):
                    self.toCreate.append(param)
            else:
                if param.isDestroyedBy(api):
                    self.toDestroy.append(param)
                localCopyParam = \
                    param.getForNonConstAccess().withModifiedName( \
                        "local_" + param.paramName)
                self.localCopied.append((param, localCopyParam))
                self.toWrite.append(localCopyParam)

def emit_parameter_encode_preamble_write(typeInfo, api, cgen):
    cgen.stmt("auto %s = mImpl->stream()" % STREAM)
    cgen.stmt("auto %s = mImpl->countingStream()" % COUNTING_STREAM)
    cgen.stmt("auto %s = mImpl->resources()" % RESOURCES)
    cgen.stmt("auto %s = mImpl->pool()" % POOL)

    encodingParams = EncodingParameters(api)

    for (origParam, localCopyParam) in encodingParams.localCopied:
        cgen.stmt(cgen.makeRichCTypeDecl(localCopyParam))
        emit_deepcopy(typeInfo, origParam, cgen)
        emit_handlemap_unwrap(typeInfo, localCopyParam, cgen)
        if localCopyParam.typeName == "VkAllocationCallbacks":
            cgen.stmt("%s = nullptr" % localCopyParam.paramName)

    cgen.stmt("%s->rewind()" % COUNTING_STREAM)
    cgen.beginBlock()

    # Use counting stream to calculate the packet size.
    for p in encodingParams.toWrite:
        emit_count_marshal(typeInfo, p, cgen)

    cgen.endBlock()

def emit_parameter_encode_write_packet_info(typeInfo, api, cgen):

    cgen.stmt("uint32_t packetSize_%s = 4 + 4 + (uint32_t)%s->bytesWritten()" % \
              (api.name, COUNTING_STREAM))
    cgen.stmt("%s->rewind()" % COUNTING_STREAM)

    cgen.stmt("uint32_t opcode_%s = OP_%s" % (api.name, api.name))
    cgen.stmt("%s->write(&opcode_%s, sizeof(uint32_t))" % (STREAM, api.name))
    cgen.stmt("%s->write(&packetSize_%s, sizeof(uint32_t))" % (STREAM, api.name))

def emit_parameter_encode_do_parameter_write(typeInfo, api, cgen):
    encodingParams = EncodingParameters(api)
    for p in encodingParams.toWrite:
        emit_marshal(typeInfo, p, cgen)

def emit_parameter_encode_read(typeInfo, api, cgen):
    encodingParams = EncodingParameters(api)

    for p in encodingParams.toRead:
        emit_unmarshal(typeInfo, p, cgen)

    for p in encodingParams.toCreate:
        emit_handlemap_create(typeInfo, p, cgen)
        emit_custom_create(typeInfo, api, cgen)

    for p in encodingParams.toDestroy:
        emit_handlemap_destroy(typeInfo, p, cgen)

    cgen.stmt("pool->freeAll()")

def emit_return_unmarshal(typeInfo, api, cgen):
    retType = api.getRetTypeExpr()

    if retType == "void":
        return

    retVar = api.getRetVarExpr()
    cgen.stmt("%s %s = (%s)0" % (retType, retVar, retType))
    cgen.stmt("%s->read(&%s, %s)" % \
              (STREAM, retVar, cgen.sizeofExpr(api.retType)))

def emit_return(typeInfo, api, cgen):
    if api.getRetTypeExpr() == "void":
        return

    retVar = api.getRetVarExpr()
    cgen.stmt("return %s" % retVar)

def emit_default_encoding(typeInfo, api, cgen):
    emit_parameter_encode_preamble_write(typeInfo, api, cgen)
    emit_parameter_encode_write_packet_info(typeInfo, api, cgen)
    emit_parameter_encode_do_parameter_write(typeInfo, api, cgen)
    emit_parameter_encode_read(typeInfo, api, cgen)
    emit_return_unmarshal(typeInfo, api, cgen)
    emit_return(typeInfo, api, cgen)

def emit_only_goldfish_custom(typeInfo, api, cgen):
    cgen.vkApiCall(api, customPrefix="goldfish_")
    emit_return(typeInfo, api, cgen)

## Custom encoding definitions##################################################

def encode_vkFlushMappedMemoryRanges(typeInfo, api, cgen):
    emit_parameter_encode_preamble_write(typeInfo, api, cgen)

    def emit_flush_ranges(streamVar):
        cgen.beginFor("uint32_t i = 0", "i < memoryRangeCount", "++i")
        cgen.stmt("auto range = pMemoryRanges[i]")
        cgen.stmt("auto memory = pMemoryRanges[i].memory")
        cgen.stmt("auto size = pMemoryRanges[i].size")
        cgen.stmt("auto offset = pMemoryRanges[i].offset")
        cgen.stmt("auto goldfishMem = as_goldfish_VkDeviceMemory(memory)")
        cgen.stmt("size_t streamSize = 0")
        cgen.stmt("if (!goldfishMem) { %s->write(&streamSize, sizeof(size_t)); continue; }" % streamVar)
        cgen.stmt("auto hostPtr = goldfishMem->ptr")
        cgen.stmt("auto actualSize = size == VK_WHOLE_SIZE ? goldfishMem->size : size")
        cgen.stmt("if (!hostPtr) { %s->write(&streamSize, sizeof(size_t)); continue; }" % streamVar)
        cgen.stmt("streamSize = actualSize")
        cgen.stmt("%s->write(&streamSize, sizeof(size_t))" % streamVar)
        cgen.stmt("uint8_t* targetRange = hostPtr + offset")
        cgen.stmt("%s->write(targetRange, actualSize)" % streamVar)
        cgen.endFor()
    
    emit_flush_ranges(COUNTING_STREAM)

    emit_parameter_encode_write_packet_info(typeInfo, api, cgen)
    emit_parameter_encode_do_parameter_write(typeInfo, api, cgen)

    emit_flush_ranges(STREAM)

    emit_parameter_encode_read(typeInfo, api, cgen)
    emit_return_unmarshal(typeInfo, api, cgen)
    emit_return(typeInfo, api, cgen)

def encode_vkInvalidateMappedMemoryRanges(typeInfo, api, cgen):
    emit_parameter_encode_preamble_write(typeInfo, api, cgen)
    emit_parameter_encode_write_packet_info(typeInfo, api, cgen)
    emit_parameter_encode_do_parameter_write(typeInfo, api, cgen)
    emit_parameter_encode_read(typeInfo, api, cgen)
    emit_return_unmarshal(typeInfo, api, cgen)

    def emit_invalidate_ranges(streamVar):
        cgen.beginFor("uint32_t i = 0", "i < memoryRangeCount", "++i")
        cgen.stmt("auto range = pMemoryRanges[i]")
        cgen.stmt("auto memory = pMemoryRanges[i].memory")
        cgen.stmt("auto size = pMemoryRanges[i].size")
        cgen.stmt("auto offset = pMemoryRanges[i].offset")
        cgen.stmt("auto goldfishMem = as_goldfish_VkDeviceMemory(memory)")
        cgen.stmt("size_t streamSize = 0")
        cgen.stmt("if (!goldfishMem) { %s->read(&streamSize, sizeof(size_t)); continue; }" % streamVar)
        cgen.stmt("auto hostPtr = goldfishMem->ptr")
        cgen.stmt("auto actualSize = size == VK_WHOLE_SIZE ? goldfishMem->size : size")
        cgen.stmt("if (!hostPtr) { %s->read(&streamSize, sizeof(size_t)); continue; }" % streamVar)
        cgen.stmt("streamSize = actualSize")
        cgen.stmt("%s->read(&streamSize, sizeof(size_t))" % streamVar)
        cgen.stmt("uint8_t* targetRange = hostPtr + offset")
        cgen.stmt("%s->read(targetRange, actualSize)" % streamVar)
        cgen.endFor()

    emit_invalidate_ranges(STREAM)

    emit_return(typeInfo, api, cgen)

custom_encodes = {
    "vkEnumerateInstanceVersion" : emit_only_goldfish_custom,
    "vkEnumerateDeviceExtensionProperties" : emit_only_goldfish_custom,
    "vkGetPhysicalDeviceProperties2" : emit_only_goldfish_custom,
    "vkMapMemory" : emit_only_goldfish_custom,
    "vkUnmapMemory" : emit_only_goldfish_custom,
    "vkFlushMappedMemoryRanges" : encode_vkFlushMappedMemoryRanges,
    "vkInvalidateMappedMemoryRanges" : encode_vkInvalidateMappedMemoryRanges,
}

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

        self.module.appendHeader(self.cgenHeader.swapCode())

        if api.name in custom_encodes.keys():
            self.module.appendImpl(self.cgenImpl.makeFuncImpl(
                apiImpl, lambda cgen: custom_encodes[api.name](self.typeInfo, api, cgen)))
        else:
            self.module.appendImpl(self.cgenImpl.makeFuncImpl(apiImpl,
                lambda cgen: emit_default_encoding(self.typeInfo, api, cgen)))

    def onEnd(self,):
        self.module.appendHeader(encoder_decl_postamble)
        self.cgenHeader.decrIndent()
