from .common.codegen import CodeGen, VulkanWrapperGenerator, VulkanAPIWrapper
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType, DISPATCHABLE_HANDLE_TYPES, NON_DISPATCHABLE_HANDLE_TYPES

from .marshaling import VulkanMarshalingCodegen
from .transform import TransformCodegen, genTransformsForVulkanType

from .wrapperdefs import API_PREFIX_MARSHAL
from .wrapperdefs import API_PREFIX_UNMARSHAL
from .wrapperdefs import VULKAN_STREAM_TYPE

from .baseprotoconversion import VulkanStructToProtoCodegen, VulkanProtoToStructCodegen

from copy import copy

decoder_snapshot_decl_preamble = """

namespace android {
namespace base {
class Pool;
} // namespace base {
} // namespace android {

class VkDecoderSnapshot {
public:
    VkDecoderSnapshot();
    ~VkDecoderSnapshot();
"""

decoder_snapshot_decl_postamble = """
private:
    class Impl;
    std::unique_ptr<Impl> mImpl;

};
"""

decoder_snapshot_impl_preamble ="""

using android::base::EntityManager;
using android::base::ComponentManager;
using android::base::UnpackedComponentManager;

using namespace goldfish_vk;

class VkDecoderSnapshot::Impl {
public:
    Impl() { }

"""

decoder_snapshot_impl_postamble = """
private:
    struct ApiInfo {
        goldfish_vk_proto::VkApiCall apiCall;
    };

    using ApiTrace = EntityManager<32, 16, 16, ApiInfo>;
    using ApiHandle = ApiTrace::EntityHandle;

    struct HandleReconstruction {
        std::vector<ApiHandle> apiRefs;
    };

    using HandleReconstructions = UnpackedComponentManager<32, 16, 16, HandleReconstruction>;

    ApiTrace mApiTrace;

#define DEFINE_HANDLE_RECONSTRUCTION_MEMBER(type) \\
    HandleReconstructions mReconstructions_##type; \\
    void addReconstruction_##type(const type* toAdd, uint32_t count) { \\
        if (!toAdd) return; \\
        for (uint32_t i = 0; i < count; ++i) { \\
            mReconstructions_##type.add((uint64_t)(uintptr_t)toAdd[i], HandleReconstruction()); \\
        } \\
    } \\
    void removeReconstruction_##type(const type* toRemove, uint32_t count) { \\
        if (!toRemove) return; \\
        for (uint32_t i = 0; i < count; ++i) { \\
            mReconstructions_##type.remove((uint64_t)(uintptr_t)toRemove[i]); \\
        } \\
    } \\

    GOLDFISH_VK_LIST_HANDLE_TYPES(DEFINE_HANDLE_RECONSTRUCTION_MEMBER)

};

VkDecoderSnapshot::VkDecoderSnapshot() :
    mImpl(new VkDecoderSnapshot::Impl()) { }

VkDecoderSnapshot::~VkDecoderSnapshot() = default;
"""

AUXILIARY_SNAPSHOT_API_TYPES = [
    "android::base::Pool",
]

AUXILIARY_SNAPSHOT_API_PARAM_NAMES = [
    "input_result",
]

def emit_impl(typeInfo, api, cgen):

    cgen.line("// TODO: Implement")

    traceCreate = False

    for p in api.parameters:

        if not (p.isHandleType):
            continue

        lenExpr = cgen.generalLengthAccess(p)

        if lenExpr is None:
            lenExpr = "1"

        if p.pointerIndirectionLevels > 0:
            access = p.paramName
        else:
            access = "(&%s)" % p.paramName

        if p.isCreatedBy(api):
            cgen.line("// %s create" % p.paramName)
            cgen.stmt("addReconstruction_%s(%s, %s)" % (p.typeName, access, lenExpr));
            traceCreate = True

        if p.isDestroyedBy(api):
            cgen.line("// %s destroy" % p.paramName)
            cgen.stmt("removeReconstruction_%s(%s, %s)" % (p.typeName, access, lenExpr));

    if traceCreate:
        cgen.stmt("DefaultHandleMapping someHandleMapping")
        cgen.stmt("auto apiHandle = mApiTrace.add(ApiInfo(), 1 /* type tag */)")
        cgen.stmt("auto apiInfo = mApiTrace.get(apiHandle)")
        cgen.stmt("auto proto = apiInfo->apiCall.mutable_api_%s()" % api.name.lower())
        for p in api.parameters:

            if p.typeName in AUXILIARY_SNAPSHOT_API_TYPES:
                continue

            if p.paramName in AUXILIARY_SNAPSHOT_API_PARAM_NAMES:
                continue

            if typeInfo.categoryOf(p.typeName) in ["struct", "union"]:
                iterateVulkanType(typeInfo, p, VulkanStructToProtoCodegen( \
                    cgen, "&someHandleMapping", "%s" % (p.paramName), "proto"))

def emit_passthrough_to_impl(typeInfo, api, cgen):
    cgen.vkApiCall(api, customPrefix = "mImpl->")

class VulkanDecoderSnapshot(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.typeInfo = typeInfo

        self.cgenHeader = CodeGen()
        self.cgenHeader.incrIndent()

        self.cgenImpl = CodeGen()

        self.currentFeature = None

        self.feature_apis = []

    def onBegin(self,):
        self.module.appendHeader(decoder_snapshot_decl_preamble)
        self.module.appendImpl(decoder_snapshot_impl_preamble)

    def onBeginFeature(self, featureName):
        VulkanWrapperGenerator.onBeginFeature(self, featureName)
        self.currentFeature = featureName

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

        api = self.typeInfo.apis[name]


        additionalParams = \
            [makeVulkanTypeSimple(False, "android::base::Pool", 1, "pool"),]

        if api.retType.typeName != "void":
            additionalParams.append( \
                makeVulkanTypeSimple(False, api.retType.typeName, 0, "input_result"))

        apiForSnapshot = \
            api.withCustomParameters( \
                additionalParams + \
                api.parameters).withCustomReturnType( \
                    makeVulkanTypeSimple(False, "void", 0, "void"))

        self.feature_apis.append((self.currentFeature, apiForSnapshot))

        self.cgenHeader.stmt(self.cgenHeader.makeFuncProto(apiForSnapshot))
        self.module.appendHeader(self.cgenHeader.swapCode())

        self.cgenImpl.emitFuncImpl( \
            apiForSnapshot, lambda cgen: emit_impl(self.typeInfo, apiForSnapshot, cgen))
        self.module.appendImpl(self.cgenImpl.swapCode())

    def onEnd(self,):
        self.module.appendHeader(decoder_snapshot_decl_postamble)
        self.module.appendImpl(decoder_snapshot_impl_postamble)
        self.cgenHeader.decrIndent()

        for feature, api in self.feature_apis:
            if feature is not None:
                self.cgenImpl.line("#ifdef %s" % feature)

            apiImplShell = \
                api.withModifiedName("VkDecoderSnapshot::" + api.name)

            self.cgenImpl.emitFuncImpl( \
                apiImplShell, lambda cgen: emit_passthrough_to_impl(self.typeInfo, api, cgen))

            if feature is not None:
                self.cgenImpl.line("#endif")

        self.module.appendImpl(self.cgenImpl.swapCode())

