from .common.codegen import CodeGen, VulkanWrapperGenerator
from .common.vulkantypes import HANDLE_TYPES

from .marshaling import VulkanMarshalingCodegen
from .handlemap import HandleMapCodegen

from .wrapperdefs import API_PREFIX_MARSHAL
from .wrapperdefs import API_PREFIX_UNMARSHAL

resourcetracker_decl_preamble = """
class ResourceTracker {
public:
    ResourceTracker();
    ~ResourceTracker();
    static ResourceTracker* get();
    VulkanHandleMapping* createMapping();
    VulkanHandleMapping* unwrapMapping();
    VulkanHandleMapping* destroyMapping();
    VulkanHandleMapping* identityMapping();
private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};
"""

resourcetracker_impl_postamble ="""
class ResourceTracker::Impl {
public:
    Impl() = default;
    CreateMapping createMapping;
    UnwrapMapping unwrapMapping;
    DestroyMapping destroyMapping;
    IdentityMapping identityMapping;
};

ResourceTracker::ResourceTracker() : mImpl(new ResourceTracker::Impl()) { }
ResourceTracker::~ResourceTracker() { }

VulkanHandleMapping* ResourceTracker::createMapping() {
    return &mImpl->createMapping;
}

VulkanHandleMapping* ResourceTracker::unwrapMapping() {
    return &mImpl->unwrapMapping;
}

VulkanHandleMapping* ResourceTracker::destroyMapping() {
    return &mImpl->destroyMapping;
}

VulkanHandleMapping* ResourceTracker::identityMapping() {
    return &mImpl->identityMapping;
}

static ResourceTracker* sTracker = nullptr;

// static
ResourceTracker* ResourceTracker::get() {
    if (!sTracker) {
        // To be initialized once on vulkan device open.
        sTracker = new ResourceTracker;
    }
    return sTracker;
}
"""

def emit_begin_public_class(cgen, decl):
    cgen.line(decl + " {")
    cgen.line("public:")
    cgen.incrIndent()

def emit_end_class(cgen):
    cgen.decrIndent()
    cgen.line("};")

def emit_mapping_impl(cgen, className, foreachNormal, foreach_u64, foreach_from_u64):
    emit_begin_public_class(cgen, "class %s : public VulkanHandleMapping" % className)
    cgen.line("virtual ~%s() { }" % className)
    for h in HANDLE_TYPES:

        cgen.line("void mapHandles_%s(%s* handles, size_t count) override" % (h, h))
        cgen.beginBlock()
        cgen.beginFor("size_t i = 0", "i < count", "++i")
        foreachNormal(cgen, h)
        cgen.endFor()
        cgen.endBlock()

        cgen.line("void mapHandles_%s_u64(const %s* handles, uint64_t* handle_u64s, size_t count) override" % (h, h))
        cgen.beginBlock()
        cgen.beginFor("size_t i = 0", "i < count", "++i")
        foreach_u64(cgen, h)
        cgen.endFor()
        cgen.endBlock()

        cgen.line("void mapHandles_u64_%s(const uint64_t* handle_u64s, %s* handles, size_t count) override" % (h, h))
        cgen.beginBlock()
        cgen.beginFor("size_t i = 0", "i < count", "++i")
        foreach_from_u64(cgen, h)
        cgen.endFor()
        cgen.endBlock()

    emit_end_class(cgen)

class ResourceTracker(VulkanWrapperGenerator):
    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)

        self.typeInfo = typeInfo

        self.cgen = CodeGen()

    def onBegin(self,):
        VulkanWrapperGenerator.onBegin(self);

        self.module.appendHeader(resourcetracker_decl_preamble)
        cgen = self.cgen

        emit_mapping_impl(cgen, "CreateMapping",
            lambda cgen, h: cgen.stmt("handles[i] = new_from_host_%s(handles[i])" % h),
            lambda cgen, h: cgen.stmt("handle_u64s[i] = (uint64_t)(uintptr_t)new_from_host_%s(handles[i])" % h),
            lambda cgen, h: cgen.stmt("handles[i] = (%s)(uintptr_t)new_from_host_%s((%s)(uintptr_t)handle_u64s[i])" % (h, h, h)))

        emit_mapping_impl(cgen, "UnwrapMapping",
            lambda cgen, h: cgen.stmt("handles[i] = get_host_%s(handles[i])" % h),
            lambda cgen, h: cgen.stmt("handle_u64s[i] = (uint64_t)(uintptr_t)get_host_%s(handles[i])" % h),
            lambda cgen, h: cgen.stmt("handles[i] = (%s)(uintptr_t)get_host_%s((%s)(uintptr_t)handle_u64s[i])" % (h, h, h)))

        emit_mapping_impl(cgen, "DestroyMapping",
            lambda cgen, h: cgen.stmt("delete_goldfish_%s(handles[i])" % h),
            lambda cgen, h: cgen.stmt("(void)handle_u64s[i]; delete_goldfish_%s(handles[i])" % h),
            lambda cgen, h: cgen.stmt("(void)handles[i]; delete_goldfish_%s((%s)(uintptr_t)handle_u64s[i])" % (h, h)))

        emit_mapping_impl(cgen, "IdentityMapping",
            lambda cgen, h: cgen.stmt("handles[i] = vk_handle_identity_%s(handles[i])" % h),
            lambda cgen, h: cgen.stmt("handle_u64s[i] = (uint64_t)(uintptr_t)vk_handle_identity_%s(handles[i])" % h),
            lambda cgen, h: cgen.stmt("handles[i] = (%s)(uintptr_t)vk_handle_identity_%s((%s)(uintptr_t)handle_u64s[i])" % (h, h, h)))

        self.module.appendImpl(cgen.swapCode())
        self.module.appendImpl(resourcetracker_impl_postamble)

    def onEnd(self,):
        VulkanWrapperGenerator.onEnd(self);
