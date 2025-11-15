#ifndef CUGL_INTERNAL_H
#define CUGL_INTERNAL_H

// MARK: COMMON

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cugl/cugl.h"
#include "shader_interface.h"
#include "vk_mem_alloc.h"

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

#define Assert(X) if((X) == 0) __builtin_debugtrap()
#define ArrayCount(X) (sizeof(X)/sizeof(*(X)))
#define ClampAB(X, A, B) ((X) < (A) ? (A) : ((X) > (B) ? (B) : (X)))
#define Clamp01(X) ClampAB(X, 0, 1)

// MARK: GL ENUM INFO

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

typedef struct texture_target_info {
    u32 Index;
    texture_dimension Dimension;
} texture_target_info;

typedef struct primitive_info {
    VkPrimitiveTopology VulkanPrimitve;
} primitive_info;

typedef struct shader_type_info {
    shader_type InterfaceType;
    VkShaderStageFlags VulkanBit;
} shader_type_info;

#define BUFFER_TARGET_COUNT (15)
#define TEXTURE_SLOT_COUNT (128)
#define TEXTURE_TARGET_COUNT (11)

int GetBufferTargetInfo(GLenum Target, buffer_target_info *OutInfo);
int GetTextureTargetInfo(GLenum Target, texture_target_info *OutInfo);
int GetPrimitiveInfo(GLenum Mode, primitive_info *OutInfo);
int GetVulkanBlendOp(GLenum mode, VkBlendOp *OutBlendOp);
int GetVulkanBlendFactor(GLenum factor, VkBlendFactor *OutBlendFactor);
int GetShaderTypeInfo(GLenum type, shader_type_info *OutInfo);

// MARK: CORE

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

config GetDefaultConfig(int Width, int Height);

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

void GenerateErrorMsg(context *C, GLenum Error, GLenum Source, const char *Msg);
void GenerateOther(context *C, GLenum Source, const char *Msg);
int RequireRoomForNewObjects(context *Context, u64 Count);
int RequireRoomForNewCommands(context *C, u64 Count);
int PushCommand(context *C, command Command);
int CreateObjects(context *Context, object Template, u64 Count, GLuint *OutHandles);
int CreateObjectsSizei(context *Context, object Template, GLsizei Count, GLuint *OutHandles);
void DeleteObject(context *C, object *Object);
void DeleteObjects(context *Context, u64 Count, const GLuint *Handles, object_type ExpectedType);
void DeleteObjectsSizei(context *Context, GLsizei Count, const GLuint *Handles, object_type ExpectedType);
void DeleteShader(context *C, object *Object);
void DeleteProgram(context *C, object *Object);
// NOTE(blackedout): Unsafe function, do not expose to raw user input
void GetObject(context *C, GLuint H, object **OutObject);

int CheckObjectCreated(context *C, GLuint H);
int CheckObjectTypeGet(context *C, GLuint H, object_type Type, object **OutObject);
int CheckObjectType(context *C, GLuint H, object_type Type);
int HandledCheckProgramGet(context *C, GLuint H, object **OutObject);
int HandledCheckShaderGet(context *C, GLuint H, object **OutObject);

void SetVertexArrayAttribEnabled(context *C, GLuint H, GLuint Index, int IsEnabled);
void HandledCheckCapSet(context *C, GLenum Cap, int Enabled);
int VulkanCheck(context *C, VkResult Result, const char *Call);
int CreateFramebuffer(context *C, u32 ColorImageCount);

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

void SetPipelineStateInfos(pipeline_state_info *Infos, const VkPhysicalDeviceLimits *Limits, u32 *OutByteCount);

// NOTE(blackedout):
// command categories
// - just change the state (e.g. ClearColor)
// 1: change the curent state
// - use portion of state (e.g. Clear: affected by framebuffer, draw buffers, clear color, scissor)
// 1: select a saved state that matches the current state, if none exists save the current state and use that
// 2: mark the portions of state that the command depends on as fixed
// 3: record the command with a reference to the state it uses

// MARK: VULKAN HELPERS

int VulkanCreateInstance(context *C, const char **RequiredInstanceExtensions, uint32_t RequiredInstanceExtensionCount);
int VulkanCreateDevice(context *C, VkSurfaceKHR Surface);

#endif