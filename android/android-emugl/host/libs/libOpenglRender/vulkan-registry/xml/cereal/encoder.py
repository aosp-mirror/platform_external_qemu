from .common.codegen import CodeGen, VulkanWrapperGenerator
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType

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
    iterateVulkanType(
        typeInfo, param,
        VulkanMarshalingCodegen( \
           cgen, COUNTING_STREAM, param.paramName,
           API_PREFIX_MARSHAL, direction="write"))

def emit_marshal(typeInfo, param, cgen):
    iterateVulkanType(
        typeInfo, param,
        VulkanMarshalingCodegen( \
           cgen, STREAM, param.paramName,
           API_PREFIX_MARSHAL, direction="write"))

def emit_unmarshal(typeInfo, param, cgen):
    iterateVulkanType(
        typeInfo, param,
        VulkanMarshalingCodegen( \
           cgen, STREAM, param.paramName,
           API_PREFIX_UNMARSHAL, direction="read"))

def emit_deepcopy(typeInfo, param, cgen):
    iterateVulkanType(typeInfo, param, DeepcopyCodegen(
        cgen, [param.paramName, "local_" + param.paramName], "pool", "deepcopy_"))

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

def emit_handlemap_destroy(typeInfo, param, cgen):
    iterateVulkanType(typeInfo, param, HandleMapCodegen(
        cgen, None, "resources->destroyMapping()", "handlemap_",
        lambda vtype: typeInfo.isHandleType(vtype)
    ))

def emit_parameter_encode(typeInfo, api, cgen):
    cgen.stmt("auto %s = mImpl->stream()" % STREAM)
    cgen.stmt("auto %s = mImpl->countingStream()" % COUNTING_STREAM)
    cgen.stmt("auto %s = mImpl->resources()" % RESOURCES)
    cgen.stmt("auto %s = mImpl->pool()" % POOL)

    finalArgs = []

    paramsToRead = []
    paramsToCreate = []

    paramsToDestroy = []

    localCopiedParams = []
    paramsToWrite = []

    for param in api.parameters:
        if param.possiblyOutput():
            paramsToWrite.append(param)
            paramsToRead.append(param)
            if param.isCreatedBy(api):
                paramsToCreate.append(param)
            finalArgs.append(param)
        elif param.isDestroyedBy(api):
            paramsToWrite.append(param)
            paramsToDestroy.append(param)
            finalArgs.append(param)
        else:
            localCopyParam = \
                param.getForNonConstAccess().withModifiedName( \
                    "local_" + param.paramName)
            localCopiedParams.append((param, localCopyParam))
            finalArgs.append(localCopyParam)
            cgen.stmt(cgen.makeRichCTypeDecl(localCopyParam))
            emit_deepcopy(typeInfo, param, cgen)
            emit_handlemap_unwrap(typeInfo, localCopyParam, cgen)
            paramsToWrite.append(localCopyParam)

    cgen.stmt("%s->rewind()" % COUNTING_STREAM)
    cgen.beginBlock()

    # Use counting stream to calculate the packet size.
    for p in paramsToWrite:
        emit_count_marshal(typeInfo, p, cgen)

    cgen.endBlock()

    cgen.stmt("uint32_t packetSize_%s = 4 + 4 + (uint32_t)%s->bytesWritten()" % \
              (api.name, COUNTING_STREAM))
    cgen.stmt("%s->rewind()" % COUNTING_STREAM)

    cgen.stmt("uint32_t opcode_%s = OP_%s" % (api.name, api.name))
    cgen.stmt("%s->write(&opcode_%s, sizeof(uint32_t))" % (STREAM, api.name))
    cgen.stmt("%s->write(&packetSize_%s, sizeof(uint32_t))" % (STREAM, api.name))

    for p in paramsToWrite:
        emit_marshal(typeInfo, p, cgen)

    for p in paramsToRead:
        emit_unmarshal(typeInfo, p, cgen)

    for p in paramsToCreate:
        emit_handlemap_create(typeInfo, p, cgen)

    for p in paramsToDestroy:
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

## Custom encoding definitions##################################################
def encode_vkMapMemory(typeInfo, api, cgen):
    emit_parameter_encode(typeInfo, api, cgen)
    emit_return_unmarshal(typeInfo, api, cgen)

    needWriteback = \
        "((%s == VK_SUCCESS) && ppData && size > 0)" % api.getRetVarExpr()
    cgen.beginIf(needWriteback)
    cgen.stmt("*ppData = aligned_buf_alloc(1024 /* pick large alignment */, size);");
    cgen.stmt("%s->read(*ppData, size)" % (STREAM))
    cgen.endIf()

    # TODO:
    # The custom part: Depending on the return value,
    # allocate some buffer (TODO: dma map) the result and return it to the user
    # if ppData is not null.

    emit_return(typeInfo, api, cgen)

custom_encodes = {
    "vkMapMemory" : encode_vkMapMemory,
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

        def makeImpl(cgen):
            emit_parameter_encode(self.typeInfo, api, cgen)
            emit_return_unmarshal(self.typeInfo, api, cgen)
            emit_return(self.typeInfo, api, cgen)

        self.module.appendHeader(self.cgenHeader.swapCode())

        if api.name in custom_encodes.keys():
            self.module.appendImpl(self.cgenImpl.makeFuncImpl(
                apiImpl, lambda cgen: custom_encodes[api.name](self.typeInfo, api, cgen)))
        else:
            self.module.appendImpl(self.cgenImpl.makeFuncImpl(apiImpl, makeImpl))

    def onEnd(self,):
        self.module.appendHeader(encoder_decl_postamble)
        self.cgenHeader.decrIndent()
