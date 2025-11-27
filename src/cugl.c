#include "cugl/cugl.h"
#include "internal.h"
#include "shader_interface.h"
#include "vulkan/vulkan_core.h"

#include <stdio.h>

void cuglSwapBuffers(void) {
    const char *Name = "cuglSwapBuffers";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);
    
    for(u32 I = 1; I < C->PipelineStates.Count; ++I) {
        CheckPipeline(C, I);
    }

    uint32_t AcquiredImageIndex = 0;
    VulkanCheckReturn(vkAcquireNextImageKHR(C->Device, C->Swapchain, UINT64_MAX, C->Semaphores[semaphore_PREV_PRESENT_DONE], VK_NULL_HANDLE, &AcquiredImageIndex));

    VkCommandBufferBeginInfo BeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = 0,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = 0,
    };
    VulkanCheckReturn(vkBeginCommandBuffer(C->CommandBuffers[command_buffer_GRAPHICS], &BeginInfo));

    VkCommandBuffer GraphicsCommandBuffer = C->CommandBuffers[command_buffer_GRAPHICS];
    object *ObjectF = 0;
    GLuint Fbo = 0;
    u32 PipelineIndex = 0;
    for(u32 I = 0; I < C->Commands.Count; ++I) {
        command *Command = ArrayData(command, C->Commands) + I;
        switch(Command->Type) {
        case command_BIND_PIPELINE: {
            PipelineIndex = Command->BindPipeline.PipelineIndex;
            pipeline_state_header *Header = GetPipelineState(C, Command->BindPipeline.PipelineIndex, pipeline_state_HEADER);
            Header->LastBoundSwapCounter = C->SwapCounter;
            vkCmdBindPipeline(GraphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Header->Pipeline);
        } break;
        case command_BIND_UNIFORMS: {
            pipeline_state_header *Header = GetPipelineState(C, PipelineIndex, pipeline_state_HEADER);
            pipeline_state_program *State = GetPipelineState(C, PipelineIndex, pipeline_state_PROGRAM);
            object *ObjectP = 0;
            Assert(0 == CheckObjectTypeGet(C, State->Program, object_PROGRAM, &ObjectP));

            uint32_t Offset = ObjectP->Program.AlignedUniformByteCount*Command->BindUniforms.Index;
            vkCmdBindDescriptorSets(GraphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Header->Layout, 0, 1, &ObjectP->Program.UniformDescriptorSet, 1, &Offset);
        } break;
        case command_CLEAR: {
            if(Fbo == 0) {
                // TOOD(blackedout): Depth, stencil
                pipeline_state_clear_color *State = GetPipelineState(C, PipelineIndex, pipeline_state_CLEAR_COLOR);
                VkClearAttachment ClearAttachment = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .colorAttachment = 0,
                    .clearValue.color.float32 = { State->R, State->G, State->B, State->A },
                };
                // TODO(blackedout): Clip with scissor
                VkClearRect ClearRect = {
                    .rect = {
                        .offset = { .x = 0, .y = 0 },
                        .extent = ObjectF->Framebuffer.Extent,
                    },
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                };
                vkCmdClearAttachments(GraphicsCommandBuffer, 1, &ClearAttachment, 1, &ClearRect);
            } else {
                // TODO(blackedout): Look up draw buffer in subpass state
            }
        } break;
        case command_BIND_VERTEX_BUFFER: {
            vkCmdBindVertexBuffers(GraphicsCommandBuffer, Command->BindVertexBuffer.BindingIndex, 1, &Command->BindVertexBuffer.Buffer, &Command->BindVertexBuffer.Offset);
        } break;
        case command_DRAW: {
            vkCmdDraw(GraphicsCommandBuffer, Command->Draw.VertexCount, Command->Draw.InstanceCount, Command->Draw.VertexOffset, Command->Draw.InstanceOffset);
        } break;
        case command_BEGIN_RENDER_PASS: {
            ObjectF = 0;
            Fbo = Command->BeginRenderPass.Fbo;
            Assert(0 == CheckObjectTypeGet(C, Command->BeginRenderPass.Fbo, object_FRAMEBUFFER, &ObjectF));
            
            VkFramebuffer Framebuffer = VK_NULL_HANDLE;
            if(Command->BeginRenderPass.Fbo == 0) {
                Framebuffer = C->SwapchainFramebuffers[AcquiredImageIndex];
            } else {
                Framebuffer = ObjectF->Framebuffer.Framebuffer;
            }
            VkClearValue ClearValue = {
                .color = {
                    .float32[0] = 0.0f,
                    .float32[1] = 0.5f,
                    .float32[2] = 0.0f,
                    .float32[3] = 0.0f
                }
            };
            VkRenderPassBeginInfo RenderPassBeginInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext = 0,
                .renderPass = ObjectF->Framebuffer.RenderPass,
                .framebuffer = Framebuffer,
                .renderArea = {
                    .offset.x = 0,
                    .offset.y = 0,
                    .extent.width = ObjectF->Framebuffer.Extent.width,
                    .extent.height = ObjectF->Framebuffer.Extent.height,
                },
                .clearValueCount = 1,
                .pClearValues = &ClearValue,
            };
            vkCmdBeginRenderPass(GraphicsCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        } break;
        default: {

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
        .pImageIndices = &AcquiredImageIndex,
        .pResults = 0,
    };
    VulkanCheckReturn(vkQueuePresentKHR(SurfaceQueue, &PresentInfo));

    VulkanCheckReturn(vkQueueWaitIdle(SurfaceQueue));

    VulkanCheckReturn(vkResetCommandBuffer(C->CommandBuffers[command_buffer_GRAPHICS], 0));

    for(u32 I = 1; I < C->Objects.Capacity; ++I) {
        object *Object = 0;
        if(0 == CheckObjectTypeGet(C, I, object_PROGRAM, &Object)) {
            u8 *LastUniformData = ArrayData(u8, Object->Program.UniformBuffer) + (Object->Program.UniformBuffer.Count - 1)*Object->Program.AlignedUniformByteCount;
            memcpy(Object->Program.UniformBuffer.Data, LastUniformData, Object->Program.AlignedUniformByteCount);
            Object->Program.UniformBuffer.Count = 1;
            Object->Program.LatestUsedUniformsIndex = 0;
            Object->Program.LatestUniformsUsed = 0;
        }
    }

    // NOTE(blackedout): Erase pipeline states whose lifetime exceeds the maximum
    u32 DeleteCount = 0;
    for(u32 I = 1; I < C->PipelineStates.Count; ++I) {
        pipeline_state_header *Header = GetPipelineState(C, I, pipeline_state_HEADER);
        u64 UnusedSwapCount = C->SwapCounter - Header->LastBoundSwapCounter;
        if(UnusedSwapCount > PIPELINE_UNUSED_SWAP_COUNTER_DELETE) {
            vkDestroyPipelineLayout(C->Device, Header->Layout, 0);
            vkDestroyPipeline(C->Device, Header->Pipeline, 0);
            printf("Deleted pipeline %d", (int)I);
            ++DeleteCount;
        } else if(DeleteCount) {
            void *SrcData = GetPipelineState(C, I, pipeline_state_HEADER);
            void *DstData = GetPipelineState(C, I - DeleteCount, pipeline_state_HEADER);
            memcpy(DstData, SrcData, C->PipelineStateByteCount);
        }
    }
    C->PipelineStates.Count -= DeleteCount;

    ArrayClear(&C->Commands, sizeof(command));
    C->LastPipelineIndex = 0;
    C->IsPipelineSet = 0;
    ++C->SwapCounter;
}

int cuglCreateContext(const context_create_params *Params) {
    const char *Name = "cuglCreateContext";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT, 1);

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
    VulkanCheckGoto(vkGetSwapchainImagesKHR(C->Device, C->Swapchain, &C->SwapchainImageCount, 0), label_Error);
    C->SwapchainImages = calloc(C->SwapchainImageCount, sizeof(VkImage));
    C->SwapchainImageViews = calloc(C->SwapchainImageCount, sizeof(VkImageView));
    C->SwapchainFramebuffers = calloc(C->SwapchainImageCount, sizeof(VkFramebuffer));
    VulkanCheckGoto(vkGetSwapchainImagesKHR(C->Device, C->Swapchain, &C->SwapchainImageCount, C->SwapchainImages), label_Error);

    for(u32 I = 0; I < C->SwapchainImageCount; ++I) {
        VkImageViewCreateInfo ImageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .image = C->SwapchainImages[I],
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

        VulkanCheckGoto(vkCreateImageView(C->Device, &ImageViewCreateInfo, 0, C->SwapchainImageViews + I), label_Error);
        ++CreatedImageViewCount;
    }

    {
        GLuint DefaultFbo = 0;
        object *Object = 0;
        ArrayRequireRoom(&C->Objects, 2, sizeof(object), INITIAL_OBJECT_CAPACITY);
        GenObject(C, &DefaultFbo, &Object);
        Assert(DefaultFbo == 0);
        Object->Type = object_FRAMEBUFFER;
        CreateObject(C, Object);

        GLuint DummyRbo = 0;
        object *ObjectR = 0;
        GenObject(C, &DummyRbo, &ObjectR);
        ObjectR->Type = object_RENDERBUFFER;
        ObjectR->Renderbuffer.Image = VK_NULL_HANDLE;
        ObjectR->Renderbuffer.ImageView = VK_NULL_HANDLE;
        ObjectR->Renderbuffer.Extent = C->DeviceInfo.SurfaceCapabilities.currentExtent;

        Object->Framebuffer.ColorAttachments[0].IsDrawBuffer = 1;
        Object->Framebuffer.ColorAttachments[0].Rbo = DummyRbo;
        Object->Framebuffer.ColorAttachmentRange = 1;
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

    C->Config = GetDefaultConfig(C->DeviceInfo.SurfaceCapabilities.currentExtent.width, C->DeviceInfo.SurfaceCapabilities.currentExtent.height);

    // TODO error check
    CreatePipelineStates(C, &C->DeviceInfo.Properties.limits);
    

    Result = 0;
    goto label_Exit;

label_Error:
    for(u32 I = 0; I < CreatedSemaphoreCount; ++I) {
        vkDestroySemaphore(C->Device, C->Semaphores[I], 0);
    }
    for(u32 I = 0; I < CreatedImageViewCount; ++I) {
        vkDestroyImageView(C->Device, C->SwapchainImageViews[I], 0);
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

void glActiveShaderProgram(GLuint pipeline, GLuint program) {}
void glActiveTexture(GLenum texture) {
    const char *Name = "glActiveTexture";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);
    if(texture < GL_TEXTURE0 || GL_TEXTURE0 + TEXTURE_SLOT_COUNT <= texture) {
        const char *Msg = "An INVALID_ENUM error is generated if an invalid texture is specified. texture is a symbolic constant of the form TEXTUREi, indicating that texture unit iis to be modified. Each TEXTUREiadheres to TEXTUREi=TEXTURE0 + i, where i is in the range zero to k-1, and kis the value of MAX_COMBINED_TEXTURE_IMAGE_UNITS.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    C->ActiveTextureIndex = texture - GL_TEXTURE0;
}
void glAttachShader(GLuint program, GLuint shader) {
    const char *Name = "glAttachShader";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *ObjectP = 0, *ObjectS = 0;
    if(HandledCheckProgramGet(C, program, &ObjectP, Name) || HandledCheckShaderGet(C, shader, &ObjectS, Name)) {
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
void glBeginConditionalRender(GLuint id, GLenum mode) {}
void glBeginQuery(GLenum target, GLuint id) {}
void glBeginQueryIndexed(GLenum target, GLuint index, GLuint id) {}
void glBeginTransformFeedback(GLenum primitiveMode) {}
void glBindAttribLocation(GLuint program, GLuint index, const GLchar * name) {}
void glBindBuffer(GLenum target, GLuint buffer) {
    const char *Name = "glBindBuffer";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    buffer_target_info TargetInfo = {0};
    if(GetBufferTargetInfo(target, &TargetInfo)) {
        const char *Msg = "An INVALID_ENUM error is generated if target is not one of the targets listed in table 6.1.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
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
void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {}
void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {}
void glBindBuffersBase(GLenum target, GLuint first, GLsizei count, const GLuint * buffers) {}
void glBindBuffersRange(GLenum target, GLuint first, GLsizei count, const GLuint * buffers, const GLintptr * offsets, const GLsizeiptr * sizes) {}
void glBindFragDataLocation(GLuint program, GLuint color, const GLchar * name) {}
void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const GLchar * name) {}
void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    const char *Name = "glBindFramebuffer";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    if(framebuffer != 0 && CheckObjectType(C, framebuffer, object_FRAMEBUFFER)) {
        const char *Msg = "An INVALID_OPERATION error is generated if framebuffer is not zero or a name returned from a previous call to GenFramebuffers, or if such a name has since been deleted with DeleteFramebuffers.";
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    pipeline_state_framebuffer *State = GetCurrentPipelineState(C, pipeline_state_FRAMEBUFFER);
    int WasNone = 1;
    if(target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
        C->BoundReadFbo = framebuffer;
        State->ReadFbo = framebuffer;
        WasNone = 0;
    }
    if(target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
        C->BoundDrawFbo = framebuffer;
        State->DrawFbo = framebuffer;
        WasNone = 0;
    }
    if(WasNone) {
        const char *Msg = "An INVALID_ENUM error is generated if target is not DRAW_FRAMEBUFFER, READ_FRAMEBUFFER, or FRAMEBUFFER.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
    }
}
void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {}
void glBindImageTextures(GLuint first, GLsizei count, const GLuint * textures) {}
void glBindProgramPipeline(GLuint pipeline) {}
void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {}
void glBindSampler(GLuint unit, GLuint sampler) {}
void glBindSamplers(GLuint first, GLsizei count, const GLuint * samplers) {}
void glBindTexture(GLenum target, GLuint texture) {
    const char *Name = "glBindTexture";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);
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
void glBindTextureUnit(GLuint unit, GLuint texture) {}
void glBindTextures(GLuint first, GLsizei count, const GLuint * textures) {}
void glBindTransformFeedback(GLenum target, GLuint id) {}
void glBindVertexArray(GLuint array) {
    const char *Name = "glBindVertexArray";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(array != 0 && CheckObjectTypeGet(C, array, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ARRAY_INVALID);

    if(C->BoundVao == array) {
        return;
    }
    
    C->BoundVao = array;

    if(array == 0) {
        ClearCurrentPipelineState(C, pipeline_state_VERTEX_INPUT_ATTRIBUTES);
        ClearCurrentPipelineState(C, pipeline_state_VERTEX_INPUT_BINDINGS);
        return;
    }

    pipeline_state_info AttributeInfo = C->PipelineStateInfos[pipeline_state_VERTEX_INPUT_ATTRIBUTES];
    pipeline_state_info BindingInfo = C->PipelineStateInfos[pipeline_state_VERTEX_INPUT_ATTRIBUTES];
    if(Object->VertexArray.InputAttributes == 0) {
        // NOTE(blackedout): Specs say to create the state here, not in glGen
        Object->VertexArray.InputAttributes = calloc(AttributeInfo.InstanceCount, sizeof(*Object->VertexArray.InputAttributes));
        Object->VertexArray.InputBindings = calloc(BindingInfo.InstanceCount, sizeof(*Object->VertexArray.InputBindings));
        // TOOD(blackedout): Error handling
    }

    CurrentPipelineStateFromPtr(C, Object->VertexArray.InputAttributes, pipeline_state_VERTEX_INPUT_ATTRIBUTES);
    CurrentPipelineStateFromPtr(C, Object->VertexArray.InputBindings, pipeline_state_VERTEX_INPUT_BINDINGS);
}
void glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    const char *Name = "glBindVertexBuffer";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object), gl_error_BIND_VERTEX_BUFFER_NONE_BOUND);
    SetVertexInputBinding(C, Object, 1, bindingindex, buffer, offset, stride, Name);
}
void glBindVertexBuffers(GLuint first, GLsizei count, const GLuint * buffers, const GLintptr * offsets, const GLsizei * strides) {
    const char *Name = "glBindVertexBuffers";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    // TODO(blackedout):
    Assert(0);
}
void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    const char *Name = "glBlendColor";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);
    C->Config.BlendState.blendConstants[0] = Clamp01(red);
    C->Config.BlendState.blendConstants[1] = Clamp01(green);
    C->Config.BlendState.blendConstants[2] = Clamp01(blue);
    C->Config.BlendState.blendConstants[3] = Clamp01(alpha);
}
void glBlendEquation(GLenum mode) {
    const char *Name = "glBlendEquation";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    VkBlendOp VulkanBlendOp;
    if(GetVulkanBlendOp(mode, &VulkanBlendOp)) {
        const char *Msg = "An INVALID_ENUM error is generated if any of mode, modeRGB, or mode Alpha are not one of the blend equation modes in table 17.1.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    C->Config.BlendAttachmentState.colorBlendOp = VulkanBlendOp;
    C->Config.BlendAttachmentState.alphaBlendOp = VulkanBlendOp;
}
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    const char *Name = "glBlendEquationSeparate";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    VkBlendOp BlendOpColor, BlendOpAlpha;
    if(GetVulkanBlendOp(modeRGB, &BlendOpColor) || GetVulkanBlendOp(modeAlpha, &BlendOpAlpha)) {
        const char *Msg = "An INVALID_ENUM error is generated if any of mode, modeRGB, or mode Alpha are not one of the blend equation modes in table 17.1.";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    C->Config.BlendAttachmentState.colorBlendOp = BlendOpColor;
    C->Config.BlendAttachmentState.alphaBlendOp = BlendOpAlpha;
}
void glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {}
void glBlendEquationi(GLuint buf, GLenum mode) {}
void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    const char *Name = "glBlendFunc";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

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
void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    const char *Name = "glBlendFuncSeparate";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

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
void glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {}
void glBlendFunci(GLuint buf, GLenum src, GLenum dst) {}
void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}
void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {}
void glBufferData(GLenum target, GLsizeiptr size, const void * data, GLenum usage) {
    const char *Name = "glBufferData";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

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
void glBufferStorage(GLenum target, GLsizeiptr size, const void * data, GLbitfield flags) {}
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void * data) {}
GLenum glCheckFramebufferStatus(GLenum target) {return GL_FRAMEBUFFER_COMPLETE;}
GLenum glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) {return GL_FRAMEBUFFER_COMPLETE;}
void glClampColor(GLenum target, GLenum clamp) {}
void glClear(GLbitfield mask) {
    const char *Name = "glClear";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

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
    u32 SubpassIndex = 0;
    CheckGL(PotentiallySaveSubpass(C, &SubpassIndex), gl_error_OUT_OF_MEMORY);
    // TODO(blackedout): Make pipeline have a is fixed bit for each state type, such that a draw which doesn't use a program
    // can leave the program state as unset, such that a following draw command is able to edit the program state
    pipeline_state_type Types[] = {
        pipeline_state_CLEAR_COLOR,
        pipeline_state_VIEWPORT,
        pipeline_state_SCISSOR,
        pipeline_state_FRAMEBUFFER,
        pipeline_state_DRAW_BUFFERS,
    };
    CheckGL(UseCurrentPipelineState(C, ArrayCount(Types), Types), gl_error_OUT_OF_MEMORY);

    command Command = {0};
    Command.Type = command_CLEAR;
    Command.Clear.SubpassIndex = SubpassIndex;
    PushCommand(C, Command);
}
void glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void * data) {}
void glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void * data) {}
void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {}
void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat * value) {}
void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint * value) {}
void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint * value) {}
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    const char *Name = "glClearColor";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);
    
    pipeline_state_clear_color ClearColor = {
        .R = red, .G = green, .B = blue, .A = alpha
    };
    pipeline_state_clear_color *State = GetCurrentPipelineState(C, pipeline_state_CLEAR_COLOR);
    *State = ClearColor;
}
void glClearDepth(GLdouble depth) {
    const char *Name = "glClearDepth";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    pipeline_state_clear_depth *State = GetCurrentPipelineState(C, pipeline_state_CLEAR_DEPTH);
    *State = (GLfloat)depth;
}
void glClearDepthf(GLfloat d) {
    const char *Name = "glClearDepthf";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    pipeline_state_clear_depth *State = GetCurrentPipelineState(C, pipeline_state_CLEAR_DEPTH);
    *State = d;
}
void glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void * data) {}
void glClearNamedBufferSubData(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void * data) {}
void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {}
void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat * value) {}
void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint * value) {}
void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint * value) {}
void glClearStencil(GLint s) {
    const char *Name = "glClearStencil";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    pipeline_state_clear_stencil *State = GetCurrentPipelineState(C, pipeline_state_CLEAR_STENCIL);
    *State = s;
}
void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void * data) {}
void glClearTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * data) {}
GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {return GL_CONDITION_SATISFIED;}
void glClipControl(GLenum origin, GLenum depth) {}
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {}
void glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a) {}
void glCompileShader(GLuint shader) {
    const char *Name = "glCompileShader";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);
    object *Object = 0;
    if(HandledCheckShaderGet(C, shader, &Object, Name)) {
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
void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void * data) {}
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void * data) {}
void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void * data) {}
void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void * data) {}
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void * data) {}
void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void * data) {}
void glCompressedTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void * data) {}
void glCompressedTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void * data) {}
void glCompressedTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void * data) {}
void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {}
void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {}
void glCopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {}
void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) {}
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {}
void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {}
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {}
void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {}
void glCopyTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {}
void glCopyTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {}
void glCopyTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {}
void glCreateBuffers(GLsizei n, GLuint * buffers) {
    NoContextCreateObjects(n, object_BUFFER, buffers, "glCreateBuffers");
}
void glCreateFramebuffers(GLsizei n, GLuint * framebuffers) {
    NoContextCreateObjects(n, object_FRAMEBUFFER, framebuffers, "glCreateFramebuffers");
}
GLuint glCreateProgram(void) {
    GLuint Result = 0;
    NoContextCreateObjects(1, object_PROGRAM, &Result, "glCreateProgram");
    return Result;
}
void glCreateProgramPipelines(GLsizei n, GLuint * pipelines) {
    NoContextCreateObjects(n, object_PROGRAM_PIPELINE, pipelines, "glCreateProgramPipelines");
}
void glCreateQueries(GLenum target, GLsizei n, GLuint * ids) {
    NoContextCreateObjects(n, object_QUERY, ids, "glCreateQueries");
}
void glCreateRenderbuffers(GLsizei n, GLuint * renderbuffers) {
    NoContextCreateObjects(n, object_RENDERBUFFER, renderbuffers, "glCreateRenderbuffers");
}
void glCreateSamplers(GLsizei n, GLuint * samplers) {
    NoContextCreateObjects(n, object_SAMPLER, samplers, "glCreateSamplers");
}
GLuint glCreateShader(GLenum type) {
    const char *Name = "glCreateShader";
    GLuint Result = 0;
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT, Result);
    
    shader_type_info TypeInfo = {0};
    CheckGL(GetShaderTypeInfo(type, &TypeInfo), gl_error_SHADER_TYPE, Result);

    ArrayRequireRoom(&C->Objects, 1, sizeof(object), INITIAL_OBJECT_CAPACITY);
    object *Object = 0;
    GenObject(C, &Result, &Object);
    Object->Type = object_SHADER;
    Object->Shader.Type = type;
    CreateObject(C, Object);

    ReleaseContext(C, Name);
    return Result;
}
GLuint glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const* strings) {return 1;}
void glCreateTextures(GLenum target, GLsizei n, GLuint * textures) {
    NoContextCreateObjects(n, object_TEXTURE, textures, "glCreateTextures");
}
void glCreateTransformFeedbacks(GLsizei n, GLuint * ids) {
    NoContextCreateObjects(n, object_TRANSFORM_FEEDBACK, ids, "glCreateTransformFeedbacks");
}
void glCreateVertexArrays(GLsizei n, GLuint * arrays) {
    NoContextCreateObjects(n, object_VERTEX_ARRAY, arrays, "glCreateVertexArrays");
}
void glCullFace(GLenum mode) {
    const char *Name = "glCullFace";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

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
void glDebugMessageCallback(GLDEBUGPROC callback, const void * userParam) {
    const char *Name = "glDebugMessageCallback";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    C->DebugCallback = callback;
    C->DebugCallbackUser = userParam;
}
void glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint * ids, GLboolean enabled) {}
void glDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * buf) {}
void glDeleteBuffers(GLsizei n, const GLuint * buffers) {
    const char *Name = "glDeleteBuffers";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);
    DeleteObjectsSizei(C, n, buffers, object_BUFFER);
}
void glDeleteFramebuffers(GLsizei n, const GLuint * framebuffers) {
    const char *Name = "glDeleteFramebuffers";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);
    DeleteObjectsSizei(C, n, framebuffers, object_FRAMEBUFFER);
}
void glDeleteProgram(GLuint program) {
    const char *Name = "glDeleteProgram";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);
    
    if(program == 0) {
        return;
    }

    object *Object = 0;
    if(HandledCheckProgramGet(C, program, &Object, Name)) {
        return;
    }

    // TODO(blackedout): When to delete exactly

    Object->Program.ShouldDelete = GL_TRUE;
    //DeleteObjects(C, 1, &program, object_PROGRAM);
}
void glDeleteProgramPipelines(GLsizei n, const GLuint * pipelines) {}
void glDeleteQueries(GLsizei n, const GLuint * ids) {
    const char *Name = "glDeleteQueries";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    DeleteObjectsSizei(C, n, ids, object_QUERY);
}
void glDeleteRenderbuffers(GLsizei n, const GLuint * renderbuffers) {
    const char *Name = "glDeleteRenderbuffers";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    DeleteObjectsSizei(C, n, renderbuffers, object_RENDERBUFFER);
}
void glDeleteSamplers(GLsizei count, const GLuint * samplers) {}
void glDeleteShader(GLuint shader) {
    const char *Name = "glDeleteShader";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);
    
    if(shader == 0) {
        return;
    }

    object *Object = 0;
    if(HandledCheckShaderGet(C, shader, &Object, Name)) {
        return;
    }

    if(Object->Shader.Program) {
        Object->Shader.ShouldDelete = GL_TRUE;
    } else {
        DeleteShader(C, Object);
        DeleteObjects(C, 1, &shader, object_SHADER);
    }
}
void glDeleteSync(GLsync sync) {
    const char *Name = "glDeleteSync";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);
    // TODO(blackedout): This pointer cast is potentially dangerous
    DeleteObjects(C, 1, (GLuint *)&sync, object_SYNC);
}
void glDeleteTextures(GLsizei n, const GLuint * textures) {
    const char *Name = "glDeleteTextures";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    DeleteObjectsSizei(C, n, textures, object_TEXTURE);
}
void glDeleteTransformFeedbacks(GLsizei n, const GLuint * ids) {}
void glDeleteVertexArrays(GLsizei n, const GLuint * arrays) {
    const char *Name = "glDeleteVertexArrays";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);
    
    CheckGL(n < 0, gl_error_N_NEGATIVE);
    // TODO(blackedout): Silently ignore unused handles in arrays
    DeleteObjectsSizei(C, n, arrays, object_VERTEX_ARRAY);
    // TODO(blackedout): Revert binding to zero if is bound
}
void glDepthFunc(GLenum func) {
    const char *Name = "glDepthFunc";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

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
void glDepthMask(GLboolean flag) {}
void glDepthRange(GLdouble n, GLdouble f) {
    const char *Name = "glDepthRange";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    pipeline_state_viewport *State = GetCurrentPipelineState(C, pipeline_state_VIEWPORT);
    for(u32 I = 0; I < C->PipelineStateInfos[pipeline_state_VIEWPORT].InstanceCount; ++I) {
        State[I].minDepth = Clamp01((float)n);
        State[I].maxDepth = Clamp01((float)f);
    }
}
void glDepthRangeArrayv(GLuint first, GLsizei count, const GLdouble * v) {
    const char *Name = "glDepthRangeArrayv";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    CheckGL(count < 0, gl_error_VIEWPORT_COUNT_NEGATIVE);
    CheckGL(first + count > C->PipelineStateInfos[pipeline_state_VIEWPORT].InstanceCount, gl_error_VIEWPORT_INDEX);

    pipeline_state_viewport *State = GetCurrentPipelineState(C, pipeline_state_VIEWPORT);
    for(u32 I = 0; I < count; ++I) {
        State[first + I].minDepth = Clamp01((float)v[2*I + 0]);
        State[first + I].maxDepth = Clamp01((float)v[2*I + 1]);
    }
}
void glDepthRangeIndexed(GLuint index, GLdouble n, GLdouble f) {
    const char *Name = "glDepthRangeIndexed";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    CheckGL(index >= C->PipelineStateInfos[pipeline_state_VIEWPORT].InstanceCount, gl_error_VIEWPORT_INDEX);

    pipeline_state_viewport *State = GetCurrentPipelineState(C, pipeline_state_VIEWPORT);
    State[index].minDepth = Clamp01((float)n);
    State[index].maxDepth = Clamp01((float)f);
}
void glDepthRangef(GLfloat n, GLfloat f) {
    const char *Name = "glDepthRangef";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    pipeline_state_viewport *State = GetCurrentPipelineState(C, pipeline_state_VIEWPORT);
    for(u32 I = 0; I < C->PipelineStateInfos[pipeline_state_VIEWPORT].InstanceCount; ++I) {
        State[I].minDepth = Clamp01((float)n);
        State[I].maxDepth = Clamp01((float)f);
    }
}
void glDetachShader(GLuint program, GLuint shader) {}
void glDisable(GLenum cap) {
    const char *Name = "glDisable";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    HandledCheckCapSet(C, cap, 0);
}
void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    const char *Name = "glDisableVertexArrayAttrib";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object), gl_error_XABLE_VERTEX_ARRAY_ATTRIB_INVALID);
    SetVertexInputAttributeEnabled(C, Object, C->BoundVao == vaobj, index, 0, Name);
}
void glDisableVertexAttribArray(GLuint index) {
    const char *Name = "glDisableVertexAttribArray";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object), gl_error_XABLE_VERTEX_ATTRIB_ARRAY_NONE_BOUND);
    SetVertexInputAttributeEnabled(C, Object, 1, index, 0, Name);
}
void glDisablei(GLenum target, GLuint index) {}
void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {}
void glDispatchComputeIndirect(GLintptr indirect) {}
void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    const char *Name = "glDrawArrays";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    CheckGL(first < 0, gl_error_DRAW_FIRST_NEGATIVE);
    CheckGL(count < 0, gl_error_DRAW_COUNT_NEGATIVE);

    primitive_info PrimitiveInfo = {0};
    CheckGL(GetPrimitiveInfo(mode, &PrimitiveInfo), gl_error_DRAW_MODE);

    if(mode == GL_LINE_LOOP) {
        // TODO(blackedout): Handle special case
    }

    pipeline_state_primitive_type *State = GetCurrentPipelineState(C, pipeline_state_PRIMITIVE_TYPE);
    State->Type = mode;
    
    u32 SubpassIndex = 0;
    CheckGL(PotentiallySaveSubpass(C, &SubpassIndex), gl_error_OUT_OF_MEMORY);
    pipeline_state_type Types[] = {
        pipeline_state_VIEWPORT,
        pipeline_state_SCISSOR,
        pipeline_state_FRAMEBUFFER,
        pipeline_state_DRAW_BUFFERS,
        pipeline_state_VERTEX_INPUT_ATTRIBUTES,
        pipeline_state_VERTEX_INPUT_BINDINGS,
        pipeline_state_PROGRAM,
        pipeline_state_PRIMITIVE_TYPE,
    };
    CheckGL(UseCurrentPipelineState(C, ArrayCount(Types), Types), gl_error_OUT_OF_MEMORY);

    command Command = {
        .Type = command_DRAW,
        .Draw = {
            .VertexCount = count,
            .VertexOffset = first,
            .InstanceCount = 1,
            .InstanceOffset = 0,
        },
    };
    CheckGL(PushCommand(C, Command), gl_error_OUT_OF_MEMORY);
}
void glDrawArraysIndirect(GLenum mode, const void * indirect) {}
void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {}
void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) {}
void glDrawBuffer(GLenum buf) {
    const char *Name = "glDrawBuffer";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    // NOTE(blackedout): Assert here because if none is bound the default one is affected
    object *Object = 0;
    Assert(0 == CheckObjectTypeGet(C, C->BoundDrawFbo, object_FRAMEBUFFER, &Object));

    color_attachment_info Info = {0};
    CheckGL(GetColorAttachmentInfo(buf, Object->Framebuffer.ColorAttachmentCapacity, &Info), gl_error_DRAW_BUFFER_BUF_INVALID);

    if(C->BoundDrawFbo == 0) {
        CheckGL(Info.Flags & color_attachment_info_IS_INVALID_FOR_DEFAULT_FBO, gl_error_DRAW_BUFFER_BUF_INVALID_DEFAULT_FRAMEBUFFER);
        Assert(0);
    } else {
        CheckGL(Info.Flags & color_attachment_info_IS_INVALID_FOR_OTHER_FBO, gl_error_DRAW_BUFFER_BUF_INVALID_OTHER_FRAMEBUFFER);

        u32 I = 0;
        for(; I < Info.Index; ++I) {
            Object->Framebuffer.ColorAttachments[I].IsDrawBuffer = 0;
        }
        Object->Framebuffer.ColorAttachments[I++].IsDrawBuffer = 1;
        Object->Framebuffer.ColorAttachmentRange = I;
        for(; I < Object->Framebuffer.ColorAttachmentCapacity; ++I) {
            Object->Framebuffer.ColorAttachments[I].IsDrawBuffer = 0;
        }
    }

    ReleaseContext(C, Name);
}
void glDrawBuffers(GLsizei n, const GLenum * bufs) {
    const char *Name = "glDrawBuffers";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    // NOTE(blackedout): Assert here because if none is bound the default one is affected
    object *Object = 0;
    Assert(0 == CheckObjectTypeGet(C, C->BoundDrawFbo, object_FRAMEBUFFER, &Object));

    Assert(0);

    ReleaseContext(C, Name);
}
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void * indices) {}
void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void * indices, GLint basevertex) {}
void glDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect) {}
void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount) {}
void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount, GLuint baseinstance) {}
void glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount, GLint basevertex) {}
void glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) {}
void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void * indices) {}
void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void * indices, GLint basevertex) {}
void glDrawTransformFeedback(GLenum mode, GLuint id) {}
void glDrawTransformFeedbackInstanced(GLenum mode, GLuint id, GLsizei instancecount) {}
void glDrawTransformFeedbackStream(GLenum mode, GLuint id, GLuint stream) {}
void glDrawTransformFeedbackStreamInstanced(GLenum mode, GLuint id, GLuint stream, GLsizei instancecount) {}
void glEnable(GLenum cap) {
    const char *Name = "glEnable";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    HandledCheckCapSet(C, cap, 1);
}
void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    const char *Name = "glEnableVertexArrayAttrib";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object), gl_error_XABLE_VERTEX_ARRAY_ATTRIB_INVALID);
    SetVertexInputAttributeEnabled(C, Object, C->BoundVao == vaobj, index, 1, Name);
}
void glEnableVertexAttribArray(GLuint index) {
    const char *Name = "glEnableVertexAttribArray";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object), gl_error_XABLE_VERTEX_ATTRIB_ARRAY_NONE_BOUND);
    SetVertexInputAttributeEnabled(C, Object, 1, index, 1, Name);
}
void glEnablei(GLenum target, GLuint index) {}
void glEndConditionalRender(void) {}
void glEndQuery(GLenum target) {}
void glEndQueryIndexed(GLenum target, GLuint index) {}
void glEndTransformFeedback(void) {}
GLsync glFenceSync(GLenum condition, GLbitfield flags) {return (GLsync)1;}
void glFinish(void) {}
void glFlush(void) {}
void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {}
void glFlushMappedNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length) {}
void glFramebufferParameteri(GLenum target, GLenum pname, GLint param) {}
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {}
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) {}
void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {}
void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) {}
void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {}
void glFrontFace(GLenum mode) {
    const char *Name = "glFrontFace";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

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
void glGenBuffers(GLsizei n, GLuint * buffers) {
    NoContextGenObjects(n, object_BUFFER, buffers, "glGenBuffers");
}
void glGenFramebuffers(GLsizei n, GLuint * framebuffers) {
    NoContextGenObjects(n, object_FRAMEBUFFER, framebuffers, "glGenFramebuffers");
}
void glGenProgramPipelines(GLsizei n, GLuint * pipelines) {
    NoContextGenObjects(n, object_PROGRAM_PIPELINE, pipelines, "glGenProgramPipelines");
}
void glGenQueries(GLsizei n, GLuint * ids) {
    NoContextGenObjects(n, object_QUERY, ids, "glGenQueries");
}
void glGenRenderbuffers(GLsizei n, GLuint * renderbuffers) {
    NoContextGenObjects(n, object_RENDERBUFFER, renderbuffers, "glGenRenderbuffers");
}
void glGenSamplers(GLsizei count, GLuint * samplers) {
    NoContextGenObjects(count, object_SAMPLER, samplers, "glGenSamplers");
}
void glGenTextures(GLsizei n, GLuint * textures) {
    NoContextGenObjects(n, object_TEXTURE, textures, "glGenTextures");
}
void glGenTransformFeedbacks(GLsizei n, GLuint * ids) {
    NoContextGenObjects(n, object_TRANSFORM_FEEDBACK, ids, "glGenTransformFeedbacks");
}
void glGenVertexArrays(GLsizei n, GLuint * arrays) {
    NoContextGenObjects(n, object_VERTEX_ARRAY, arrays, "glGenVertexArrays");
}
void glGenerateMipmap(GLenum target) {}
void glGenerateTextureMipmap(GLuint texture) {}
void glGetActiveAtomicCounterBufferiv(GLuint program, GLuint bufferIndex, GLenum pname, GLint * params) {}
void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name) {}
void glGetActiveSubroutineName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei * length, GLchar * name) {}
void glGetActiveSubroutineUniformName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei * length, GLchar * name) {}
void glGetActiveSubroutineUniformiv(GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint * values) {}
void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name) {}
void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei * length, GLchar * uniformBlockName) {}
void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint * params) {}
void glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei * length, GLchar * uniformName) {}
void glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint * uniformIndices, GLenum pname, GLint * params) {}
void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei * count, GLuint * shaders) {}
GLint glGetAttribLocation(GLuint program, const GLchar * name) {return 1;}
void glGetBooleani_v(GLenum target, GLuint index, GLboolean * data) {}
void glGetBooleanv(GLenum pname, GLboolean * data) {}
void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 * params) {}
void glGetBufferParameteriv(GLenum target, GLenum pname, GLint * params) {}
void glGetBufferPointerv(GLenum target, GLenum pname, void ** params) {}
void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void * data) {}
void glGetCompressedTexImage(GLenum target, GLint level, void * img) {}
void glGetCompressedTextureImage(GLuint texture, GLint level, GLsizei bufSize, void * pixels) {}
void glGetCompressedTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void * pixels) {}
GLuint glGetDebugMessageLog(GLuint count, GLsizei bufSize, GLenum * sources, GLenum * types, GLuint * ids, GLenum * severities, GLsizei * lengths, GLchar * messageLog) {return 1;}
void glGetDoublei_v(GLenum target, GLuint index, GLdouble * data) {}
void glGetDoublev(GLenum pname, GLdouble * data) {}
GLenum glGetError(void) {
    const char *Name = "glGetError";
    context *C = 0;
    // TODO(blackedout): What to do here?
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT, GL_INVALID_OPERATION);

    GLenum Result = C->ErrorFlag;
    C->ErrorFlag = GL_NO_ERROR;
    return Result;
}
void glGetFloati_v(GLenum target, GLuint index, GLfloat * data) {}
void glGetFloatv(GLenum pname, GLfloat * data) {}
GLint glGetFragDataIndex(GLuint program, const GLchar * name) {return 1;}
GLint glGetFragDataLocation(GLuint program, const GLchar * name) {return 1;}
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint * params) {}
void glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint * params) {}
GLenum glGetGraphicsResetStatus(void) {return GL_NO_ERROR;}
void glGetInteger64i_v(GLenum target, GLuint index, GLint64 * data) {}
void glGetInteger64v(GLenum pname, GLint64 * data) {}
void glGetIntegeri_v(GLenum target, GLuint index, GLint * data) {}
void glGetIntegerv(GLenum pname, GLint * data) {
#define MakeCase(K, V) case (K): *data = (V); break
    switch(pname) {
    MakeCase(GL_MAX_TEXTURE_IMAGE_UNITS, TEXTURE_SLOT_COUNT);
    }
#undef MakeCase
}
void glGetInternalformati64v(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint64 * params) {}
void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint * params) {}
void glGetMultisamplefv(GLenum pname, GLuint index, GLfloat * val) {}
void glGetNamedBufferParameteri64v(GLuint buffer, GLenum pname, GLint64 * params) {}
void glGetNamedBufferParameteriv(GLuint buffer, GLenum pname, GLint * params) {}
void glGetNamedBufferPointerv(GLuint buffer, GLenum pname, void ** params) {}
void glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, void * data) {}
void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint * params) {}
void glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname, GLint * param) {}
void glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname, GLint * params) {}
void glGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei * length, GLchar * label) {}
void glGetObjectPtrLabel(const void * ptr, GLsizei bufSize, GLsizei * length, GLchar * label) {}
void glGetPointerv(GLenum pname, void ** params) {}
void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei * length, GLenum * binaryFormat, void * binary) {}
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog) {
    const char *Name = "glGetProgramInfoLog";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    if(HandledCheckProgramGet(C, program, &Object, Name)) {
        return;
    }

    if(bufSize < 0) {
        const char *Msg = "glGetProgramInfoLog: An INVALID_VALUE error is generated if bufSize is negative.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    u64 Length = 0;
    const char *Log = 0;
    GlslangProgramGetLog(&Object->Program.GlslangProgram, &Length, &Log);

    u64 InfoLogCopyLength = bufSize < Length ? bufSize : Length;
    memcpy(infoLog, Log, InfoLogCopyLength);
    *length = InfoLogCopyLength;
}
void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint * params) {}
void glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei * length, GLchar * infoLog) {}
void glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint * params) {}
GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar * name) {return 1;}
GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar * name) {return 1;}
GLint glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface, const GLchar * name) {return 1;}
void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei * length, GLchar * name) {}
void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum * props, GLsizei count, GLsizei * length, GLint * params) {}
void glGetProgramStageiv(GLuint program, GLenum shadertype, GLenum pname, GLint * values) {}
void glGetProgramiv(GLuint program, GLenum pname, GLint * params) {
    const char *Name = "glGetProgramiv";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    if(HandledCheckProgramGet(C, program, &Object, Name)) {
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
        GlslangProgramGetLog(&Object->Program.GlslangProgram, &Length, &Log);
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
void glGetQueryBufferObjecti64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {}
void glGetQueryBufferObjectiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {}
void glGetQueryBufferObjectui64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {}
void glGetQueryBufferObjectuiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {}
void glGetQueryIndexediv(GLenum target, GLuint index, GLenum pname, GLint * params) {}
void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 * params) {}
void glGetQueryObjectiv(GLuint id, GLenum pname, GLint * params) {}
void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 * params) {}
void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint * params) {}
void glGetQueryiv(GLenum target, GLenum pname, GLint * params) {}
void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint * params) {}
void glGetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint * params) {}
void glGetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint * params) {}
void glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat * params) {}
void glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint * params) {}
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog) {
    const char *Name = "glGetShaderInfoLog";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    if(HandledCheckShaderGet(C, shader, &Object, Name)) {
        return;
    }

    if(bufSize < 0) {
        const char *Msg = "glGetShaderInfoLog: An INVALID_VALUE error is generated if bufSize is negative.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    u64 Length = 0;
    const char *Log = 0;
    GlslangShaderGetLog(&Object->Shader.GlslangShader, &Length, &Log);

    u64 InfoLogCopyLength = bufSize < Length ? bufSize : Length;
    memcpy(infoLog, Log, InfoLogCopyLength);
    *length = InfoLogCopyLength;
}
void glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint * range, GLint * precision) {}
void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * source) {}
void glGetShaderiv(GLuint shader, GLenum pname, GLint * params) {
    const char *Name = "glGetShaderiv";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    if(HandledCheckShaderGet(C, shader, &Object, Name)) {
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
        GlslangShaderGetLog(&Object->Shader.GlslangShader, &Length, &Log);
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
        return;
    }
    }
}
const GLubyte * glGetString(GLenum name) { return 0; }
const GLubyte * glGetStringi(GLenum name, GLuint index) { return 0; }
GLuint glGetSubroutineIndex(GLuint program, GLenum shadertype, const GLchar * name) {return 1;}
GLint glGetSubroutineUniformLocation(GLuint program, GLenum shadertype, const GLchar * name) {return 1;}
void glGetSynciv(GLsync sync, GLenum pname, GLsizei count, GLsizei * length, GLint * values) {}
void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void * pixels) {}
void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat * params) {}
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint * params) {}
void glGetTexParameterIiv(GLenum target, GLenum pname, GLint * params) {}
void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint * params) {}
void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat * params) {}
void glGetTexParameteriv(GLenum target, GLenum pname, GLint * params) {}
void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void * pixels) {}
void glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname, GLfloat * params) {}
void glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname, GLint * params) {}
void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint * params) {}
void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint * params) {}
void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat * params) {}
void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint * params) {}
void glGetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void * pixels) {}
void glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLsizei * size, GLenum * type, GLchar * name) {}
void glGetTransformFeedbacki64_v(GLuint xfb, GLenum pname, GLuint index, GLint64 * param) {}
void glGetTransformFeedbacki_v(GLuint xfb, GLenum pname, GLuint index, GLint * param) {}
void glGetTransformFeedbackiv(GLuint xfb, GLenum pname, GLint * param) {}
GLuint glGetUniformBlockIndex(GLuint program, const GLchar * uniformBlockName) {return 1;}
void glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const* uniformNames, GLuint * uniformIndices) {}
GLint glGetUniformLocation(GLuint program, const GLchar * name) {
    const char *Name = "glGetUniformLocation";
    GLint Result = -1;
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT, Result);

    object *Object = 0;
    if(HandledCheckProgramGet(C, program, &Object, Name)) {
        return Result;
    }

    ProgramGetUniformLocation(&Object->Program.GlslangProgram, name, &Result);
    return Result;
}
void glGetUniformSubroutineuiv(GLenum shadertype, GLint location, GLuint * params) {}
void glGetUniformdv(GLuint program, GLint location, GLdouble * params) {}
void glGetUniformfv(GLuint program, GLint location, GLfloat * params) {}
void glGetUniformiv(GLuint program, GLint location, GLint * params) {}
void glGetUniformuiv(GLuint program, GLint location, GLuint * params) {}
void glGetVertexArrayIndexed64iv(GLuint vaobj, GLuint index, GLenum pname, GLint64 * param) {}
void glGetVertexArrayIndexediv(GLuint vaobj, GLuint index, GLenum pname, GLint * param) {}
void glGetVertexArrayiv(GLuint vaobj, GLenum pname, GLint * param) {}
void glGetVertexAttribIiv(GLuint index, GLenum pname, GLint * params) {}
void glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint * params) {}
void glGetVertexAttribLdv(GLuint index, GLenum pname, GLdouble * params) {}
void glGetVertexAttribPointerv(GLuint index, GLenum pname, void ** pointer) {}
void glGetVertexAttribdv(GLuint index, GLenum pname, GLdouble * params) {}
void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat * params) {}
void glGetVertexAttribiv(GLuint index, GLenum pname, GLint * params) {}
void glGetnCompressedTexImage(GLenum target, GLint lod, GLsizei bufSize, void * pixels) {}
void glGetnTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void * pixels) {}
void glGetnUniformdv(GLuint program, GLint location, GLsizei bufSize, GLdouble * params) {}
void glGetnUniformfv(GLuint program, GLint location, GLsizei bufSize, GLfloat * params) {}
void glGetnUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint * params) {}
void glGetnUniformuiv(GLuint program, GLint location, GLsizei bufSize, GLuint * params) {}
void glHint(GLenum target, GLenum mode) {}
void glInvalidateBufferData(GLuint buffer) {}
void glInvalidateBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr length) {}
void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments) {}
void glInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum * attachments) {}
void glInvalidateNamedFramebufferSubData(GLuint framebuffer, GLsizei numAttachments, const GLenum * attachments, GLint x, GLint y, GLsizei width, GLsizei height) {}
void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments, GLint x, GLint y, GLsizei width, GLsizei height) {}
void glInvalidateTexImage(GLuint texture, GLint level) {}
void glInvalidateTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth) {}
GLboolean glIsBuffer(GLuint buffer) {
    return NoContextIsObjectType(buffer, object_BUFFER, "glIsBuffer");
}
GLboolean glIsEnabled(GLenum cap) {return GL_TRUE;}
GLboolean glIsEnabledi(GLenum target, GLuint index) {return GL_TRUE;}
GLboolean glIsFramebuffer(GLuint framebuffer) {
    return NoContextIsObjectType(framebuffer, object_FRAMEBUFFER, "glIsFramebuffer");
}
GLboolean glIsProgram(GLuint program) {
    return NoContextIsObjectType(program, object_PROGRAM, "glIsProgram");
}
GLboolean glIsProgramPipeline(GLuint pipeline) {
    return NoContextIsObjectType(pipeline, object_PROGRAM_PIPELINE, "glIsProgramPipeline");
}
GLboolean glIsQuery(GLuint id) {
    return NoContextIsObjectType(id, object_QUERY, "glIsQuery");
}
GLboolean glIsRenderbuffer(GLuint renderbuffer) {
    return NoContextIsObjectType(renderbuffer, object_RENDERBUFFER, "glIsRenderbuffer");
}
GLboolean glIsSampler(GLuint sampler) {
    return NoContextIsObjectType(sampler, object_SAMPLER, "glIsSampler");
}
GLboolean glIsShader(GLuint shader) {
    return NoContextIsObjectType(shader, object_SHADER, "glIsShader");
}
GLboolean glIsSync(GLsync sync) {
    // TODO(blackedout): Cast
    return NoContextIsObjectType((GLuint)sync, object_SYNC, "glIsSync");
}
GLboolean glIsTexture(GLuint texture) {
    return NoContextIsObjectType(texture, object_TEXTURE, "glIsTexture");
}
GLboolean glIsTransformFeedback(GLuint id) {
    return NoContextIsObjectType(id, object_TRANSFORM_FEEDBACK, "glIsTransformFeedback");
}
GLboolean glIsVertexArray(GLuint array) {
    return NoContextIsObjectType(array, object_VERTEX_ARRAY, "glIsVertexArray");
}
void glLineWidth(GLfloat width) {
    const char *Name = "glLineWidth";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    CheckGL(width <= (GLfloat)0.0, gl_error_LINE_WIDTH_LE_ZERO);

    C->Config.RasterState.lineWidth = (float)width;
}
void glLinkProgram(GLuint program) {
    const char *Name = "glLinkProgram";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    if(HandledCheckProgramGet(C, program, &Object, Name)) {
        return;
    }
    
    glslang_program *GlslangProgram = &Object->Program.GlslangProgram;
    for(u32 I = 0; I < Object->Program.AttachedShaderCount; ++I) {
        GlslangProgramAddShader(GlslangProgram, &Object->Program.AttachedShaders[I]->Shader.GlslangShader);
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
        GlslangGetSpirv(GlslangProgram, &ObjectS->Shader.GlslangShader, &SpirvBytes, &SpirvByteCount);

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

    VkDescriptorSetLayoutBinding LayoutBindings[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
            .pImmutableSamplers = 0,
        }
    };

    VkDescriptorSetLayoutCreateInfo SetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .bindingCount = ArrayCount(LayoutBindings),
        .pBindings = LayoutBindings,
    };

    VulkanCheckReturn(vkCreateDescriptorSetLayout(C->Device, &SetLayoutCreateInfo, 0, &Object->Program.DescriptorSetLayout));

    VkDescriptorPoolSize DescriptorPoolSizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 1,
        }
    };
    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = ArrayCount(DescriptorPoolSizes),
        .pPoolSizes = DescriptorPoolSizes,
    };
    VulkanCheckReturn(vkCreateDescriptorPool(C->Device, &DescriptorPoolCreateInfo, 0, &Object->Program.DescriptorPool));

    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = 0,
        .descriptorPool = Object->Program.DescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &Object->Program.DescriptorSetLayout,
    };

    VulkanCheckReturn(vkAllocateDescriptorSets(C->Device, &DescriptorSetAllocateInfo, &Object->Program.UniformDescriptorSet));
    u64 MinAlignment = C->DeviceInfo.Properties.limits.minUniformBufferOffsetAlignment;
    Object->Program.AlignedUniformByteCount = MinAlignment*((GlslangProgram->UniformByteCount + MinAlignment - 1)/MinAlignment);
    CheckGL(ArrayRequireRoom(&Object->Program.UniformBuffer, 1, Object->Program.AlignedUniformByteCount, 1), gl_error_OUT_OF_MEMORY);
    Object->Program.UniformBuffer.Count = 1;
    memset(Object->Program.UniformBuffer.Data, 0, Object->Program.AlignedUniformByteCount);
    printf("raw %u, aligned %u, %f wasted\n", Object->Program.GlslangProgram.UniformByteCount, Object->Program.AlignedUniformByteCount, 1 - (Object->Program.GlslangProgram.UniformByteCount/(double)Object->Program.AlignedUniformByteCount));
}
void glLogicOp(GLenum opcode) {}
void * glMapBuffer(GLenum target, GLenum access) {return 0;}
void * glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {return 0;}
void * glMapNamedBuffer(GLuint buffer, GLenum access) {return 0;}
void * glMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access) {return 0;}
void glMemoryBarrier(GLbitfield barriers) {}
void glMemoryBarrierByRegion(GLbitfield barriers) {}
void glMinSampleShading(GLfloat value) {}
void glMultiDrawArrays(GLenum mode, const GLint * first, const GLsizei * count, GLsizei drawcount) {}
void glMultiDrawArraysIndirect(GLenum mode, const void * indirect, GLsizei drawcount, GLsizei stride) {}
void glMultiDrawArraysIndirectCount(GLenum mode, const void * indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {}
void glMultiDrawElements(GLenum mode, const GLsizei * count, GLenum type, const void *const* indices, GLsizei drawcount) {}
void glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei * count, GLenum type, const void *const* indices, GLsizei drawcount, const GLint * basevertex) {}
void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect, GLsizei drawcount, GLsizei stride) {}
void glMultiDrawElementsIndirectCount(GLenum mode, GLenum type, const void * indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {}
void glNamedBufferData(GLuint buffer, GLsizeiptr size, const void * data, GLenum usage) {}
void glNamedBufferStorage(GLuint buffer, GLsizeiptr size, const void * data, GLbitfield flags) {}
void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const void * data) {}
void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf) {}
void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum * bufs) {}
void glNamedFramebufferParameteri(GLuint framebuffer, GLenum pname, GLint param) {}
void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum src) {}
void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {}
void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) {}
void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) {}
void glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) {}
void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {}
void glObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar * label) {}
void glObjectPtrLabel(const void * ptr, GLsizei length, const GLchar * label) {}
void glPatchParameterfv(GLenum pname, const GLfloat * values) {}
void glPatchParameteri(GLenum pname, GLint value) {}
void glPauseTransformFeedback(void) {}
void glPixelStoref(GLenum pname, GLfloat param) {}
void glPixelStorei(GLenum pname, GLint param) {}
void glPointParameterf(GLenum pname, GLfloat param) {}
void glPointParameterfv(GLenum pname, const GLfloat * params) {}
void glPointParameteri(GLenum pname, GLint param) {}
void glPointParameteriv(GLenum pname, const GLint * params) {}
void glPointSize(GLfloat size) {}
void glPolygonMode(GLenum face, GLenum mode) {
    const char *Name = "glPolygonMode";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

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
void glPolygonOffset(GLfloat factor, GLfloat units) {
    const char *Name = "glPolygonOffset";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    C->Config.RasterState.depthBiasConstantFactor = units;
    C->Config.RasterState.depthBiasSlopeFactor = factor;
}
void glPolygonOffsetClamp(GLfloat factor, GLfloat units, GLfloat clamp) {}
void glPopDebugGroup(void) {}
void glPrimitiveRestartIndex(GLuint index) {}
void glProgramBinary(GLuint program, GLenum binaryFormat, const void * binary, GLsizei length) {}
void glProgramParameteri(GLuint program, GLenum pname, GLint value) {}
void glProgramUniform1d(GLuint program, GLint location, GLdouble v0) {}
void glProgramUniform1dv(GLuint program, GLint location, GLsizei count, const GLdouble * value) {}
void glProgramUniform1f(GLuint program, GLint location, GLfloat v0) {}
void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {}
void glProgramUniform1i(GLuint program, GLint location, GLint v0) {}
void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint * value) {}
void glProgramUniform1ui(GLuint program, GLint location, GLuint v0) {}
void glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {}
void glProgramUniform2d(GLuint program, GLint location, GLdouble v0, GLdouble v1) {}
void glProgramUniform2dv(GLuint program, GLint location, GLsizei count, const GLdouble * value) {}
void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1) {}
void glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {}
void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1) {}
void glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint * value) {}
void glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1) {}
void glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {}
void glProgramUniform3d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2) {}
void glProgramUniform3dv(GLuint program, GLint location, GLsizei count, const GLdouble * value) {}
void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {}
void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {}
void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) {}
void glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint * value) {}
void glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) {}
void glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {}
void glProgramUniform4d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3) {}
void glProgramUniform4dv(GLuint program, GLint location, GLsizei count, const GLdouble * value) {}
void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {}
void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {}
void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {}
void glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint * value) {}
void glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {}
void glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {}
void glProgramUniformMatrix2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glProgramUniformMatrix2x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glProgramUniformMatrix2x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glProgramUniformMatrix3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glProgramUniformMatrix3x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glProgramUniformMatrix3x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glProgramUniformMatrix4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glProgramUniformMatrix4x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glProgramUniformMatrix4x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glProvokingVertex(GLenum mode) {}
void glPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar * message) {}
void glQueryCounter(GLuint id, GLenum target) {}
void glReadBuffer(GLenum src) {}
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void * pixels) {}
void glReadnPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void * data) {}
void glReleaseShaderCompiler(void) {}
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {}
void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {}
void glResumeTransformFeedback(void) {}
void glSampleCoverage(GLfloat value, GLboolean invert) {}
void glSampleMaski(GLuint maskNumber, GLbitfield mask) {}
void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint * param) {}
void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint * param) {}
void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {}
void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat * param) {}
void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) {}
void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint * param) {}
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    const char *Name = "glScissor";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    CheckGL(width < 0 || height < 0, gl_error_SCISSOR_WIDTH_HEIGHT_NEGATIVE);

    pipeline_state_scissor *State = GetCurrentPipelineState(C, pipeline_state_SCISSOR);
    for(u32 I = 0; I < C->PipelineStateInfos[pipeline_state_SCISSOR].InstanceCount; ++I) {
        State[I].offset.x = (int32_t)x;
        State[I].offset.y = (int32_t)y;
        State[I].extent.width = (uint32_t)width;
        State[I].extent.height = (uint32_t)height;
    }
}
void glScissorArrayv(GLuint first, GLsizei count, const GLint * v) {
    const char *Name = "glScissorArrayv";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    for(u32 I = 0; I < count; ++I) {
        CheckGL(v[4*I + 2] < 0 || v[4*I + 3] < 0, gl_error_SCISSOR_WIDTH_HEIGHT_NEGATIVE);
    }
    CheckGL(count < 0, gl_error_SCISSOR_COUNT_NEGATIVE);
    CheckGL(first + count > C->PipelineStateInfos[pipeline_state_SCISSOR].InstanceCount, gl_error_SCISSOR_INDEX);

    pipeline_state_scissor *State = GetCurrentPipelineState(C, pipeline_state_SCISSOR);
    for(u32 I = 0; I < count; ++I) {
        State[first + I].offset.x = (int32_t)v[4*I + 0];
        State[first + I].offset.y = (int32_t)v[4*I + 1];
        State[first + I].extent.width = (uint32_t)v[4*I + 2];
        State[first + I].extent.height = (uint32_t)v[4*I + 3];
    }
}
void glScissorIndexed(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height) {
    const char *Name = "glScissorIndexed";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    CheckGL(width < 0 || height < 0, gl_error_SCISSOR_WIDTH_HEIGHT_NEGATIVE);
    CheckGL(index >= C->PipelineStateInfos[pipeline_state_SCISSOR].InstanceCount, gl_error_SCISSOR_INDEX);

    pipeline_state_scissor *State = GetCurrentPipelineState(C, pipeline_state_SCISSOR);
    State[index].offset.x = (int32_t)left;
    State[index].offset.y = (int32_t)bottom;
    State[index].extent.width = (uint32_t)width;
    State[index].extent.height = (uint32_t)height;
}
void glScissorIndexedv(GLuint index, const GLint * v) {
    const char *Name = "glScissorIndexed";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    CheckGL(v[2] < 0 || v[3] < 0, gl_error_SCISSOR_WIDTH_HEIGHT_NEGATIVE);
    CheckGL(index >= C->PipelineStateInfos[pipeline_state_SCISSOR].InstanceCount, gl_error_SCISSOR_INDEX);

    pipeline_state_scissor *State = GetCurrentPipelineState(C, pipeline_state_SCISSOR);
    State[index].offset.x = (int32_t)v[0];
    State[index].offset.y = (int32_t)v[1];
    State[index].extent.width = (uint32_t)v[2];
    State[index].extent.height = (uint32_t)v[3];
}
void glShaderBinary(GLsizei count, const GLuint * shaders, GLenum binaryFormat, const void * binary, GLsizei length) {}
void glShaderSource(GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length) {
    const char *Name = "glShaderSource";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    if(HandledCheckShaderGet(C, shader, &Object, Name)) {
        return;
    }
    CheckGL(count < 0, gl_error_SHADER_SOURCE_COUNT_NEGATIVE);

    u64 TotalSourceLength = 0;
    for(GLsizei I = 0; I < count; ++I) {
        TotalSourceLength += (u64)length[I];
    }

    char *Source = calloc(TotalSourceLength + 1, 1);
    CheckGL(Source == 0, gl_error_OUT_OF_MEMORY);

    Object->Shader.SourceBytes = Source;
    Object->Shader.SourceByteCount = TotalSourceLength;
    for(GLsizei I = 0; I < count; ++I) {
        memcpy(Source, string[I], length[I]);
        Source += length[I];
    }
}
void glShaderStorageBlockBinding(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding) {}
void glSpecializeShader(GLuint shader, const GLchar * pEntryPoint, GLuint numSpecializationConstants, const GLuint * pConstantIndex, const GLuint * pConstantValue) {}
void glStencilFunc(GLenum func, GLint ref, GLuint mask) {}
void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {}
void glStencilMask(GLuint mask) {}
void glStencilMaskSeparate(GLenum face, GLuint mask) {}
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {}
void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {}
void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer) {}
void glTexBufferRange(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {}
void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void * pixels) {}
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels) {
    const char *Name = "glTexImage2D";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

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
void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {}
void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * pixels) {}
void glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {}
void glTexParameterIiv(GLenum target, GLenum pname, const GLint * params) {}
void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint * params) {}
void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {}
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat * params) {}
void glTexParameteri(GLenum target, GLenum pname, GLint param) {}
void glTexParameteriv(GLenum target, GLenum pname, const GLint * params) {}
void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) {}
void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {}
void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {}
void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {}
void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {}
void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void * pixels) {}
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels) {}
void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels) {}
void glTextureBarrier(void) {}
void glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer) {}
void glTextureBufferRange(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {}
void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint * params) {}
void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint * params) {}
void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param) {}
void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat * param) {}
void glTextureParameteri(GLuint texture, GLenum pname, GLint param) {}
void glTextureParameteriv(GLuint texture, GLenum pname, const GLint * param) {}
void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) {}
void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {}
void glTextureStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {}
void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {}
void glTextureStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {}
void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void * pixels) {}
void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels) {}
void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels) {}
void glTextureView(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) {}
void glTransformFeedbackBufferBase(GLuint xfb, GLuint index, GLuint buffer) {}
void glTransformFeedbackBufferRange(GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {}
void glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const* varyings, GLenum bufferMode) {}
void glUniform1d(GLint location, GLdouble x) {
    NoContextSetUniformData(location, &x, 1, 1, GL_DOUBLE, __func__);
}
void glUniform1dv(GLint location, GLsizei count, const GLdouble * value) {
    NoContextSetUniformData(location, value, 1, count, GL_DOUBLE, __func__);
}
void glUniform1f(GLint location, GLfloat v0) {
    NoContextSetUniformData(location, &v0, 1, 1, GL_FLOAT, __func__);
}
void glUniform1fv(GLint location, GLsizei count, const GLfloat * value) {
    NoContextSetUniformData(location, value, 1, count, GL_FLOAT, __func__);
}
void glUniform1i(GLint location, GLint v0) {
    NoContextSetUniformData(location, &v0, 1, 1, GL_INT, __func__);
}
void glUniform1iv(GLint location, GLsizei count, const GLint * value) {
    NoContextSetUniformData(location, value, 1, count, GL_INT, __func__);
}
void glUniform1ui(GLint location, GLuint v0) {
    NoContextSetUniformData(location, &v0, 1, 1, GL_UNSIGNED_INT, __func__);
}
void glUniform1uiv(GLint location, GLsizei count, const GLuint * value) {
    NoContextSetUniformData(location, value, 1, count, GL_UNSIGNED_INT, __func__);
}
void glUniform2d(GLint location, GLdouble x, GLdouble y) {
    GLdouble v[] = { x, y };
    NoContextSetUniformData(location, v, 2, 1, GL_DOUBLE, __func__);
}
void glUniform2dv(GLint location, GLsizei count, const GLdouble * value) {
    NoContextSetUniformData(location, value, 2, count, GL_DOUBLE, __func__);
}
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    GLfloat v[] = { v0, v1 };
    NoContextSetUniformData(location, v, 2, 1, GL_FLOAT, __func__);
}
void glUniform2fv(GLint location, GLsizei count, const GLfloat * value) {
    NoContextSetUniformData(location, value, 2, count, GL_FLOAT, __func__);
}
void glUniform2i(GLint location, GLint v0, GLint v1) {
    GLint v[] = { v0, v1 };
    NoContextSetUniformData(location, v, 2, 1, GL_INT, __func__);
}
void glUniform2iv(GLint location, GLsizei count, const GLint * value) {
    NoContextSetUniformData(location, value, 2, count, GL_INT, __func__);
}
void glUniform2ui(GLint location, GLuint v0, GLuint v1) {
    GLuint v[] = { v0, v1 };
    NoContextSetUniformData(location, v, 2, 1, GL_UNSIGNED_INT, __func__);
}
void glUniform2uiv(GLint location, GLsizei count, const GLuint * value) {
    NoContextSetUniformData(location, value, 2, count, GL_UNSIGNED_INT, __func__);
}
void glUniform3d(GLint location, GLdouble x, GLdouble y, GLdouble z) {
    GLdouble v[] = { x, y, z };
    NoContextSetUniformData(location, v, 3, 1, GL_DOUBLE, __func__);
}
void glUniform3dv(GLint location, GLsizei count, const GLdouble * value) {
    NoContextSetUniformData(location, value, 3, count, GL_DOUBLE, __func__);
}
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    GLfloat v[] = { v0, v1, v2 };
    NoContextSetUniformData(location, v, 3, 1, GL_FLOAT, __func__);
}
void glUniform3fv(GLint location, GLsizei count, const GLfloat * value) {
    NoContextSetUniformData(location, value, 3, count, GL_FLOAT, __func__);
}
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
    GLint v[] = { v0, v1, v2 };
    NoContextSetUniformData(location, v, 3, 1, GL_INT, __func__);
}
void glUniform3iv(GLint location, GLsizei count, const GLint * value) {
    NoContextSetUniformData(location, value, 3, count, GL_INT, __func__);
}
void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) {
    GLuint v[] = { v0, v1, v2 };
    NoContextSetUniformData(location, v, 3, 1, GL_UNSIGNED_INT, __func__);
}
void glUniform3uiv(GLint location, GLsizei count, const GLuint * value) {
    NoContextSetUniformData(location, value, 3, count, GL_UNSIGNED_INT, __func__);
}
void glUniform4d(GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GLdouble v[] = { x, y, z, w };
    NoContextSetUniformData(location, v, 4, 1, GL_DOUBLE, __func__);
}
void glUniform4dv(GLint location, GLsizei count, const GLdouble * value) {
    NoContextSetUniformData(location, value, 4, count, GL_DOUBLE, __func__);
}
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GLfloat v[] = { v0, v1, v2, v3 };
    NoContextSetUniformData(location, v, 4, 1, GL_FLOAT, __func__);
}
void glUniform4fv(GLint location, GLsizei count, const GLfloat * value) {
    NoContextSetUniformData(location, value, 4, count, GL_FLOAT, __func__);
}
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    GLint v[] = { v0, v1, v2, v3 };
    NoContextSetUniformData(location, v, 4, 1, GL_INT, __func__);
}
void glUniform4iv(GLint location, GLsizei count, const GLint * value) {
    NoContextSetUniformData(location, value, 4, count, GL_INT, __func__);
}
void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    GLuint v[] = { v0, v1, v2, v3 };
    NoContextSetUniformData(location, v, 4, 1, GL_UNSIGNED_INT, __func__);
}
void glUniform4uiv(GLint location, GLsizei count, const GLuint * value) {
    NoContextSetUniformData(location, value, 4, count, GL_UNSIGNED_INT, __func__);
}
void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {}
void glUniformMatrix2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glUniformMatrix2x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glUniformMatrix2x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glUniformMatrix3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glUniformMatrix3x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glUniformMatrix3x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glUniformMatrix4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glUniformMatrix4x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glUniformMatrix4x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble * value) {}
void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {}
void glUniformSubroutinesuiv(GLenum shadertype, GLsizei count, const GLuint * indices) {}
GLboolean glUnmapBuffer(GLenum target) {return GL_TRUE;}
GLboolean glUnmapNamedBuffer(GLuint buffer) {return GL_TRUE;}
void glUseProgram(GLuint program) {
    const char *Name = "glUseProgram";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    if(program != 0) {
        object *Object = 0;
        HandledCheckProgramGet(C, program, &Object, Name);
        CheckGL(Object->Program.LinkStatus == GL_FALSE, gl_error_PROGRAM_NOT_LINkED_SUCCESSFULLY);
    }

    if(C->ActiveProgram != program) {
        pipeline_state_program *State = GetCurrentPipelineState(C, pipeline_state_PROGRAM);
        State->Program = program;
        C->ActiveProgram = program;
    }
}
void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program) {}
void glValidateProgram(GLuint program) {}
void glValidateProgramPipeline(GLuint pipeline) {}
void glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex, GLuint bindingindex) {
    const char *Name = "glVertexArrayAttribBinding";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, vaobj, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ARRAY_ATTRIB_BINDING_VAO_INVALID);
    SetVertexInputAttributeBinding(C, Object, C->BoundVao == vaobj, attribindex, bindingindex, Name);
}
void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    const char *Name = "glVertexArrayAttribFormat";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, vaobj, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ARRAY_ATTRIB_FORMAT_VAO_INVALID);

    u32 IntegerHandlingBits = vertex_input_attribute_INTEGER_HANDLING_ENABLED | (normalized == GL_FALSE ? vertex_input_attribute_INTEGER_NORMALIZE : 0);
    SetVertexInputAttributeFormat(C, Object, C->BoundVao == vaobj, attribindex, size, type, relativeoffset, IntegerHandlingBits, Name);
}
void glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    const char *Name = "glVertexArrayAttribIFormat";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, vaobj, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ARRAY_ATTRIB_FORMAT_VAO_INVALID);
    SetVertexInputAttributeFormat(C, Object, C->BoundVao == vaobj, attribindex, size, type, relativeoffset, 0, Name);
}
void glVertexArrayAttribLFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    const char *Name = "glVertexArrayAttribLFormat";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, vaobj, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ARRAY_ATTRIB_FORMAT_VAO_INVALID);
    SetVertexInputAttributeFormat(C, Object, C->BoundVao == vaobj, attribindex, size, type, relativeoffset, 0, Name);
}
void glVertexArrayBindingDivisor(GLuint vaobj, GLuint bindingindex, GLuint divisor) {}
void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer) {}
void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    const char *Name = "glVertexArrayVertexBuffer";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, vaobj, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ARRAY_VERTEX_BUFFER_VAO_INVALID);
    SetVertexInputBinding(C, Object, C->BoundVao == vaobj, bindingindex, buffer, offset, stride, Name);
}
void glVertexArrayVertexBuffers(GLuint vaobj, GLuint first, GLsizei count, const GLuint * buffers, const GLintptr * offsets, const GLsizei * strides) {
    const char *Name = "glVertexArrayVertexBuffers";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    // TODO(blackedout):
    Assert(0);
}
void glVertexAttrib1d(GLuint index, GLdouble x) {}
void glVertexAttrib1dv(GLuint index, const GLdouble * v) {}
void glVertexAttrib1f(GLuint index, GLfloat x) {}
void glVertexAttrib1fv(GLuint index, const GLfloat * v) {}
void glVertexAttrib1s(GLuint index, GLshort x) {}
void glVertexAttrib1sv(GLuint index, const GLshort * v) {}
void glVertexAttrib2d(GLuint index, GLdouble x, GLdouble y) {}
void glVertexAttrib2dv(GLuint index, const GLdouble * v) {}
void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {}
void glVertexAttrib2fv(GLuint index, const GLfloat * v) {}
void glVertexAttrib2s(GLuint index, GLshort x, GLshort y) {}
void glVertexAttrib2sv(GLuint index, const GLshort * v) {}
void glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {}
void glVertexAttrib3dv(GLuint index, const GLdouble * v) {}
void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {}
void glVertexAttrib3fv(GLuint index, const GLfloat * v) {}
void glVertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z) {}
void glVertexAttrib3sv(GLuint index, const GLshort * v) {}
void glVertexAttrib4Nbv(GLuint index, const GLbyte * v) {}
void glVertexAttrib4Niv(GLuint index, const GLint * v) {}
void glVertexAttrib4Nsv(GLuint index, const GLshort * v) {}
void glVertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w) {}
void glVertexAttrib4Nubv(GLuint index, const GLubyte * v) {}
void glVertexAttrib4Nuiv(GLuint index, const GLuint * v) {}
void glVertexAttrib4Nusv(GLuint index, const GLushort * v) {}
void glVertexAttrib4bv(GLuint index, const GLbyte * v) {}
void glVertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {}
void glVertexAttrib4dv(GLuint index, const GLdouble * v) {}
void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {}
void glVertexAttrib4fv(GLuint index, const GLfloat * v) {}
void glVertexAttrib4iv(GLuint index, const GLint * v) {}
void glVertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w) {}
void glVertexAttrib4sv(GLuint index, const GLshort * v) {}
void glVertexAttrib4ubv(GLuint index, const GLubyte * v) {}
void glVertexAttrib4uiv(GLuint index, const GLuint * v) {}
void glVertexAttrib4usv(GLuint index, const GLushort * v) {}
void glVertexAttribBinding(GLuint attribindex, GLuint bindingindex) {
    const char *Name = "glVertexAttribBinding";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ATTRIB_BINDING_NONE_BOUND);
    SetVertexInputAttributeBinding(C, Object, 1, attribindex, bindingindex, Name);
}
void glVertexAttribDivisor(GLuint index, GLuint divisor) {}
void glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    const char *Name = "glVertexAttribFormat";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ATTRIB_FORMAT_NONE_BOUND);

    u32 IntegerHandlingBits = vertex_input_attribute_INTEGER_HANDLING_ENABLED | (normalized == GL_FALSE ? vertex_input_attribute_INTEGER_NORMALIZE : 0);
    SetVertexInputAttributeFormat(C, Object, 1, attribindex, size, type, relativeoffset, IntegerHandlingBits, Name);
}
void glVertexAttribI1i(GLuint index, GLint x) {}
void glVertexAttribI1iv(GLuint index, const GLint * v) {}
void glVertexAttribI1ui(GLuint index, GLuint x) {}
void glVertexAttribI1uiv(GLuint index, const GLuint * v) {}
void glVertexAttribI2i(GLuint index, GLint x, GLint y) {}
void glVertexAttribI2iv(GLuint index, const GLint * v) {}
void glVertexAttribI2ui(GLuint index, GLuint x, GLuint y) {}
void glVertexAttribI2uiv(GLuint index, const GLuint * v) {}
void glVertexAttribI3i(GLuint index, GLint x, GLint y, GLint z) {}
void glVertexAttribI3iv(GLuint index, const GLint * v) {}
void glVertexAttribI3ui(GLuint index, GLuint x, GLuint y, GLuint z) {}
void glVertexAttribI3uiv(GLuint index, const GLuint * v) {}
void glVertexAttribI4bv(GLuint index, const GLbyte * v) {}
void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w) {}
void glVertexAttribI4iv(GLuint index, const GLint * v) {}
void glVertexAttribI4sv(GLuint index, const GLshort * v) {}
void glVertexAttribI4ubv(GLuint index, const GLubyte * v) {}
void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) {}
void glVertexAttribI4uiv(GLuint index, const GLuint * v) {}
void glVertexAttribI4usv(GLuint index, const GLushort * v) {}
void glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    const char *Name = "glVertexAttribIFormat";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ATTRIB_FORMAT_NONE_BOUND);
    SetVertexInputAttributeFormat(C, Object, 1, attribindex, size, type, relativeoffset, 0, Name);
}
void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer) {
    const char *Name = "glVertexAttribIPointer";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ATTRIB_POINTER_NONE_BOUND);
    SetVertexInputAttributePointer(C, Object, index, size, type, stride, pointer, 0, Name);
}
void glVertexAttribL1d(GLuint index, GLdouble x) {}
void glVertexAttribL1dv(GLuint index, const GLdouble * v) {}
void glVertexAttribL2d(GLuint index, GLdouble x, GLdouble y) {}
void glVertexAttribL2dv(GLuint index, const GLdouble * v) {}
void glVertexAttribL3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {}
void glVertexAttribL3dv(GLuint index, const GLdouble * v) {}
void glVertexAttribL4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {}
void glVertexAttribL4dv(GLuint index, const GLdouble * v) {}
void glVertexAttribLFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    const char *Name = "glVertexAttribLFormat";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ATTRIB_FORMAT_NONE_BOUND);
    SetVertexInputAttributeFormat(C, Object, 1, attribindex, size, type, relativeoffset, 0, Name);
}
void glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer) {
    const char *Name = "glVertexAttribLPointer";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ATTRIB_POINTER_NONE_BOUND);
    SetVertexInputAttributePointer(C, Object, index, size, type, stride, pointer, 0, Name);
}
void glVertexAttribP1ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {}
void glVertexAttribP1uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint * value) {}
void glVertexAttribP2ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {}
void glVertexAttribP2uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint * value) {}
void glVertexAttribP3ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {}
void glVertexAttribP3uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint * value) {}
void glVertexAttribP4ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {}
void glVertexAttribP4uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint * value) {}
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer) {
    const char *Name = "glVertexAttribPointer";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    // NOTE(blackedout): Remember that `pointer` specifies the offset of the binding, NOT the relative offset of the format.
    // Since this function sets both anyway, it doesn't make a difference.

    object *Object = 0;
    CheckGL(CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object), gl_error_VERTEX_ATTRIB_POINTER_NONE_BOUND);

    u32 IntegerHandlingBits = vertex_input_attribute_INTEGER_HANDLING_ENABLED | (normalized == GL_FALSE ? vertex_input_attribute_INTEGER_NORMALIZE : 0);
    SetVertexInputAttributePointer(C, Object, index, size, type, stride, pointer, IntegerHandlingBits, Name);
}
void glVertexBindingDivisor(GLuint bindingindex, GLuint divisor) {}
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    const char *Name = "glViewport";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    pipeline_state_viewport *State = GetCurrentPipelineState(C, pipeline_state_VIEWPORT);
    for(u32 I = 0; I < C->PipelineStateInfos[pipeline_state_VIEWPORT].InstanceCount; ++I) {
        State[I].x = (float)x;
        State[I].y = (float)y;
        State[I].width = (float)width;
        State[I].height = (float)height;
    }
}
void glViewportArrayv(GLuint first, GLsizei count, const GLfloat * v) {
    const char *Name = "glViewportArrayv";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    CheckGL(count < 0, gl_error_VIEWPORT_COUNT_NEGATIVE);
    CheckGL(first + count > C->PipelineStateInfos[pipeline_state_VIEWPORT].InstanceCount, gl_error_VIEWPORT_INDEX);
    
    pipeline_state_viewport *State = GetCurrentPipelineState(C, pipeline_state_VIEWPORT);
    for(u32 I = 0; I < count; ++I) {
        State[first + I].x = v[4*I + 0];
        State[first + I].y = v[4*I + 1];
        State[first + I].width = v[4*I + 2];
        State[first + I].height = v[4*I + 3];
    }
}
void glViewportIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h) {
    const char *Name = "glViewportIndexedf";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    CheckGL(index >= C->PipelineStateInfos[pipeline_state_VIEWPORT].InstanceCount, gl_error_VIEWPORT_INDEX);

    pipeline_state_viewport *State = GetCurrentPipelineState(C, pipeline_state_VIEWPORT);
    State[index].x = x;
    State[index].y = y;
    State[index].width = w;
    State[index].height = h;
}
void glViewportIndexedfv(GLuint index, const GLfloat * v) {
    const char *Name = "glViewportIndexedfv";
    context *C = 0;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT);

    CheckGL(index >= C->PipelineStateInfos[pipeline_state_VIEWPORT].InstanceCount, gl_error_VIEWPORT_INDEX);

    pipeline_state_viewport *State = GetCurrentPipelineState(C, pipeline_state_VIEWPORT);
    State[index].x = v[0];
    State[index].y = v[1];
    State[index].width = v[2];
    State[index].height = v[3];
}
void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {}