from .common.codegen import CodeGen, VulkanWrapperGenerator
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType

from .marshaling import VulkanMarshalingCodegen
from .handlemap import HandleMapCodegen
from .deepcopy import DeepcopyCodegen
from .transform import TransformCodegen, genTransformsForVulkanType

from .wrapperdefs import API_PREFIX_MARSHAL
from .wrapperdefs import API_PREFIX_UNMARSHAL
from .wrapperdefs import VULKAN_STREAM_TYPE_GUEST

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
    Impl(IOStream* stream) : m_stream(stream), m_logEncodes(false) {
        const char* emuVkLogEncodesPropName = "qemu.vk.log";
        char encodeProp[PROPERTY_VALUE_MAX];
        if (property_get(emuVkLogEncodesPropName, encodeProp, nullptr) > 0) {
            m_logEncodes = atoi(encodeProp) > 0;
        }
    }
    VulkanCountingStream* countingStream() { return &m_countingStream; }
    %s* stream() { return &m_stream; }
    Pool* pool() { return &m_pool; }
    ResourceTracker* resources() { return ResourceTracker::get(); }
    Validation* validation() { return &m_validation; }

    void log(const char* text) {
        if (!m_logEncodes) return;
        ALOGD(\"encoder log: %%s\", text);
    }
private:
    VulkanCountingStream m_countingStream;
    %s m_stream;
    Pool m_pool { 8, 4096, 64 };

    Validation m_validation;
    bool m_logEncodes;
};

VkEncoder::VkEncoder(IOStream *stream) :
    mImpl(new VkEncoder::Impl(stream)) { }

#define VALIDATE_RET(retType, success, validate) \\
    retType goldfish_vk_validateResult = validate; \\
    if (goldfish_vk_validateResult != success) return goldfish_vk_validateResult; \\

#define VALIDATE_VOID(validate) \\
    VkResult goldfish_vk_validateResult = validate; \\
    if (goldfish_vk_validateResult != VK_SUCCESS) return; \\

""" % (VULKAN_STREAM_TYPE_GUEST, VULKAN_STREAM_TYPE_GUEST)

COUNTING_STREAM = "countingStream"
STREAM = "stream"
RESOURCES = "resources"
POOL = "pool"

ENCODER_PREVALIDATED_APIS = [
    "vkFlushMappedMemoryRanges",
    "vkInvalidateMappedMemoryRanges",
]

ENCODER_CUSTOM_RESOURCE_PREPROCESS = [
    "vkMapMemoryIntoAddressSpaceGOOGLE",
    "vkDestroyDevice",
]

ENCODER_CUSTOM_RESOURCE_POSTPROCESS = [
    "vkCreateInstance",
    "vkCreateDevice",
    "vkMapMemoryIntoAddressSpaceGOOGLE",
    "vkGetPhysicalDeviceMemoryProperties",
    "vkGetPhysicalDeviceMemoryProperties2",
    "vkGetPhysicalDeviceMemoryProperties2KHR",
    "vkCreateDescriptorUpdateTemplate",
    "vkCreateDescriptorUpdateTemplateKHR",
]

ENCODER_EXPLICIT_FLUSHED_APIS = [
    "vkDestroyDevice",
]

SUCCESS_RET_TYPES = {
    "VkResult" : "VK_SUCCESS",
    "void" : None,
    # TODO: Put up success results for other return types here.
}

ENCODER_THIS_PARAM = makeVulkanTypeSimple(False, "VkEncoder", 1, "this")

# Common components of encoding a Vulkan API call
def make_event_handler_call(
    handler_access,
    api,
    context_param,
    input_result_param,
    cgen,
    suffix=""):
    extraParams = [context_param.paramName]
    if input_result_param:
        extraParams.append(input_result_param)
    return cgen.makeCallExpr( \
               "%s->on_%s%s" % (handler_access, api.name, suffix),
               extraParams + \
               [p.paramName for p in api.parameters])

def emit_custom_pre_validate(typeInfo, api, cgen):
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

def emit_custom_resource_preprocess(typeInfo, api, cgen):
    if api.name in ENCODER_CUSTOM_RESOURCE_PREPROCESS:
        cgen.stmt( \
            make_event_handler_call( \
                "mImpl->resources()", api,
                ENCODER_THIS_PARAM,
                SUCCESS_RET_TYPES[api.getRetTypeExpr()],
                cgen, suffix="_pre"))

def emit_custom_resource_postprocess(typeInfo, api, cgen):
    if api.name in ENCODER_CUSTOM_RESOURCE_POSTPROCESS:
        cgen.stmt(make_event_handler_call( \
            "mImpl->resources()",
            api,
            ENCODER_THIS_PARAM,
            api.getRetVarExpr(),
            cgen))

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

def emit_transform(typeInfo, param, cgen, variant="tohost"):
    res = \
        iterateVulkanType(typeInfo, param, TransformCodegen( \
            cgen, param.paramName, "mImpl->resources()", "transform_%s_" % variant, variant))
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
    cgen.stmt("AEMU_SCOPED_TRACE(\"%s encode\")" % api.name)

    cgen.stmt("mImpl->log(\"start %s\")" % api.name)
    emit_custom_pre_validate(typeInfo, api, cgen);
    emit_custom_resource_preprocess(typeInfo, api, cgen);

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

    apiForTransform = \
        api.withCustomParameters( \
            map(lambda p: p[1], \
                encodingParams.localCopied))

    # Apply transforms if applicable.
    # Apply transform to API itself:
    genTransformsForVulkanType(
        "mImpl->resources()",
        apiForTransform,
        lambda p: cgen.generalAccess(p, parentVarName = None, asPtr = True),
        lambda p: cgen.generalLengthAccess(p, parentVarName = None),
        cgen)

    # For all local copied parameters, run the transforms
    for localParam in apiForTransform.parameters:
        emit_transform(typeInfo, localParam, cgen, variant="tohost")

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

    cgen.stmt("AEMU_SCOPED_TRACE(\"%s readParams\")" % api.name)

    for p in encodingParams.toRead:
        if p.action == "create":
            cgen.stmt(
                "%s->setHandleMapping(%s->createMapping())" % \
                (STREAM, RESOURCES))
        emit_unmarshal(typeInfo, p, cgen)
        if p.action == "create":
            cgen.stmt(
                "%s->unsetHandleMapping()" % STREAM)
        emit_transform(typeInfo, p, cgen, variant="fromhost")

def emit_post(typeInfo, api, cgen):
    encodingParams = EncodingParameters(api)

    emit_custom_resource_postprocess(typeInfo, api, cgen)

    for p in encodingParams.toDestroy:
        emit_handlemap_destroy(typeInfo, p, cgen)

    if api.name in ENCODER_EXPLICIT_FLUSHED_APIS:
        cgen.stmt("stream->flush()");

def emit_return_unmarshal(typeInfo, api, cgen):
    cgen.stmt("AEMU_SCOPED_TRACE(\"%s returnUnmarshal\")" % api.name)

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
    cgen.stmt("mImpl->log(\"finish %s\");" % api.name);
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
    emit_post(typeInfo, api, cgen)
    emit_return(typeInfo, api, cgen)

## Custom encoding definitions##################################################

def emit_only_goldfish_custom(typeInfo, api, cgen):
    cgen.stmt("AEMU_SCOPED_TRACE(\"%s custom\")" % api.name)

    cgen.stmt("mImpl->log(\"custom start %s\");" % api.name)
    cgen.vkApiCall( \
        api,
        customPrefix="mImpl->resources()->on_",
        customParameters=custom_encoder_args(api) + \
            [p.paramName for p in api.parameters])
    emit_return(typeInfo, api, cgen)

def emit_only_resource_event(typeInfo, api, cgen):
    cgen.stmt("AEMU_SCOPED_TRACE(\"%s resourceEvent\")" % api.name)

    input_result = None
    retExpr = api.getRetVarExpr()

    if retExpr:
        retType = api.getRetTypeExpr()
        input_result = SUCCESS_RET_TYPES[retType]
        cgen.stmt("%s %s = (%s)0" % (retType, retExpr, retType))

    cgen.stmt(
        (("%s = " % retExpr) if retExpr else "") +
        make_event_handler_call(
            "mImpl->resources()",
            api,
            ENCODER_THIS_PARAM,
            input_result, cgen))

    if retExpr:
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
        cgen.beginIf("!resources->usingDirectMapping()")
        cgen.beginFor("uint32_t i = 0", "i < memoryRangeCount", "++i")
        cgen.stmt("auto range = pMemoryRanges[i]")
        cgen.stmt("auto memory = pMemoryRanges[i].memory")
        cgen.stmt("auto size = pMemoryRanges[i].size")
        cgen.stmt("auto offset = pMemoryRanges[i].offset")
        cgen.stmt("uint64_t streamSize = 0")
        cgen.stmt("if (!memory) { %s->write(&streamSize, sizeof(uint64_t)); continue; }" % streamVar)
        cgen.stmt("auto hostPtr = resources->getMappedPointer(memory)")
        cgen.stmt("auto actualSize = size == VK_WHOLE_SIZE ? resources->getMappedSize(memory) : size")
        cgen.stmt("if (!hostPtr) { %s->write(&streamSize, sizeof(uint64_t)); continue; }" % streamVar)
        cgen.stmt("streamSize = actualSize")
        cgen.stmt("%s->write(&streamSize, sizeof(uint64_t))" % streamVar)
        cgen.stmt("uint8_t* targetRange = hostPtr + offset")
        cgen.stmt("%s->write(targetRange, actualSize)" % streamVar)
        cgen.endFor()
        cgen.endIf()

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
        cgen.beginIf("!resources->usingDirectMapping()")
        cgen.beginFor("uint32_t i = 0", "i < memoryRangeCount", "++i")
        cgen.stmt("auto range = pMemoryRanges[i]")
        cgen.stmt("auto memory = pMemoryRanges[i].memory")
        cgen.stmt("auto size = pMemoryRanges[i].size")
        cgen.stmt("auto offset = pMemoryRanges[i].offset")
        cgen.stmt("uint64_t streamSize = 0")
        cgen.stmt("if (!memory) { %s->read(&streamSize, sizeof(uint64_t)); continue; }" % streamVar)
        cgen.stmt("auto hostPtr = resources->getMappedPointer(memory)")
        cgen.stmt("auto actualSize = size == VK_WHOLE_SIZE ? resources->getMappedSize(memory) : size")
        cgen.stmt("if (!hostPtr) { %s->read(&streamSize, sizeof(uint64_t)); continue; }" % streamVar)
        cgen.stmt("streamSize = actualSize")
        cgen.stmt("%s->read(&streamSize, sizeof(uint64_t))" % streamVar)
        cgen.stmt("uint8_t* targetRange = hostPtr + offset")
        cgen.stmt("%s->read(targetRange, actualSize)" % streamVar)
        cgen.endFor()
        cgen.endIf()

    emit_invalidate_ranges(STREAM)

    emit_return(typeInfo, api, cgen)

def unwrap_VkNativeBufferANDROID():
    def mapOp(cgen, orig, local):
        cgen.stmt("mImpl->resources()->unwrap_VkNativeBufferANDROID(%s, %s)" %
                  (orig.paramName, local.paramName))
    return { "pCreateInfo" : { "mapOp" : mapOp } }

def unwrap_vkAcquireImageANDROID_nativeFenceFd():
    def mapOp(cgen, orig, local):
        cgen.stmt("mImpl->resources()->unwrap_vkAcquireImageANDROID_nativeFenceFd(%s, &%s)" %
                  (orig.paramName, local.paramName))
    return { "nativeFenceFd" : { "mapOp" : mapOp } }

custom_encodes = {
    "vkMapMemory" : emit_only_resource_event,
    "vkUnmapMemory" : emit_only_resource_event,
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
