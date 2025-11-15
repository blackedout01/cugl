#include "internal.h"

#include <stdlib.h>
#include <string.h>

#include "vulkan/vulkan_beta.h"
#include "vulkan/vk_enum_string_helper.h"

#ifdef __APPLE__
#ifndef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
#error "Vulkan headers outdated"
#endif
#endif

int VulkanCreateInstance(context *C, const char **RequiredInstanceExtensions, uint32_t RequiredInstanceExtensionCount) {
    VkLayerProperties *InstanceLayerProperties = 0;
    const char **InstanceExtensions = 0;
    int Result = 0;
    {
        VkApplicationInfo VulkanApplicationInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = 0,
            .pApplicationName = "",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_0,
        };

        uint32_t InstanceLayerPropertyCount;
        VulkanCheckGoto(vkEnumerateInstanceLayerProperties(&InstanceLayerPropertyCount, 0), label_Error);

        InstanceLayerProperties = malloc(InstanceLayerPropertyCount*sizeof(VkLayerProperties));
        if(InstanceLayerProperties == 0) {
            GenerateErrorMsg(C, GL_OUT_OF_MEMORY, GL_DEBUG_SOURCE_API, "VulkanCreateInstance");
            goto label_Error;
        }
        VulkanCheckGoto(vkEnumerateInstanceLayerProperties(&InstanceLayerPropertyCount, InstanceLayerProperties), label_Error);

        const char *RequestedLayers[] = {
            "VK_LAYER_KHRONOS_validation"
        };
        const char *AvaiableRequestedLayers[ArrayCount(RequestedLayers)];
        uint32_t AvaiableRequestedLayerCount = 0;
        for(uint32_t I = 0; I < ArrayCount(RequestedLayers); ++I) {
            for(uint32_t J = 0; J < InstanceLayerPropertyCount; ++J) {
                if(strcmp(RequestedLayers[I], InstanceLayerProperties[J].layerName) == 0) {
                    AvaiableRequestedLayers[AvaiableRequestedLayerCount++] = RequestedLayers[I];
                    break;
                }
            }
        }
        if(AvaiableRequestedLayerCount == 0) {
            GenerateOther(C, GL_DEBUG_SOURCE_API, "Validation layer not found.");
        }

        const char *AdditionalInstanceExtensions[] = {
#ifdef __APPLE__
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#endif
        };

        VkInstanceCreateFlags CreateFlags = 0;
#ifdef __APPLE__
        CreateFlags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        const uint32_t InstanceExtensionCount = RequiredInstanceExtensionCount + ArrayCount(AdditionalInstanceExtensions);
        InstanceExtensions = malloc(InstanceExtensionCount*sizeof(const char *));
        memcpy(InstanceExtensions, RequiredInstanceExtensions, RequiredInstanceExtensionCount*sizeof(const char *));
        memcpy(InstanceExtensions + RequiredInstanceExtensionCount, AdditionalInstanceExtensions, sizeof(AdditionalInstanceExtensions));

        VkInstanceCreateInfo InstanceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = 0,
            .flags = CreateFlags,
            .pApplicationInfo = &VulkanApplicationInfo,
            .enabledLayerCount = AvaiableRequestedLayerCount,
            .ppEnabledLayerNames = AvaiableRequestedLayers,
            .enabledExtensionCount = InstanceExtensionCount,
            .ppEnabledExtensionNames = InstanceExtensions,
        };
        
        VulkanCheckGoto(vkCreateInstance(&InstanceCreateInfo, 0, &C->Instance), label_Error);
    }

    goto label_Exit;
label_Error:
    Result = 1;
label_Exit:
    free(InstanceExtensions);
    free(InstanceLayerProperties);
    return Result;
}

int VulkanCreateDevice(context *C, VkSurfaceKHR Surface) {
    typedef enum feature_flags {
        feature_HAS_GRAPHICS_QUEUE = 0x01,
        feature_HAS_SURFACE_QUEUE = 0x02,
        feature_HAS_SWAPCHAIN_EXTENSION = 0x04,
        feature_HAS_PORTABILITY_SUBSET_EXTENSION = 0x08,
        feature_HAS_DEPTH_FORMAT = 0x10,
        feature_HAS_SURFACE_FORMAT = 0x11,

        feature_USABLE_MASK_A = feature_HAS_GRAPHICS_QUEUE | feature_HAS_SURFACE_QUEUE | feature_HAS_SWAPCHAIN_EXTENSION | feature_HAS_DEPTH_FORMAT | feature_HAS_SURFACE_FORMAT,

#ifdef __APPLE__
        feature_USABLE_MASK = feature_USABLE_MASK_A | feature_HAS_PORTABILITY_SUBSET_EXTENSION
#else
        feature_USABLE_MASK = feature_USABLE_MASK_A
#endif
    } feature_flags;

#if 0
    typedef struct {
        const char *VulkanName;
        VmaAllocatorCreateFlagBits VmaBit;
    } vma_extension_mapping;

    // VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT Use if single threaded
    static vma_extension_mapping VmaExtensionMap[] = {
        { "VK_KHR_get_memory_requirements2", VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT },
        //{ "VK_KHR_dedicated_allocation", VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT }, // BOTH MUST BE AVAILABLE
        { "VK_KHR_bind_memory2", VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT },
        { "VK_EXT_memory_budget", VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT },
        { "VK_AMD_device_coherent_memory", VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT }, // Additional requirement must be met
        { "VK_KHR_buffer_device_address", VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT }, // Additional requirement must be met
        { "VK_EXT_memory_priority", VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT }, // Additional requirement must be met
        { "VK_KHR_maintenance4", VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT },
        { "VK_KHR_maintenance5", VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT },
        { "VK_KHR_external_memory_win32", VMA_ALLOCATOR_CREATE_KHR_EXTERNAL_MEMORY_WIN32_BIT },
    };
#endif

    int Result = 1;

    C->Device = VK_NULL_HANDLE;
    C->Allocator = VK_NULL_HANDLE;
    VkPhysicalDevice *PhysicalDevices = 0;
    VkQueueFamilyProperties *QueueFamilyProperties = 0;
    VkExtensionProperties *ExtensionProperties = 0;
    VkSurfaceFormatKHR *SurfaceFormats = 0;
    VkPresentModeKHR *PresentModes = 0;
    {
        uint32_t PhysicalDeviceCount = 0;
        VulkanCheckGoto(vkEnumeratePhysicalDevices(C->Instance, &PhysicalDeviceCount, 0), label_Error);
        if(PhysicalDeviceCount == 0) {
            GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_API, "CreateContext: No physical devices");
            goto label_Error;
        }
        PhysicalDevices = malloc(PhysicalDeviceCount*sizeof(PhysicalDevices));
        if(PhysicalDevices == 0) {
            goto label_Error;
        }
        VulkanCheckGoto(vkEnumeratePhysicalDevices(C->Instance, &PhysicalDeviceCount, PhysicalDevices), label_Error);

        uint32_t BestPhysicalDeviceScore = 0;
        device_info BestPhysicalDeviceInfo = {0};
        for(uint32_t I = 0; I < PhysicalDeviceCount; ++I) {
            device_info PhysicalDeviceInfo = {0};
            VkPhysicalDevice PhysicalDevice = PhysicalDevices[I];
            PhysicalDeviceInfo.PhysicalDevice = PhysicalDevice;
            
            vkGetPhysicalDeviceFeatures(PhysicalDevice, &PhysicalDeviceInfo.Features);
            vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceInfo.Properties);

            uint32_t QueueFamilyPropertyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, 0);
            QueueFamilyProperties = malloc(QueueFamilyPropertyCount*sizeof(VkQueueFamilyProperties));
            if(QueueFamilyProperties == 0) {
                goto label_Error;
            }
            vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, QueueFamilyProperties);

            uint32_t ExtensionPropertyCount = 0;
            VulkanCheckGoto(vkEnumerateDeviceExtensionProperties(PhysicalDevice, 0, &ExtensionPropertyCount, 0), label_Error);
            ExtensionProperties = malloc(ExtensionPropertyCount*sizeof(VkExtensionProperties));
            if(ExtensionProperties == 0) {
                goto label_Error;
            }
            VulkanCheckGoto(vkEnumerateDeviceExtensionProperties(PhysicalDevice, 0, &ExtensionPropertyCount, ExtensionProperties), label_Error);
            
            feature_flags FeatureFlags = 0;
            for(uint32_t J = 0; J < QueueFamilyPropertyCount; ++J) {
                int IsGraphics = (QueueFamilyProperties[J].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
                
                VkBool32 IsSurfaceSupported;
                VulkanCheckGoto(vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, J, Surface, &IsSurfaceSupported), label_Error);

                // TODO(blackedout): Make sure these are actually equal if any pair has both capabilities ?
                if(IsGraphics) {
                    PhysicalDeviceInfo.QueueFamilyIndices[queue_GRAPHICS] = J;
                    FeatureFlags |= feature_HAS_GRAPHICS_QUEUE;
                }
                if(IsSurfaceSupported) {
                    PhysicalDeviceInfo.QueueFamilyIndices[queue_SURFACE] = J;
                    FeatureFlags |= feature_HAS_SURFACE_QUEUE;
                }
            }
            
            VmaAllocatorCreateFlags VmaCreateFlags = 0;
            for(uint32_t J = 0; J < ExtensionPropertyCount; ++J) {
                const char *ExtensionName = ExtensionProperties[J].extensionName;
                if(strcmp(ExtensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                    FeatureFlags |= feature_HAS_SWAPCHAIN_EXTENSION;
                }
                
                if(strcmp(ExtensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0) {
                    FeatureFlags |= feature_HAS_PORTABILITY_SUBSET_EXTENSION;
                }

#if 0
                // NOTE(blackedout): Check if extension is part of the vma extensions, so that vma can be told that it will be enabled
                for(uint32_t K = 0; K < ArrayCount(VmaExtensionMap); ++K) {
                    if(strcmp(ExtensionName, VmaExtensionMap[K].VulkanName) == 0) {
                        VmaCreateFlags |= VmaExtensionMap[K].VmaBit;
                    }
                }
#endif
            }

            uint32_t DeviceTypeScore;
            switch(PhysicalDeviceInfo.Properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                DeviceTypeScore = 3; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                DeviceTypeScore = 2; break;
            default:
                DeviceTypeScore = 1; break;
            }
            
            uint32_t SurfaceFormatCount = 0;
            VulkanCheckGoto(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &SurfaceFormatCount, 0), label_Error);
            SurfaceFormats = malloc(SurfaceFormatCount*sizeof(VkSurfaceFormatKHR));
            if(SurfaceFormats == 0) {
                goto label_Error;
            }
            VulkanCheckGoto(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &SurfaceFormatCount, SurfaceFormats), label_Error);
            
            // NOTE(blackedout): Since VK_COLOR_SPACE_SRGB_NONLINEAR_KHR is the only non extension surface color space,
            // search for a surface format that is _SRBG (we don't want shaders to output srgb manually).
            uint32_t BestSurfaceFormatScore = 0;
            for(uint32_t I = 0; I < SurfaceFormatCount; ++I) {
                VkSurfaceFormatKHR SurfaceFormat = SurfaceFormats[I];
                uint32_t Score = 0;
                if(SurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    if(SurfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB) {
                        Score = 3;
                    } else if(SurfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB) {
                        Score = 2;
                    } else if(strstr(string_VkFormat(SurfaceFormat.format), "_SRGB")) {
                        Score = 1;
                    }
                }
                if(Score > BestSurfaceFormatScore) {
                    BestSurfaceFormatScore = Score;
                    PhysicalDeviceInfo.InitialSurfaceFormat = SurfaceFormat;
                    FeatureFlags |= feature_HAS_SURFACE_FORMAT;
                }
            }
            
            uint32_t PresentModeCount = 0;
            VulkanCheckGoto(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentModeCount, 0), label_Error);
            PresentModes = malloc(PresentModeCount*sizeof(VkPresentModeKHR));
            if(PresentModes == 0) {
                goto label_Error;
            }
            VulkanCheckGoto(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentModeCount, PresentModes), label_Error);

            uint32_t BestPresentModeScore = 0;
            for(uint32_t I = 0; I < PresentModeCount; ++I) {
                uint32_t Score = (PresentModes[I] == VK_PRESENT_MODE_FIFO_KHR) ? 1 : ((PresentModes[I] == VK_PRESENT_MODE_MAILBOX_KHR) ? 2 : 0);
                if(Score > BestPresentModeScore) {
                    BestPresentModeScore = Score;
                    PhysicalDeviceInfo.BestPresentMode = PresentModes[I];
                }
            }
            if(BestPresentModeScore == 0) {
                goto label_Error;
            }
            
            VkFormat DepthFormats[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
            for(uint32_t J = 0; J < ArrayCount(DepthFormats); ++J) {
                VkFormatProperties FormatProperties;
                vkGetPhysicalDeviceFormatProperties(PhysicalDevice, DepthFormats[I], &FormatProperties);

                if(FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                    PhysicalDeviceInfo.DepthFormat = DepthFormats[I];
                    FeatureFlags |= feature_HAS_DEPTH_FORMAT;
                    break;
                }
            }

            VulkanCheckGoto(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &PhysicalDeviceInfo.SurfaceCapabilities), label_Error);

            VkSampleCountFlags PossibleSampleCountFlags = PhysicalDeviceInfo.Properties.limits.sampledImageColorSampleCounts & PhysicalDeviceInfo.Properties.limits.sampledImageDepthSampleCounts;
            for(VkSampleCountFlagBits SampleCount = VK_SAMPLE_COUNT_64_BIT; SampleCount > 0; SampleCount = (VkSampleCountFlagBits)(SampleCount >> 1)) {
                if(SampleCount & PossibleSampleCountFlags) {
                    PhysicalDeviceInfo.MaxSampleCount = SampleCount;
                    break;
                }
            }

            uint32_t FeatureScore = 0;
#if 0
            if(Features.samplerAnisotropy) {
                ++FeatureScore;
            }
#endif

            uint32_t Score = 0;
            if(FeatureFlags & feature_USABLE_MASK) {
                // NOTE(blackedout): As long as they are usable, just pick the best once (hence the added 1)
                Score = 1 + 3*DeviceTypeScore + BestSurfaceFormatScore + BestPresentModeScore + FeatureScore;
                if(Score > BestPhysicalDeviceScore) {
                    BestPhysicalDeviceScore = Score;
                    BestPhysicalDeviceInfo = PhysicalDeviceInfo;
                }
            }

            free(PresentModes);
            PresentModes = 0;
            free(SurfaceFormats);
            SurfaceFormats = 0;
            free(ExtensionProperties);
            ExtensionProperties = 0;
            free(QueueFamilyProperties);
            QueueFamilyProperties = 0;
        }

        if(BestPhysicalDeviceScore == 0) {
            goto label_Error;
        }

        float QueuePriorities[] = { 1.0f };
        VkDeviceQueueCreateInfo QueueCreateInfos[2] = {
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = 0,
                .flags = 0,
                .queueFamilyIndex = BestPhysicalDeviceInfo.QueueFamilyIndices[queue_GRAPHICS],
                .queueCount = 1,
                .pQueuePriorities = QueuePriorities
            },
        };

        uint32_t QueueCreateInfoCount = 1;
        if(BestPhysicalDeviceInfo.QueueFamilyIndices[queue_GRAPHICS] != BestPhysicalDeviceInfo.QueueFamilyIndices[queue_SURFACE]) {
            QueueCreateInfos[1] = QueueCreateInfos[0];
            QueueCreateInfos[1].queueFamilyIndex = BestPhysicalDeviceInfo.QueueFamilyIndices[queue_SURFACE];
            QueueCreateInfoCount = 2;
        }

        const char *ExtensionNames[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
            VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
#endif
        };

#if 0
        const char *FinalExtensionNames[ArrayCount(ExtensionNames) + ArrayCount(VmaExtensionMap)];
        for(uint32_t I = 0; I < ExtensionNameCount; ++I, ++FinalExtensionNameCount) {
            FinalExtensionNames[I] = ExtensionNames[I];
        }
        for(uint32_t I = 0; I < ArrayCount(VmaExtensionMap); ++I) {
            if(BestPhysicalDeviceVmaCreateFlags & VmaExtensionMap[I].VmaBit) {
                FinalExtensionNames[FinalExtensionNameCount++] = VmaExtensionMap[I].VulkanName;
            }
        }
#else
        const char **FinalExtensionNames = ExtensionNames;
        uint32_t FinalExtensionNameCount = ArrayCount(ExtensionNames);
#endif

        VkDeviceCreateInfo DeviceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .queueCreateInfoCount = QueueCreateInfoCount,
            .pQueueCreateInfos = QueueCreateInfos,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = 0,
            .enabledExtensionCount = FinalExtensionNameCount,
            .ppEnabledExtensionNames = FinalExtensionNames,
            // NOTE(blackedout): Enable all features since we don't know what's coming
            .pEnabledFeatures = &BestPhysicalDeviceInfo.Features,
        };
        VulkanCheckGoto(vkCreateDevice(BestPhysicalDeviceInfo.PhysicalDevice, &DeviceCreateInfo, 0, &C->Device), label_Error);
        C->DeviceInfo = BestPhysicalDeviceInfo;

        VmaAllocatorCreateInfo AllocatorCreateInfo = {
            .flags = 0, // TODO
            .physicalDevice = C->DeviceInfo.PhysicalDevice,
            .device = C->Device,
            .preferredLargeHeapBlockSize = 0,
            .pAllocationCallbacks = 0,
            .pDeviceMemoryCallbacks = 0,
            .pHeapSizeLimit = 0,
            .pVulkanFunctions = 0,
            .instance = C->Instance,
            .vulkanApiVersion = VK_API_VERSION_1_0
        };
        VulkanCheckGoto(vmaCreateAllocator(&AllocatorCreateInfo, &C->Allocator), label_Error);
    }

    Result = 0;
    goto label_Exit;
    
label_Error:
    vmaDestroyAllocator(C->Allocator);
    vkDestroyDevice(C->Device, 0);
label_Exit:
    free(PresentModes);
    free(SurfaceFormats);
    free(ExtensionProperties);
    free(QueueFamilyProperties);
    free(PhysicalDevices);
    return Result;
}