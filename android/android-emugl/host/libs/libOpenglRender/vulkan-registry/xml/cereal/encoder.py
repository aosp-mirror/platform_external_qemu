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
    Validation* validation() { return &m_validation; }
private:
    VulkanCountingStream m_countingStream;
    VulkanStream m_stream;
    Pool m_pool { 8, 4096, 64 };

    Validation m_validation;
};

VkEncoder::VkEncoder(IOStream *stream) :
    mImpl(new VkEncoder::Impl(stream)) { }

#define VALIDATE_RET(retType, success, validate) \\
    retType goldfish_vk_validateResult = validate; \\
    if (goldfish_vk_validateResult != success) return goldfish_vk_validateResult; \\

#define VALIDATE_VOID(validate) \\
    VkResult goldfish_vk_validateResult = validate; \\
    if (goldfish_vk_validateResult != VK_SUCCESS) return; \\

"""

COUNTING_STREAM = "countingStream"
STREAM = "stream"
RESOURCES = "resources"
POOL = "pool"

ENCODER_PREVALIDATED_APIS = [
    "vkFlushMappedMemoryRanges",
    "vkInvalidateMappedMemoryRanges",
]

SUCCESS_RET_TYPES = {
    "VkResult" : "VK_SUCCESS",
    # TODO: Put up success results for other return types here.
}

ENCODER_THIS_PARAM = makeVulkanTypeSimple(False, "VkEncoder", 1, "this")

# Common components of encoding a Vulkan API call
def make_event_handler_call(
    handler_access,
    api,
    context_param,
    input_result_param,
    cgen):
    return cgen.makeCallExpr( \
               "%s->on_%s" % (handler_access, api.name),
               [context_param.paramName, input_result_param] + \
               [p.paramName for p in api.parameters])

def emit_custom_prevalidate(typeInfo, api, cgen):
    if api.name in ENCODER_PREVALIDATED_APIS:
        callExpr = \
            make_event_handler_call( \
                "mImpl->validation()", api,
                ENCODER_THIS_PARAM,
                SUCCESS_RET_TYPES[api.getRetTypeExpr()],
                cgen)

        if api.getRetTypeExpr() == "void":
            cgen.stmt("VALIDATE_VOID(%s)" % callExpr)
        else:
            cgen.stmt("VALIDATE_RET(%s, %s, %s)" % \
                (api.getRetTypeExpr(),
                 SUCCESS_RET_TYPES[api.getRetTypeExpr()],
                 callExpr))

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
    forOutput = param.isHandleType() and ("out" in param.inout)
    if forOutput:
        cgen.stmt("%s->unsetHandleMapping() /* emit_marshal, is handle, possibly out */" % STREAM)

    res = \
        iterateVulkanType(
            typeInfo, param,
            VulkanMarshalingCodegen( \
               cgen, STREAM, param.paramName,
               API_PREFIX_MARSHAL, direction="write"))
    if not res:
        cgen.stmt("(void)%s" % param.paramName)

    if forOutput:
        cgen.stmt("%s->setHandleMapping(resources->unwrapMapping())" % STREAM)

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

def emit_handlemap_create(typeInfo, param, cgen):
    iterateVulkanType(typeInfo, param, HandleMapCodegen(
        cgen, None, "resources->createMapping()", "handlemap_",
        lambda vtype: typeInfo.isHandleType(vtype.typeName)
    ))

def custom_encoder_args(api):
    params = ["this"]
    if api.getRetVarExpr() is not None:
        params.append(api.getRetVarExpr())
    return params

def emit_custom_create(typeInfo, api, cgen):
    if api.name in CUSTOM_CREATE_APIS:
        cgen.funcCall(
            None,
            "goldfish_" + api.name,
            custom_encoder_args(api) + \
            [p.paramName for p in api.parameters])

def emit_handlemap_destroy(typeInfo, param, cgen):
    iterateVulkanType(typeInfo, param, HandleMapCodegen(
        cgen, None, "resources->destroyMapping()", "handlemap_",
        lambda vtype: typeInfo.isHandleType(vtype.typeName)
    ))

class EncodingParameters(object):
    def __init__(self, api):
        self.localCopied = []
        self.toWrite = []
        self.toRead = []
        self.toCreate = []
        self.toDestroy = []

        for param in api.parameters:
            param.action = None
            param.inout = "in"

            if param.possiblyOutput():
                param.inout += "out"
                self.toWrite.append(param)
                self.toRead.append(param)
                if param.isCreatedBy(api):
                    self.toCreate.append(param)
                    param.action = "create"
            else:
                if param.isDestroyedBy(api):
                    self.toDestroy.append(param)
                    param.action = "destroy"
                localCopyParam = \
                    param.getForNonConstAccess().withModifiedName( \
                        "local_" + param.paramName)
                self.localCopied.append((param, localCopyParam))
                self.toWrite.append(localCopyParam)

def emit_parameter_encode_preamble_write(typeInfo, api, cgen):
    emit_custom_prevalidate(typeInfo, api, cgen);

    cgen.stmt("auto %s = mImpl->stream()" % STREAM)
    cgen.stmt("auto %s = mImpl->countingStream()" % COUNTING_STREAM)
    cgen.stmt("auto %s = mImpl->resources()" % RESOURCES)
    cgen.stmt("auto %s = mImpl->pool()" % POOL)
    cgen.stmt("%s->setHandleMapping(%s->unwrapMapping())" % (STREAM, RESOURCES))

    encodingParams = EncodingParameters(api)
    for (_, localCopyParam) in encodingParams.localCopied:
        cgen.stmt(cgen.makeRichCTypeDecl(localCopyParam))

def emit_parameter_encode_copy_unwrap_count(typeInfo, api, cgen, customUnwrap=None):
    encodingParams = EncodingParameters(api)

    for (origParam, localCopyParam) in encodingParams.localCopied:
        shouldCustomCopy = \
            customUnwrap and \
            origParam.paramName in customUnwrap and \
            "copyOp" in customUnwrap[origParam.paramName]

        if shouldCustomCopy:
            customUnwrap[origParam.paramName]["copyOp"](cgen, origParam, localCopyParam)
        else:
            emit_deepcopy(typeInfo, origParam, cgen)

    for (origParam, localCopyParam) in encodingParams.localCopied:
        shouldCustomMap = \
            customUnwrap and \
            origParam.paramName in customUnwrap and \
            "mapOp" in customUnwrap[origParam.paramName]

        if shouldCustomMap:
            customUnwrap[origParam.paramName]["mapOp"](cgen, origParam, localCopyParam)
        else:
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
        if p.action == "create":
            cgen.stmt(
                "%s->setHandleMapping(%s->createMapping())" % \
                (STREAM, RESOURCES))
        emit_unmarshal(typeInfo, p, cgen)
        if p.action == "create":
            cgen.stmt(
                "%s->unsetHandleMapping()" % STREAM)

def emit_custom_create_destroy(typeInfo, api, cgen):
    encodingParams = EncodingParameters(api)

    for p in encodingParams.toCreate:
        emit_custom_create(typeInfo, api, cgen)

    for p in encodingParams.toDestroy:
        emit_handlemap_destroy(typeInfo, p, cgen)

def emit_return_unmarshal(typeInfo, api, cgen):
    retType = api.getRetTypeExpr()

    if retType == "void":
        return

    retVar = api.getRetVarExpr()
    cgen.stmt("%s %s = (%s)0" % (retType, retVar, retType))
    cgen.stmt("%s->read(&%s, %s)" % \
              (STREAM, retVar, cgen.sizeofExpr(api.retType)))
    cgen.stmt("%s->clearPool()" % COUNTING_STREAM)
    cgen.stmt("%s->clearPool()" % STREAM)
    cgen.stmt("pool->freeAll()")

def emit_return(typeInfo, api, cgen):
    if api.getRetTypeExpr() == "void":
        return

    retVar = api.getRetVarExpr()
    cgen.stmt("return %s" % retVar)

def emit_default_encoding(typeInfo, api, cgen):
    emit_parameter_encode_preamble_write(typeInfo, api, cgen)
    emit_parameter_encode_copy_unwrap_count(typeInfo, api, cgen)
    emit_parameter_encode_write_packet_info(typeInfo, api, cgen)
    emit_parameter_encode_do_parameter_write(typeInfo, api, cgen)
    emit_parameter_encode_read(typeInfo, api, cgen)
    emit_return_unmarshal(typeInfo, api, cgen)
    emit_custom_create_destroy(typeInfo, api, cgen)
    emit_return(typeInfo, api, cgen)

## Custom encoding definitions##################################################

def emit_only_goldfish_custom(typeInfo, api, cgen):
    cgen.vkApiCall( \
        api,
        customPrefix="goldfish_",
        customParameters=custom_encoder_args(api) + \
            [p.paramName for p in api.parameters])
    emit_return(typeInfo, api, cgen)

def emit_with_custom_unwrap(custom):
    def call(typeInfo, api, cgen):
        emit_parameter_encode_preamble_write(typeInfo, api, cgen)
        emit_parameter_encode_copy_unwrap_count(
            typeInfo, api, cgen, customUnwrap=custom)
        emit_parameter_encode_write_packet_info(typeInfo, api, cgen)
        emit_parameter_encode_do_parameter_write(typeInfo, api, cgen)
        emit_parameter_encode_read(typeInfo, api, cgen)
        emit_return_unmarshal(typeInfo, api, cgen)
        emit_return(typeInfo, api, cgen)
    return call

def encode_vkFlushMappedMemoryRanges(typeInfo, api, cgen):
    emit_parameter_encode_preamble_write(typeInfo, api, cgen)
    emit_parameter_encode_copy_unwrap_count(typeInfo, api, cgen)

    def emit_flush_ranges(streamVar):
        cgen.beginFor("uint32_t i = 0", "i < memoryRangeCount", "++i")
        cgen.stmt("auto range = pMemoryRanges[i]")
        cgen.stmt("auto memory = pMemoryRanges[i].memory")
        cgen.stmt("auto size = pMemoryRanges[i].size")
        cgen.stmt("auto offset = pMemoryRanges[i].offset")
        cgen.stmt("auto goldfishMem = as_goldfish_VkDeviceMemory(memory)")
        cgen.stmt("uint64_t streamSize = 0")
        cgen.stmt("if (!goldfishMem) { %s->write(&streamSize, sizeof(uint64_t)); continue; }" % streamVar)
        cgen.stmt("auto hostPtr = goldfishMem->ptr")
        cgen.stmt("auto actualSize = size == VK_WHOLE_SIZE ? goldfishMem->size : size")
        cgen.stmt("if (!hostPtr) { %s->write(&streamSize, sizeof(uint64_t)); continue; }" % streamVar)
        cgen.stmt("streamSize = actualSize")
        cgen.stmt("%s->write(&streamSize, sizeof(uint64_t))" % streamVar)
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
    emit_parameter_encode_copy_unwrap_count(typeInfo, api, cgen)
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
        cgen.stmt("uint64_t streamSize = 0")
        cgen.stmt("if (!goldfishMem) { %s->read(&streamSize, sizeof(uint64_t)); continue; }" % streamVar)
        cgen.stmt("auto hostPtr = goldfishMem->ptr")
        cgen.stmt("auto actualSize = size == VK_WHOLE_SIZE ? goldfishMem->size : size")
        cgen.stmt("if (!hostPtr) { %s->read(&streamSize, sizeof(uint64_t)); continue; }" % streamVar)
        cgen.stmt("streamSize = actualSize")
        cgen.stmt("%s->read(&streamSize, sizeof(uint64_t))" % streamVar)
        cgen.stmt("uint8_t* targetRange = hostPtr + offset")
        cgen.stmt("%s->read(targetRange, actualSize)" % streamVar)
        cgen.endFor()

    emit_invalidate_ranges(STREAM)

    emit_return(typeInfo, api, cgen)

def unwrap_VkNativeBufferANDROID():
    def mapOp(cgen, orig, local):
        cgen.stmt("goldfish_unwrap_VkNativeBufferANDROID(%s, %s)" %
                  (orig.paramName, local.paramName))
    return { "pCreateInfo" : { "mapOp" : mapOp } }

def unwrap_vkAcquireImageANDROID_nativeFenceFd():
    def mapOp(cgen, orig, local):
        cgen.stmt("goldfish_unwrap_vkAcquireImageANDROID_nativeFenceFd(%s, &%s)" %
                  (orig.paramName, local.paramName))
    return { "nativeFenceFd" : { "mapOp" : mapOp } }

custom_encodes = {
    "vkEnumerateInstanceVersion" : emit_only_goldfish_custom,
    "vkEnumerateDeviceExtensionProperties" : emit_only_goldfish_custom,
    "vkGetPhysicalDeviceProperties2" : emit_only_goldfish_custom,
    "vkMapMemory" : emit_only_goldfish_custom,
    "vkUnmapMemory" : emit_only_goldfish_custom,
    "vkFlushMappedMemoryRanges" : encode_vkFlushMappedMemoryRanges,
    "vkInvalidateMappedMemoryRanges" : encode_vkInvalidateMappedMemoryRanges,
    "vkCreateImage" : emit_with_custom_unwrap(unwrap_VkNativeBufferANDROID()),
    "vkAcquireImageANDROID" : emit_with_custom_unwrap(unwrap_vkAcquireImageANDROID_nativeFenceFd()),
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
