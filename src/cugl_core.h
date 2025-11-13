#ifndef CUGL_CORE_H
#define CUGL_CORE_H

#include "vulkan/vulkan.h"
#include "vulkan/vulkan_beta.h"
#include "vulkan/vk_enum_string_helper.h"
#include "shader_interface.h"
#include "vk_mem_alloc.h"

#include "cugl/cugl.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define Assert(X) if((X) == 0) __builtin_debugtrap()
#define ArrayCount(X) (sizeof(X)/sizeof(*(X)))
#define ClampAB(X, A, B) ((X) < (A) ? (A) : ((X) > (B) ? (B) : (X)))
#define Clamp01(X) ClampAB(X, 0, 1)

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

#define INITIAL_OBJECT_CAPACITY (1024)
#define INITIAL_COMMAND_CAPACITY (1024)

#define VulkanCheckGoto(Call, Label) if(VulkanCheck(C, Call, #Call)) goto Label;
#define VulkanCheckReturn(Call) if(VulkanCheck(C, Call, #Call)) return;

typedef enum object_type {
    object_NONE = 0,
    object_BUFFER,
    object_FRAMEBUFFER,
    object_PROGRAM,
    object_QUERY,
    object_RENDERBUFFER,
    object_SHADER,
    object_SYNC,
    object_TEXTURE,
object_VERTEX_ARRAY,
} object_type;

typedef enum texture_dimension {
    texture_dimension_UNSPECIFIED = 0,
    texture_dimension_1D,
    texture_dimension_2D,
    texture_dimension_3D,
} texture_dimension;

typedef struct buffer_target_info {
    u32 Index;
    VkBufferUsageFlags VulkanUsage;
} buffer_target_info;

#define BUFFER_TARGET_COUNT (15)
int GetBufferTargetInfo(GLenum Target, buffer_target_info *OutInfo) {
#define MakeCase(T, ...) case (T): { buffer_target_info I = { __VA_ARGS__ }; *OutInfo = I; }; return 0;
    switch(Target) {
    MakeCase(GL_ARRAY_BUFFER, 0, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    MakeCase(GL_ATOMIC_COUNTER_BUFFER, 1, 0); // TODO
    MakeCase(GL_COPY_READ_BUFFER, 2, 0); // TODO
    MakeCase(GL_COPY_WRITE_BUFFER, 3, 0); // TODO
    MakeCase(GL_DISPATCH_INDIRECT_BUFFER, 4, 0); // TODO
    MakeCase(GL_DRAW_INDIRECT_BUFFER, 5); // TODO
    MakeCase(GL_ELEMENT_ARRAY_BUFFER, 6, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    MakeCase(GL_PARAMETER_BUFFER, 7, 0); // TODO
    MakeCase(GL_PIXEL_PACK_BUFFER, 8, 0); // TODO
    MakeCase(GL_PIXEL_UNPACK_BUFFER, 9, 0); // TODO
    MakeCase(GL_QUERY_BUFFER, 10, 0); // TODO
    MakeCase(GL_SHADER_STORAGE_BUFFER, 11, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    MakeCase(GL_TEXTURE_BUFFER, 12, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT); // TODO
    MakeCase(GL_TRANSFORM_FEEDBACK_BUFFER, 13, 0); // TODO
    MakeCase(GL_UNIFORM_BUFFER, 14, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    default: return 1;
    }
#undef MakeCase
}

typedef struct texture_target_info {
    u32 Index;
    texture_dimension Dimension;
} texture_target_info;

#define TEXTURE_SLOT_COUNT (128)
#define TEXTURE_TARGET_COUNT (11)
int GetTextureTargetInfo(GLenum Target, texture_target_info *OutInfo) {
#define MakeCase(T, ...) case (T): { texture_target_info I = { __VA_ARGS__ }; *OutInfo = I; } return 0
    switch(Target) {
    MakeCase(GL_TEXTURE_1D, 0, texture_dimension_1D);
    MakeCase(GL_TEXTURE_2D, 1, texture_dimension_2D);
    MakeCase(GL_TEXTURE_3D, 2, texture_dimension_3D);
    MakeCase(GL_TEXTURE_1D_ARRAY, 3, texture_dimension_2D);
    MakeCase(GL_TEXTURE_2D_ARRAY, 4, texture_dimension_3D);
    MakeCase(GL_TEXTURE_RECTANGLE, 5, texture_dimension_2D);
    MakeCase(GL_TEXTURE_BUFFER, 6, texture_dimension_2D);
    MakeCase(GL_TEXTURE_CUBE_MAP, 7, texture_dimension_2D);
    MakeCase(GL_TEXTURE_CUBE_MAP_ARRAY, 8, texture_dimension_3D);
    MakeCase(GL_TEXTURE_2D_MULTISAMPLE, 9, texture_dimension_2D);
    MakeCase(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 10, texture_dimension_2D);
    default: return 1;
    }
#undef MakeCase
}

typedef struct primitive_info {
    VkPrimitiveTopology VulkanPrimitve;
} primitive_info;
int GetPrimitiveInfo(GLenum Mode, primitive_info *OutInfo) {
#define MakeCase(T, ...) case (T): { primitive_info I = { __VA_ARGS__ }; *OutInfo = I; } return 0
    switch(Mode) {
    MakeCase(GL_POINTS, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    MakeCase(GL_LINE_STRIP, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
    MakeCase(GL_LINE_LOOP, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP); // TODO(blackedout): Not supported by Vulkan, handle custom
    MakeCase(GL_LINES, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
    MakeCase(GL_LINE_STRIP_ADJACENCY, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY);
    MakeCase(GL_LINES_ADJACENCY, VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY);
    MakeCase(GL_TRIANGLE_STRIP, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
    MakeCase(GL_TRIANGLE_FAN, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
    MakeCase(GL_TRIANGLES, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    MakeCase(GL_TRIANGLE_STRIP_ADJACENCY, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY);
    MakeCase(GL_TRIANGLES_ADJACENCY, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY);
    MakeCase(GL_PATCHES, VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
    default: return 1;
    }
#undef MakeCase
}

int GetVulkanBlendOp(GLenum mode, VkBlendOp *OutBlendOp) {
#define MakeCase(Key, Value) case (Key): *OutBlendOp = (Value); break
    switch(mode) {
    MakeCase(GL_FUNC_ADD, VK_BLEND_OP_ADD);
    MakeCase(GL_FUNC_SUBTRACT, VK_BLEND_OP_SUBTRACT);
    MakeCase(GL_FUNC_REVERSE_SUBTRACT, VK_BLEND_OP_REVERSE_SUBTRACT);
    MakeCase(GL_MIN, VK_BLEND_OP_MIN);
    MakeCase(GL_MAX, VK_BLEND_OP_MAX);
    default:
        return 1;
    }
#undef MakeCase
    return 0;
}

int GetVulkanBlendFactor(GLenum factor, VkBlendFactor *OutBlendFactor) {
#define MakeCase(Key, Value) case (Key): *OutBlendFactor = (Value); return 0
    switch(factor) {
    MakeCase(GL_ZERO, VK_BLEND_FACTOR_ZERO);
    MakeCase(GL_ONE, VK_BLEND_FACTOR_ONE);
    MakeCase(GL_SRC_COLOR, VK_BLEND_FACTOR_SRC_COLOR);
    MakeCase(GL_ONE_MINUS_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
    MakeCase(GL_DST_COLOR, VK_BLEND_FACTOR_DST_COLOR);
    MakeCase(GL_ONE_MINUS_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR);
    MakeCase(GL_SRC_ALPHA, VK_BLEND_FACTOR_SRC_ALPHA);
    MakeCase(GL_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    MakeCase(GL_DST_ALPHA, VK_BLEND_FACTOR_DST_ALPHA);
    MakeCase(GL_ONE_MINUS_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA);
    MakeCase(GL_CONSTANT_COLOR, VK_BLEND_FACTOR_CONSTANT_COLOR);
    MakeCase(GL_ONE_MINUS_CONSTANT_COLOR, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR);
    MakeCase(GL_CONSTANT_ALPHA, VK_BLEND_FACTOR_CONSTANT_ALPHA);
    MakeCase(GL_ONE_MINUS_CONSTANT_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA);
    default:
        return 1;
    }
#undef MakeCase
}

typedef struct shader_type_info {
    shader_type InterfaceType;
    VkShaderStageFlags VulkanBit;
} shader_type_info;

int GetShaderTypeInfo(GLenum type, shader_type_info *OutInfo) {
#define MakeCase(T, ...) case (T): { shader_type_info I = { __VA_ARGS__ }; *OutInfo = I; } return 0
    switch(type) {
    MakeCase(GL_VERTEX_SHADER, shader_VERTEX, VK_SHADER_STAGE_VERTEX_BIT);
    MakeCase(GL_FRAGMENT_SHADER, shader_FRAGMENT, VK_SHADER_STAGE_FRAGMENT_BIT);
    MakeCase(GL_COMPUTE_SHADER, shader_COMPUTE, VK_SHADER_STAGE_COMPUTE_BIT);
    MakeCase(GL_GEOMETRY_SHADER, shader_GEOMETRY, VK_SHADER_STAGE_GEOMETRY_BIT);
    MakeCase(GL_TESS_CONTROL_SHADER, shader_TESSELATION_CONTROL, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
    MakeCase(GL_TESS_EVALUATION_SHADER, shader_TESSELATION_EVALUATE, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    default:
        return 1;
    }
#undef MakeCase
}

#define VERTEX_ATTRIB_CAPACITY (8)
#define PROGRAM_SHADER_CAPACITY (6)

typedef struct object {
    object_type Type;

    union {
        struct {
            u64 NextFreeIndex;
        } None;
        struct {
            VkBuffer Buffer;
            VmaAllocation Allocation;
            VkBuffer StagingBuffer;
            VmaAllocation StagingAllocation;
        } Buffer;
        struct {
            VkClearValue ClearValue;
            uint32_t ColorImageCount;
            VkImage *ColorImages;
            VkImageView *ColorImageViews;
            // NOTE(blackedout): Indices into `ColorImages` that specify which color images are affected by draw operations (such as glClear)
            // For the default framebuffer, the indices point into the virtual image reference buffer { fromt, back },
            // because which of these images is front or back potentially changes for each frame.
            u32 ColorDrawCount;
            u32 *ColorDrawIndices;
        } Framebuffer;
        struct {
            u64 AttachedShaderCount;
            struct object *AttachedShaders[PROGRAM_SHADER_CAPACITY];
            GLenum ShouldDelete;
            GLenum LinkStatus;
            glslang_program GlslangProgram;
        } Program;
        struct {
            GLenum Type;
            char *SourceBytes;
            u64 SourceByteCount;
            GLenum ShouldDelete;
            GLenum CompileStatus;
            glslang_shader GlslangShader;
            struct object *Program;
            char *SpirvBytes;
            u64 SpirvByteCount;
            VkShaderModule VulkanModule;
        } Shader;
        struct {
            texture_dimension Dimension;
        } Texture;
        struct {
            VkVertexInputBindingDescription Binding;
            struct {
                GLuint Buffer;
                VkVertexInputAttributeDescription Desc;
                int Enabled;
            } Attribs[VERTEX_ATTRIB_CAPACITY];
        } VertexArray;
    };
} object;

typedef enum command_type {
    command_NONE = 0,
    command_CLEAR,
    command_DRAW,
    command_BIND_VERTEX_BUFFERS,
    command_BIND_PROGRAM,
} command_type;

typedef struct command {
    command_type Type;

    int ChangePipeline;
    u32 PipelineIndex;

    union {
        struct {
            GLuint VertexArray;
        } BindVertexBuffers;
        struct {
            GLuint Program;
        } BindProgram;
        struct {

        } Clear;
        struct {
            u64 VertexCount;
            u64 VertexOffset;
            u64 InstanceCount;
            u64 InstanceOffset;
        } Draw;
    };
} command;

typedef struct config {
    int CullingEnabled;
    int PolygonOffsetFillEnabled;
    int PolygonOffsetLineEnabled;
    int PolygonOffsetPointEnabled;
    int ScissorEnabled;
    VkPipelineRasterizationStateCreateInfo RasterState;
    VkPipelineDepthStencilStateCreateInfo DepthStencilState;
    VkPipelineColorBlendAttachmentState BlendAttachmentState;
    VkPipelineColorBlendStateCreateInfo BlendState;
    VkViewport Viewport;
    VkRect2D Scissor;
} config;

config GetDefaultConfig(int Width, int Height) {
    // NOTE(blackedout): Set by the default values of OpenGL
    VkPipelineRasterizationStateCreateInfo RasterState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0, // TODO
        .depthClampEnable = 0,
        .rasterizerDiscardEnable = 0,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = 0, // TODO
        .depthBiasConstantFactor = 0.0f, // TODO
        .depthBiasClamp = 0, // TODO
        .depthBiasSlopeFactor = 0.0f, // TODO
        .lineWidth = 1.0f,
    };
    
    VkPipelineDepthStencilStateCreateInfo DepthStencilState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0, // TODO
        .depthTestEnable = 0,
        .depthWriteEnable = 0, // TODO
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = 0, // TODO
        .stencilTestEnable = 0,
        .front.failOp = 0, // TODO
        .front.passOp = 0, // TODO
        .front.depthFailOp = 0, // TODO
        .front.compareOp = 0, // TODO
        .front.compareMask = 0, // TODO
        .front.writeMask = 0, // TODO
        .front.reference = 0, // TODO
        .back.failOp = 0, // TODO
        .back.passOp = 0, // TODO
        .back.depthFailOp = 0, // TODO
        .back.compareOp = 0, // TODO
        .back.compareMask = 0, // TODO
        .back.writeMask = 0, // TODO
        .back.reference = 0, // TODO
        .minDepthBounds = 0, // TODO
        .maxDepthBounds = 0, // TODO
    };

    VkPipelineColorBlendAttachmentState BlendAttachmentState = {
        .blendEnable = 0,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, // TODO
    };

    VkPipelineColorBlendStateCreateInfo BlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0, // TODO
        .logicOpEnable = 0, // TODO
        .logicOp = 0, // TODO
        .attachmentCount = 0,
        .pAttachments = 0,
        .blendConstants[0] = 0.0f,
        .blendConstants[1] = 0.0f,
        .blendConstants[2] = 0.0f,
        .blendConstants[3] = 0.0f,
    };

    VkViewport Viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = Width,
        .height= Height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D Scissor = {
        .offset.x = 0.0f,
        .offset.y = 0.0f,
        .extent.width = Width,
        .extent.height = Height,
    };

    config Result = {
        .CullingEnabled = 0,
        .PolygonOffsetFillEnabled = 0,
        .PolygonOffsetLineEnabled = 0,
        .PolygonOffsetPointEnabled = 0,
        .RasterState = RasterState,
        .DepthStencilState = DepthStencilState,
        .BlendAttachmentState = BlendAttachmentState,
        .BlendState = BlendState,
        .Viewport = Viewport,
        .Scissor = Scissor,
    };

    return Result;
}

typedef enum queue_type {
    queue_GRAPHICS,
    queue_SURFACE,
    queue_COUNT,
} queue_type;

typedef struct device_info {
    uint32_t QueueFamilyIndices[queue_COUNT];
    VkSurfaceFormatKHR InitialSurfaceFormat;
    VkPhysicalDeviceProperties Properties;
    VkPhysicalDeviceFeatures Features;
    VkSurfaceCapabilitiesKHR SurfaceCapabilities;
    VkFormat DepthFormat;
    VkPhysicalDevice PhysicalDevice;
    VkPresentModeKHR BestPresentMode;
    VkSampleCountFlagBits MaxSampleCount;
    VmaAllocationCreateFlags VmaCreateFlags;
} device_info;

typedef struct swapchain_image {
    VkImage Image;
    VkImageView View;
} swapchain_image;

enum {
    framebuffer_READ = 0,
    framebuffer_WRITE,
    framebuffer_COUNT,
};

enum {
    front_back_FRONT = 0,
    front_back_BACK,
    front_back_COUNT
};

enum {
    semaphore_PREV_PRESENT_DONE = 0,
    semaphore_RENDER_COMPLETE,
    semaphore_COUNT,
};

enum {
    command_buffer_GRAPHICS = 0,
    command_buffer_TRANSFER,
    command_buffer_GRAPHICS2,
    command_buffer_COUNT,
};

enum {
    fence_TRANSFER = 0,
    fence_COUNT,
};

typedef struct context {
    // NOTE(blackedout): Contiguous free list
    u64 ObjectCapacity;
    u64 ObjectCount;
    object *Objects;
    u64 FreeObjectCount;
    u64 NextFreeObjectIndex;

    u64 CommandCapacity;
    u64 CommandCount;
    command *Commands;

    GLenum ErrorFlag;
    GLDEBUGPROC DebugCallback;
    const void *DebugCallbackUser;

    GLuint BoundBuffers[BUFFER_TARGET_COUNT];
    GLuint BoundFramebuffers[framebuffer_COUNT];
    GLuint BoundTextures[TEXTURE_TARGET_COUNT*TEXTURE_SLOT_COUNT];
    u32 ActiveTextureIndex;
    GLuint BoundVertexArray;
    GLuint ActiveProgram;

    config Config;

    VkInstance Instance;
    VkSurfaceKHR Surface;
    VkDevice Device;
    device_info DeviceInfo;
    VmaAllocator Allocator;
    VkSwapchainKHR Swapchain;
    u32 SwapchainImageCount;
    swapchain_image *SwapchainImages;
    VkCommandPool GraphicsCommandPool;

    int IsSingleBuffered;
    u32 CurrentFrontBackIndices[front_back_COUNT];

    u32 SemaphoreCount;
    VkSemaphore *Semaphores;
    
    VkCommandBuffer CommandBuffers[command_buffer_COUNT];
    VkFence Fences[fence_COUNT];

    VkPrimitiveTopology CurrentPrimitiveTopoloy;
    int IsPrimitiveTopologySet;
} context;

void GenerateErrorMsg(context *C, GLenum Error, GLenum Source, const char *Msg) {
#if 0
    DEBUG_TYPE_ERROR
    DEBUG_TYPE_DEPRECATED_BEHAVIOR
    DEBUG_TYPE_UNDEFINED_BEHAVIOR
    DEBUG_TYPE_PORTABILITY
    DEBUG_TYPE_PERFORMANCE
    DEBUG_TYPE_OTHER
    DEBUG_TYPE_MARKER
    
    DEBUG_SOURCE_API
    DEBUG_SOURCE_WINDOW_SYSTEM
    DEBUG_SOURCE_SHADER_COMPILER
    DEBUG_SOURCE_THIRD_PARTY
    DEBUG_SOURCE_APPLICATION
    DEBUG_SOURCE_OTHER
#endif
    if(Msg == 0) {
        Assert(0);
    }
    if(C->ErrorFlag == GL_NO_ERROR) {
        C->ErrorFlag = Error;
    }
    // NOTE(blackedout): If error is already set, the spec says to do nothing

    if(C->DebugCallback == 0) {
        return;
    }
    C->DebugCallback(Source, GL_DEBUG_TYPE_ERROR, 0, 0, 0, Msg, C->DebugCallbackUser);
}

void GenerateError(context *C, GLenum Error, GLenum Source) {
    GenerateErrorMsg(C, Error, Source, 0);
}
void GenerateOther(context *C, GLenum Source, const char *Msg) {
    if(C->DebugCallback == 0) {
        return;
    }
    C->DebugCallback(Source, GL_DEBUG_TYPE_OTHER, 0, 0, strlen(Msg), Msg, C->DebugCallbackUser);
}

int RequireRoomForNewObjects(context *Context, u64 Count) {
    u64 FreeObjectCount = Context->ObjectCapacity - Context->ObjectCount;
    if(Count <= FreeObjectCount) {
        return 0;
    }

    if(Context->ObjectCapacity == 0) {
        object *NewObjects = calloc(INITIAL_OBJECT_CAPACITY, sizeof(object));
        if(NewObjects == 0) {
            GenerateError(Context, GL_OUT_OF_MEMORY, GL_DEBUG_SOURCE_API);
            return 1;
        }
        Context->Objects = NewObjects;
        Context->ObjectCapacity = INITIAL_OBJECT_CAPACITY;
        return 0;
    }

    u64 NewObjectCapacity = 2*Context->ObjectCapacity;
    object *NewObjects = calloc(NewObjectCapacity, sizeof(object));
    if(NewObjects == 0) {
        GenerateError(Context, GL_OUT_OF_MEMORY, GL_DEBUG_SOURCE_API);
        return 1;
    }
    memcpy(NewObjects, Context->Objects, Context->ObjectCapacity*sizeof(object));
    Context->ObjectCapacity = NewObjectCapacity;
    free(Context->Objects);
    Context->Objects = NewObjects;

    return 0;
}

int RequireRoomForNewCommands(context *C, u64 Count) {
    if(C->CommandCount + Count < C->CommandCapacity) {
        return 0;
    }

    if(C->CommandCapacity == 0) {
        command *NewCommands = calloc(INITIAL_COMMAND_CAPACITY, sizeof(command));
        if(NewCommands == 0) {
            GenerateError(C, GL_OUT_OF_MEMORY, GL_DEBUG_SOURCE_API);
            return 1;
        }
        C->Commands = NewCommands;
        C->CommandCapacity = INITIAL_COMMAND_CAPACITY;
        return 0;
    }

    u64 NewCommandCapacity = 2*C->CommandCapacity;
    command *NewCommands = calloc(NewCommandCapacity, sizeof(command));
    if(NewCommands == 0) {
        GenerateError(C, GL_OUT_OF_MEMORY, GL_DEBUG_SOURCE_API);
        return 1;
    }
    memcpy(NewCommands, C->Commands, C->CommandCapacity*sizeof(command));
    C->CommandCapacity = NewCommandCapacity;
    free(C->Commands);
    C->Commands = NewCommands;

    return 0;
}

int PushCommand(context *C, command Command) {
    if(RequireRoomForNewCommands(C, 1)) {
        return 1;
    }
    C->Commands[C->CommandCount++] = Command;

    return 0;
}

int CreateObjects(context *Context, object Template, u64 Count, GLuint *OutHandles) {
    if(RequireRoomForNewObjects(Context, Count)) {
        return 1;
    }

    object *Objects = Context->Objects;
    for(u64 I = 0; I < Count; ++I) {
        u64 Index = Context->NextFreeObjectIndex;

        if(Objects[Index].Type != object_NONE) {
            GenerateError(Context, 0, GL_DEBUG_SOURCE_API);
            return 1;
        }

        u64 NextFreeIndex = Objects[Index].None.NextFreeIndex;
        Objects[Index] = Template;
        OutHandles[I] = Index;
        
        if(Context->FreeObjectCount == 0) {
            // NOTE(blackedout): Object was just appended
            ++Context->NextFreeObjectIndex;
        } else {
            Context->NextFreeObjectIndex = NextFreeIndex;
            --Context->FreeObjectCount;
        }
        
        ++Context->ObjectCount;
    }

    return 0;
}

int CreateObjectsSizei(context *Context, object Template, GLsizei Count, GLuint *OutHandles) {
    if(Count < 0) {
        GenerateError(Context, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION);
        return 1;
    }

    return CreateObjects(Context, Template, (u64)Count, OutHandles);
}

void DeleteObject(context *C, object *Object) {
    object *Objects = C->Objects;
    u64 Index = Object - Objects;
    Assert(0 <= Index && Index < C->ObjectCapacity);
    Assert(Object->Type != object_NONE);

    Object->Type = object_NONE;
    Object->None.NextFreeIndex = C->NextFreeObjectIndex;
    C->NextFreeObjectIndex = Index;
    ++C->FreeObjectCount;
    ++C->ObjectCount;
}

void DeleteObjects(context *Context, u64 Count, const GLuint *Handles, object_type ExpectedType) {
    object *Objects = Context->Objects;
    u64 Capacity = Context->ObjectCapacity;
    for(u64 I = 0; I < Count; ++I) {
        u64 Index = (u64)Handles[I];

        if(Index >= Capacity || Objects[Index].Type == object_NONE) {
            // NOTE(blackedout): Silently ignore, as specified
            continue;
        }

        if(Objects[Index].Type != ExpectedType) {
            GenerateError(Context, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION);
        }

        DeleteObject(Context, Objects + Index);
    }
}

void DeleteObjectsSizei(context *Context, GLsizei Count, const GLuint *Handles, object_type ExpectedType) {
    if(Count < 0) {
        GenerateError(Context, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION);
    }

    DeleteObjects(Context, (u64)Count, Handles, ExpectedType);
}

void DeleteShader(context *C, object *Object) {
    Assert(Object->Type == object_SHADER);

    free(Object->Shader.SourceBytes);
    Object->Shader.SourceBytes = 0;
    GlslangShaderDelete(Object->Shader.GlslangShader);
    Object->Shader.GlslangShader = 0;

    DeleteObject(C, Object);
}

void DeleteProgram(context *C, object *Object) {
    Assert(Object->Type == object_PROGRAM);

    for(u32 I = 0; I < Object->Program.AttachedShaderCount; ++I) {
        Object->Program.AttachedShaders[I]->Shader.Program = 0;
        if(Object->Program.AttachedShaders[I]->Shader.ShouldDelete) {
            DeleteShader(C, Object->Program.AttachedShaders[I]);
        }
    }

    GlslangProgramDelete(Object->Program.GlslangProgram);
    DeleteObject(C, Object);
}

// NOTE(blackedout): Unsafe function, do not expose to raw user input
void GetObject(context *C, GLuint H, object **OutObject) {
    Assert(H < C->ObjectCapacity);
    u64 Index = (u64)H;
    *OutObject = C->Objects + Index;
}

// NOTE(blackedout): MARK: Check functions

int CheckObjectCreated(context *C, GLuint H) {
    if(H >= C->ObjectCapacity) {
        return 1;
    }
    object *Object = 0;
    GetObject(C, H, &Object);
    return Object->Type == object_NONE;
}

int CheckObjectTypeGet(context *C, GLuint H, object_type Type, object **OutObject) {
    if(H >= C->ObjectCapacity) {
        return 1;
    }

    object *Object = 0;
    GetObject(C, H, &Object);
    if(OutObject) {
        *OutObject = Object;
    }
    return Object->Type != Type;
}

int CheckObjectType(context *C, GLuint H, object_type Type) {
    return CheckObjectTypeGet(C, H, Type, 0);
}

int HandledCheckProgramGet(context *C, GLuint H, object **OutObject) {
    if(CheckObjectType(C, H, object_SHADER) == 0) {
        const char *Msg = "An INVALID_OPERATION error is generated if program is the name of a shader object.";
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return 1;
    }

    if(CheckObjectTypeGet(C, H, object_PROGRAM, OutObject)) {
        const char *Msg = "An INVALID_VALUE error is generated if program is not the name of either a program or shader object.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return 1;
    }
    return 0;
}

int HandledCheckShaderGet(context *C, GLuint H, object **OutObject) {
    if(CheckObjectType(C, H, object_PROGRAM) == 0) {
        const char *Msg = "An INVALID_OPERATION error is generated if shader is the name of a program object.";
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return 1;
    }

    if(CheckObjectTypeGet(C, H, object_SHADER, OutObject)) {
        const char *Msg = "An INVALID_VALUE error is generated if shader is not the name of either a program or shader object.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return 1;
    }
    return 0;
}

// NOTE(blackedout): MARK: Helper function

void SetVertexArrayAttribEnabled(context *C, GLuint H, GLuint Index, int IsEnabled) {
    object *Object;
    if(CheckObjectTypeGet(C, H, object_VERTEX_ARRAY, &Object)) {
        const char *Msg = "An INVALID_OPERATION error is generated by EnableVertexAttribArray and DisableVertexAttribArray if no vertex array object is bound.";
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    if(Index >= VERTEX_ATTRIB_CAPACITY) {
        const char *Msg = "An INVALID_VALUE error is generated if index is greater than or equal to the value of MAX_VERTEX_ATTRIBS.";
        GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
        return;
    }

    Object->VertexArray.Attribs[Index].Enabled = IsEnabled;
}

void HandledCheckCapSet(context *C, GLenum Cap, int Enabled) {
    switch(Cap) {
    case GL_BLEND: {} break;
    case GL_COLOR_LOGIC_OP: {} break;
    case GL_CULL_FACE: { C->Config.CullingEnabled = Enabled; } break;
    case GL_DEBUG_OUTPUT: {} break;
    case GL_DEBUG_OUTPUT_SYNCHRONOUS: {} break;
    case GL_DEPTH_CLAMP: { C->Config.RasterState.depthClampEnable = Enabled; } break;
    case GL_DEPTH_TEST: { C->Config.DepthStencilState.depthTestEnable = Enabled; } break;
    case GL_DITHER: {} break; // NOTE(blackedout): Default is 1
    case GL_FRAMEBUFFER_SRGB: {} break;
    case GL_LINE_SMOOTH: {} break;
    case GL_MULTISAMPLE: {} break; // NOTE(blackedout): Default is 1
    case GL_POLYGON_OFFSET_FILL: { C->Config.PolygonOffsetFillEnabled = Enabled; } break;
    case GL_POLYGON_OFFSET_LINE: { C->Config.PolygonOffsetLineEnabled = Enabled; } break;
    case GL_POLYGON_OFFSET_POINT: { C->Config.PolygonOffsetPointEnabled = Enabled; } break;
    case GL_POLYGON_SMOOTH: {} break;
    case GL_PRIMITIVE_RESTART: {} break;
    case GL_PRIMITIVE_RESTART_FIXED_INDEX: {} break;
    case GL_RASTERIZER_DISCARD: { C->Config.RasterState.rasterizerDiscardEnable = Enabled; } break;
    case GL_SAMPLE_ALPHA_TO_COVERAGE: {} break;
    case GL_SAMPLE_ALPHA_TO_ONE: {} break;
    case GL_SAMPLE_COVERAGE: {} break;
    case GL_SAMPLE_SHADING: {} break;
    case GL_SAMPLE_MASK: {} break;
    case GL_SCISSOR_TEST: { C->Config.ScissorEnabled = Enabled; } break; // TODO
    case GL_STENCIL_TEST: { C->Config.DepthStencilState.stencilTestEnable = Enabled; } break;
    case GL_TEXTURE_CUBE_MAP_SEAMLESS: {} break;
    case GL_PROGRAM_POINT_SIZE: {} break;
    default: {
        const char *Msg = "glEnable/glDisable: invalid cap";
        GenerateErrorMsg(C, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, Msg);
    } break;
    }
}

static int VulkanCheck(context *C, VkResult Result, const char *Call) {
    if(Result != VK_SUCCESS) {
        GenerateErrorMsg(C, GL_INVALID_OPERATION, GL_DEBUG_SOURCE_API, Call);
        return 1;
    }
    return 0;
}

int CreateFramebuffer(context *C, u32 ColorImageCount) {
    int Result = 1;

    object Template = {0};
    Template.Type = object_FRAMEBUFFER;
    // TODO(blackedout): Handle frees
    Template.Framebuffer.ColorImages = malloc(ColorImageCount*sizeof(VkImage));
    Template.Framebuffer.ColorImageViews = malloc(ColorImageCount*sizeof(VkImageView));
    Template.Framebuffer.ColorDrawIndices = malloc(ColorImageCount*sizeof(u32));

    

    GLuint Index;
    if(CreateObjects(C, Template, 1, &Index)) {
        goto label_Error;
    }

    Result = 0;
    goto label_Exit;

label_Error:;
label_Exit:
    return Result;
}

static context GlobalContext = {0};

// NOTE(blackedout): MARK: Vulkan

#ifdef __APPLE__
#ifndef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
#error "Vulkan headers outdated"
#endif
#endif

static int VulkanCreateInstance(context *C, const char **RequiredInstanceExtensions, uint32_t RequiredInstanceExtensionCount) {
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

static int VulkanCreateDevice(context *C, VkSurfaceKHR Surface) {
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

typedef struct pipeline_state_clear_color {
    GLfloat r, g, b, a;
} pipeline_state_clear_color;

typedef struct pipeline_state_viewport {
    GLint x, y;
    GLsizei w, h;
} pipeline_state_viewport;

typedef struct pipeline_state_scissor {
    GLint x, y;
    GLsizei w, h;
} pipeline_state_scissor;

// NOTE(blackedout): The actual pipeline state infos depend on runtime data (physical device limits), so these params
// are used to automatically create them once the runtime data is available.
typedef struct pipeline_state_info_params {
    u32 InstanceByteCount;
    u32 LimitsByteOffset;
} pipeline_state_info_params;

typedef struct pipeline_state_info {
    u32 ByteOffset;
    u32 InstaceByteCount;
    u32 InstanceCount;
} pipeline_state_info;

typedef enum pipeline_state_type {
    pipeline_state_CLEAR_COLOR,
    pipeline_state_VIEWPORT,
    pipeline_state_SCISSOR,

    pipeline_state_COUNT
} pipeline_state_type;

// NOTE(blackedout): See https://docs.vulkan.org/spec/latest/chapters/limits.html (2025-11-13)
static pipeline_state_info_params PipelineStateInfoParams[] = {
#define MakeStaticEntry(EnumType, StateType) [(EnumType)] = { sizeof(StateType), ~((u32)0) }
#define MakeDynamicEntry(EnumType, StateType, LimitsByteOffset) [(EnumType)] = { sizeof(StateType), (u32)(LimitsByteOffset) }

    MakeStaticEntry(pipeline_state_CLEAR_COLOR, pipeline_state_clear_color),
    MakeDynamicEntry(pipeline_state_VIEWPORT, pipeline_state_viewport, offsetof(VkPhysicalDeviceLimits,  maxViewports)),
    MakeStaticEntry(pipeline_state_SCISSOR, pipeline_state_scissor),

#undef MakeDynamicEntry
#undef MakeStaticEntry
};

void SetPipelineStateInfos(pipeline_state_info *Infos, const VkPhysicalDeviceLimits *Limits, u32 *OutByteCount) {
    u32 ByteOffset = 0;
    for(u32 I = 0; I < ArrayCount(PipelineStateInfoParams); ++I) {
        pipeline_state_info_params Params = PipelineStateInfoParams[I];
        u32 InstanceCount = 1;
        if(Params.LimitsByteOffset != (~((u32)0))) {
            InstanceCount = *(uint32_t *)(((const u8 *)Limits) + Params.LimitsByteOffset);
        }

        pipeline_state_info Info = {
            .ByteOffset = ByteOffset,
            .InstaceByteCount = Params.InstanceByteCount,
            .InstanceCount = InstanceCount,
        };
        Infos[I] = Info;
        ByteOffset += Info.InstaceByteCount*Info.InstanceCount;
    }
    *OutByteCount = ByteOffset;
}

// NOTE(blackedout):
// command categories
// - just change the state (e.g. ClearColor)
// 1: change the curent state
// - use portion of state (e.g. Clear: affected by framebuffer, draw buffers, clear color, scissor)
// 1: select a saved state that matches the current state, if none exists save the current state and use that
// 2: mark the portions of state that the command depends on as fixed
// 3: record the command with a reference to the state it uses

#endif