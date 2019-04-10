from .common.codegen import CodeGen, VulkanWrapperGenerator, VulkanAPIWrapper
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType, DISPATCHABLE_HANDLE_TYPES, NON_DISPATCHABLE_HANDLE_TYPES

from .marshaling import VulkanMarshalingCodegen
from .transform import TransformCodegen, genTransformsForVulkanType

from .wrapperdefs import API_PREFIX_MARSHAL
from .wrapperdefs import API_PREFIX_UNMARSHAL
from .wrapperdefs import VULKAN_STREAM_TYPE

from copy import copy

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

using android::base::System;

class VkDecoder::Impl {
public:
    Impl() : m_logCalls(System::get()->envGet("ANDROID_EMU_VK_LOG_CALLS") == "1"), m_vk(vkDispatch()), m_state(VkDecoderGlobalState::get()) { }
    %s* stream() { return &m_vkStream; }
    VulkanMemReadingStream* readStream() { return &m_vkMemReadingStream; }

    size_t decode(void* buf, size_t bufsize, IOStream* stream);

private:
    bool m_logCalls;
    VulkanDispatch* m_vk;
    VkDecoderGlobalState* m_state;
    %s m_vkStream { nullptr };
    VulkanMemReadingStream m_vkMemReadingStream { nullptr };
    BoxedHandleUnwrapMapping m_boxedHandleUnwrapMapping;
    BoxedHandleCreateMapping m_boxedHandleCreateMapping;
    BoxedHandleDestroyMapping m_boxedHandleDestroyMapping;
    BoxedHandleUnwrapAndDeleteMapping m_boxedHandleUnwrapAndDeleteMapping;
    android::base::Pool m_pool { 8, 4096, 64 };
};

VkDecoder::VkDecoder() :
    mImpl(new VkDecoder::Impl()) { }

VkDecoder::~VkDecoder() = default;

size_t VkDecoder::decode(void* buf, size_t bufsize, IOStream* stream) {
    return mImpl->decode(buf, bufsize, stream);
}

// VkDecoder::Impl::decode to follow
""" % (VULKAN_STREAM_TYPE, VULKAN_STREAM_TYPE)

READ_STREAM = "vkReadStream"
WRITE_STREAM = "vkStream"

def emit_param_decl_for_reading(param, cgen):
    if param.staticArrExpr:
        cgen.stmt(
            cgen.makeRichCTypeDecl(param.getForNonConstAccess()))
    else:
        cgen.stmt(
            cgen.makeRichCTypeDecl(param))

def emit_unmarshal(typeInfo, param, cgen):
    iterateVulkanType(typeInfo, param, VulkanMarshalingCodegen(
        cgen,
        READ_STREAM,
        param.paramName,
        API_PREFIX_UNMARSHAL,
        direction="read",
        dynAlloc=True))

def emit_dispatch_unmarshal(typeInfo, param, cgen):
    cgen.stmt("// Begin manual dispatchable handle unboxing for %s" % param.paramName)
    cgen.stmt("%s->unsetHandleMapping()" % READ_STREAM)
    iterateVulkanType(typeInfo, param, VulkanMarshalingCodegen(
        cgen,
        READ_STREAM,
        param.paramName,
        API_PREFIX_UNMARSHAL,
        direction="read",
        dynAlloc=True))
    cgen.stmt("auto unboxed_%s = unbox_%s(%s)" %
              (param.paramName, param.typeName, param.paramName))
    cgen.stmt("auto vk = dispatch_%s(%s)" %
              (param.typeName, param.paramName))
    cgen.stmt("%s->setHandleMapping(&m_boxedHandleUnwrapMapping)" % READ_STREAM)
    cgen.stmt("// End manual dispatchable handle unboxing for %s" % param.paramName)

def emit_transform(typeInfo, param, cgen, variant="tohost"):
    res = \
        iterateVulkanType(typeInfo, param, TransformCodegen( \
            cgen, param.paramName, "m_state", "transform_%s_" % variant, variant))
    if not res:
        cgen.stmt("(void)%s" % param.paramName)

def emit_marshal(typeInfo, param, cgen):
    iterateVulkanType(typeInfo, param, VulkanMarshalingCodegen(
        cgen,
        WRITE_STREAM,
        param.paramName,
        API_PREFIX_MARSHAL,
        direction="write"))

class DecodingParameters(object):
    def __init__(self, api):
        self.params = []
        self.toRead = []
        self.toWrite = []

        i = 0

        for param in api.parameters:
            param.nonDispatchableHandleCreate = False
            param.nonDispatchableHandleDestroy = False
            param.dispatchHandle = False

            if i == 0 and param.isDispatchableHandleType():
                param.dispatchHandle = True

            if param.isNonDispatchableHandleType() and param.isCreatedBy(api):
                param.nonDispatchableHandleCreate = True

            if param.isNonDispatchableHandleType() and param.isDestroyedBy(api):
                param.nonDispatchableHandleDestroy = True

            self.toRead.append(param)

            if param.possiblyOutput():
                self.toWrite.append(param)

            self.params.append(param)

            i += 1

def emit_decode_parameters(typeInfo, api, cgen):

    decodingParams = DecodingParameters(api)

    paramsToRead = decodingParams.toRead

    for p in paramsToRead:
        emit_param_decl_for_reading(p, cgen)

    i = 0
    for p in paramsToRead:
        if p.dispatchHandle:
            emit_dispatch_unmarshal(typeInfo, p, cgen)
        else:
            if p.nonDispatchableHandleDestroy:
                cgen.stmt("// Begin manual non dispatchable handle destroy unboxing for %s" % p.paramName)
                cgen.stmt("%s->setHandleMapping(&m_boxedHandleUnwrapAndDeleteMapping)" % READ_STREAM)

            if p.possiblyOutput():
                cgen.stmt("// Begin manual dispatchable handle unboxing for %s" % p.paramName)
                cgen.stmt("%s->unsetHandleMapping()" % READ_STREAM)

            emit_unmarshal(typeInfo, p, cgen)

            if p.nonDispatchableHandleDestroy:
                cgen.stmt("%s->setHandleMapping(&m_boxedHandleUnwrapMapping)" % READ_STREAM)
                cgen.stmt("// End manual non dispatchable handle destroy unboxing for %s" % p.paramName)

            if p.possiblyOutput():
                cgen.stmt("%s->setHandleMapping(&m_boxedHandleUnwrapMapping)" % READ_STREAM)
                cgen.stmt("// End manual dispatchable handle unboxing for %s" % p.paramName)
        i += 1

    for p in paramsToRead:
        emit_transform(typeInfo, p, cgen, variant="tohost");

def emit_call_log(api, cgen):
    cgen.beginIf("m_logCalls")
    cgen.stmt("fprintf(stderr, \"call %s\\n\");" % api.name)
    cgen.endIf()

def emit_dispatch_call(api, cgen):

    decodingParams = DecodingParameters(api)

    customParams = []

    for (i, p) in enumerate(api.parameters):
        customParam = p.paramName
        if decodingParams.params[i].dispatchHandle:
            customParam = "unboxed_%s" % p.paramName
        customParams.append(customParam)

    cgen.vkApiCall(api, customPrefix="vk->", customParameters=customParams)

def emit_global_state_wrapped_call(api, cgen):
    customParams = ["&m_pool"] + list(map(lambda p: p.paramName, api.parameters))
    cgen.vkApiCall(api, customPrefix="m_state->on_", \
        customParameters=customParams)

def emit_decode_parameters_writeback(typeInfo, api, cgen, autobox=True):
    decodingParams = DecodingParameters(api)

    paramsToWrite = decodingParams.toWrite

    cgen.stmt("%s->unsetHandleMapping()" % WRITE_STREAM)

    for p in paramsToWrite:
        emit_transform(typeInfo, p, cgen, variant="fromhost")

        if autobox and p.nonDispatchableHandleCreate:
            cgen.stmt("// Begin auto non dispatchable handle create for %s" % p.paramName)
            cgen.stmt("%s->setHandleMapping(&m_boxedHandleCreateMapping)" % WRITE_STREAM)

        if (not autobox) and p.nonDispatchableHandleCreate:
            cgen.stmt("// Begin manual non dispatchable handle create for %s" % p.paramName)
            cgen.stmt("%s->unsetHandleMapping()" % WRITE_STREAM)

        emit_marshal(typeInfo, p, cgen)

        if autobox and p.nonDispatchableHandleCreate:
            cgen.stmt("// Begin auto non dispatchable handle create for %s" % p.paramName)
            cgen.stmt("%s->setHandleMapping(&m_boxedHandleUnwrapMapping)" % WRITE_STREAM)

        if (not autobox) and p.nonDispatchableHandleCreate:
            cgen.stmt("// Begin manual non dispatchable handle create for %s" % p.paramName)
            cgen.stmt("%s->setHandleMapping(&m_boxedHandleUnwrapMapping)" % WRITE_STREAM)

def emit_decode_return_writeback(api, cgen):
    retTypeName = api.getRetTypeExpr()
    if retTypeName != "void":
        retVar = api.getRetVarExpr()
        cgen.stmt("%s->write(&%s, %s)" %
            (WRITE_STREAM, retVar, cgen.sizeofExpr(api.retType)))

def emit_decode_finish(cgen):
    cgen.stmt("%s->clearPool()" % READ_STREAM)
    cgen.stmt("m_pool.freeAll()")
    cgen.stmt("%s->commitWrite()" % WRITE_STREAM)

def emit_snapshot(typeInfo, api, cgen):

    additionalParams = [makeVulkanTypeSimple(False, "android::base::Pool", 1, "&m_pool")]

    retTypeName = api.getRetTypeExpr()
    if retTypeName != "void":
        retVar = api.getRetVarExpr()
        additionalParams.append(makeVulkanTypeSimple(False, retTypeName, 0, retVar))

    customParams = additionalParams + api.parameters

    apiForSnapshot = \
        api.withCustomReturnType(makeVulkanTypeSimple(False, "void", 0, "void")). \
            withCustomParameters(customParams)

    cgen.vkApiCall(apiForSnapshot, customPrefix="m_state->snapshot()->")

def emit_default_decoding(typeInfo, api, cgen):
    emit_call_log(api, cgen)
    emit_decode_parameters(typeInfo, api, cgen)
    emit_dispatch_call(api, cgen)
    emit_decode_parameters_writeback(typeInfo, api, cgen)
    emit_decode_return_writeback(api, cgen)
    emit_decode_finish(cgen)
    emit_snapshot(typeInfo, api, cgen);

def emit_global_state_wrapped_decoding(typeInfo, api, cgen):
    emit_call_log(api, cgen)
    emit_decode_parameters(typeInfo, api, cgen)
    emit_global_state_wrapped_call(api, cgen)
    emit_decode_parameters_writeback(typeInfo, api, cgen, autobox=False)
    emit_decode_return_writeback(api, cgen)
    emit_decode_finish(cgen)
    emit_snapshot(typeInfo, api, cgen);

## Custom decoding definitions##################################################
def decode_vkFlushMappedMemoryRanges(typeInfo, api, cgen):
    emit_call_log(api, cgen)
    emit_decode_parameters(typeInfo, api, cgen)

    cgen.beginIf("!m_state->usingDirectMapping()")
    cgen.beginFor("uint32_t i = 0", "i < memoryRangeCount", "++i")
    cgen.stmt("auto range = pMemoryRanges[i]")
    cgen.stmt("auto memory = pMemoryRanges[i].memory")
    cgen.stmt("auto size = pMemoryRanges[i].size")
    cgen.stmt("auto offset = pMemoryRanges[i].offset")
    cgen.stmt("uint64_t readStream = 0")
    cgen.stmt("%s->read(&readStream, sizeof(uint64_t))" % READ_STREAM)
    cgen.stmt("auto hostPtr = m_state->getMappedHostPointer(memory)")
    cgen.stmt("if (!hostPtr && readStream > 0) abort()")
    cgen.stmt("if (!hostPtr) continue")
    cgen.stmt("uint8_t* targetRange = hostPtr + offset")
    cgen.stmt("%s->read(targetRange, readStream)" % READ_STREAM)
    cgen.endFor()
    cgen.endIf()

    emit_dispatch_call(api, cgen)
    emit_decode_parameters_writeback(typeInfo, api, cgen)
    emit_decode_return_writeback(api, cgen)
    emit_decode_finish(cgen)
    emit_snapshot(typeInfo, api, cgen);

def decode_vkInvalidateMappedMemoryRanges(typeInfo, api, cgen):
    emit_call_log(api, cgen)
    emit_decode_parameters(typeInfo, api, cgen)
    emit_dispatch_call(api, cgen)
    emit_decode_parameters_writeback(typeInfo, api, cgen)
    emit_decode_return_writeback(api, cgen)

    cgen.beginIf("!m_state->usingDirectMapping()")
    cgen.beginFor("uint32_t i = 0", "i < memoryRangeCount", "++i")
    cgen.stmt("auto range = pMemoryRanges[i]")
    cgen.stmt("auto memory = range.memory")
    cgen.stmt("auto size = range.size")
    cgen.stmt("auto offset = range.offset")
    cgen.stmt("auto hostPtr = m_state->getMappedHostPointer(memory)")
    cgen.stmt("auto actualSize = size == VK_WHOLE_SIZE ? m_state->getDeviceMemorySize(memory) : size")
    cgen.stmt("uint64_t writeStream = 0")
    cgen.stmt("if (!hostPtr) { %s->write(&writeStream, sizeof(uint64_t)); continue; }" % WRITE_STREAM)
    cgen.stmt("uint8_t* targetRange = hostPtr + offset")
    cgen.stmt("writeStream = actualSize")
    cgen.stmt("%s->write(&writeStream, sizeof(uint64_t))" % WRITE_STREAM)
    cgen.stmt("%s->write(targetRange, actualSize)" % WRITE_STREAM)
    cgen.endFor()
    cgen.endIf()

    emit_decode_finish(cgen)
    emit_snapshot(typeInfo, api, cgen);

custom_decodes = {
    "vkEnumerateInstanceVersion" : emit_global_state_wrapped_decoding,
    "vkCreateInstance" : emit_global_state_wrapped_decoding,
    "vkDestroyInstance" : emit_global_state_wrapped_decoding,
    "vkEnumeratePhysicalDevices" : emit_global_state_wrapped_decoding,

    "vkGetPhysicalDeviceFeatures" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceFeatures2" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceFeatures2KHR" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceFormatProperties" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceFormatProperties2" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceFormatProperties2KHR" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceImageFormatProperties" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceImageFormatProperties2" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceImageFormatProperties2KHR" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceProperties" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceProperties2" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceProperties2KHR" : emit_global_state_wrapped_decoding,

    "vkGetPhysicalDeviceMemoryProperties" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceMemoryProperties2" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceMemoryProperties2KHR" : emit_global_state_wrapped_decoding,

    "vkGetPhysicalDeviceExternalSemaphoreProperties" : emit_global_state_wrapped_decoding,
    "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR" : emit_global_state_wrapped_decoding,

    "vkCreateBuffer" : emit_global_state_wrapped_decoding,
    "vkDestroyBuffer" : emit_global_state_wrapped_decoding,

    "vkBindBufferMemory" : emit_global_state_wrapped_decoding,
    "vkBindBufferMemory2" : emit_global_state_wrapped_decoding,
    "vkBindBufferMemory2KHR" : emit_global_state_wrapped_decoding,

    "vkCreateDevice" : emit_global_state_wrapped_decoding,
    "vkGetDeviceQueue" : emit_global_state_wrapped_decoding,
    "vkDestroyDevice" : emit_global_state_wrapped_decoding,

    "vkBindImageMemory" : emit_global_state_wrapped_decoding,
    "vkCreateImage" : emit_global_state_wrapped_decoding,
    "vkCreateImageView" : emit_global_state_wrapped_decoding,
    "vkCreateSampler" : emit_global_state_wrapped_decoding,
    "vkDestroyImage" : emit_global_state_wrapped_decoding,
    "vkDestroyImageView" : emit_global_state_wrapped_decoding,
    "vkDestroySampler" : emit_global_state_wrapped_decoding,
    "vkCmdCopyBufferToImage" : emit_global_state_wrapped_decoding,
    "vkCmdCopyImageToBuffer" : emit_global_state_wrapped_decoding,
    "vkGetImageMemoryRequirements" : emit_global_state_wrapped_decoding,
    "vkGetImageMemoryRequirements2" : emit_global_state_wrapped_decoding,
    "vkGetImageMemoryRequirements2KHR" : emit_global_state_wrapped_decoding,

    "vkUpdateDescriptorSets" : emit_global_state_wrapped_decoding,

    "vkAllocateMemory" : emit_global_state_wrapped_decoding,
    "vkFreeMemory" : emit_global_state_wrapped_decoding,
    "vkMapMemory" : emit_global_state_wrapped_decoding,
    "vkUnmapMemory" : emit_global_state_wrapped_decoding,
    "vkFlushMappedMemoryRanges" : decode_vkFlushMappedMemoryRanges,
    "vkInvalidateMappedMemoryRanges" : decode_vkInvalidateMappedMemoryRanges,

    "vkAllocateCommandBuffers" : emit_global_state_wrapped_decoding,
    "vkCmdExecuteCommands" : emit_global_state_wrapped_decoding,
    "vkQueueSubmit" : emit_global_state_wrapped_decoding,
    "vkBeginCommandBuffer" : emit_global_state_wrapped_decoding,
    "vkResetCommandBuffer" : emit_global_state_wrapped_decoding,
    "vkFreeCommandBuffers" : emit_global_state_wrapped_decoding,
    "vkCreateCommandPool" : emit_global_state_wrapped_decoding,
    "vkDestroyCommandPool" : emit_global_state_wrapped_decoding,
    "vkResetCommandPool" : emit_global_state_wrapped_decoding,
    "vkCmdPipelineBarrier" : emit_global_state_wrapped_decoding,
    "vkCmdBindPipeline" : emit_global_state_wrapped_decoding,
    "vkCmdBindDescriptorSets" : emit_global_state_wrapped_decoding,

    # VK_ANDROID_native_buffer
    "vkGetSwapchainGrallocUsageANDROID" : emit_global_state_wrapped_decoding,
    "vkGetSwapchainGrallocUsage2ANDROID" : emit_global_state_wrapped_decoding,
    "vkAcquireImageANDROID" : emit_global_state_wrapped_decoding,
    "vkQueueSignalReleaseImageANDROID" : emit_global_state_wrapped_decoding,

    "vkCreateSemaphore" : emit_global_state_wrapped_decoding,
    "vkGetSemaphoreFdKHR" : emit_global_state_wrapped_decoding,
    "vkImportSemaphoreFdKHR" : emit_global_state_wrapped_decoding,
    "vkDestroySemaphore" : emit_global_state_wrapped_decoding,

    # VK_GOOGLE_address_space
    "vkMapMemoryIntoAddressSpaceGOOGLE" : emit_global_state_wrapped_decoding,

    # VK_GOOGLE_color_buffer
    "vkRegisterImageColorBufferGOOGLE" : emit_global_state_wrapped_decoding,
    "vkRegisterBufferColorBufferGOOGLE" : emit_global_state_wrapped_decoding,

    # Descriptor update templates
    "vkCreateDescriptorUpdateTemplate" : emit_global_state_wrapped_decoding,
    "vkCreateDescriptorUpdateTemplateKHR" : emit_global_state_wrapped_decoding,
    "vkDestroyDescriptorUpdateTemplate" : emit_global_state_wrapped_decoding,
    "vkDestroyDescriptorUpdateTemplateKHR" : emit_global_state_wrapped_decoding,
    "vkUpdateDescriptorSetWithTemplateSizedGOOGLE" : emit_global_state_wrapped_decoding,

    # VK_GOOGLE_async_command_buffer
    "vkBeginCommandBufferAsyncGOOGLE" : emit_global_state_wrapped_decoding,
    "vkEndCommandBufferAsyncGOOGLE" : emit_global_state_wrapped_decoding,
    "vkResetCommandBufferAsyncGOOGLE" : emit_global_state_wrapped_decoding,
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
        self.cgen.stmt("%s->setHandleMapping(&m_boxedHandleUnwrapMapping)" % READ_STREAM)
        self.cgen.stmt("auto vk = m_vk")

        self.cgen.line("switch (opcode)")
        self.cgen.beginBlock() # switch stmt

        self.module.appendImpl(self.cgen.swapCode())

    def onGenCmd(self, cmdinfo, name, alias):
        typeInfo = self.typeInfo
        cgen = self.cgen
        api = typeInfo.apis[name]

        cgen.line("case OP_%s:" % name)
        cgen.beginBlock()

        if api.name in custom_decodes.keys():
            custom_decodes[api.name](typeInfo, api, cgen)
        else:
            emit_decode_parameters(typeInfo, api, cgen)
            emit_dispatch_call(api, cgen)
            emit_decode_parameters_writeback(typeInfo, api, cgen)
            emit_decode_return_writeback(api, cgen)
            emit_decode_finish(cgen)
            emit_snapshot(typeInfo, api, cgen);

        cgen.stmt("break")
        cgen.endBlock()
        self.module.appendImpl(self.cgen.swapCode())

    def onEnd(self,):
        self.cgen.line("default:")
        self.cgen.beginBlock()
        self.cgen.stmt("return ptr - (unsigned char *)buf")
        self.cgen.endBlock()

        self.cgen.endBlock() # switch stmt

        self.cgen.stmt("ptr += packetLen")
        self.cgen.endBlock() # while loop

        self.cgen.stmt("return ptr - (unsigned char*)buf;")
        self.cgen.endBlock() # function body
        self.module.appendImpl(self.cgen.swapCode())
