#include "cugl/cugl.h"
#include "cugl_core.h"

void cuglSwapBuffers(void) {
    context *C = &GlobalContext;

    typedef struct pipeline_state_x {
        GLuint Program;
        GLuint Vao;

        VkPipeline Pipeline;
        VkPipelineLayout Layout;
    } pipeline_state;

    u32 PipelineCount = 0;
    pipeline_state PipelineStates[16];
    u32 CurrentPipelineIndex = 0;

    pipeline_state CurrentPipelineState = {0};
    for(u64 I = 0; I < C->CommandCount; ++I) {
        command *Command = C->Commands + I;
        switch(Command->Type) {
        case command_BIND_PROGRAM: {
            CurrentPipelineState.Program = Command->BindProgram.Program;

            int FoundMatch = 0;
            u32 MatchIndex = 0;
            for(u32 J = 0; J < PipelineCount; ++I) {
                if(PipelineStates[J].Program == CurrentPipelineState.Program && PipelineStates[J].Vao == CurrentPipelineState.Vao) {
                    FoundMatch = 1;
                    MatchIndex = J;
                }
            }
            // First try to find a pipeline state that matches the current state

            if(FoundMatch) {
                if(MatchIndex == CurrentPipelineIndex) {
                    // Do nothing
                } else {
                    CurrentPipelineIndex = MatchIndex;
                    Command->ChangePipeline = 1;
                    Command->PipelineIndex = CurrentPipelineIndex;
                }
            } else {
                // NOTE(blackedout): Create new pipeline state
                CurrentPipelineIndex = PipelineCount++;
                PipelineStates[CurrentPipelineIndex] = CurrentPipelineState;
                Command->ChangePipeline = 1;
                Command->PipelineIndex = CurrentPipelineIndex;
            }
        } break;
        case command_BIND_VERTEX_BUFFERS: {
            // TODO(blackedout): Change pipeline only if different input attribute state
            CurrentPipelineState.Vao = Command->BindVertexBuffers.VertexArray;
            PipelineStates[CurrentPipelineIndex] = CurrentPipelineState;
        } break;
        default: {

        } break;
        }
    }

    VkRenderPass RenderPass = VK_NULL_HANDLE;
    {
        VkAttachmentDescription AttachmentDescriptions[] = {
            {
                .flags = 0,
                .format = C->DeviceInfo.InitialSurfaceFormat.format,
                .samples = 1,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
        };

        VkAttachmentReference AttachmentRefs[] = {
            {
                .attachment = 0, // NOTE(blackedout): Fragment shader layout index
                .layout = VK_IMAGE_LAYOUT_GENERAL,
            },
        };

        VkSubpassDescription SubpassDescription = {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = AttachmentRefs + 0,
            .pResolveAttachments = 0,
            .pDepthStencilAttachment = 0,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = 0,
        };

        VkRenderPassCreateInfo RenderPassCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .attachmentCount = ArrayCount(AttachmentDescriptions),
            .pAttachments = AttachmentDescriptions,
            .subpassCount = 1,
            .pSubpasses = &SubpassDescription,
            .dependencyCount = 0,
            .pDependencies = 0,
        };

        VulkanCheckReturn(vkCreateRenderPass(C->Device, &RenderPassCreateInfo, 0, &RenderPass));
    }

    for(u32 I = 0; I < PipelineCount; ++I) {
        pipeline_state *PipelineState = PipelineStates + I;

        // NOTE(blackedout): First, validate if a pipeline can be created
        if(PipelineState->Program == 0 || PipelineState->Vao == 0) {
            GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, "glSwapBuffers: invalid pipeline");
            continue;
        }

        object *ObjectP = 0;
        GetObject(C, PipelineState->Program, &ObjectP);
        VkPipelineShaderStageCreateInfo PipelineStageCreateInfos[PROGRAM_SHADER_CAPACITY] = {0};
        for(u32 J = 0; J < ObjectP->Program.AttachedShaderCount; ++J) {
            object *ObjectS = ObjectP->Program.AttachedShaders[J];
            shader_type_info TypeInfo = {0};
            GetShaderTypeInfo(ObjectS->Shader.Type, &TypeInfo); // TODO(blackedout): Error handling
            VkPipelineShaderStageCreateInfo ShaderStageCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = 0,
                .flags = 0,
                .stage = TypeInfo.VulkanBit,
                .module = ObjectS->Shader.VulkanModule,
                .pName = "main", // NOTE(blackedout): Entry point
                .pSpecializationInfo = 0
            };
            PipelineStageCreateInfos[J] = ShaderStageCreateInfo;
        }
        
#if 0
        VkDynamicState VulkanDynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .dynamicStateCount = ArrayCount(VulkanDynamicStates),
            .pDynamicStates = VulkanDynamicStates
        };
#endif

        VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .topology = C->CurrentPrimitiveTopoloy,
            .primitiveRestartEnable = VK_FALSE,
        };

        VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .viewportCount = 1,
            .pViewports = &C->Config.Viewport,
            .scissorCount = 1,
            .pScissors = &C->Config.Scissor,
        };

        VkPipelineMultisampleStateCreateInfo PipelineMultiSampleStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .rasterizationSamples = 1,
            .sampleShadingEnable = VK_FALSE, // TODO(blackedout): Enable this?
            .minSampleShading = 1.0f,
            .pSampleMask = 0,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE
        };
        
        VkStencilOpState EmptyStencilOpState = {0};
        VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = EmptyStencilOpState,
            .back = EmptyStencilOpState,
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        };

        VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_CLEAR,
            .attachmentCount = 1,
            .pAttachments = &C->Config.BlendAttachmentState,
            .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
        };

        VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .setLayoutCount = 0,
            .pSetLayouts = 0,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = 0,
        };
        VulkanCheckReturn(vkCreatePipelineLayout(C->Device, &PipelineLayoutCreateInfo, 0, &PipelineState->Layout));

        object *ObjectV = 0;
        GetObject(C, PipelineState->Vao, &ObjectV);

        u32 AttribDescCount = 0;
        VkVertexInputAttributeDescription AttributeDescriptions[VERTEX_ATTRIB_CAPACITY] = {0};
        for(u32 J = 0; J < VERTEX_ATTRIB_CAPACITY; ++J) {
            if(ObjectV->VertexArray.Attribs[J].Enabled) {
                AttributeDescriptions[AttribDescCount++] = ObjectV->VertexArray.Attribs[J].Desc;
            }
        }
        VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &ObjectV->VertexArray.Binding,
            .vertexAttributeDescriptionCount = AttribDescCount,
            .pVertexAttributeDescriptions = AttributeDescriptions,
        };

        VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .stageCount = ObjectP->Program.AttachedShaderCount,
            .pStages = PipelineStageCreateInfos,
            .pVertexInputState = &PipelineVertexInputStateCreateInfo,
            .pInputAssemblyState = &PipelineInputAssemblyStateCreateInfo,
            .pTessellationState = 0,
            .pViewportState = &PipelineViewportStateCreateInfo,
            .pRasterizationState = &C->Config.RasterState,
            .pMultisampleState = &PipelineMultiSampleStateCreateInfo,
            .pDepthStencilState = &PipelineDepthStencilStateCreateInfo,
            .pColorBlendState = &PipelineColorBlendStateCreateInfo,
            //.pDynamicState = &PipelineDynamicStateCreateInfo,
            .layout = PipelineState->Layout,
            .renderPass = RenderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };

        VulkanCheckReturn(vkCreateGraphicsPipelines(C->Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, 0, &PipelineState->Pipeline));
    }

    uint32_t AcquiredImageIndex = 0;
    VulkanCheckReturn(vkAcquireNextImageKHR(C->Device, C->Swapchain, UINT64_MAX, C->Semaphores[semaphore_PREV_PRESENT_DONE], VK_NULL_HANDLE, &AcquiredImageIndex));
    if(C->IsSingleBuffered) {
        C->CurrentFrontBackIndices[front_back_FRONT] = AcquiredImageIndex;
        C->CurrentFrontBackIndices[front_back_BACK] = AcquiredImageIndex;
    } else {
        C->CurrentFrontBackIndices[front_back_FRONT] = AcquiredImageIndex ^ 1;
        C->CurrentFrontBackIndices[front_back_BACK] = AcquiredImageIndex;
    }

    object *Object = 0;
    GetObject(C, 0, &Object);
    VkFramebuffer Framebuffer = VK_NULL_HANDLE;
    {
        VkFramebufferCreateInfo FramebufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .renderPass = RenderPass,
            .attachmentCount = 1,
            .pAttachments = &Object->Framebuffer.ColorImageViews[C->CurrentFrontBackIndices[front_back_BACK]],
            .width = C->DeviceInfo.SurfaceCapabilities.currentExtent.width,
            .height = C->DeviceInfo.SurfaceCapabilities.currentExtent.height,
            .layers = 1,
        };

        VulkanCheckReturn(vkCreateFramebuffer(C->Device, &FramebufferCreateInfo, 0, &Framebuffer));
    }

    VkCommandBufferBeginInfo BeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = 0,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = 0,
    };
    VulkanCheckReturn(vkBeginCommandBuffer(C->CommandBuffers[command_buffer_GRAPHICS], &BeginInfo));

    VkRenderPassBeginInfo RenderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = 0,
        .renderPass = RenderPass,
        .framebuffer = Framebuffer,
        .renderArea = {
            .offset.x = 0,
            .offset.y = 0,
            .extent.width = C->DeviceInfo.SurfaceCapabilities.currentExtent.width,
            .extent.height = C->DeviceInfo.SurfaceCapabilities.currentExtent.height,
        },
        .clearValueCount = 1,
        .pClearValues = &Object->Framebuffer.ClearValue,
    };

    VkCommandBuffer GraphicsCommandBuffer = C->CommandBuffers[command_buffer_GRAPHICS];
    vkCmdBeginRenderPass(GraphicsCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    
    for(u32 I = 0; I < C->CommandCount; ++I) {
        command *Command = C->Commands + I;
        if(Command->ChangePipeline) {
            pipeline_state *PipelineState = PipelineStates + Command->PipelineIndex;
            vkCmdBindPipeline(GraphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineState->Pipeline);
        }

        switch(Command->Type) {
        default:
        case command_BIND_PROGRAM:
            break;
        case command_BIND_VERTEX_BUFFERS: {
            object *Object = 0;
            GetObject(C, Command->BindVertexBuffers.VertexArray, &Object);
            VkBuffer Buffers[VERTEX_ATTRIB_CAPACITY] = {0};
            VkDeviceSize Offsets[VERTEX_ATTRIB_CAPACITY] = {0};
            u32 BufferCount = 0;
            for(u32 J = 0; J < VERTEX_ATTRIB_CAPACITY; ++J) {
                object *ObjectB = 0;
                GetObject(C, Object->VertexArray.Attribs[J].Buffer, &ObjectB);
                if(Object->VertexArray.Attribs[J].Enabled) {
                    Buffers[BufferCount++] = ObjectB->Buffer.Buffer;
                }
            }
            
            vkCmdBindVertexBuffers(GraphicsCommandBuffer, 0, BufferCount, Buffers, Offsets);

        } break;
        case command_DRAW: {
            vkCmdDraw(GraphicsCommandBuffer, Command->Draw.VertexCount, Command->Draw.InstanceCount, Command->Draw.VertexOffset, Command->Draw.InstanceOffset);
        } break;
        }
    }

    vkCmdEndRenderPass(C->CommandBuffers[command_buffer_GRAPHICS]);

#if 0
    VkImageMemoryBarrier FromPresentBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = 0,
        .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = Object->Framebuffer.ColorImages[C->CurrentFrontBackIndices[front_back_BACK]],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vkCmdPipelineBarrier(C->CommandBuffers[command_buffer_GRAPHICS], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &FromPresentBarrier);

    VkImageMemoryBarrier ToPresentBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = 0,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = Object->Framebuffer.ColorImages[C->CurrentFrontBackIndices[front_back_BACK]],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vkCmdPipelineBarrier(C->CommandBuffers[command_buffer_GRAPHICS], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0, 0, 1, &ToPresentBarrier);
#endif

    VulkanCheckReturn(vkEndCommandBuffer(C->CommandBuffers[command_buffer_GRAPHICS]));

    VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo SubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = 0,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &C->Semaphores[semaphore_PREV_PRESENT_DONE],
        .pWaitDstStageMask = &WaitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &C->CommandBuffers[command_buffer_GRAPHICS],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &C->Semaphores[semaphore_RENDER_COMPLETE],
    };

    VkQueue GraphicsQueue, SurfaceQueue;
    vkGetDeviceQueue(C->Device, C->DeviceInfo.QueueFamilyIndices[queue_GRAPHICS], 0, &GraphicsQueue);
    if(C->DeviceInfo.QueueFamilyIndices[queue_GRAPHICS] == C->DeviceInfo.QueueFamilyIndices[queue_SURFACE]) {
        SurfaceQueue = GraphicsQueue;
    } else {
        vkGetDeviceQueue(C->Device, C->DeviceInfo.QueueFamilyIndices[queue_SURFACE], 0, &SurfaceQueue);
    }
    VulkanCheckReturn(vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE));

    VkPresentInfoKHR PresentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = 0,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &C->Semaphores[semaphore_RENDER_COMPLETE],
        .swapchainCount = 1,
        .pSwapchains = &C->Swapchain,
        .pImageIndices = &C->CurrentFrontBackIndices[front_back_BACK],
        .pResults = 0,
    };
    VulkanCheckReturn(vkQueuePresentKHR(SurfaceQueue, &PresentInfo));

    VulkanCheckReturn(vkQueueWaitIdle(SurfaceQueue));

    VulkanCheckReturn(vkResetCommandBuffer(C->CommandBuffers[command_buffer_GRAPHICS], 0));

    vkDestroyFramebuffer(C->Device, Framebuffer, 0);
    for(u32 I = 0; I < PipelineCount; ++I) {
        vkDestroyPipeline(C->Device, PipelineStates[I].Pipeline, 0);
        vkDestroyPipelineLayout(C->Device, PipelineStates[I].Layout, 0);
    }
    
    vkDestroyRenderPass(C->Device, RenderPass, 0);

    C->CommandCount = 0;
    memset(C->Commands, 0, C->CommandCapacity*sizeof(command));
    if(C->ActiveProgram) {
        command Command = {
            .Type = command_BIND_PROGRAM,
            .BindProgram = {
                .Program = C->ActiveProgram
            }
        };
        PushCommand(C, Command);
    }
    if(C->BoundVertexArray) {
        command Command = {
            .Type = command_BIND_VERTEX_BUFFERS,
            .BindVertexBuffers = {
                .VertexArray = C->BoundVertexArray
            }
        };
        PushCommand(C, Command);
    }
}

int cuglCreateContext(const context_create_params *Params) {
    context *C = &GlobalContext;

    C->Instance = VK_NULL_HANDLE;
    C->Surface = VK_NULL_HANDLE;
    C->Device = VK_NULL_HANDLE;
    C->Allocator = VK_NULL_HANDLE;
    C->Swapchain = VK_NULL_HANDLE;
    C->GraphicsCommandPool = VK_NULL_HANDLE;
    for(u32 I = 0; I < command_buffer_COUNT; ++I) {
        C->CommandBuffers[I] = VK_NULL_HANDLE;
    }
    for(u32 I = 0; I < fence_COUNT; ++I) {
        C->Fences[I] = VK_NULL_HANDLE;
    }

    u32 CreatedImageViewCount = 0;
    u32 CreatedSemaphoreCount = 0;
    u32 CreatedFenceCount = 0;
    object *Object = 0;

    int Result = 1;
    if(VulkanCreateInstance(C, Params->RequiredInstanceExtensions, Params->RequiredInstanceExtensionCount)) {
        goto label_Error;
    }

    VulkanCheckGoto(Params->CreateSurface(C->Instance, 0, &C->Surface, Params->User), label_Error);

    if(VulkanCreateDevice(C, C->Surface)) {
        goto label_Error;
    }

    {
        VkCommandPoolCreateInfo GraphicsCommandPoolCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = 0,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = C->DeviceInfo.QueueFamilyIndices[queue_GRAPHICS]
        };
        VulkanCheckGoto(vkCreateCommandPool(C->Device, &GraphicsCommandPoolCreateInfo, 0, &C->GraphicsCommandPool), label_Error);

        VkCommandBufferAllocateInfo GraphicsCommandBufferAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = 0,
            .commandPool = C->GraphicsCommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = command_buffer_COUNT,
        };
        VulkanCheckGoto(vkAllocateCommandBuffers(C->Device, &GraphicsCommandBufferAllocateInfo, C->CommandBuffers), label_Error);
    }

    {
        VkFenceCreateInfo CreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
        };

        for(u32 I = 0; I < fence_COUNT; ++I) {
            VulkanCheckGoto(vkCreateFence(C->Device, &CreateInfo, 0, &C->Fences[I]), label_Error);
            ++CreatedFenceCount;
        }
    }

    VkSwapchainCreateInfoKHR SwapchainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = 0,
        .flags = 0,
        .surface = C->Surface,
        .minImageCount = 2,
        .imageFormat = C->DeviceInfo.InitialSurfaceFormat.format,
        .imageColorSpace = C->DeviceInfo.InitialSurfaceFormat.colorSpace,
        .imageExtent = C->DeviceInfo.SurfaceCapabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = queue_COUNT,
        .pQueueFamilyIndices = C->DeviceInfo.QueueFamilyIndices,
        .preTransform = C->DeviceInfo.SurfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = C->DeviceInfo.BestPresentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };
    if(C->DeviceInfo.QueueFamilyIndices[queue_GRAPHICS] == C->DeviceInfo.QueueFamilyIndices[queue_SURFACE]) {
        --SwapchainCreateInfo.queueFamilyIndexCount;
        SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VulkanCheckGoto(vkCreateSwapchainKHR(C->Device, &SwapchainCreateInfo, 0, &C->Swapchain), label_Error);
    uint32_t SwapchainImageCount = 0;
    VulkanCheckGoto(vkGetSwapchainImagesKHR(C->Device, C->Swapchain, &SwapchainImageCount, 0), label_Error);
    if(CreateFramebuffer(C, SwapchainImageCount)) {
        goto label_Error;
    }
    
    GetObject(C, 0, &Object);
    VulkanCheckGoto(vkGetSwapchainImagesKHR(C->Device, C->Swapchain, &SwapchainImageCount, Object->Framebuffer.ColorImages), label_Error);

    for(u32 I = 0; I < SwapchainImageCount; ++I) {
        VkImageViewCreateInfo ImageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .image = Object->Framebuffer.ColorImages[I],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = C->DeviceInfo.InitialSurfaceFormat.format,
            .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VulkanCheckGoto(vkCreateImageView(C->Device, &ImageViewCreateInfo, 0, Object->Framebuffer.ColorImageViews + I), label_Error);
        ++CreatedImageViewCount;
    }

    // TODO(blackedout): Put this default somewhere else?
    if(Params->IsSingleBuffered) { 
        Object->Framebuffer.ColorDrawCount = 1;
        Object->Framebuffer.ColorDrawIndices[0] = 0;
    } else {
        Object->Framebuffer.ColorDrawCount = 1;
        Object->Framebuffer.ColorDrawIndices[0] = 1;
    }

    {
        C->SemaphoreCount = semaphore_COUNT;
        C->Semaphores = malloc(semaphore_COUNT*sizeof(VkSemaphore));
        if(C->Semaphores == 0) {
            goto label_Error;
        }
        

        VkSemaphoreCreateInfo SemaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
        };
        
        for(u32 I = 0; I < semaphore_COUNT; ++I) {
            VulkanCheckGoto(vkCreateSemaphore(C->Device, &SemaphoreCreateInfo, 0, C->Semaphores + I), label_Error);
            ++CreatedSemaphoreCount;
        }
    }

    C->IsSingleBuffered = Params->IsSingleBuffered;
    C->Config = GetDefaultConfig(C->DeviceInfo.SurfaceCapabilities.currentExtent.width, C->DeviceInfo.SurfaceCapabilities.currentExtent.height);

    
    pipeline_state_info PipelineStateInfos[pipeline_state_COUNT] = {0};
    u32 PipelineStateByteCount = 0;
    SetPipelineStateInfos(PipelineStateInfos, &C->DeviceInfo.Properties.limits, &PipelineStateByteCount);


    Result = 0;
    goto label_Exit;

label_Error:
    for(u32 I = 0; I < CreatedSemaphoreCount; ++I) {
        vkDestroySemaphore(C->Device, C->Semaphores[I], 0);
    }
    for(u32 I = 0; I < CreatedImageViewCount; ++I) {
        vkDestroyImageView(C->Device, Object->Framebuffer.ColorImageViews[I], 0);
    }
    DeleteObject(C, Object);
    free(C->SwapchainImages);
    for(u32 I = 0; I < CreatedFenceCount; ++I) {
        vkDestroyFence(C->Device, C->Fences[I], 0);
    }
    vkFreeCommandBuffers(C->Device, C->GraphicsCommandPool, command_buffer_COUNT, C->CommandBuffers);
    vkDestroyCommandPool(C->Device, C->GraphicsCommandPool, 0);
    vmaDestroyAllocator(C->Allocator);
    vkDestroySwapchainKHR(C->Device, C->Swapchain, 0);
    vkDestroyDevice(C->Device, 0);
    vkDestroySurfaceKHR(C->Instance, C->Surface, 0);
    vkDestroyInstance(C->Instance, 0);
label_Exit:
    return Result;
}

void cubyz_glActiveShaderProgram(GLuint pipeline, GLuint program) {}
void cubyz_glActiveTexture(GLenum texture) {
    context *C = &GlobalContext;
    if(texture < GL_TEXTURE0 || GL_TEXTURE0 + TEXTURE_SLOT_COUNT <= texture) {
        const char *Msg = "An INVALID_ENUM error is generated if an invalid texture is specified. texture is a symbolic constant of the form TEXTUREi, indicating that texture unit iis to be modified. Each TEXTUREiadheres to TEXTUREi=TEXTURE0 + i, where i is in the range zero to k-1, and kis the value of MAX_COMBINED_TEXTURE_IMAGE_UNITS.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    C->ActiveTextureIndex = texture - GL_TEXTURE0;
}
void cubyz_glAttachShader(GLuint program, GLuint shader) {
    context *C = &GlobalContext;
    object *ObjectP = 0, *ObjectS = 0;
    if(HandledCheckProgramGet(C, program, &ObjectP) || HandledCheckShaderGet(C, shader, &ObjectS)) {
        return;
    }

    for(u32 I = 0; I < ObjectP->Program.AttachedShaderCount; ++I) {
        if(ObjectP->Program.AttachedShaders[I] == ObjectS) {
            const char *Msg = "An INVALID_OPERATION error is generated if shader is already attached to program.";
            GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, Msg);
            return;
        }
    }

    if(ObjectP->Program.AttachedShaderCount >= 6) {
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_API, "Custom: glAttachShader too many shaders");
        return;
    }
    ObjectP->Program.AttachedShaders[ObjectP->Program.AttachedShaderCount++] = ObjectS;
    ObjectS->Shader.Program = ObjectP;
}
void cubyz_glBeginConditionalRender(GLuint id, GLenum mode) {}
void cubyz_glBeginQuery(GLenum target, GLuint id) {}
void cubyz_glBeginQueryIndexed(GLenum target, GLuint index, GLuint id) {}
void cubyz_glBeginTransformFeedback(GLenum primitiveMode) {}
void cubyz_glBindAttribLocation(GLuint program, GLuint index, const GLchar * name) {}
void cubyz_glBindBuffer(GLenum target, GLuint buffer) {
    context *C = &GlobalContext;
    buffer_target_info TargetInfo = {0};
    if(GetBufferTargetInfo(target, &TargetInfo)) {
        const char *Msg = "An INVALID_ENUM error is generated if target is not one of the targets listed in table 6.1.";
        GenerateErrorMsg(&GlobalContext, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    if(buffer != 0 && CheckObjectCreated(C, buffer)) {
        // NOTE(blackedout): Named object creation not supported
        const char *Msg = "An INVALID_OPERATION error is generated if buffer is not zero or a name returned from a previous call to GenBuffers, or if such a name has since been deleted with DeleteBuffers.";
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_API, Msg);
        return;
    }

    C->BoundBuffers[TargetInfo.Index] = buffer;
}
void cubyz_glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {}
void cubyz_glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {}
void cubyz_glBindBuffersBase(GLenum target, GLuint first, GLsizei count, const GLuint * buffers) {}
void cubyz_glBindBuffersRange(GLenum target, GLuint first, GLsizei count, const GLuint * buffers, const GLintptr * offsets, const GLsizeiptr * sizes) {}
void cubyz_glBindFragDataLocation(GLuint program, GLuint color, const GLchar * name) {}
void cubyz_glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const GLchar * name) {}
void cubyz_glBindFramebuffer(GLenum target, GLuint framebuffer) {
    context *C = &GlobalContext;
    if(framebuffer != 0 && CheckObjectType(&GlobalContext, framebuffer, object_FRAMEBUFFER)) {
        const char *Msg = "An INVALID_OPERATION error is generated if framebuffer is not zero or a name returned from a previous call to GenFramebuffers, or if such a name has since been deleted with DeleteFramebuffers.";
        GenerateErrorMsg(&GlobalContext, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    int WasNone = 1;
    if(target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
        C->BoundFramebuffers[framebuffer_READ] = framebuffer;
        WasNone = 0;
    }
    if(target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
        C->BoundFramebuffers[framebuffer_WRITE] = framebuffer;
        WasNone = 0;
    }
    if(WasNone) {
        const char *Msg = "An INVALID_ENUM error is generated if target is not DRAW_FRAMEBUFFER, READ_FRAMEBUFFER, or FRAMEBUFFER.";
        GenerateErrorMsg(&GlobalContext, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
    }
}
void cubyz_glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {}
void cubyz_glBindImageTextures(GLuint first, GLsizei count, const GLuint * textures) {}
void cubyz_glBindProgramPipeline(GLuint pipeline) {}
void cubyz_glBindRenderbuffer(GLenum target, GLuint renderbuffer) {}
void cubyz_glBindSampler(GLuint unit, GLuint sampler) {}
void cubyz_glBindSamplers(GLuint first, GLsizei count, const GLuint * samplers) {}
void cubyz_glBindTexture(GLenum target, GLuint texture) {
    context *C = &GlobalContext;
    if(texture == 0) {
        return;
    }

    texture_target_info TargetInfo = {0};
    if(GetTextureTargetInfo(target, &TargetInfo)) {
        const char *Msg = "An INVALID_ENUM error is generated if target is not one of the texture targets described in the introduction to section 8.1.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    if(texture != 0) {
        object *Object = 0;
        if(CheckObjectTypeGet(C, texture, object_TEXTURE, &Object)) {
            const char *Msg = "An INVALID_OPERATION error is generated if texture is not zero or a name returned from a previous call to GenTextures, or if such a name has since been deleted.";
            GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, Msg);
            return;
        }

        if(Object->Texture.Dimension != texture_dimension_UNSPECIFIED && Object->Texture.Dimension != TargetInfo.Dimension) {
            const char *Msg = "An INVALID_OPERATION error is generated if an attempt is made to bind a texture object of different dimensionality than the specified target.";
            GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, Msg);
            return;
        }
    }
    
    C->BoundTextures[TargetInfo.Index*TEXTURE_TARGET_COUNT + C->ActiveTextureIndex] = texture;
}
void cubyz_glBindTextureUnit(GLuint unit, GLuint texture) {}
void cubyz_glBindTextures(GLuint first, GLsizei count, const GLuint * textures) {}
void cubyz_glBindTransformFeedback(GLenum target, GLuint id) {}
void cubyz_glBindVertexArray(GLuint array) {
    context *C = &GlobalContext;

    if(array != 0 && CheckObjectType(C, array, object_VERTEX_ARRAY)) {
        const char *Msg = "An INVALID_OPERATION error is generated if array is not zero or a name returned from a previous call to CreateVertexArrays or GenVertexArrays, or if such a name has since been deleted with DeleteVertexArrays.";
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    if(C->BoundVertexArray != array) {
        command Command = {
            .Type = command_BIND_VERTEX_BUFFERS,
            .BindVertexBuffers = {
                .VertexArray = array
            }
        };
        PushCommand(C, Command);
        C->BoundVertexArray = array;
    }
}
void cubyz_glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {}
void cubyz_glBindVertexBuffers(GLuint first, GLsizei count, const GLuint * buffers, const GLintptr * offsets, const GLsizei * strides) {}
void cubyz_glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    context *C = &GlobalContext;
    C->Config.BlendState.blendConstants[0] = Clamp01(red);
    C->Config.BlendState.blendConstants[1] = Clamp01(green);
    C->Config.BlendState.blendConstants[2] = Clamp01(blue);
    C->Config.BlendState.blendConstants[3] = Clamp01(alpha);
}
void cubyz_glBlendEquation(GLenum mode) {
    context *C = &GlobalContext;

    VkBlendOp VulkanBlendOp;
    if(GetVulkanBlendOp(mode, &VulkanBlendOp)) {
        const char *Msg = "An INVALID_ENUM error is generated if any of mode, modeRGB, or mode Alpha are not one of the blend equation modes in table 17.1.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    C->Config.BlendAttachmentState.colorBlendOp = VulkanBlendOp;
    C->Config.BlendAttachmentState.alphaBlendOp = VulkanBlendOp;
}
void cubyz_glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    context *C = &GlobalContext;

    VkBlendOp BlendOpColor, BlendOpAlpha;
    if(GetVulkanBlendOp(modeRGB, &BlendOpColor) || GetVulkanBlendOp(modeAlpha, &BlendOpAlpha)) {
        const char *Msg = "An INVALID_ENUM error is generated if any of mode, modeRGB, or mode Alpha are not one of the blend equation modes in table 17.1.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    C->Config.BlendAttachmentState.colorBlendOp = BlendOpColor;
    C->Config.BlendAttachmentState.alphaBlendOp = BlendOpAlpha;
}
void cubyz_glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {}
void cubyz_glBlendEquationi(GLuint buf, GLenum mode) {}
void cubyz_glBlendFunc(GLenum sfactor, GLenum dfactor) {
    context *C = &GlobalContext;

    VkBlendFactor FactorSrc, FactorDst;
    if(GetVulkanBlendFactor(sfactor, &FactorSrc) || GetVulkanBlendFactor(dfactor, &FactorDst)) {
        const char *Msg = "An INVALID_ENUM error is generated if any of src, dst, srcRGB, dstRGB, srcAlpha, or dstAlpha are not one of the blend functions in table 17.2.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    C->Config.BlendAttachmentState.srcColorBlendFactor = FactorSrc;
    C->Config.BlendAttachmentState.srcAlphaBlendFactor = FactorSrc;
    C->Config.BlendAttachmentState.dstColorBlendFactor = FactorDst;
    C->Config.BlendAttachmentState.dstAlphaBlendFactor = FactorDst;
}
void cubyz_glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    context *C = &GlobalContext;

    VkBlendFactor FactorSrcColor, FactorSrcAlpha, FactorDstColor, FactorDstAlpha;
    if(GetVulkanBlendFactor(sfactorRGB, &FactorSrcColor) || GetVulkanBlendFactor(dfactorRGB, &FactorDstColor) ||
        GetVulkanBlendFactor(sfactorAlpha, &FactorSrcAlpha) || GetVulkanBlendFactor(dfactorAlpha, &FactorDstAlpha)) {
        const char *Msg = "An INVALID_ENUM error is generated if any of src, dst, srcRGB, dstRGB, srcAlpha, or dstAlpha are not one of the blend functions in table 17.2.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    C->Config.BlendAttachmentState.srcColorBlendFactor = FactorSrcColor;
    C->Config.BlendAttachmentState.srcAlphaBlendFactor = FactorSrcAlpha;
    C->Config.BlendAttachmentState.dstColorBlendFactor = FactorDstColor;
    C->Config.BlendAttachmentState.dstAlphaBlendFactor = FactorDstAlpha;
}
void cubyz_glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {}
void cubyz_glBlendFunci(GLuint buf, GLenum src, GLenum dst) {}
void cubyz_glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}
void cubyz_glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}
void cubyz_glBufferData(GLenum target, GLsizeiptr size, const void * data, GLenum usage) {
    context *C = &GlobalContext;
    buffer_target_info TargetInfo = {0};
    if(GetBufferTargetInfo(target, &TargetInfo)) {
        const char *Msg = "An INVALID_ENUM error is generated by BufferData if target is not one of the targets listed in table 6.1.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    if(size < 0) {
        const char *Msg = "An INVALID_VALUE error is generated if size is negative.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    GLuint H = C->BoundBuffers[TargetInfo.Index];
    if(H == 0) {
        const char *Msg = "An INVALID_OPERATION error is generated by BufferData if zero is bound to target.";
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    object *Object = 0;
    if(CheckObjectTypeGet(C, C->BoundBuffers[TargetInfo.Index], object_BUFFER, &Object)) {
        const char *Msg = "glBufferData: invalid object";
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_API, Msg);
        return;
    }

#if 0
    switch(usage) {
    case GL_STATIC_DRAW: {

    } break;  
    default:
        break;
    }
#endif

    { // NOTE(blackedout): Device buffer is always created here
        VkBufferCreateInfo BufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | TargetInfo.VulkanUsage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &C->DeviceInfo.QueueFamilyIndices[queue_GRAPHICS],
        };
        VmaAllocationCreateInfo AllocationCreateInfo = {
            .flags = 0,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .preferredFlags = 0,
            .memoryTypeBits = 0,
            .pool = 0,
            .pUserData = 0,
            .priority = 0,
        };
        VkBuffer Buffer = VK_NULL_HANDLE;
        VmaAllocation Allocation = VK_NULL_HANDLE;
        VulkanCheckReturn(vmaCreateBuffer(C->Allocator, &BufferCreateInfo, &AllocationCreateInfo, &Object->Buffer.Buffer, &Object->Buffer.Allocation, 0));
    }

    // NOTE(blackedout): Copy data only if provided by using a temporary staging buffer
    if(data) {
        VkBufferCreateInfo StagingBufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &C->DeviceInfo.QueueFamilyIndices[queue_GRAPHICS],
        };
        VmaAllocationCreateInfo StagingAllocationCreateInfo = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            .preferredFlags = 0,
            .memoryTypeBits = 0,
            .pool = 0,
            .pUserData = 0,
            .priority = 0,
        };

        VkBuffer StagingBuffer = VK_NULL_HANDLE;
        VmaAllocation StagingAllocation = VK_NULL_HANDLE;
        VmaAllocationInfo StagingInfo = {0};
        VulkanCheckReturn(vmaCreateBuffer(C->Allocator, &StagingBufferCreateInfo, &StagingAllocationCreateInfo, &StagingBuffer, &StagingAllocation, &StagingInfo));
        void *MappedData = 0;
        VulkanCheckReturn(vmaMapMemory(C->Allocator, StagingAllocation, &MappedData));
        memcpy(MappedData, data, size);
        vmaUnmapMemory(C->Allocator, StagingAllocation);

        VkCommandBufferBeginInfo BeginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = 0,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = 0,
        };

        VkCommandBuffer TransferCommandBuffer = C->CommandBuffers[command_buffer_TRANSFER];
        VulkanCheckReturn(vkBeginCommandBuffer(TransferCommandBuffer, &BeginInfo));
        VkBufferCopy Copy = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size,
        };
        vkCmdCopyBuffer(TransferCommandBuffer, StagingBuffer, Object->Buffer.Buffer, 1, &Copy);
        VulkanCheckReturn(vkEndCommandBuffer(TransferCommandBuffer));

        // TODO(blackedout): Should probably use a separate queue here
        VkQueue TransferQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(C->Device, C->DeviceInfo.QueueFamilyIndices[queue_GRAPHICS], 0, &TransferQueue);
        VkSubmitInfo SubmitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = 0,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = 0,
            .pWaitDstStageMask = 0,
            .commandBufferCount = 1,
            .pCommandBuffers = &TransferCommandBuffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = 0,
        };
        
        VulkanCheckReturn(vkQueueSubmit(TransferQueue, 1, &SubmitInfo, C->Fences[fence_TRANSFER]));
        vkWaitForFences(C->Device, 1, C->Fences + fence_TRANSFER, VK_TRUE, UINT64_MAX);
        vkResetCommandBuffer(TransferCommandBuffer, 0);

        vmaDestroyBuffer(C->Allocator, StagingBuffer, StagingAllocation);
    }
}
void cubyz_glBufferStorage(GLenum target, GLsizeiptr size, const void * data, GLbitfield flags) {}
void cubyz_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void * data) {}
GLenum cubyz_glCheckFramebufferStatus(GLenum target) {return GL_FRAMEBUFFER_COMPLETE;}
GLenum cubyz_glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) {return GL_FRAMEBUFFER_COMPLETE;}
void cubyz_glClampColor(GLenum target, GLenum clamp) {}
void cubyz_glClear(GLbitfield mask) {
    context *C = &GlobalContext;
    object *Object;
    GLuint H = C->BoundFramebuffers[framebuffer_WRITE];
    if(CheckObjectTypeGet(C, H, object_FRAMEBUFFER, &Object)) {
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, "glClear: TODO invalid framebuffer");
        return;
    }

    // TODO(blackedout): Depth (which buffer to choose?)
#if 0
    VkImageAspectFlags AspectMask = VK_IMAGE_ASPECT_NONE;
    if(mask & GL_DEPTH_BUFFER_BIT) {
        AspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if(mask & GL_STENCIL_BUFFER_BIT) {
        AspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
#endif

    // TODO(blackedout): Scissor bounds cleared region?

    command Command = {0};
    Command.Type = command_CLEAR;
    PushCommand(C, Command);

#if 0
    if(mask & GL_COLOR_BUFFER_BIT) {
        VkImageSubresourceRange ColorSubresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        VkCommandBuffer CommandBuffer = C->SecondaryCommandBuffer;
        VkImage *ColorImages = Object->Framebuffer.ColorImages;
        VkClearColorValue *ClearColor = &Object->Framebuffer.ClearValue.color;
        if(H == 0) {
            // NOTE(blackedout): Default framebuffer
            for(u32 I = 0; I < Object->Framebuffer.ColorDrawCount; ++I) {
                u32 FrontBackIndex = Object->Framebuffer.ColorDrawIndices[I];
                u32 DrawIndex = C->CurrentFrontBackIndices[FrontBackIndex];
                vkCmdClearColorImage(CommandBuffer, ColorImages[DrawIndex], VK_IMAGE_LAYOUT_GENERAL, ClearColor, 1, &ColorSubresourceRange);
            }
        } else {
            for(u32 I = 0; I < Object->Framebuffer.ColorDrawCount; ++I) {
                u32 DrawIndex = Object->Framebuffer.ColorDrawIndices[I];
                vkCmdClearColorImage(CommandBuffer, ColorImages[DrawIndex], VK_IMAGE_LAYOUT_GENERAL, ClearColor, 1, &ColorSubresourceRange);
            }
        }
    }
#endif
}
void cubyz_glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void * data) {}
void cubyz_glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void * data) {}
void cubyz_glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {}
void cubyz_glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat * value) {}
void cubyz_glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint * value) {}
void cubyz_glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint * value) {}
void cubyz_glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    context *C = &GlobalContext;
    object *Object;
    if(CheckObjectTypeGet(C, C->BoundFramebuffers[framebuffer_WRITE], object_FRAMEBUFFER, &Object)) {
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, "glClearColor: TODO invalid framebuffer");
        return;
    }

    VkClearColorValue ClearColor = { 
        .float32 = { red, green, blue, alpha }
    };
    Object->Framebuffer.ClearValue.color = ClearColor;
}
void cubyz_glClearDepth(GLdouble depth) {
    context *C = &GlobalContext;
    object *Object;
    if(CheckObjectTypeGet(C, C->BoundFramebuffers[framebuffer_WRITE], object_FRAMEBUFFER, &Object)) {
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, "glClearDepth: TODO invalid framebuffer");
        return;
    }

    Object->Framebuffer.ClearValue.depthStencil.depth = (float)depth;
}
void cubyz_glClearDepthf(GLfloat d) {
    context *C = &GlobalContext;
    object *Object;
    if(CheckObjectTypeGet(C, C->BoundFramebuffers[framebuffer_WRITE], object_FRAMEBUFFER, &Object)) {
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, "glClearDepthf: TODO invalid framebuffer");
        return;
    }

    Object->Framebuffer.ClearValue.depthStencil.depth = d;
}
void cubyz_glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void * data) {}
void cubyz_glClearNamedBufferSubData(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void * data) {}
void cubyz_glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {}
void cubyz_glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat * value) {}
void cubyz_glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint * value) {}
void cubyz_glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint * value) {}
void cubyz_glClearStencil(GLint s) {
    context *C = &GlobalContext;
    object *Object;
    if(CheckObjectTypeGet(C, C->BoundFramebuffers[framebuffer_WRITE], object_FRAMEBUFFER, &Object)) {
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, "glClearStencil: TODO invalid framebuffer");
        return;
    }

    Object->Framebuffer.ClearValue.depthStencil.stencil = (uint32_t)s;
}
void cubyz_glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void * data) {}
void cubyz_glClearTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * data) {}
GLenum cubyz_glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {return GL_CONDITION_SATISFIED;}
void cubyz_glClipControl(GLenum origin, GLenum depth) {}
void cubyz_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {}
void cubyz_glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a) {}
void cubyz_glCompileShader(GLuint shader) {
    context *C = &GlobalContext;
    object *Object = 0;
    if(HandledCheckShaderGet(C, shader, &Object)) {
        return;
    }

    GLuint Result = 0;
    shader_type_info TypeInfo = {0};
    if(GetShaderTypeInfo(Object->Shader.Type, &TypeInfo)) {
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_API, "Custom: glCompileShader invalid shader type");
        return;
    }

    switch(GlslangShaderCreateAndParse(TypeInfo.InterfaceType, Object->Shader.SourceBytes, Object->Shader.SourceByteCount, &Object->Shader.GlslangShader)) {
    case glslang_error_INPUT:
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_API, "Custom: glCompileShader invalid shader type");
        return;
    case glslang_error_OUT_OF_MEMORY:
        GenerateErrorMsg(C, GL_OUT_OF_MEMORY, GL_DEBUG_SOURCE_API, "Custom: glCompileShader out of memory");
        return;
    case glslang_error_COMPILATION_FAILED:
        Object->Shader.CompileStatus = GL_FALSE;
        break;
    case glslang_error_NONE:
        Object->Shader.CompileStatus = GL_TRUE;
        break;
    }
}
void cubyz_glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void * data) {}
void cubyz_glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void * data) {}
void cubyz_glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void * data) {}
void cubyz_glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void * data) {}
void cubyz_glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void * data) {}
void cubyz_glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void * data) {}
void cubyz_glCompressedTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void * data) {}
void cubyz_glCompressedTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void * data) {}
void cubyz_glCompressedTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void * data) {}
void cubyz_glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {}
void cubyz_glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {}
void cubyz_glCopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {}
void cubyz_glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) {}
void cubyz_glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {}
void cubyz_glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {}
void cubyz_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {}
void cubyz_glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {}
void cubyz_glCopyTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {}
void cubyz_glCopyTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {}
void cubyz_glCopyTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {}
void cubyz_glCreateBuffers(GLsizei n, GLuint * buffers) {}
void cubyz_glCreateFramebuffers(GLsizei n, GLuint * framebuffers) {}
GLuint cubyz_glCreateProgram(void) {
    context *C = &GlobalContext;
    GLuint Result = 0;
    object Template = {0};
    Template.Type = object_PROGRAM;
    if(CreateObjects(C, Template, 1, &Result)) {
        return Result;
    }

    object *Object = 0;
    GetObject(C, Result, &Object);
    if(GlslangProgramCreate(&Object->Program.GlslangProgram)) {
        DeleteObjects(C, 1, &Result, object_PROGRAM);
        Result = 0;

        const char *Msg = "Custom: glCreateProgram out of memory";
        GenerateErrorMsg(C, GL_OUT_OF_MEMORY, GL_DEBUG_SOURCE_API, Msg);
    }

    return Result;
}
void cubyz_glCreateProgramPipelines(GLsizei n, GLuint * pipelines) {}
void cubyz_glCreateQueries(GLenum target, GLsizei n, GLuint * ids) {}
void cubyz_glCreateRenderbuffers(GLsizei n, GLuint * renderbuffers) {}
void cubyz_glCreateSamplers(GLsizei n, GLuint * samplers) {}
GLuint cubyz_glCreateShader(GLenum type) {
    context *C = &GlobalContext;

    GLuint Result = 0;
    shader_type_info TypeInfo = {0};
    if(GetShaderTypeInfo(type, &TypeInfo)) {
        const char *Msg = "glCreateShader: An INVALID_ENUM error is generated and zero is returned if type is not one of the values in table 7.1.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return Result;
    }

    object Template = {0};
    Template.Type = object_SHADER;
    Template.Shader.Type = type;
    CreateObjects(C, Template, 1, &Result);
    
    return Result;
}
GLuint cubyz_glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const* strings) {return 1;}
void cubyz_glCreateTextures(GLenum target, GLsizei n, GLuint * textures) {}
void cubyz_glCreateTransformFeedbacks(GLsizei n, GLuint * ids) {}
void cubyz_glCreateVertexArrays(GLsizei n, GLuint * arrays) {}
void cubyz_glCullFace(GLenum mode) {
    context *C = &GlobalContext;

#define MakeCase(K, V) case (K): C->Config.RasterState.cullMode = (V); break
    switch(mode) {
    MakeCase(GL_FRONT, VK_CULL_MODE_FRONT_BIT);
    MakeCase(GL_BACK, VK_CULL_MODE_BACK_BIT);
    MakeCase(GL_FRONT_AND_BACK, VK_CULL_MODE_FRONT_AND_BACK);
    default: {
        const char *Msg = "glCullFace: invalid mode";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
    } break;
    }
#undef MakeCase
}
void cubyz_glDebugMessageCallback(GLDEBUGPROC callback, const void * userParam) {
    (&GlobalContext)->DebugCallback = callback;
    (&GlobalContext)->DebugCallbackUser = userParam;
}
void cubyz_glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint * ids, GLboolean enabled) {}
void cubyz_glDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * buf) {}
void cubyz_glDeleteBuffers(GLsizei n, const GLuint * buffers) {
    DeleteObjectsSizei(&GlobalContext, n, buffers, object_BUFFER);
}
void cubyz_glDeleteFramebuffers(GLsizei n, const GLuint * framebuffers) {
    DeleteObjectsSizei(&GlobalContext, n, framebuffers, object_FRAMEBUFFER);
}
void cubyz_glDeleteProgram(GLuint program) {
    context *C = &GlobalContext;
    if(program == 0) {
        return;
    }

    object *Object = 0;
    if(HandledCheckProgramGet(C, program, &Object)) {
        return;
    }

    // TODO(blackedout): When to delete exactly

    Object->Program.ShouldDelete = GL_TRUE;
    //DeleteObjects(C, 1, &program, object_PROGRAM);
}
void cubyz_glDeleteProgramPipelines(GLsizei n, const GLuint * pipelines) {}
void cubyz_glDeleteQueries(GLsizei n, const GLuint * ids) {
    DeleteObjectsSizei(&GlobalContext, n, ids, object_QUERY);
}
void cubyz_glDeleteRenderbuffers(GLsizei n, const GLuint * renderbuffers) {
    DeleteObjectsSizei(&GlobalContext, n, renderbuffers, object_RENDERBUFFER);
}
void cubyz_glDeleteSamplers(GLsizei count, const GLuint * samplers) {}
void cubyz_glDeleteShader(GLuint shader) {
    context *C = &GlobalContext;
    if(shader == 0) {
        return;
    }

    object *Object = 0;
    if(HandledCheckShaderGet(C, shader, &Object)) {
        return;
    }

    if(Object->Shader.Program) {
        Object->Shader.ShouldDelete = GL_TRUE;
    } else {
        DeleteShader(C, Object);
        DeleteObjects(C, 1, &shader, object_SHADER);
    }
}
void cubyz_glDeleteSync(GLsync sync) {
    // TODO(blackedout): This pointer cast is potentially dangerous
    DeleteObjects(&GlobalContext, 1, (GLuint *)&sync, object_SYNC);
}
void cubyz_glDeleteTextures(GLsizei n, const GLuint * textures) {
    DeleteObjectsSizei(&GlobalContext, n, textures, object_TEXTURE);
}
void cubyz_glDeleteTransformFeedbacks(GLsizei n, const GLuint * ids) {}
void cubyz_glDeleteVertexArrays(GLsizei n, const GLuint * arrays) {
    DeleteObjectsSizei(&GlobalContext, n, arrays, object_VERTEX_ARRAY);
}
void cubyz_glDepthFunc(GLenum func) {
    context *C = &GlobalContext;

#define MakeCase(K, V) case (K): C->Config.DepthStencilState.depthCompareOp = (V); break
    switch(func) {
    MakeCase(GL_NEVER, VK_COMPARE_OP_NEVER);
    MakeCase(GL_LESS, VK_COMPARE_OP_LESS);
    MakeCase(GL_EQUAL, VK_COMPARE_OP_EQUAL);
    MakeCase(GL_LEQUAL, VK_COMPARE_OP_LESS_OR_EQUAL);
    MakeCase(GL_GREATER, VK_COMPARE_OP_GREATER);
    MakeCase(GL_NOTEQUAL, VK_COMPARE_OP_NOT_EQUAL);
    MakeCase(GL_GEQUAL, VK_COMPARE_OP_GREATER_OR_EQUAL);
    MakeCase(GL_ALWAYS, VK_COMPARE_OP_ALWAYS);
    default: {
        const char *Msg = "glDepthFunc: invalid func";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
    } break;
    }
#undef MakeCase
}
void cubyz_glDepthMask(GLboolean flag) {}
void cubyz_glDepthRange(GLdouble n, GLdouble f) {}
void cubyz_glDepthRangeArrayv(GLuint first, GLsizei count, const GLdouble * v) {}
void cubyz_glDepthRangeIndexed(GLuint index, GLdouble n, GLdouble f) {}
void cubyz_glDepthRangef(GLfloat n, GLfloat f) {}
void cubyz_glDetachShader(GLuint program, GLuint shader) {}
void cubyz_glDisable(GLenum cap) {
    HandledCheckCapSet(&GlobalContext, cap, 0);
}
void cubyz_glDisableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    SetVertexArrayAttribEnabled(&GlobalContext, vaobj, index, 0);
}
void cubyz_glDisableVertexAttribArray(GLuint index) {
    context *C = &GlobalContext;
    SetVertexArrayAttribEnabled(C, C->BoundVertexArray, index, 0);
}
void cubyz_glDisablei(GLenum target, GLuint index) {}
void cubyz_glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {}
void cubyz_glDispatchComputeIndirect(GLintptr indirect) {}
void cubyz_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    context *C = &GlobalContext;

    if(first < 0) {
        const char *Msg = "Specifying first < 0 results in undefined behavior. Generating an INVALID_VALUE error is recommended in this case.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    if(count < 0) {
        const char *Msg = "An INVALID_VALUE error is generated if count is negative.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    primitive_info PrimitiveInfo = {0};
    if(GetPrimitiveInfo(mode, &PrimitiveInfo)) {
        const char *Msg = "An INVALID_ENUM error is generated if mode is not one of the primitive types defined in section 10.1.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    if(mode == GL_LINE_LOOP) {
        // TODO(blackedout): Handle special case
    }

    if(C->IsPrimitiveTopologySet) {
        // NOTE(blackedout): Flush
    }
    C->IsPrimitiveTopologySet = 1;
    C->CurrentPrimitiveTopoloy = PrimitiveInfo.VulkanPrimitve;

    command Command = {
        .Type = command_DRAW,
        .Draw = {
            .VertexCount = count,
            .VertexOffset = first,
            .InstanceCount = 1,
            .InstanceOffset = 0,
        },
    };
    PushCommand(C, Command);
}
void cubyz_glDrawArraysIndirect(GLenum mode, const void * indirect) {}
void cubyz_glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {}
void cubyz_glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) {}
void cubyz_glDrawBuffer(GLenum buf) {}
void cubyz_glDrawBuffers(GLsizei n, const GLenum * bufs) {}
void cubyz_glDrawElements(GLenum mode, GLsizei count, GLenum type, const void * indices) {}
void cubyz_glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void * indices, GLint basevertex) {}
void cubyz_glDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect) {}
void cubyz_glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount) {}
void cubyz_glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount, GLuint baseinstance) {}
void cubyz_glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount, GLint basevertex) {}
void cubyz_glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) {}
void cubyz_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void * indices) {}
void cubyz_glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void * indices, GLint basevertex) {}
void cubyz_glDrawTransformFeedback(GLenum mode, GLuint id) {}
void cubyz_glDrawTransformFeedbackInstanced(GLenum mode, GLuint id, GLsizei instancecount) {}
void cubyz_glDrawTransformFeedbackStream(GLenum mode, GLuint id, GLuint stream) {}
void cubyz_glDrawTransformFeedbackStreamInstanced(GLenum mode, GLuint id, GLuint stream, GLsizei instancecount) {}
void cubyz_glEnable(GLenum cap) {
    context *C = &GlobalContext;
    HandledCheckCapSet(&GlobalContext, cap, 1);
}
void cubyz_glEnableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    SetVertexArrayAttribEnabled(&GlobalContext, vaobj, index, 1);
}
void cubyz_glEnableVertexAttribArray(GLuint index) {
    context *C = &GlobalContext;
    SetVertexArrayAttribEnabled(C, C->BoundVertexArray, index, 1);
}
void cubyz_glEnablei(GLenum target, GLuint index) {}
void cubyz_glEndConditionalRender(void) {}
void cubyz_glEndQuery(GLenum target) {}
void cubyz_glEndQueryIndexed(GLenum target, GLuint index) {}
void cubyz_glEndTransformFeedback(void) {}
GLsync cubyz_glFenceSync(GLenum condition, GLbitfield flags) {return (GLsync)1;}
void cubyz_glFinish(void) {}
void cubyz_glFlush(void) {}
void cubyz_glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {}
void cubyz_glFlushMappedNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length) {}
void cubyz_glFramebufferParameteri(GLenum target, GLenum pname, GLint param) {}
void cubyz_glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {}
void cubyz_glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) {}
void cubyz_glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
void cubyz_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
void cubyz_glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) {}
void cubyz_glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {}
void cubyz_glFrontFace(GLenum mode) {
    context *C = &GlobalContext;

#define MakeCase(K, V) case (K): C->Config.RasterState.frontFace = (V); break
    switch(mode) {
    MakeCase(GL_CW, VK_FRONT_FACE_CLOCKWISE);
    MakeCase(GL_CCW, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    default: {
        const char *Msg = "glFrontFace: invalid mode";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
    } break;
    }
#undef MakeCase
}
void cubyz_glGenBuffers(GLsizei n, GLuint * buffers) {
    object Template = {0};
    Template.Type = object_BUFFER;
    CreateObjectsSizei(&GlobalContext, Template, n, buffers);
}
void cubyz_glGenFramebuffers(GLsizei n, GLuint * framebuffers) {
    object Template = {0};
    Template.Type = object_FRAMEBUFFER;
    CreateObjectsSizei(&GlobalContext, Template, n, framebuffers);
}
void cubyz_glGenProgramPipelines(GLsizei n, GLuint * pipelines) {}
void cubyz_glGenQueries(GLsizei n, GLuint * ids) {
    object Template = {0};
    Template.Type = object_QUERY;
    CreateObjectsSizei(&GlobalContext, Template, n, ids);
}
void cubyz_glGenRenderbuffers(GLsizei n, GLuint * renderbuffers) {}
void cubyz_glGenSamplers(GLsizei count, GLuint * samplers) {}
void cubyz_glGenTextures(GLsizei n, GLuint * textures) {
    object Template = {0};
    Template.Type = object_TEXTURE;
    CreateObjectsSizei(&GlobalContext, Template, n, textures);
}
void cubyz_glGenTransformFeedbacks(GLsizei n, GLuint * ids) {}
void cubyz_glGenVertexArrays(GLsizei n, GLuint * arrays) {
    object Template = {0};
    Template.Type = object_VERTEX_ARRAY;
    CreateObjectsSizei(&GlobalContext, Template, n, arrays);
}
void cubyz_glGenerateMipmap(GLenum target) {}
void cubyz_glGenerateTextureMipmap(GLuint texture) {}
void cubyz_glGetActiveAtomicCounterBufferiv(GLuint program, GLuint bufferIndex, GLenum pname, GLint * params) {}
void cubyz_glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name) {}
void cubyz_glGetActiveSubroutineName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei * length, GLchar * name) {}
void cubyz_glGetActiveSubroutineUniformName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei * length, GLchar * name) {}
void cubyz_glGetActiveSubroutineUniformiv(GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint * values) {}
void cubyz_glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name) {}
void cubyz_glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei * length, GLchar * uniformBlockName) {}
void cubyz_glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint * params) {}
void cubyz_glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei * length, GLchar * uniformName) {}
void cubyz_glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint * uniformIndices, GLenum pname, GLint * params) {}
void cubyz_glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei * count, GLuint * shaders) {}
GLint cubyz_glGetAttribLocation(GLuint program, const GLchar * name) {return 1;}
void cubyz_glGetBooleani_v(GLenum target, GLuint index, GLboolean * data) {}
void cubyz_glGetBooleanv(GLenum pname, GLboolean * data) {}
void cubyz_glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 * params) {}
void cubyz_glGetBufferParameteriv(GLenum target, GLenum pname, GLint * params) {}
void cubyz_glGetBufferPointerv(GLenum target, GLenum pname, void ** params) {}
void cubyz_glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void * data) {}
void cubyz_glGetCompressedTexImage(GLenum target, GLint level, void * img) {}
void cubyz_glGetCompressedTextureImage(GLuint texture, GLint level, GLsizei bufSize, void * pixels) {}
void cubyz_glGetCompressedTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void * pixels) {}
GLuint cubyz_glGetDebugMessageLog(GLuint count, GLsizei bufSize, GLenum * sources, GLenum * types, GLuint * ids, GLenum * severities, GLsizei * lengths, GLchar * messageLog) {return 1;}
void cubyz_glGetDoublei_v(GLenum target, GLuint index, GLdouble * data) {}
void cubyz_glGetDoublev(GLenum pname, GLdouble * data) {}
GLenum cubyz_glGetError(void) {
    context *C = &GlobalContext;
    GLenum Result = C->ErrorFlag;
    C->ErrorFlag = GL_NO_ERROR;
    return Result;
}
void cubyz_glGetFloati_v(GLenum target, GLuint index, GLfloat * data) {}
void cubyz_glGetFloatv(GLenum pname, GLfloat * data) {}
GLint cubyz_glGetFragDataIndex(GLuint program, const GLchar * name) {return 1;}
GLint cubyz_glGetFragDataLocation(GLuint program, const GLchar * name) {return 1;}
void cubyz_glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint * params) {}
void cubyz_glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint * params) {}
GLenum cubyz_glGetGraphicsResetStatus(void) {return GL_NO_ERROR;}
void cubyz_glGetInteger64i_v(GLenum target, GLuint index, GLint64 * data) {}
void cubyz_glGetInteger64v(GLenum pname, GLint64 * data) {}
void cubyz_glGetIntegeri_v(GLenum target, GLuint index, GLint * data) {}
void cubyz_glGetIntegerv(GLenum pname, GLint * data) {
#define MakeCase(K, V) case (K): *data = (V); break
    switch(pname) {
    MakeCase(GL_MAX_TEXTURE_IMAGE_UNITS, TEXTURE_SLOT_COUNT);
    }
#undef MakeCase
}
void cubyz_glGetInternalformati64v(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint64 * params) {}
void cubyz_glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint * params) {}
void cubyz_glGetMultisamplefv(GLenum pname, GLuint index, GLfloat * val) {}
void cubyz_glGetNamedBufferParameteri64v(GLuint buffer, GLenum pname, GLint64 * params) {}
void cubyz_glGetNamedBufferParameteriv(GLuint buffer, GLenum pname, GLint * params) {}
void cubyz_glGetNamedBufferPointerv(GLuint buffer, GLenum pname, void ** params) {}
void cubyz_glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, void * data) {}
void cubyz_glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint * params) {}
void cubyz_glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname, GLint * param) {}
void cubyz_glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname, GLint * params) {}
void cubyz_glGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei * length, GLchar * label) {}
void cubyz_glGetObjectPtrLabel(const void * ptr, GLsizei bufSize, GLsizei * length, GLchar * label) {}
void cubyz_glGetPointerv(GLenum pname, void ** params) {}
void cubyz_glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei * length, GLenum * binaryFormat, void * binary) {}
void cubyz_glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog) {
    context *C = &GlobalContext;
    object *Object = 0;
    if(HandledCheckProgramGet(C, program, &Object)) {
        return;
    }

    if(bufSize < 0) {
        const char *Msg = "glGetProgramInfoLog: An INVALID_VALUE error is generated if bufSize is negative.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    u64 Length = 0;
    const char *Log = 0;
    GlslangProgramGetLog(Object->Program.GlslangProgram, &Length, &Log);

    u64 InfoLogCopyLength = bufSize < Length ? bufSize : Length;
    memcpy(infoLog, Log, InfoLogCopyLength);
    *length = InfoLogCopyLength;
}
void cubyz_glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint * params) {}
void cubyz_glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei * length, GLchar * infoLog) {}
void cubyz_glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint * params) {}
GLuint cubyz_glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar * name) {return 1;}
GLint cubyz_glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar * name) {return 1;}
GLint cubyz_glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface, const GLchar * name) {return 1;}
void cubyz_glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei * length, GLchar * name) {}
void cubyz_glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum * props, GLsizei count, GLsizei * length, GLint * params) {}
void cubyz_glGetProgramStageiv(GLuint program, GLenum shadertype, GLenum pname, GLint * values) {}
void cubyz_glGetProgramiv(GLuint program, GLenum pname, GLint * params) {
    context *C = &GlobalContext;
    object *Object = 0;
    if(HandledCheckProgramGet(C, program, &Object)) {
        return;
    }

    switch(pname) {
    case GL_DELETE_STATUS: {
        *params = Object->Program.ShouldDelete;
    } break;
    case GL_LINK_STATUS: {
        *params = Object->Program.LinkStatus;
    } break;
    case GL_INFO_LOG_LENGTH: {
        u64 Length = 0;
        const char *Log = 0;
        GlslangProgramGetLog(Object->Program.GlslangProgram, &Length, &Log);
        *params = Length;
    } break;
    case GL_ATTACHED_SHADERS: {
        *params = Object->Program.AttachedShaderCount;
    } break;
    // TODO(blackedout): Implement
    default: {
        const char *Msg = "glGetProgramiv: An INVALID_ENUM error is generated if pname is not one of the values listed above.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
    }
    }
}
void cubyz_glGetQueryBufferObjecti64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {}
void cubyz_glGetQueryBufferObjectiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {}
void cubyz_glGetQueryBufferObjectui64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {}
void cubyz_glGetQueryBufferObjectuiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {}
void cubyz_glGetQueryIndexediv(GLenum target, GLuint index, GLenum pname, GLint * params) {}
void cubyz_glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 * params) {}
void cubyz_glGetQueryObjectiv(GLuint id, GLenum pname, GLint * params) {}
void cubyz_glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 * params) {}
void cubyz_glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint * params) {}
void cubyz_glGetQueryiv(GLenum target, GLenum pname, GLint * params) {}
void cubyz_glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint * params) {}
void cubyz_glGetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint * params) {}
void cubyz_glGetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint * params) {}
void cubyz_glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat * params) {}
void cubyz_glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint * params) {}
void cubyz_glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog) {
    context *C = &GlobalContext;
    object *Object = 0;
    if(HandledCheckShaderGet(C, shader, &Object)) {
        return;
    }

    if(bufSize < 0) {
        const char *Msg = "glGetShaderInfoLog: An INVALID_VALUE error is generated if bufSize is negative.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    u64 Length = 0;
    const char *Log = 0;
    GlslangShaderGetLog(Object->Shader.GlslangShader, &Length, &Log);

    u64 InfoLogCopyLength = bufSize < Length ? bufSize : Length;
    memcpy(infoLog, Log, InfoLogCopyLength);
    *length = InfoLogCopyLength;
}
void cubyz_glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint * range, GLint * precision) {}
void cubyz_glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * source) {}
void cubyz_glGetShaderiv(GLuint shader, GLenum pname, GLint * params) {
    context *C = &GlobalContext;
    object *Object = 0;
    if(HandledCheckShaderGet(C, shader, &Object)) {
        return;
    }

    if(params == 0) {
        return;
    }

    switch(pname) {
    case GL_SHADER_TYPE: {
        *params = Object->Shader.Type;
    } break;
    case GL_DELETE_STATUS: {
        *params = Object->Shader.ShouldDelete;
    } break;
    case GL_COMPILE_STATUS: {
        *params = Object->Shader.CompileStatus;
    } break;
    case GL_INFO_LOG_LENGTH: {
        u64 Length = 0;
        const char *Log = 0;
        GlslangShaderGetLog(Object->Shader.GlslangShader, &Length, &Log);
        *params = Length;
    } break;
    case GL_SHADER_SOURCE_LENGTH: {
        *params = Object->Shader.SourceByteCount;
    } break;
    case GL_SPIR_V_BINARY: {
        // TODO(blackedout):
        *params = GL_FALSE;
    } break;
    default: {
        const char *Msg = "glGetShaderiv: An INVALID_ENUM error is generated if pname is not SHADER_TYPE, DELETE_STATUS, COMPILE_STATUS, INFO_LOG_LENGTH, SHADER_SOURCE_LENGTH, or SPIR_V_BINARY.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
    }
    }
}
const GLubyte * cubyz_glGetString(GLenum name) { return 0; }
const GLubyte * cubyz_glGetStringi(GLenum name, GLuint index) { return 0; }
GLuint cubyz_glGetSubroutineIndex(GLuint program, GLenum shadertype, const GLchar * name) {return 1;}
GLint cubyz_glGetSubroutineUniformLocation(GLuint program, GLenum shadertype, const GLchar * name) {return 1;}
void cubyz_glGetSynciv(GLsync sync, GLenum pname, GLsizei count, GLsizei * length, GLint * values) {}
void cubyz_glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void * pixels) {}
void cubyz_glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat * params) {}
void cubyz_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint * params) {}
void cubyz_glGetTexParameterIiv(GLenum target, GLenum pname, GLint * params) {}
void cubyz_glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint * params) {}
void cubyz_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat * params) {}
void cubyz_glGetTexParameteriv(GLenum target, GLenum pname, GLint * params) {}
void cubyz_glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void * pixels) {}
void cubyz_glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname, GLfloat * params) {}
void cubyz_glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname, GLint * params) {}
void cubyz_glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint * params) {}
void cubyz_glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint * params) {}
void cubyz_glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat * params) {}
void cubyz_glGetTextureParameteriv(GLuint texture, GLenum pname, GLint * params) {}
void cubyz_glGetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void * pixels) {}
void cubyz_glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLsizei * size, GLenum * type, GLchar * name) {}
void cubyz_glGetTransformFeedbacki64_v(GLuint xfb, GLenum pname, GLuint index, GLint64 * param) {}
void cubyz_glGetTransformFeedbacki_v(GLuint xfb, GLenum pname, GLuint index, GLint * param) {}
void cubyz_glGetTransformFeedbackiv(GLuint xfb, GLenum pname, GLint * param) {}
GLuint cubyz_glGetUniformBlockIndex(GLuint program, const GLchar * uniformBlockName) {return 1;}
void cubyz_glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const* uniformNames, GLuint * uniformIndices) {}
GLint cubyz_glGetUniformLocation(GLuint program, const GLchar * name) {
    context *C = &GlobalContext;
    object *Object = 0;
    if(HandledCheckProgramGet(C, program, &Object)) {
        return -1;
    }

    GLint Result = -1;
    GlslangProgramGetLocation(Object->Program.GlslangProgram, name, &Result);
    return Result;
}
void cubyz_glGetUniformSubroutineuiv(GLenum shadertype, GLint location, GLuint * params) {}
void cubyz_glGetUniformdv(GLuint program, GLint location, GLdouble * params) {}
void cubyz_glGetUniformfv(GLuint program, GLint location, GLfloat * params) {}
void cubyz_glGetUniformiv(GLuint program, GLint location, GLint * params) {}
void cubyz_glGetUniformuiv(GLuint program, GLint location, GLuint * params) {}
void cubyz_glGetVertexArrayIndexed64iv(GLuint vaobj, GLuint index, GLenum pname, GLint64 * param) {}
void cubyz_glGetVertexArrayIndexediv(GLuint vaobj, GLuint index, GLenum pname, GLint * param) {}
void cubyz_glGetVertexArrayiv(GLuint vaobj, GLenum pname, GLint * param) {}
void cubyz_glGetVertexAttribIiv(GLuint index, GLenum pname, GLint * params) {}
void cubyz_glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint * params) {}
void cubyz_glGetVertexAttribLdv(GLuint index, GLenum pname, GLdouble * params) {}
void cubyz_glGetVertexAttribPointerv(GLuint index, GLenum pname, void ** pointer) {}
void cubyz_glGetVertexAttribdv(GLuint index, GLenum pname, GLdouble * params) {}
void cubyz_glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat * params) {}
void cubyz_glGetVertexAttribiv(GLuint index, GLenum pname, GLint * params) {}
void cubyz_glGetnCompressedTexImage(GLenum target, GLint lod, GLsizei bufSize, void * pixels) {}
void cubyz_glGetnTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void * pixels) {}
void cubyz_glGetnUniformdv(GLuint program, GLint location, GLsizei bufSize, GLdouble * params) {}
void cubyz_glGetnUniformfv(GLuint program, GLint location, GLsizei bufSize, GLfloat * params) {}
void cubyz_glGetnUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint * params) {}
void cubyz_glGetnUniformuiv(GLuint program, GLint location, GLsizei bufSize, GLuint * params) {}
void cubyz_glHint(GLenum target, GLenum mode) {}
void cubyz_glInvalidateBufferData(GLuint buffer) {}
void cubyz_glInvalidateBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr length) {}
void cubyz_glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments) {}
void cubyz_glInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum * attachments) {}
void cubyz_glInvalidateNamedFramebufferSubData(GLuint framebuffer, GLsizei numAttachments, const GLenum * attachments, GLint x, GLint y, GLsizei width, GLsizei height) {}
void cubyz_glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments, GLint x, GLint y, GLsizei width, GLsizei height) {}
void cubyz_glInvalidateTexImage(GLuint texture, GLint level) {}
void cubyz_glInvalidateTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth) {}
GLboolean cubyz_glIsBuffer(GLuint buffer) {return GL_TRUE;}
GLboolean cubyz_glIsEnabled(GLenum cap) {return GL_TRUE;}
GLboolean cubyz_glIsEnabledi(GLenum target, GLuint index) {return GL_TRUE;}
GLboolean cubyz_glIsFramebuffer(GLuint framebuffer) {return GL_TRUE;}
GLboolean cubyz_glIsProgram(GLuint program) {return GL_TRUE;}
GLboolean cubyz_glIsProgramPipeline(GLuint pipeline) {return GL_TRUE;}
GLboolean cubyz_glIsQuery(GLuint id) {return GL_TRUE;}
GLboolean cubyz_glIsRenderbuffer(GLuint renderbuffer) {return GL_TRUE;}
GLboolean cubyz_glIsSampler(GLuint sampler) {return GL_TRUE;}
GLboolean cubyz_glIsShader(GLuint shader) {return GL_TRUE;}
GLboolean cubyz_glIsSync(GLsync sync) {return GL_TRUE;}
GLboolean cubyz_glIsTexture(GLuint texture) {return GL_TRUE;}
GLboolean cubyz_glIsTransformFeedback(GLuint id) {return GL_TRUE;}
GLboolean cubyz_glIsVertexArray(GLuint array) {return GL_TRUE;}
void cubyz_glLineWidth(GLfloat width) {
    context *C = &GlobalContext;
    if(width <= (GLfloat)0.0) {
        const char *Msg = "glLineWidth: An INVALID_VALUE error is generated if width is less than or equal to zero.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    C->Config.RasterState.lineWidth = (float)width;
}
#include "stdio.h"
void cubyz_glLinkProgram(GLuint program) {
    context *C = &GlobalContext;
    object *Object = 0;
    if(HandledCheckProgramGet(C, program, &Object)) {
        return;
    }
    
    glslang_program GlslangProgram = Object->Program.GlslangProgram;
    for(u32 I = 0; I < Object->Program.AttachedShaderCount; ++I) {
        GlslangProgramAddShader(GlslangProgram, Object->Program.AttachedShaders[I]->Shader.GlslangShader);
    }
    
    if(GlslangProgramLink(GlslangProgram)) {
        Object->Program.LinkStatus = GL_FALSE;
        return;
    }

    Object->Program.LinkStatus = GL_TRUE;

    for(u32 I = 0; I < Object->Program.AttachedShaderCount; ++I) {
        object *ObjectS = Object->Program.AttachedShaders[I];
        // TODO(blackedout): Error handling
        unsigned char *SpirvBytes = 0;
        u64 SpirvByteCount = 0;
        GlslangGetSpirv(GlslangProgram, ObjectS->Shader.GlslangShader, &SpirvBytes, &SpirvByteCount);

        VkShaderModuleCreateInfo ModuleCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .codeSize = SpirvByteCount,
            .pCode = (uint32_t *)SpirvBytes,
        };

        VulkanCheckReturn(vkCreateShaderModule(C->Device, &ModuleCreateInfo, 0, &ObjectS->Shader.VulkanModule));

        // TODO(blackedout): Fix leak
        free(SpirvBytes);
    }
}
void cubyz_glLogicOp(GLenum opcode) {}
void * cubyz_glMapBuffer(GLenum target, GLenum access) {return 0;}
void * cubyz_glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {return 0;}
void * cubyz_glMapNamedBuffer(GLuint buffer, GLenum access) {return 0;}
void * cubyz_glMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access) {return 0;}
void cubyz_glMemoryBarrier(GLbitfield barriers) {}
void cubyz_glMemoryBarrierByRegion(GLbitfield barriers) {}
void cubyz_glMinSampleShading(GLfloat value) {}
void cubyz_glMultiDrawArrays(GLenum mode, const GLint * first, const GLsizei * count, GLsizei drawcount) {}
void cubyz_glMultiDrawArraysIndirect(GLenum mode, const void * indirect, GLsizei drawcount, GLsizei stride) {}
void cubyz_glMultiDrawArraysIndirectCount(GLenum mode, const void * indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {}
void cubyz_glMultiDrawElements(GLenum mode, const GLsizei * count, GLenum type, const void *const* indices, GLsizei drawcount) {}
void cubyz_glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei * count, GLenum type, const void *const* indices, GLsizei drawcount, const GLint * basevertex) {}
void cubyz_glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect, GLsizei drawcount, GLsizei stride) {}
void cubyz_glMultiDrawElementsIndirectCount(GLenum mode, GLenum type, const void * indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {}
void cubyz_glNamedBufferData(GLuint buffer, GLsizeiptr size, const void * data, GLenum usage) {}
void cubyz_glNamedBufferStorage(GLuint buffer, GLsizeiptr size, const void * data, GLbitfield flags) {}
void cubyz_glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const void * data) {}
void cubyz_glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf) {}
void cubyz_glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum * bufs) {}
void cubyz_glNamedFramebufferParameteri(GLuint framebuffer, GLenum pname, GLint param) {}
void cubyz_glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum src) {}
void cubyz_glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {}
void cubyz_glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) {}
void cubyz_glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) {}
void cubyz_glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) {}
void cubyz_glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {}
void cubyz_glObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar * label) {}
void cubyz_glObjectPtrLabel(const void * ptr, GLsizei length, const GLchar * label) {}
void cubyz_glPatchParameterfv(GLenum pname, const GLfloat * values) {}
void cubyz_glPatchParameteri(GLenum pname, GLint value) {}
void cubyz_glPauseTransformFeedback(void) {}
void cubyz_glPixelStoref(GLenum pname, GLfloat param) {}
void cubyz_glPixelStorei(GLenum pname, GLint param) {}
void cubyz_glPointParameterf(GLenum pname, GLfloat param) {}
void cubyz_glPointParameterfv(GLenum pname, const GLfloat * params) {}
void cubyz_glPointParameteri(GLenum pname, GLint param) {}
void cubyz_glPointParameteriv(GLenum pname, const GLint * params) {}
void cubyz_glPointSize(GLfloat size) {}
void cubyz_glPolygonMode(GLenum face, GLenum mode) {
    context *C = &GlobalContext;
    if(face != GL_FRONT_AND_BACK) {
        const char *Msg = "glPolygonMode: Separate polygon draw mode - PolygonMode face values of FRONT and BACK; polygons are always drawn in the same mode, no matter which face is being rasterized.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
    }

#define MakeCase(K, V) case (K): C->Config.RasterState.polygonMode = (V); break
    switch(mode) {
    MakeCase(GL_POINT, VK_POLYGON_MODE_POINT);
    MakeCase(GL_LINE, VK_POLYGON_MODE_LINE);
    MakeCase(GL_FILL, VK_POLYGON_MODE_FILL);
    default: {
        const char *Msg = "glPolygonMode: invalid mode";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
    } break;
    }
#undef MakeCase
}
void cubyz_glPolygonOffset(GLfloat factor, GLfloat units) {
    context *C = &GlobalContext;
    C->Config.RasterState.depthBiasConstantFactor = units;
    C->Config.RasterState.depthBiasSlopeFactor = factor;
}
void cubyz_glPolygonOffsetClamp(GLfloat factor, GLfloat units, GLfloat clamp) {}
void cubyz_glPopDebugGroup(void) {}
void cubyz_glPrimitiveRestartIndex(GLuint index) {}
void cubyz_glProgramBinary(GLuint program, GLenum binaryFormat, const void * binary, GLsizei length) {}
void cubyz_glProgramParameteri(GLuint program, GLenum pname, GLint value) {}
void cubyz_glProgramUniform1d(GLuint program, GLint location, GLdouble v0) {}
void cubyz_glProgramUniform1dv(GLuint program, GLint location, GLsizei count, const GLdouble * value) {}
void cubyz_glProgramUniform1f(GLuint program, GLint location, GLfloat v0) {}
void cubyz_glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {}
void cubyz_glProgramUniform1i(GLuint program, GLint location, GLint v0) {}
void cubyz_glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint * value) {}
void cubyz_glProgramUniform1ui(GLuint program, GLint location, GLuint v0) {}
void cubyz_glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {}
void cubyz_glProgramUniform2d(GLuint program, GLint location, GLdouble v0, GLdouble v1) {}
void cubyz_glProgramUniform2dv(GLuint program, GLint location, GLsizei count, const GLdouble * value) {}
void cubyz_glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1) {}
void cubyz_glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {}
void cubyz_glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1) {}
void cubyz_glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint * value) {}
void cubyz_glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1) {}
void cubyz_glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {}
void cubyz_glProgramUniform3d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2) {}
void cubyz_glProgramUniform3dv(GLuint program, GLint location, GLsizei count, const GLdouble * value) {}
void cubyz_glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {}
void cubyz_glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {}
void cubyz_glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) {}
void cubyz_glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint * value) {}
void cubyz_glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) {}
void cubyz_glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {}
void cubyz_glProgramUniform4d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3) {}
void cubyz_glProgramUniform4dv(GLuint program, GLint location, GLsizei count, const GLdouble * value) {}
void cubyz_glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {}
void cubyz_glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {}
void cubyz_glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {}
void cubyz_glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint * value) {}
void cubyz_glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {}
void cubyz_glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {}
void cubyz_glProgramUniformMatrix2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glProgramUniformMatrix2x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glProgramUniformMatrix2x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glProgramUniformMatrix3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glProgramUniformMatrix3x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glProgramUniformMatrix3x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glProgramUniformMatrix4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glProgramUniformMatrix4x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glProgramUniformMatrix4x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glProvokingVertex(GLenum mode) {}
void cubyz_glPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar * message) {}
void cubyz_glQueryCounter(GLuint id, GLenum target) {}
void cubyz_glReadBuffer(GLenum src) {}
void cubyz_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void * pixels) {}
void cubyz_glReadnPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void * data) {}
void cubyz_glReleaseShaderCompiler(void) {}
void cubyz_glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {}
void cubyz_glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {}
void cubyz_glResumeTransformFeedback(void) {}
void cubyz_glSampleCoverage(GLfloat value, GLboolean invert) {}
void cubyz_glSampleMaski(GLuint maskNumber, GLbitfield mask) {}
void cubyz_glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint * param) {}
void cubyz_glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint * param) {}
void cubyz_glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {}
void cubyz_glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat * param) {}
void cubyz_glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) {}
void cubyz_glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint * param) {}
void cubyz_glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    context *C = &GlobalContext;
    VkRect2D Scissor = {
        .offset.x = (float)x,
        .offset.y = (float)y,
        .extent.width = (float)width,
        .extent.height = (float)height,
    };
    C->Config.Scissor = Scissor;
}
void cubyz_glScissorArrayv(GLuint first, GLsizei count, const GLint * v) {}
void cubyz_glScissorIndexed(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height) {}
void cubyz_glScissorIndexedv(GLuint index, const GLint * v) {}
void cubyz_glShaderBinary(GLsizei count, const GLuint * shaders, GLenum binaryFormat, const void * binary, GLsizei length) {}
void cubyz_glShaderSource(GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length) {
    context *C = &GlobalContext;

    object *Object = 0;
    if(HandledCheckShaderGet(C, shader, &Object)) {
        return;
    }

    if(count < 0) {
        const char *Msg = "glShaderSource: An INVALID_VALUE error is generated if count is negative.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    u64 TotalByteCount = 0;
    for(GLsizei I = 0; I < count; ++I) {
        TotalByteCount += (u64)length[I];
    }

    char *Bytes = malloc(TotalByteCount);
    if(Bytes == 0) {
        GenerateErrorMsg(C, GL_OUT_OF_MEMORY, GL_DEBUG_SOURCE_API, "glShaderSource");
        return;
    }

    Object->Shader.SourceBytes = Bytes;
    Object->Shader.SourceByteCount = TotalByteCount;
    for(GLsizei I = 0; I < count; ++I) {
        memcpy(Bytes, string[I], length[I]);
        Bytes += length[I];
    }
}
void cubyz_glShaderStorageBlockBinding(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding) {}
void cubyz_glSpecializeShader(GLuint shader, const GLchar * pEntryPoint, GLuint numSpecializationConstants, const GLuint * pConstantIndex, const GLuint * pConstantValue) {}
void cubyz_glStencilFunc(GLenum func, GLint ref, GLuint mask) {}
void cubyz_glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {}
void cubyz_glStencilMask(GLuint mask) {}
void cubyz_glStencilMaskSeparate(GLenum face, GLuint mask) {}
void cubyz_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {}
void cubyz_glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {}
void cubyz_glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer) {}
void cubyz_glTexBufferRange(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {}
void cubyz_glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void * pixels) {}
void cubyz_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels) {
    context *C = &GlobalContext;
    if(level < 0) {
        const char *Msg = "An INVALID_VALUE error is generated if level is negative.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    if(width < 0 || height < 0) {
        const char *Msg = "An INVALID_VALUE error is generated if width, height, or depth (if each argument is present) is negative.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    if(border != 0) {
        const char *Msg = "An INVALID_VALUE error is generated if border is not zero.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    // TODO(blackedout):

    GLenum TextureTarget = 0;
    switch(target) {
    case GL_TEXTURE_2D:
    case GL_TEXTURE_1D_ARRAY:
    case GL_TEXTURE_RECTANGLE:
        
        break;
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        break;
    case GL_PROXY_TEXTURE_2D:
    case GL_PROXY_TEXTURE_1D_ARRAY:
    case GL_PROXY_TEXTURE_RECTANGLE:
    case GL_PROXY_TEXTURE_CUBE_MAP: {
        // NOTE(blackedout): Not supported
    } return;
    default: {
        const char *Msg = "An INVALID_ENUM error is generated if target is not one of the valid targets listed for each TexImage*D command.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
    } return;
    }
    
    // GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA, GL_RED_INTEGER, GL_RG_INTEGER, GL_RGB_INTEGER, GL_BGR_INTEGER, GL_RGBA_INTEGER, GL_BGRA_INTEGER, GL_STENCIL_INDEX, GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL
    // GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_INT, GL_INT, GL_HALF_FLOAT, GL_FLOAT, GL_UNSIGNED_BYTE_3_3_2, GL_UNSIGNED_BYTE_2_3_3_REV, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_5_6_5_REV, GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_UNSIGNED_SHORT_5_5_5_1, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_UNSIGNED_INT_8_8_8_8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_UNSIGNED_INT_10_10_10_2, and GL_UNSIGNED_INT_2_10_10_10_REV
}
void cubyz_glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {}
void cubyz_glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * pixels) {}
void cubyz_glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {}
void cubyz_glTexParameterIiv(GLenum target, GLenum pname, const GLint * params) {}
void cubyz_glTexParameterIuiv(GLenum target, GLenum pname, const GLuint * params) {}
void cubyz_glTexParameterf(GLenum target, GLenum pname, GLfloat param) {}
void cubyz_glTexParameterfv(GLenum target, GLenum pname, const GLfloat * params) {}
void cubyz_glTexParameteri(GLenum target, GLenum pname, GLint param) {}
void cubyz_glTexParameteriv(GLenum target, GLenum pname, const GLint * params) {}
void cubyz_glTexStorage1D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) {}
void cubyz_glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {}
void cubyz_glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {}
void cubyz_glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {}
void cubyz_glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {}
void cubyz_glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void * pixels) {}
void cubyz_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels) {}
void cubyz_glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels) {}
void cubyz_glTextureBarrier(void) {}
void cubyz_glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer) {}
void cubyz_glTextureBufferRange(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {}
void cubyz_glTextureParameterIiv(GLuint texture, GLenum pname, const GLint * params) {}
void cubyz_glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint * params) {}
void cubyz_glTextureParameterf(GLuint texture, GLenum pname, GLfloat param) {}
void cubyz_glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat * param) {}
void cubyz_glTextureParameteri(GLuint texture, GLenum pname, GLint param) {}
void cubyz_glTextureParameteriv(GLuint texture, GLenum pname, const GLint * param) {}
void cubyz_glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) {}
void cubyz_glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {}
void cubyz_glTextureStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {}
void cubyz_glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {}
void cubyz_glTextureStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {}
void cubyz_glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void * pixels) {}
void cubyz_glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels) {}
void cubyz_glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels) {}
void cubyz_glTextureView(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) {}
void cubyz_glTransformFeedbackBufferBase(GLuint xfb, GLuint index, GLuint buffer) {}
void cubyz_glTransformFeedbackBufferRange(GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {}
void cubyz_glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const* varyings, GLenum bufferMode) {}
void cubyz_glUniform1d(GLint location, GLdouble x) {}
void cubyz_glUniform1dv(GLint location, GLsizei count, const GLdouble * value) {}
void cubyz_glUniform1f(GLint location, GLfloat v0) {}
void cubyz_glUniform1fv(GLint location, GLsizei count, const GLfloat * value) {}
void cubyz_glUniform1i(GLint location, GLint v0) {}
void cubyz_glUniform1iv(GLint location, GLsizei count, const GLint * value) {}
void cubyz_glUniform1ui(GLint location, GLuint v0) {}
void cubyz_glUniform1uiv(GLint location, GLsizei count, const GLuint * value) {}
void cubyz_glUniform2d(GLint location, GLdouble x, GLdouble y) {}
void cubyz_glUniform2dv(GLint location, GLsizei count, const GLdouble * value) {}
void cubyz_glUniform2f(GLint location, GLfloat v0, GLfloat v1) {}
void cubyz_glUniform2fv(GLint location, GLsizei count, const GLfloat * value) {}
void cubyz_glUniform2i(GLint location, GLint v0, GLint v1) {}
void cubyz_glUniform2iv(GLint location, GLsizei count, const GLint * value) {}
void cubyz_glUniform2ui(GLint location, GLuint v0, GLuint v1) {}
void cubyz_glUniform2uiv(GLint location, GLsizei count, const GLuint * value) {}
void cubyz_glUniform3d(GLint location, GLdouble x, GLdouble y, GLdouble z) {}
void cubyz_glUniform3dv(GLint location, GLsizei count, const GLdouble * value) {}
void cubyz_glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {}
void cubyz_glUniform3fv(GLint location, GLsizei count, const GLfloat * value) {}
void cubyz_glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {}
void cubyz_glUniform3iv(GLint location, GLsizei count, const GLint * value) {}
void cubyz_glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) {}
void cubyz_glUniform3uiv(GLint location, GLsizei count, const GLuint * value) {}
void cubyz_glUniform4d(GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {}
void cubyz_glUniform4dv(GLint location, GLsizei count, const GLdouble * value) {}
void cubyz_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {}
void cubyz_glUniform4fv(GLint location, GLsizei count, const GLfloat * value) {}
void cubyz_glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {}
void cubyz_glUniform4iv(GLint location, GLsizei count, const GLint * value) {}
void cubyz_glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {}
void cubyz_glUniform4uiv(GLint location, GLsizei count, const GLuint * value) {}
void cubyz_glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {}
void cubyz_glUniformMatrix2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glUniformMatrix2x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glUniformMatrix2x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glUniformMatrix3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glUniformMatrix3x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glUniformMatrix3x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glUniformMatrix4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glUniformMatrix4x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glUniformMatrix4x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void cubyz_glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void cubyz_glUniformSubroutinesuiv(GLenum shadertype, GLsizei count, const GLuint * indices) {}
GLboolean cubyz_glUnmapBuffer(GLenum target) {return GL_TRUE;}
GLboolean cubyz_glUnmapNamedBuffer(GLuint buffer) {return GL_TRUE;}
void cubyz_glUseProgram(GLuint program) {
    context *C = &GlobalContext;
    if(program != 0) {
        object *Object = 0;
        if(HandledCheckProgramGet(C, program, &Object)) {
            return;
        }
    }
    if(C->ActiveProgram != program) {
        command Command = {
            .Type = command_BIND_PROGRAM,
            .BindProgram = {
                .Program = program
            },
        };
        PushCommand(C, Command);
        C->ActiveProgram = program;
    }
}
void cubyz_glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program) {}
void cubyz_glValidateProgram(GLuint program) {}
void cubyz_glValidateProgramPipeline(GLuint pipeline) {}
void cubyz_glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex, GLuint bindingindex) {}
void cubyz_glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {}
void cubyz_glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {}
void cubyz_glVertexArrayAttribLFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {}
void cubyz_glVertexArrayBindingDivisor(GLuint vaobj, GLuint bindingindex, GLuint divisor) {}
void cubyz_glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer) {}
void cubyz_glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {}
void cubyz_glVertexArrayVertexBuffers(GLuint vaobj, GLuint first, GLsizei count, const GLuint * buffers, const GLintptr * offsets, const GLsizei * strides) {}
void cubyz_glVertexAttrib1d(GLuint index, GLdouble x) {}
void cubyz_glVertexAttrib1dv(GLuint index, const GLdouble * v) {}
void cubyz_glVertexAttrib1f(GLuint index, GLfloat x) {}
void cubyz_glVertexAttrib1fv(GLuint index, const GLfloat * v) {}
void cubyz_glVertexAttrib1s(GLuint index, GLshort x) {}
void cubyz_glVertexAttrib1sv(GLuint index, const GLshort * v) {}
void cubyz_glVertexAttrib2d(GLuint index, GLdouble x, GLdouble y) {}
void cubyz_glVertexAttrib2dv(GLuint index, const GLdouble * v) {}
void cubyz_glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {}
void cubyz_glVertexAttrib2fv(GLuint index, const GLfloat * v) {}
void cubyz_glVertexAttrib2s(GLuint index, GLshort x, GLshort y) {}
void cubyz_glVertexAttrib2sv(GLuint index, const GLshort * v) {}
void cubyz_glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {}
void cubyz_glVertexAttrib3dv(GLuint index, const GLdouble * v) {}
void cubyz_glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {}
void cubyz_glVertexAttrib3fv(GLuint index, const GLfloat * v) {}
void cubyz_glVertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z) {}
void cubyz_glVertexAttrib3sv(GLuint index, const GLshort * v) {}
void cubyz_glVertexAttrib4Nbv(GLuint index, const GLbyte * v) {}
void cubyz_glVertexAttrib4Niv(GLuint index, const GLint * v) {}
void cubyz_glVertexAttrib4Nsv(GLuint index, const GLshort * v) {}
void cubyz_glVertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w) {}
void cubyz_glVertexAttrib4Nubv(GLuint index, const GLubyte * v) {}
void cubyz_glVertexAttrib4Nuiv(GLuint index, const GLuint * v) {}
void cubyz_glVertexAttrib4Nusv(GLuint index, const GLushort * v) {}
void cubyz_glVertexAttrib4bv(GLuint index, const GLbyte * v) {}
void cubyz_glVertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {}
void cubyz_glVertexAttrib4dv(GLuint index, const GLdouble * v) {}
void cubyz_glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {}
void cubyz_glVertexAttrib4fv(GLuint index, const GLfloat * v) {}
void cubyz_glVertexAttrib4iv(GLuint index, const GLint * v) {}
void cubyz_glVertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w) {}
void cubyz_glVertexAttrib4sv(GLuint index, const GLshort * v) {}
void cubyz_glVertexAttrib4ubv(GLuint index, const GLubyte * v) {}
void cubyz_glVertexAttrib4uiv(GLuint index, const GLuint * v) {}
void cubyz_glVertexAttrib4usv(GLuint index, const GLushort * v) {}
void cubyz_glVertexAttribBinding(GLuint attribindex, GLuint bindingindex) {}
void cubyz_glVertexAttribDivisor(GLuint index, GLuint divisor) {}
void cubyz_glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {}
void cubyz_glVertexAttribI1i(GLuint index, GLint x) {}
void cubyz_glVertexAttribI1iv(GLuint index, const GLint * v) {}
void cubyz_glVertexAttribI1ui(GLuint index, GLuint x) {}
void cubyz_glVertexAttribI1uiv(GLuint index, const GLuint * v) {}
void cubyz_glVertexAttribI2i(GLuint index, GLint x, GLint y) {}
void cubyz_glVertexAttribI2iv(GLuint index, const GLint * v) {}
void cubyz_glVertexAttribI2ui(GLuint index, GLuint x, GLuint y) {}
void cubyz_glVertexAttribI2uiv(GLuint index, const GLuint * v) {}
void cubyz_glVertexAttribI3i(GLuint index, GLint x, GLint y, GLint z) {}
void cubyz_glVertexAttribI3iv(GLuint index, const GLint * v) {}
void cubyz_glVertexAttribI3ui(GLuint index, GLuint x, GLuint y, GLuint z) {}
void cubyz_glVertexAttribI3uiv(GLuint index, const GLuint * v) {}
void cubyz_glVertexAttribI4bv(GLuint index, const GLbyte * v) {}
void cubyz_glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w) {}
void cubyz_glVertexAttribI4iv(GLuint index, const GLint * v) {}
void cubyz_glVertexAttribI4sv(GLuint index, const GLshort * v) {}
void cubyz_glVertexAttribI4ubv(GLuint index, const GLubyte * v) {}
void cubyz_glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) {}
void cubyz_glVertexAttribI4uiv(GLuint index, const GLuint * v) {}
void cubyz_glVertexAttribI4usv(GLuint index, const GLushort * v) {}
void cubyz_glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {}
void cubyz_glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer) {}
void cubyz_glVertexAttribL1d(GLuint index, GLdouble x) {}
void cubyz_glVertexAttribL1dv(GLuint index, const GLdouble * v) {}
void cubyz_glVertexAttribL2d(GLuint index, GLdouble x, GLdouble y) {}
void cubyz_glVertexAttribL2dv(GLuint index, const GLdouble * v) {}
void cubyz_glVertexAttribL3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {}
void cubyz_glVertexAttribL3dv(GLuint index, const GLdouble * v) {}
void cubyz_glVertexAttribL4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {}
void cubyz_glVertexAttribL4dv(GLuint index, const GLdouble * v) {}
void cubyz_glVertexAttribLFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {}
void cubyz_glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer) {}
void cubyz_glVertexAttribP1ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {}
void cubyz_glVertexAttribP1uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint * value) {}
void cubyz_glVertexAttribP2ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {}
void cubyz_glVertexAttribP2uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint * value) {}
void cubyz_glVertexAttribP3ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {}
void cubyz_glVertexAttribP3uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint * value) {}
void cubyz_glVertexAttribP4ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {}
void cubyz_glVertexAttribP4uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint * value) {}
void cubyz_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer) {
    context *C = &GlobalContext;
    object *Object;
    if(CheckObjectTypeGet(C, C->BoundVertexArray, object_VERTEX_ARRAY, &Object)) {
        const char *Msg = "An INVALID_OPERATION error is generated if no vertex array object is bound.";
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    if(index >= VERTEX_ATTRIB_CAPACITY) {
        const char *Msg = "An INVALID_VALUE error is generated if bindingindex is greater than or equal to the value of MAX_VERTEX_ATTRIB_BINDINGS.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    buffer_target_info ArrayBufferTargetInfo;
    Assert(0 == GetBufferTargetInfo(GL_ARRAY_BUFFER, &ArrayBufferTargetInfo));

    GLuint BoundArrayBuffer = C->BoundBuffers[ArrayBufferTargetInfo.Index];
    if(CheckObjectType(C, BoundArrayBuffer, object_BUFFER)) {
        const char *Msg = "Custom: calling glVertexAttribPointer without a buffer bound to GL_ARRAY_BUFFER is invalid.";
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    VkVertexInputAttributeDescription AttribDesc = {0};
    AttribDesc.location = index;
    AttribDesc.binding = 0;
    AttribDesc.offset = (u32)(uintptr_t)pointer;

    u32 AttribByteCount = 0;

#define MakeSizeCase(Count, Format) case (Count): AttribDesc.format = (Format); break
#define MakeDefaultCase default: AttribDesc.format = VK_FORMAT_UNDEFINED; break

#define MakeCase(GlType, BitCount, VulkanType)\
    case GlType:\
        switch(size) {\
        MakeSizeCase(1, VK_FORMAT_R ## BitCount ## _ ## VulkanType);\
        MakeSizeCase(2, VK_FORMAT_R ## BitCount ## G ## BitCount ## _ ## VulkanType);\
        MakeSizeCase(3, VK_FORMAT_R ## BitCount ## G ## BitCount ## B ## BitCount ## _ ## VulkanType);\
        MakeSizeCase(4, VK_FORMAT_R ## BitCount ## G ## BitCount ## B ## BitCount ## A ## BitCount ## _ ## VulkanType);\
        MakeDefaultCase;\
        }\
        AttribByteCount = size*BitCount/8;\
        break
        
    switch(type) {
    MakeCase(GL_BYTE, 8, SINT);
    MakeCase(GL_UNSIGNED_BYTE, 8, UINT);
    MakeCase(GL_SHORT, 16, SINT);
    MakeCase(GL_UNSIGNED_SHORT, 16, UINT);
    MakeCase(GL_INT, 32, SINT);
    MakeCase(GL_UNSIGNED_INT, 32, UINT);
    MakeCase(GL_HALF_FLOAT, 16, SFLOAT);
    MakeCase(GL_FLOAT, 32, SFLOAT);
    MakeDefaultCase;
    }
#undef MakeCasae

#undef SizeDefaultCase
#undef MakeSizeCase

    if(AttribDesc.format == VK_FORMAT_UNDEFINED) {
        const char *Msg = "Custom: glVertexAttribPointer size and type combination invalid.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    VkVertexInputBindingDescription BindingDesc = {0};
    BindingDesc.stride = stride ? stride : AttribByteCount;
    BindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    u8 ZeroBlock[sizeof(VkVertexInputBindingDescription)] = {0};
    if(memcmp(&Object->VertexArray.Binding, ZeroBlock, sizeof(ZeroBlock)) != 0 && memcmp(&Object->VertexArray.Binding, &BindingDesc, sizeof(BindingDesc)) != 0) {
        // NOTE(blackedout): Previous binding has been set and is not equal to the binding of this call
        const char *Msg = "Custom: glVertexAttribPointer bindings don't match";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    Object->VertexArray.Binding = BindingDesc;
    Object->VertexArray.Attribs[index].Buffer = BoundArrayBuffer;
    Object->VertexArray.Attribs[index].Desc = AttribDesc;
}
void cubyz_glVertexBindingDivisor(GLuint bindingindex, GLuint divisor) {}
void cubyz_glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    context *C = &GlobalContext;
    VkViewport Viewport = C->Config.Viewport;
    Viewport.x = x;
    Viewport.y = y;
    Viewport.width = width;
    Viewport.height = height;
    C->Config.Viewport = Viewport;
}
void cubyz_glViewportArrayv(GLuint first, GLsizei count, const GLfloat * v) {}
void cubyz_glViewportIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h) {}
void cubyz_glViewportIndexedfv(GLuint index, const GLfloat * v) {}
void cubyz_glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {}