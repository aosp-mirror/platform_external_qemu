from .common.codegen import CodeGen, VulkanWrapperGenerator, VulkanAPIWrapper
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType, DISPATCHABLE_HANDLE_TYPES, NON_DISPATCHABLE_HANDLE_TYPES

from .marshaling import VulkanMarshalingCodegen
from .transform import TransformCodegen, genTransformsForVulkanType

from .wrapperdefs import API_PREFIX_MARSHAL
from .wrapperdefs import API_PREFIX_UNMARSHAL
from .wrapperdefs import VULKAN_STREAM_TYPE

from copy import copy

decoder_snapshot_decl_preamble = """

namespace android {
namespace base {
class BumpPool;
class Stream;
} // namespace base {
} // namespace android {

class VkDecoderSnapshot {
public:
    VkDecoderSnapshot();
    ~VkDecoderSnapshot();

    void save(android::base::Stream* stream);
    void load(android::base::Stream* stream);
"""

decoder_snapshot_decl_postamble = """
private:
    class Impl;
    std::unique_ptr<Impl> mImpl;

};
"""

decoder_snapshot_impl_preamble ="""

using namespace goldfish_vk;

class VkDecoderSnapshot::Impl {
public:
    Impl() { }

    void save(android::base::Stream* stream) {
        mReconstruction.save(stream);
    }

    void load(android::base::Stream* stream) {
        mReconstruction.load(stream);
    }

"""

decoder_snapshot_impl_postamble = """
private:
    android::base::Lock mLock;
    VkReconstruction mReconstruction;
};

VkDecoderSnapshot::VkDecoderSnapshot() :
    mImpl(new VkDecoderSnapshot::Impl()) { }

void VkDecoderSnapshot::save(android::base::Stream* stream) {
    mImpl->save(stream);
}

void VkDecoderSnapshot::load(android::base::Stream* stream) {
    mImpl->load(stream);
}

VkDecoderSnapshot::~VkDecoderSnapshot() = default;
"""

AUXILIARY_SNAPSHOT_API_BASE_PARAM_COUNT = 3

AUXILIARY_SNAPSHOT_API_PARAM_NAMES = [
    "input_result",
]

# Vulkan handle dependencies.
# (a, b): a depends on b
SNAPSHOT_HANDLE_DEPENDENCIES = [
    # Dispatchable handle types
    ("VkCommandBuffer", "VkCommandPool"),
    ("VkCommandPool", "VkDevice"),
    ("VkQueue", "VkDevice"),
    ("VkDevice", "VkPhysicalDevice"),
    ("VkPhysicalDevice", "VkInstance")] + \
    list(map(lambda handleType : (handleType, "VkDevice"), NON_DISPATCHABLE_HANDLE_TYPES))

handleDependenciesDict = dict(SNAPSHOT_HANDLE_DEPENDENCIES)

def extract_deps_vkAllocateCommandBuffers(param, access, lenExpr, api, cgen):
    cgen.stmt("mReconstruction.addHandleDependency((const uint64_t*)%s, %s, (uint64_t)(uintptr_t)%s)" % \
              (access, lenExpr, "VkDecoderGlobalState::get()->unboxed_to_boxed_non_dispatchable_VkCommandPool(pAllocateInfo->commandPool)"))

specialCaseDependencyExtractors = {
    "vkAllocateCommandBuffers" : extract_deps_vkAllocateCommandBuffers,
}

apiSequences = {
    "vkAllocateMemory" : ["vkAllocateMemory", "vkMapMemoryIntoAddressSpaceGOOGLE"]
}

apiModifies = {
    "vkMapMemoryIntoAddressSpaceGOOGLE" : ["memory"],
}

def is_modify_operation(api, param):
    if api.name in apiModifies:
        if param.paramName in apiModifies[api.name]:
            return True
    return False

def emit_impl(typeInfo, api, cgen):

    cgen.line("// TODO: Implement")

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
            cgen.stmt("android::base::AutoLock lock(mLock)")
            cgen.line("// %s create" % p.paramName)
            cgen.stmt("mReconstruction.addHandles((const uint64_t*)%s, %s)" % (access, lenExpr));

            if p.typeName in handleDependenciesDict:
                dependsOnType = handleDependenciesDict[p.typeName];
                for p2 in api.parameters:
                    if p2.typeName == dependsOnType:
                        cgen.stmt("mReconstruction.addHandleDependency((const uint64_t*)%s, %s, (uint64_t)(uintptr_t)%s)" % (access, lenExpr, p2.paramName))
                if api.name in specialCaseDependencyExtractors:
                    specialCaseDependencyExtractors[api.name](p, access, lenExpr, api, cgen)

            cgen.stmt("if (!%s) return" % access)
            cgen.stmt("auto apiHandle = mReconstruction.createApiInfo()")
            cgen.stmt("auto apiInfo = mReconstruction.getApiInfo(apiHandle)")
            cgen.stmt("mReconstruction.setApiTrace(apiInfo, OP_%s, snapshotTraceBegin, snapshotTraceBytes)" % api.name)
            cgen.stmt("mReconstruction.forEachHandleAddApi((const uint64_t*)%s, %s, apiHandle)" % (access, lenExpr))
            cgen.stmt("mReconstruction.setCreatedHandlesForApi(apiHandle, (const uint64_t*)%s, %s)" % (access, lenExpr))

        if p.isDestroyedBy(api):
            cgen.stmt("android::base::AutoLock lock(mLock)")
            cgen.line("// %s destroy" % p.paramName)
            cgen.stmt("mReconstruction.removeHandles((const uint64_t*)%s, %s)" % (access, lenExpr));

        if is_modify_operation(api, p):
            cgen.stmt("android::base::AutoLock lock(mLock)")
            cgen.line("// %s modify" % p.paramName)
            cgen.stmt("auto apiHandle = mReconstruction.createApiInfo()")
            cgen.stmt("auto apiInfo = mReconstruction.getApiInfo(apiHandle)")
            cgen.stmt("mReconstruction.setApiTrace(apiInfo, OP_%s, snapshotTraceBegin, snapshotTraceBytes)" % api.name)
            cgen.beginFor("uint32_t i = 0", "i < %s" % lenExpr, "++i")
            if p.isNonDispatchableHandleType():
                cgen.stmt("%s boxed = VkDecoderGlobalState::get()->unboxed_to_boxed_non_dispatchable_%s(%s[i])" % (p.typeName, p.typeName, access))
            else:
                cgen.stmt("%s boxed = VkDecoderGlobalState::get()->unboxed_to_boxed_%s(%s[i])" % (p.typeName, p.typeName, access))
            cgen.stmt("mReconstruction.forEachHandleAddModifyApi((const uint64_t*)(&boxed), 1, apiHandle)")
            cgen.endFor()

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

        additionalParams = [ \
            makeVulkanTypeSimple(True, "uint8_t", 1, "snapshotTraceBegin"),
            makeVulkanTypeSimple(False, "size_t", 0, "snapshotTraceBytes"),
            makeVulkanTypeSimple(False, "android::base::BumpPool", 1, "pool"),]

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

