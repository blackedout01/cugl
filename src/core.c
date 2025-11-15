#include "internal.h"

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

int VulkanCheck(context *C, VkResult Result, const char *Call) {
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