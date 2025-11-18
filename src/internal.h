#ifndef CUGL_INTERNAL_H
#define CUGL_INTERNAL_H

// MARK: COMMON

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cugl/cugl.h"
#include "shader_interface.h"
#include "vk_mem_alloc.h"
#include "vulkan/vulkan_core.h"

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

#define Concat1(A, B) A ## B
#define Concat2(A, B) Concat1(A, B)
#define StaticAssert(Condition) struct Concat2(sas_, __LINE__) { int A[-((Condition) == 0)]; } 
#define Assert(X) if((X) == 0) __builtin_debugtrap()
#define ArrayCount(X) (sizeof(X)/sizeof(*(X)))
#define ClampAB(X, A, B) ((X) < (A) ? (A) : ((X) > (B) ? (B) : (X)))
#define Clamp01(X) ClampAB(X, 0, 1)

// MARK: UTIL

typedef struct array {
    u64 Capacity;
    u64 Count;
    void *Data;
} array;
#define array(T) array

#define ArrayData(Type, Array) ((Type *)((Array).Data))
int ArrayRequireRoom(array *Array, u64 Count, u64 InstanceByteCount, u64 InitialCapacity);
void ArrayClear(array *Array, u64 InstanceByteCount);

#if 0
typedef struct free_array {
    array Array;
    u64 FreeCount;
    u64 NextFreeIndex;
} free_array;

#define FreeArrayData(Type, FreeArray) ArrayData(Type, (FreeArray).Array)
#endif

void SetFlagsEnabledU32(u32 *InOut, u32 Mask, int DoEnable);
void SetFlagsU32(u32 *InOut, u32 Mask, u32 Bits);

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

typedef struct vertex_input_attribute_size_type_info {
    u32 ByteCount;
    VkFormat VulkanFormat;
} vertex_input_attribute_size_type_info;

typedef struct shader_type_info {
    shader_type InterfaceType;
    VkShaderStageFlags VulkanBit;
} shader_type_info;

enum color_attachment_info_flags {
    color_attachment_info_IS_NONE = 0x01,
    color_attachment_info_IS_INVALID_FOR_DEFAULT_FBO = 0x02,
    color_attachment_info_IS_INVALID_FOR_OTHER_FBO = 0x04,
};

typedef struct color_attachment_info {
    u32 Flags;
    u32 Index;
} color_attachment_info;

#define BUFFER_TARGET_COUNT (15)
#define TEXTURE_SLOT_COUNT (128)
#define TEXTURE_TARGET_COUNT (11)

int GetBufferTargetInfo(GLenum Target, buffer_target_info *OutInfo);
int GetTextureTargetInfo(GLenum Target, texture_target_info *OutInfo);
int GetPrimitiveInfo(GLenum Mode, primitive_info *OutInfo);
int GetVulkanBlendOp(GLenum mode, VkBlendOp *OutBlendOp);
int GetVulkanBlendFactor(GLenum factor, VkBlendFactor *OutBlendFactor);
int GetVertexInputAttributeSizeTypeInfo(GLint Size, GLenum Type, vertex_input_attribute_size_type_info *OutInfo);
int GetShaderTypeInfo(GLenum type, shader_type_info *OutInfo);
int GetColorAttachmentInfo(GLenum buf, u32 MaxColorAttachmentCount, color_attachment_info *OutInfo);

// MARK: CORE

#define INITIAL_OBJECT_CAPACITY (1024)
#define INITIAL_COMMAND_CAPACITY (1024)
#define INITIAL_PIPELINE_STATE_CAPACITY (8)

#define VulkanCheckGoto(Call, Label) if(VulkanCheck(C, Call, #Call)) goto Label;
#define VulkanCheckReturn(Call) if(VulkanCheck(C, Call, #Call)) return;

enum vertex_array_attribute_flags {
    vertex_input_attribute_INTEGER_HANDLING_ENABLED = 0x01,
    vertex_input_attribute_INTEGER_NORMALIZE = 0x02,
    vertex_input_attribute_IS_ENABLED = 0x04,

    vertex_input_attribute_INTEGER_MASK = vertex_input_attribute_INTEGER_HANDLING_ENABLED | vertex_input_attribute_INTEGER_NORMALIZE,
};

typedef struct vertex_array_attribute {
    u32 Flags;
    GLint Size;
    GLenum Type;
    GLuint RelativeOffset;
    u32 BindingIndex;
} vertex_array_attribute;

typedef struct vertex_array_binding {
    GLuint Vbo;
    GLintptr Offset;
    GLsizei Stride;
} vertex_array_binding;

typedef enum object_type {
    object_NONE = 0,
    object_BUFFER,
    object_FRAMEBUFFER,
    object_PROGRAM,
    object_PROGRAM_PIPELINE,
    object_QUERY,
    object_RENDERBUFFER,
    object_SAMPLER,
    object_SHADER,
    object_SYNC,
    object_TEXTURE,
    object_TRANSFORM_FEEDBACK,
    object_VERTEX_ARRAY,
} object_type;

#define PROGRAM_SHADER_CAPACITY (6)

#if 0
typedef struct subpass_attachment_reference {
    // NOTE(blackedout): Index into `Framebuffer.ColorAttachments`
    // Currently equivalent to the i of GL_COLOR_ATTACHMENTi (GL_NONE is ~0)
    u32 Key;
} subpass_attachment_reference;
#endif

typedef struct render_pass_state_subpass {
    u32 BaseIndex;
    u32 ColorAttachmentCount;
} render_pass_state_subpass;

typedef struct framebuffer_attachment {
    // NOTE(blackedout): Fetch location is the index of this in the array
    int IsDrawBuffer;
    GLuint Rbo;
} framebuffer_attachment;

typedef struct object {
    object_type Type;
    int IsCreated;

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
            VkImage Image;
            VkImageView ImageView;
        } Renderbuffer;
        struct {
            u32 MaxColorAttachmentRange;
            // NOTE(blackedout): This value minus 1 is the last attachment that is set. All remaining ones are unused.
            u32 ColorAttachmentRange;
            u32 ColorAttachmentCapacity;
            framebuffer_attachment *ColorAttachments;
            framebuffer_attachment DepthAttachment;
            framebuffer_attachment StencilAttachment;
            
            array(render_pass_state_subpass) Subpasses;
            array(VkAttachmentReference) SubpassAttachments;
            VkRenderPass RenderPass;
            VkFramebuffer Framebuffer;
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
            vertex_array_attribute *InputAttributes;
            vertex_array_binding *InputBindings;
        } VertexArray;
    };
} object;

typedef enum command_type {
    command_NONE = 0,
    command_CLEAR,
    command_DRAW,
    command_BIND_VERTEX_BUFFER,
    command_BIND_PIPELINE,
    command_BEGIN_RENDER_PASS,
    command_NEXT_SUBPASS
} command_type;

typedef struct command {
    command_type Type;

    union {
        struct {
            u32 BindingIndex;
            VkBuffer Buffer;
            VkDeviceSize Offset;
        } BindVertexBuffer;
        struct {
            u32 PipelineIndex;
        } BindPipeline;
        struct {

        } Clear;
        struct {
            u64 VertexCount;
            u64 VertexOffset;
            u64 InstanceCount;
            u64 InstanceOffset;
        } Draw;
        struct {
            GLuint Fbo;
        } BeginRenderPass;
        struct {
            u32 SubpassIndex;
        } NextSubpass;
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

enum {
    framebuffer_READ = 0,
    framebuffer_WRITE,
    framebuffer_COUNT,
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

typedef enum gl_error_type {
    gl_error_OUT_OF_MEMORY,

    gl_error_ACQUIRE_CONTEXT,
    gl_error_RELEASE_CONTEXT,

    gl_error_N_NEGATIVE,

    gl_error_PROGRAM_IS_SHADER,
    gl_error_PROGRAM_INVALID,
    gl_error_PROGRAM_NOT_LINkED_SUCCESSFULLY,

    gl_error_SHADER_TYPE,
    gl_error_SHADER_IS_PROGRAM,
    gl_error_SHADER_INVALID,

    gl_error_VERTEX_ATTRIB_FORMAT_NONE_BOUND,
    gl_error_VERTEX_ARRAY_ATTRIB_FORMAT_VAO_INVALID,
    gl_error_VERTEX_ARRAY_ATTRIB_FORMAT_INDEX,
    gl_error_VERTEX_ARRAY_ATTRIB_FORMAT_SIZE,
    gl_error_VERTEX_ARRAY_ATTRIB_FORMAT_TYPE,
    // TODO(blackedout): Page 354

    gl_error_BIND_VERTEX_BUFFER_NONE_BOUND,
    gl_error_VERTEX_ARRAY_VERTEX_BUFFER_VAO_INVALID,
    gl_error_VERTEX_ARRAY_VERTEX_BUFFER_VBO_INVALID,
    gl_error_VERTEX_ARRAY_VERTEX_BUFFER_INDEX,
    gl_error_VERTEX_ARRAY_VERTEX_BUFFER_STRIDE_OFFSET,

    gl_error_BIND_VERTEX_BUFFERS_NONE_BOUND,
    gl_error_VERTEX_ARRAY_VERTEX_BUFFERS_VAO_INVALID,
    gl_error_VERTEX_ARRAY_VERTEX_BUFFERS_BINDING_INDEX,
    gl_error_VERTEX_ARRAY_VERTEX_BUFFERS_COUNT_NEGATIVE,
    gl_error_VERTEX_ARRAY_VERTEX_BUFFERS_VBO_INVALID,
    gl_error_VERTEX_ARRAY_VERTEX_BUFFERS_STRIDE_OFFSET,

    gl_error_VERTEX_ATTRIB_BINDING_NONE_BOUND,
    gl_error_VERTEX_ARRAY_ATTRIB_BINDING_VAO_INVALID,
    gl_error_VERTEX_ARRAY_ATTRIB_BINDING_ATTRIB_INDEX,
    gl_error_VERTEX_ARRAY_ATTRIB_BINDING_BINDING_INDEX,

    gl_error_VERTEX_ATTRIB_POINTER_NONE_BOUND,

    gl_error_XABLE_VERTEX_ATTRIB_ARRAY_NONE_BOUND,
    gl_error_XABLE_VERTEX_ARRAY_ATTRIB_INVALID,
    gl_error_XABLE_VERTEX_ATTRIB_INDEX,

    gl_error_DRAW_MODE,
    gl_error_DRAW_FIRST_NEGATIVE,
    gl_error_DRAW_COUNT_NEGATIVE,
    gl_error_DRAW_NO_VAO_BOUND,

    gl_error_VERTEX_ARRAY_INVALID,
    gl_error_NO_VERTEX_ARRAY,
    gl_error_VERTEX_INPUT_ATTRIBUTE_INDEX,
    gl_error_VERTEX_INPUT_BINDING_INDEX,
    gl_error_VERTEX_INPUT_ATTRIBUTE_STRIDE_NEGATIVE,
    gl_error_VERTEX_INPUT_ATTRIBUTE_STRIDE_MAX,

    gl_error_DRAW_BUFFER_BUF_INVALID,
    gl_error_DRAW_BUFFER_BUF_INVALID_DEFAULT_FRAMEBUFFER,
    gl_error_DRAW_BUFFER_BUF_INVALID_OTHER_FRAMEBUFFER,

    gl_error_LINE_WIDTH_LE_ZERO,

    gl_error_VIEWPORT_INDEX,
    gl_error_VIEWPORT_COUNT_NEGATIVE,

    gl_error_SCISSOR_INDEX,
    gl_error_SCISSOR_COUNT_NEGATIVE,
    gl_error_SCISSOR_WIDTH_HEIGHT_NEGATIVE,

    gl_error_COUNT
} gl_error_type;

typedef enum pipeline_state_type {
    pipeline_state_HEADER,
    pipeline_state_CLEAR_COLOR,
    pipeline_state_CLEAR_DEPTH,
    pipeline_state_CLEAR_STENCIL,
    pipeline_state_VIEWPORT,
    pipeline_state_SCISSOR,

    pipeline_state_FRAMEBUFFER,
    pipeline_state_DRAW_BUFFERS,

    pipeline_state_VERTEX_INPUT_ATTRIBUTES,
    pipeline_state_VERTEX_INPUT_BINDINGS,

    pipeline_state_PROGRAM,

    pipeline_state_PRIMITIVE_TYPE,

    pipeline_state_COUNT
} pipeline_state_type;

typedef struct pipeline_state_info {
    u32 ByteOffset;
    u32 InstaceByteCount;
    u32 InstanceCount;
} pipeline_state_info;

typedef struct context {
    // NOTE(blackedout): Contiguous free list
    array Objects;
    u64 FreeObjectCount;
    u64 NextFreeObjectIndex;

    array Commands;

    GLenum ErrorFlag;
    GLDEBUGPROC DebugCallback;
    const void *DebugCallbackUser;

    GLuint BoundBuffers[BUFFER_TARGET_COUNT];
    GLuint BoundFramebuffers[framebuffer_COUNT];
    GLuint BoundTextures[TEXTURE_TARGET_COUNT*TEXTURE_SLOT_COUNT];
    u32 ActiveTextureIndex;
    GLuint ActiveProgram;

    // NOTE(blackedout): Virtual state
    GLuint BoundVao;
    GLuint BoundReadFbo, BoundDrawFbo;

    config Config;

    VkInstance Instance;
    VkSurfaceKHR Surface;
    VkDevice Device;
    device_info DeviceInfo;
    VmaAllocator Allocator;
    VkSwapchainKHR Swapchain;
    u32 SwapchainImageCount;
    VkImage *SwapchainImages;
    VkImageView *SwapchainImageViews;
    VkFramebuffer *SwapchainFramebuffers;
    VkCommandPool GraphicsCommandPool;

    u32 SemaphoreCount;
    VkSemaphore *Semaphores;
    
    VkCommandBuffer CommandBuffers[command_buffer_COUNT];
    VkFence Fences[fence_COUNT];

    pipeline_state_info PipelineStateInfos[pipeline_state_COUNT];
    u32 PipelineStateByteCount;
    // NOTE(blackedout): [0] is always the current pipeline state
    array PipelineStates;

    int IsPipelineSet;
    u32 LastPipelineIndex;

    array TmpSubpasses;

    VkVertexInputBindingDescription *VertexInputBindingDescriptions;
    VkVertexInputAttributeDescription *VertexInputAttributeDescriptions;
} context;

int AcquireContext(context **OutC, const char *Name);
int ReleaseContext(context *C, const char *Name);

void GenerateErrorMsg(context *C, GLenum Error, GLenum Source, const char *Msg);
void GenerateError(context *C, gl_error_type Type, const char *FunctionName);
#define CheckGL(X, ErrorType, ...) do { if(X) { GenerateError(C, ErrorType, Name); if(C) { ReleaseContext(C, Name); } return __VA_ARGS__; } } while(0)
void GenerateOther(context *C, GLenum Source, const char *Msg);
int RequireRoomForNewObjects(context *C, u64 Count);
int RequireRoomForNewCommands(context *C, u64 Count);
int PushCommand(context *C, command Command);

// NOTE(blackedout): Return the handle of and pointer to a new zeroed out object.
// IMPORTANT: The caller must ensure there is enough space to perform this operation voa `ArrayRequireRoom`.
void GenObject(context *C, GLuint *OutHandle, object **OutObject);

// NOTE(blackedout): Create the given object by allocating its dynamic state.
// IMPORTANT: `Object->Type` must be set before calling this function. `Object->IsCreated` must be zero.
// Returns one on out of memory error, zero on success.
int CreateObject(context *C, object *Object);

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
int HandledCheckProgramGet(context *C, GLuint H, object **OutObject, const char *Name);
int HandledCheckShaderGet(context *C, GLuint H, object **OutObject, const char *Name);

void SetVertexInputAttributeFormat(context *C, object *Object, int IsCurrent, u32 Index, GLint Size, GLenum Type, GLuint RelativeOffset, u32 IntegerHandlingBits, const char *Name);
void SetVertexInputBinding(context *C, object *Object, int IsCurrent, u32 Index, GLuint Vbo, GLintptr Offset, GLsizei Stride, const char *Name);
void SetVertexInputAttributeBinding(context *C, object *Object, int IsCurrent, u32 AttributeIndex, u32 BindingIndex, const char *Name);
void SetVertexInputAttributePointer(context *C, object *Object, u32 Index, GLint Size, GLenum Type, GLsizei Stride, const void *Pointer, u32 IntegerHandlingBits, const char *Name);
void SetVertexInputAttributeEnabled(context *C, object *Object, int IsCurrent, GLuint Index, int IsEnabled, const char *Name);

void NoContextGenObjects(GLsizei Count, object_type Type, GLuint *OutHandles, const char *Name);
void NoContextCreateObjects(GLsizei Count, object_type Type, GLuint *OutHandles, const char *Name);
GLboolean NoContextIsObjectType(GLuint H, object_type Type, const char *Name);

void HandledCheckCapSet(context *C, GLenum Cap, int Enabled);
int VulkanCheck(context *C, VkResult Result, const char *Call);

int CheckFramebuffer(context *C, GLuint Fbo);
int PotentiallySaveSubpass(context *C);

typedef struct pipeline_state_header {
    int IsCreated;
    VkPipelineLayout Layout;
    VkPipeline Pipeline;
} pipeline_state_header;

typedef struct pipeline_state_clear_color {
    GLfloat R, G, B, A;
} pipeline_state_clear_color;

typedef GLfloat pipeline_state_clear_depth;
typedef GLint pipeline_state_clear_stencil;

typedef VkViewport pipeline_state_viewport;
typedef VkRect2D pipeline_state_scissor;

typedef struct pipeline_state_framebuffer {
    GLuint ReadFbo, DrawFbo;
} pipeline_state_framebuffer;

typedef struct pipeline_state_draw_buffer {
    GLenum Target;
} pipeline_state_draw_buffer;

typedef vertex_array_attribute pipeline_state_vertex_input_attribute;

typedef struct pipeline_state_vertex_input_binding {
    int IsEnabled;
    GLsizei Stride;
} pipeline_state_vertex_input_binding;

typedef struct pipeline_state_program {
    GLuint Program;
    GLuint Pipeline;
} pipeline_state_program;

typedef struct pipeline_state_primitive_type {
    GLenum Type;
} pipeline_state_primitive_type;

// NOTE(blackedout): The actual pipeline state infos depend on runtime data (physical device limits), so these params
// are used to automatically create them once the runtime data is available.
typedef struct pipeline_state_info_params {
    u32 InstanceByteCount;
    u32 LimitsByteOffset;
} pipeline_state_info_params;

int CreatePipelineStates(context *C, const VkPhysicalDeviceLimits *Limits);
void *GetPipelineState(context *C, u64 StateIndex, pipeline_state_type Type);
void ClearPipelineState(context *C, u64 StateIndex, pipeline_state_type Type);
void CopyPipelineStateToPtr(context *C, void *DstState, u64 SrcStateIndex, pipeline_state_type Type);
void CopyPipelineStateFromPtr(context *C, u64 DstStateIndex, void *SrcState, pipeline_state_type Type);
#define GetCurrentPipelineState(C, Type) GetPipelineState(C, 0, Type)
#define ClearCurrentPipelineState(C, Type) ClearPipelineState(C, 0, Type)
#define CurrentPipelineStateToPtr(C, DstState, Type) CopyPipelineStateToPtr(C, DstState, 0, Type)
#define CurrentPipelineStateFromPtr(C, SrcState, Type) CopyPipelineStateFromPtr(C, 0, SrcState, Type)

int UseCurrentPipelineState(context *C, u32 Count, pipeline_state_type *Types);
void SetDefaultPipelineState(context *C, u32 PipelineIndex);
int CheckPipeline(context *C, u32 Index);

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