from .common.codegen import CodeGen, VulkanWrapperGenerator, VulkanAPIWrapper
from .common.vulkantypes import \
    VulkanAPI, makeVulkanTypeSimple, iterateVulkanType, DISPATCHABLE_HANDLE_TYPES, NON_DISPATCHABLE_HANDLE_TYPES

from .reservedmarshaling import VulkanReservedMarshalingCodegen
from .transform import TransformCodegen, genTransformsForVulkanType

from .wrapperdefs import API_PREFIX_MARSHAL
from .wrapperdefs import API_PREFIX_UNMARSHAL, API_PREFIX_RESERVEDUNMARSHAL
from .wrapperdefs import ROOT_TYPE_DEFAULT_VALUE
from .wrapperdefs import VULKAN_STREAM_TYPE

from copy import copy

decoder_decl_preamble = """
"""

decoder_impl_preamble = """
"""

READ_STREAM = "readStream"
WRITE_STREAM = "vkStream"

# Driver workarounds for APIs that don't work well multithreaded
driver_workarounds_global_lock_apis = [
    "vkCreatePipelineLayout",
    "vkDestroyPipelineLayout",
]

MAX_STACK_ITEMS = "16"


def emit_param_decl_for_reading(param, cgen):
    if param.staticArrExpr:
        cgen.stmt(
            cgen.makeRichCTypeDecl(param.getForNonConstAccess()))
    else:
        cgen.stmt(
            cgen.makeRichCTypeDecl(param))

    if param.pointerIndirectionLevels > 0:
        lenAccess = cgen.generalLengthAccess(param)
        if not lenAccess:
            lenAccess = "1"
        arrSize = "1" if "1" == lenAccess else "MAX_STACK_ITEMS"

        typeHere = "uint8_t*" if "void" == param.typeName else param.typeName
        cgen.stmt("%s%s stack_%s[%s]" % (
            typeHere, "*" * (param.pointerIndirectionLevels - 1), param.paramName, arrSize))


def emit_unmarshal(typeInfo, param, cgen, output=False, destroy=False, noUnbox=False):
    if destroy:
        iterateVulkanType(typeInfo, param, VulkanReservedMarshalingCodegen(
            cgen,
            READ_STREAM,
            ROOT_TYPE_DEFAULT_VALUE,
            param.paramName,
            "readStreamPtrPtr",
            API_PREFIX_RESERVEDUNMARSHAL,
            "",
            direction="read",
            dynAlloc=True))
        lenAccess = cgen.generalLengthAccess(param)
        lenAccessGuard = cgen.generalLengthAccessGuard(param)
        if None == lenAccess or "1" == lenAccess:
            cgen.stmt("boxed_%s_preserve = %s" %
                      (param.paramName, param.paramName))
            cgen.stmt("%s = unbox_%s(%s)" %
                      (param.paramName, param.typeName, param.paramName))
        else:
            if lenAccessGuard is not None:
                self.cgen.beginIf(lenAccessGuard)
            cgen.beginFor("uint32_t i = 0", "i < %s" % lenAccess, "++i")
            cgen.stmt("boxed_%s_preserve[i] = %s[i]" %
                      (param.paramName, param.paramName))
            cgen.stmt("((%s*)(%s))[i] = unbox_%s(%s[i])" % (param.typeName,
                                                            param.paramName, param.typeName, param.paramName))
            cgen.endFor()
            if lenAccessGuard is not None:
                self.cgen.endIf()
    else:
        if noUnbox:
            cgen.line("// No unbox for %s" % (param.paramName))

        lenAccess = cgen.generalLengthAccess(param)
        if not lenAccess:
            lenAccess = "1"
        arrSize = "1" if "1" == lenAccess else "MAX_STACK_ITEMS"

        iterateVulkanType(typeInfo, param, VulkanReservedMarshalingCodegen(
            cgen,
            READ_STREAM,
            ROOT_TYPE_DEFAULT_VALUE,
            param.paramName,
            "readStreamPtrPtr",
            API_PREFIX_RESERVEDUNMARSHAL,
            "" if (output or noUnbox) else "unbox_",
            direction="read",
            dynAlloc=True,
            stackVar="stack_%s" % param.paramName,
            stackArrSize=arrSize))


def emit_dispatch_unmarshal(typeInfo, param, cgen, globalWrapped):
    if globalWrapped:
        cgen.stmt(
            "// Begin global wrapped dispatchable handle unboxing for %s" % param.paramName)
        iterateVulkanType(typeInfo, param, VulkanReservedMarshalingCodegen(
            cgen,
            READ_STREAM,
            ROOT_TYPE_DEFAULT_VALUE,
            param.paramName,
            "readStreamPtrPtr",
            API_PREFIX_RESERVEDUNMARSHAL,
            "",
            direction="read",
            dynAlloc=True))
    else:
        cgen.stmt(
            "// Begin non wrapped dispatchable handle unboxing for %s" % param.paramName)
        # cgen.stmt("%s->unsetHandleMapping()" % READ_STREAM)
        iterateVulkanType(typeInfo, param, VulkanReservedMarshalingCodegen(
            cgen,
            READ_STREAM,
            ROOT_TYPE_DEFAULT_VALUE,
            param.paramName,
            "readStreamPtrPtr",
            API_PREFIX_RESERVEDUNMARSHAL,
            "",
            direction="read",
            dynAlloc=True))
        cgen.stmt("auto unboxed_%s = unbox_%s(%s)" %
                  (param.paramName, param.typeName, param.paramName))
        cgen.stmt("auto vk = dispatch_%s(%s)" %
                  (param.typeName, param.paramName))
        cgen.stmt("// End manual dispatchable handle unboxing for %s" %
                  param.paramName)


def emit_transform(typeInfo, param, cgen, variant="tohost"):
    res = \
        iterateVulkanType(typeInfo, param, TransformCodegen(
            cgen, param.paramName, "globalstate", "transform_%s_" % variant, variant))
    if not res:
        cgen.stmt("(void)%s" % param.paramName)

# Everything here elides the initial arg


class DecodingParameters(object):
    def __init__(self, api):
        self.params = []
        self.toRead = []
        self.toWrite = []

        i = 0

        for param in api.parameters[1:]:
            param.nonDispatchableHandleCreate = False
            param.nonDispatchableHandleDestroy = False
            param.dispatchHandle = False
            param.dispatchableHandleCreate = False
            param.dispatchableHandleDestroy = False

            if i == 0 and param.isDispatchableHandleType():
                param.dispatchHandle = True

            if param.isNonDispatchableHandleType() and param.isCreatedBy(api):
                param.nonDispatchableHandleCreate = True

            if param.isNonDispatchableHandleType() and param.isDestroyedBy(api):
                param.nonDispatchableHandleDestroy = True

            if param.isDispatchableHandleType() and param.isCreatedBy(api):
                param.dispatchableHandleCreate = True

            if param.isDispatchableHandleType() and param.isDestroyedBy(api):
                param.dispatchableHandleDestroy = True

            self.toRead.append(param)

            if param.possiblyOutput():
                self.toWrite.append(param)

            self.params.append(param)

            i += 1


def emit_call_log(api, cgen):
    decodingParams = DecodingParameters(api)
    paramsToRead = decodingParams.toRead

    # cgen.beginIf("m_logCalls")
    paramLogFormat = "%p"
    paramLogArgs = ["(void*)boxed_dispatchHandle"]

    for p in paramsToRead:
        paramLogFormat += "0x%llx "
    for p in paramsToRead:
        paramLogArgs.append("(unsigned long long)%s" % (p.paramName))
    # cgen.stmt("fprintf(stderr, \"substream %%p: call %s %s\\n\", readStream, %s)" % (api.name, paramLogFormat, ", ".join(paramLogArgs)))
    # cgen.endIf()


def emit_decode_parameters(typeInfo, api, cgen, globalWrapped=False):

    decodingParams = DecodingParameters(api)

    paramsToRead = decodingParams.toRead

    for p in paramsToRead:
        emit_param_decl_for_reading(p, cgen)

    i = 0
    for p in paramsToRead:
        lenAccess = cgen.generalLengthAccess(p)

        if p.dispatchHandle:
            emit_dispatch_unmarshal(typeInfo, p, cgen, globalWrapped)
        else:
            destroy = p.nonDispatchableHandleDestroy or p.dispatchableHandleDestroy
            noUnbox = False

            if p.nonDispatchableHandleDestroy or p.dispatchableHandleDestroy:
                destroy = True
                cgen.stmt(
                    "// Begin manual non dispatchable handle destroy unboxing for %s" % p.paramName)
                if None == lenAccess or "1" == lenAccess:
                    cgen.stmt("%s boxed_%s_preserve" %
                              (p.typeName, p.paramName))
                else:
                    cgen.stmt("%s* boxed_%s_preserve; %s->alloc((void**)&boxed_%s_preserve, %s * sizeof(%s))" %
                              (p.typeName, p.paramName, READ_STREAM, p.paramName, lenAccess, p.typeName))

            if p.possiblyOutput():
                cgen.stmt(
                    "// Begin manual dispatchable handle unboxing for %s" % p.paramName)
                cgen.stmt("%s->unsetHandleMapping()" % READ_STREAM)

            emit_unmarshal(typeInfo, p, cgen, output=p.possiblyOutput(
            ), destroy=destroy, noUnbox=noUnbox)
        i += 1

    for p in paramsToRead:
        emit_transform(typeInfo, p, cgen, variant="tohost")

    emit_call_log(api, cgen)


def emit_dispatch_call(api, cgen):

    decodingParams = DecodingParameters(api)

    customParams = ["(VkCommandBuffer)dispatchHandle"]

    for (i, p) in enumerate(api.parameters[1:]):
        customParam = p.paramName
        if decodingParams.params[i].dispatchHandle:
            customParam = "unboxed_%s" % p.paramName
        customParams.append(customParam)

    if api.name in driver_workarounds_global_lock_apis:
        cgen.stmt("lock()")

    cgen.vkApiCall(api, customPrefix="vk->", customParameters=customParams)

    if api.name in driver_workarounds_global_lock_apis:
        cgen.stmt("unlock()")


def emit_global_state_wrapped_call(api, cgen):
    customParams = ["pool", "(VkCommandBuffer)(boxed_dispatchHandle)"] + \
        list(map(lambda p: p.paramName, api.parameters[1:]))
    cgen.vkApiCall(api, customPrefix="this->on_",
                   customParameters=customParams)


def emit_default_decoding(typeInfo, api, cgen):
    emit_decode_parameters(typeInfo, api, cgen)
    emit_dispatch_call(api, cgen)


def emit_global_state_wrapped_decoding(typeInfo, api, cgen):
    emit_decode_parameters(typeInfo, api, cgen, globalWrapped=True)
    emit_global_state_wrapped_call(api, cgen)


custom_decodes = {
    "vkCmdCopyBufferToImage": emit_global_state_wrapped_decoding,
    "vkCmdCopyImage": emit_global_state_wrapped_decoding,
    "vkCmdCopyImageToBuffer": emit_global_state_wrapped_decoding,
    "vkCmdExecuteCommands": emit_global_state_wrapped_decoding,
    "vkBeginCommandBuffer": emit_global_state_wrapped_decoding,
    "vkResetCommandBuffer": emit_global_state_wrapped_decoding,
    "vkCmdPipelineBarrier": emit_global_state_wrapped_decoding,
    "vkCmdBindPipeline": emit_global_state_wrapped_decoding,
    "vkCmdBindDescriptorSets": emit_global_state_wrapped_decoding,
    "vkCmdCopyQueryPoolResults": emit_global_state_wrapped_decoding,
    "vkBeginCommandBufferAsyncGOOGLE": emit_global_state_wrapped_decoding,
    "vkEndCommandBufferAsyncGOOGLE": emit_global_state_wrapped_decoding,
    "vkResetCommandBufferAsyncGOOGLE": emit_global_state_wrapped_decoding,
    "vkCommandBufferHostSyncGOOGLE": emit_global_state_wrapped_decoding,
}


class VulkanSubDecoder(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)
        self.typeInfo = typeInfo
        self.cgen = CodeGen()

    def onBegin(self,):
        self.module.appendImpl(
            "#define MAX_STACK_ITEMS %s\n" % MAX_STACK_ITEMS)

        self.module.appendImpl(
            "size_t subDecode(VulkanMemReadingStream* readStream, VulkanDispatch* vk, void* boxed_dispatchHandle, void* dispatchHandle, VkDeviceSize dataSize, const void* pData)\n")

        self.cgen.beginBlock()  # function body

        self.cgen.stmt("uint32_t count = 0")
        self.cgen.stmt("unsigned char *buf = (unsigned char *)pData")
        self.cgen.stmt("android::base::BumpPool* pool = readStream->pool()")
        self.cgen.stmt("unsigned char *ptr = (unsigned char *)pData")
        self.cgen.stmt(
            "const unsigned char* const end = (const unsigned char*)buf + dataSize")
        self.cgen.stmt(
            "VkDecoderGlobalState* globalstate = VkDecoderGlobalState::get()")

        self.cgen.line("while (end - ptr >= 8)")
        self.cgen.beginBlock()  # while loop

        self.cgen.stmt("uint32_t opcode = *(uint32_t *)ptr")
        self.cgen.stmt("int32_t packetLen = *(int32_t *)(ptr + 4)")
        self.cgen.stmt(
            "if (end - ptr < packetLen) return ptr - (unsigned char*)buf")

        self.cgen.stmt("%s->setBuf((uint8_t*)(ptr + 8))" % READ_STREAM)
        self.cgen.stmt(
            "uint8_t* readStreamPtr = %s->getBuf(); uint8_t** readStreamPtrPtr = &readStreamPtr" % READ_STREAM)
        self.cgen.line("switch (opcode)")
        self.cgen.beginBlock()  # switch stmt

        self.module.appendImpl(self.cgen.swapCode())

    def onGenCmd(self, cmdinfo, name, alias):
        typeInfo = self.typeInfo
        cgen = self.cgen
        api = typeInfo.apis[name]

        if "commandBuffer" != api.parameters[0].paramName:
            return

        cgen.line("case OP_%s:" % name)
        cgen.beginBlock()
        cgen.stmt("android::base::beginTrace(\"%s subdecode\")" % name)

        if api.name in custom_decodes.keys():
            custom_decodes[api.name](typeInfo, api, cgen)
        else:
            emit_default_decoding(typeInfo, api, cgen)

        cgen.stmt("android::base::endTrace()")
        cgen.stmt("break")
        cgen.endBlock()
        self.module.appendImpl(self.cgen.swapCode())

    def onEnd(self,):
        self.cgen.line("default:")
        self.cgen.beginBlock()
        self.cgen.stmt(
            "GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) << \"Unrecognized opcode \" << opcode")
        self.cgen.endBlock()

        self.cgen.endBlock()  # switch stmt

        self.cgen.stmt("++count; if (count % 1000 == 0) { pool->freeAll(); }")
        self.cgen.stmt("ptr += packetLen")
        self.cgen.endBlock()  # while loop

        self.cgen.stmt("pool->freeAll()")
        self.cgen.stmt("return ptr - (unsigned char*)buf;")
        self.cgen.endBlock()  # function body
        self.module.appendImpl(self.cgen.swapCode())
