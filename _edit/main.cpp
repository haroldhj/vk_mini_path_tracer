// Copyright 2020 NVIDIA Corporation
// SPDX-License-Identifier: Apache-2.0

#include <nvvk/context_vk.hpp>
#include <nvvk/error_vk.hpp>
#include <nvvk/resourceallocator_vk.hpp>

static const uint64_t render_width  = 800;
static const uint64_t render_height = 600;

int main(int argc, const char** argv)
{
  // Create the Vulkan context, consisting of an instance, device, physical device, and queues.
  nvvk::ContextCreateInfo deviceInfo;  // One can modify this to load different extensions
  nvvk::Context           context;     // Encapsulates device state in a single object
  deviceInfo.apiMajor = 1;
  deviceInfo.apiMinor = 4;
  deviceInfo.addDeviceExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
  VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
  deviceInfo.addDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false, &asFeatures);
  VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
  deviceInfo.addDeviceExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME, false, &rayQueryFeatures);

  context.init(deviceInfo);  // Initialize the context

  nvvk::ResourceAllocatorDedicated allocator;
  allocator.init(context, context.m_physicalDevice);

  VkDeviceSize       bufferSizeBytes = render_width * render_height * 3 * sizeof(float);
  VkBufferCreateInfo bufferCreateInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                      .size  = bufferSizeBytes,
                                      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT};

  nvvk::Buffer buffer = allocator.createBuffer(bufferCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT
                                                                     | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // Command Pool
  VkCommandPoolCreateInfo cmdPoolInfo{.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,  //
                                      .queueFamilyIndex = context.m_queueGCT};
  VkCommandPool           cmdPool;
  NVVK_CHECK(vkCreateCommandPool(context, &cmdPoolInfo, nullptr, &cmdPool));

  // Command Buffer
  constexpr uint32_t          cmdBufCnt = 1;
  VkCommandBufferAllocateInfo cmdAllocInfo{.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                           .commandPool        = cmdPool,
                                           .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                           .commandBufferCount = cmdBufCnt};
  VkCommandBuffer             cmdBuffer;
  NVVK_CHECK(vkAllocateCommandBuffers(context, &cmdAllocInfo, &cmdBuffer));

  VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                     .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  NVVK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

  const float     fillValue    = 0.5f;
  const uint32_t& fillValueU32 = reinterpret_cast<const uint32_t&>(fillValue);
  vkCmdFillBuffer(cmdBuffer, buffer.buffer, 0, bufferSizeBytes, fillValueU32);

  vkFreeCommandBuffers(context, cmdPool, cmdBufCnt, &cmdBuffer);
  vkDestroyCommandPool(context, cmdPool, nullptr);

  allocator.destroy(buffer);
  context.deinit();  // Don't forget to clean up at the end of the program!
}