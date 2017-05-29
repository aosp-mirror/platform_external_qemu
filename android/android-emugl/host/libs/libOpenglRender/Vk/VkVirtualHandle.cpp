#include "VkVirtualHandle.h"

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"

#include <unordered_map>
#include <vulkan/vulkan.h>
#include <stdio.h>

using android::base::LazyInstance;

template <typename T>
class VulkanHandleStore {
public:
    VulkanHandleStore() = default;
    uint32_t add(T hostHandle) {
        mHost2Guest[hostHandle] = currentGuestHandle;
        mGuest2Host[currentGuestHandle] = hostHandle;
        uint32_t res = currentGuestHandle;
        currentGuestHandle++;
        return res;
    }

    uint32_t addOrGet(T hostHandle) {
        if (auto elt = android::base::find(mHost2Guest, hostHandle)) {
            return *elt;
        }
        return add(hostHandle);
    }

    T getHost(uint32_t guestHandle) {
        return mGuest2Host[guestHandle];
    }

    uint32_t getGuest(T hostHandle) {
        return mHost2Guest[hostHandle];
    }

    void remove(T hostHandle) {
        auto it = mHost2Guest.find(hostHandle);
        uint32_t guestHandleToDelete = it->second;
        mHost2Guest.erase(it);
        mGuest2Host.erase(guestHandleToDelete);

        cleanup();
    }
private:
    void cleanup() {
        erases++;
        if (erases == sweepInterval) {
            uint32_t maxGuestHandle = 0;
            for (const auto it : mGuest2Host) {
                if (it.first > maxGuestHandle) {
                    maxGuestHandle = it.first;
                }
            }
            currentGuestHandle = maxGuestHandle + 1;
            erases = 0;
        }
    }

    uint32_t sweepInterval = 65536;
    uint32_t erases = 0;
    uint32_t currentGuestHandle = 0;
    std::unordered_map<T, uint32_t> mHost2Guest = {};
    std::unordered_map<uint32_t, T> mGuest2Host = {};
};

#define IMPL_FOR(type) \
static LazyInstance<VulkanHandleStore<type> > s_HandleStore_##type; \
template<> uint32_t VkVirtualHandleCreate<type>(type instance) { \
    uint32_t res = s_HandleStore_##type.ptr()->add(instance); \
    fprintf(stderr, "%s: new %s. %p -> %u\n", __func__, #type, instance, res); \
    return res; } \
template<> uint32_t VkVirtualHandleCreateOrGet<type>(type instance) { \
    uint32_t res = s_HandleStore_##type.ptr()->addOrGet(instance); \
    fprintf(stderr, "%s: new %s (or get). %p -> %u\n", __func__, #type, instance, res); \
    return res; } \
template<> void VkVirtualHandleDelete<type>(type instance) { s_HandleStore_##type.ptr()->remove(instance); } \
template<> uint32_t VkVirtualHandleGetGuest<type>(type instance) { return s_HandleStore_##type.ptr()->getGuest(instance); } \
template<> type VkVirtualHandleGetHost<type>(uint32_t handle) { \
    type hostSide = s_HandleStore_##type.ptr()->getHost(handle); \
    fprintf(stderr, "%s: %s to host. %u -> host %p\n", __func__, #type, handle, hostSide); \
    return hostSide; } \

#define LIST_VK_HANDLE_TYPES(f) \
    f(VkInstance) \
    f(VkPhysicalDevice) \
    f(VkDevice) \
    f(VkQueue) \
    f(VkDeviceMemory) \
    f(VkCommandPool) \
    f(VkCommandBuffer) \
    f(VkBuffer) \
    f(VkBufferView) \
    f(VkImage) \
    f(VkImageView) \
    f(VkRenderPass) \
    f(VkFramebuffer) \
    f(VkDescriptorPool) \
    f(VkDescriptorSet) \
    f(VkDescriptorSetLayout) \
    f(VkShaderModule) \
    f(VkSampler) \
    f(VkPipelineLayout) \
    f(VkPipelineCache) \
    f(VkPipeline) \
    f(VkFence) \
    f(VkSemaphore) \
    f(VkEvent) \
    f(VkQueryPool) \

LIST_VK_HANDLE_TYPES(IMPL_FOR)
