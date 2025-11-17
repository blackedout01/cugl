#include "cugl/cugl.h"
#include "internal.h"
#include "vulkan/vulkan_core.h"

context GlobalContext = {0};

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

int AcquireContext(context **OutC, const char *Name) {
    *OutC = &GlobalContext;
    return 0;
}
int ReleaseContext(context *C, const char *Name) {
    return 0;
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

void GenerateError(context *C, gl_error_type Type, const char *FunctionName) {
    switch(Type) {
#define MakeCaseApp(K, E, M) case (K): GenerateErrorMsg(C, E, GL_DEBUG_SOURCE_APPLICATION, M); break
#define MakeCaseApi(K, E, M) case (K): GenerateErrorMsg(C, E, GL_DEBUG_SOURCE_API, M); break
    MakeCaseApp(gl_error_N_NEGATIVE, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if n is negative.");

    MakeCaseApp(gl_error_PROGRAM_IS_SHADER, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated if program is not zero and is the name of a shader object.");
    MakeCaseApp(gl_error_PROGRAM_INVALID, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if program is neither zero nor the name of either a program or shader object.");
    MakeCaseApp(gl_error_PROGRAM_NOT_LINkED_SUCCESSFULLY, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated if program has not been linked successfully. The current rendering state is not modified.");
    
    MakeCaseApp(gl_error_SHADER_IS_PROGRAM, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated if shader is the name of a program object.");
    MakeCaseApp(gl_error_SHADER_INVALID, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if shader is not the name of either a program or shader object.");

    MakeCaseApp(gl_error_VERTEX_ATTRIB_FORMAT_NONE_BOUND, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated by VertexAttrib*Format if no vertex array object is currently bound (see section 10.3.1).");
    MakeCaseApp(gl_error_VERTEX_ARRAY_ATTRIB_FORMAT_VAO_INVALID, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated by VertexArrayAttrib*Format if vaobj is not the name of an existing vertex array object.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_ATTRIB_FORMAT_INDEX, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if attribindex is greater than or equal to the value of MAX_VERTEX_ATTRIBS.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_ATTRIB_FORMAT_SIZE, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if size is not one of the values shown in table 10.3 for the corresponding command.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_ATTRIB_FORMAT_TYPE, GL_INVALID_ENUM, "An INVALID_ENUM error is generated if type is not one of the parameter token names from table 8.2 corresponding to one of the allowed GL data types for that command as shown in table 10.3.");

    MakeCaseApp(gl_error_BIND_VERTEX_BUFFER_NONE_BOUND, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated by BindVertexBuffer if no vertex array object is bound.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_VERTEX_BUFFER_VAO_INVALID, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated by VertexArrayVertexBuffer if vaobj is not the name of an existing vertex array object.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_VERTEX_BUFFER_VBO_INVALID, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated if buffer is not zero or a name returned from a previous call to GenBuffers or CreateBuffers, or if such a name has since been deleted with DeleteBuffers.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_VERTEX_BUFFER_INDEX, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if bindingindex is greater than or equal to the value of MAX_VERTEX_ATTRIB_BINDINGS.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_VERTEX_BUFFER_STRIDE_OFFSET, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if stride or offset is negative, or if stride is greater than the value of MAX_VERTEX_ATTRIB_STRIDE.");

    MakeCaseApp(gl_error_BIND_VERTEX_BUFFERS_NONE_BOUND, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated by BindVertexBuffers if no vertex array object is bound.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_VERTEX_BUFFERS_VAO_INVALID, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated by VertexArrayVertexBuffers if vaobj is not the name of an existing vertex array object.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_VERTEX_BUFFERS_BINDING_INDEX, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated if first+ countis greater than the value of MAX_VERTEX_ATTRIB_BINDINGS.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_VERTEX_BUFFERS_COUNT_NEGATIVE, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if count is negative.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_VERTEX_BUFFERS_VBO_INVALID, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated if any value in buffers is not zero or the name of an existing buffer object (per binding).");
    MakeCaseApp(gl_error_VERTEX_ARRAY_VERTEX_BUFFERS_STRIDE_OFFSET, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if any value in offsets or strides is negative, or if any value in strides is greater than the value of MAX_VERTEX_ATTRIB_STRIDE (per binding).");

    MakeCaseApp(gl_error_VERTEX_ATTRIB_BINDING_NONE_BOUND, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated if no vertex array object is bound.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_ATTRIB_BINDING_VAO_INVALID, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated by VertexArrayAttribBinding if vaobj is not the name of an existing vertex array object.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_ATTRIB_BINDING_ATTRIB_INDEX, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if attribindex is greater than or equal to the value of MAX_VERTEX_ATTRIBS.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_ATTRIB_BINDING_BINDING_INDEX, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if bindingindex is greater than or equal to the value of MAX_VERTEX_ATTRIB_BINDINGS.");

    MakeCaseApp(gl_error_VERTEX_ATTRIB_POINTER_NONE_BOUND, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated if no vertex array object is bound.");

    MakeCaseApp(gl_error_XABLE_VERTEX_ATTRIB_ARRAY_NONE_BOUND, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated by EnableVertexAttribArray and DisableVertexAttribArray if no vertex array object is bound.");
    MakeCaseApp(gl_error_XABLE_VERTEX_ARRAY_ATTRIB_INVALID, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated by EnableVertexArrayAttrib and DisableVertexArrayAttrib if vaobj is not the name of an existing vertex array object.");
    MakeCaseApp(gl_error_XABLE_VERTEX_ATTRIB_INDEX, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if index is greater than or equal to the value of MAX_VERTEX_ATTRIBS.");

    MakeCaseApp(gl_error_DRAW_MODE, GL_INVALID_ENUM, "An INVALID_ENUM error is generated if mode is not one of the primitive types defined in section 10.1.");
    MakeCaseApp(gl_error_DRAW_FIRST_NEGATIVE, GL_INVALID_ENUM, "Specifying first < 0 results in undefined behavior. Generating an INVALID_VALUE error is recommended in this case.");
    MakeCaseApp(gl_error_DRAW_COUNT_NEGATIVE, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if count is negative.");
    MakeCaseApp(gl_error_DRAW_NO_VAO_BOUND, GL_INVALID_ENUM, "");

    MakeCaseApp(gl_error_NO_VERTEX_ARRAY, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated if no vertex array object is bound.");
    MakeCaseApp(gl_error_VERTEX_ARRAY_INVALID, GL_INVALID_OPERATION, "An INVALID_OPERATION error is generated if array is not zero or a name returned from a previous call to CreateVertexArrays or GenVertexArrays, or if such a name has since been deleted with DeleteVertexArrays.");
    MakeCaseApp(gl_error_VERTEX_INPUT_ATTRIBUTE_INDEX, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if attribindex is greater than or equal to the value of MAX_VERTEX_ATTRIBS.");
    MakeCaseApp(gl_error_VERTEX_INPUT_BINDING_INDEX, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if bindingindex is greater than or equal to the value of MAX_VERTEX_ATTRIB_BINDINGS.");
    MakeCaseApp(gl_error_VERTEX_INPUT_ATTRIBUTE_STRIDE_NEGATIVE, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if stride is negative.");
    MakeCaseApp(gl_error_VERTEX_INPUT_ATTRIBUTE_STRIDE_MAX, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if stride is greater than the value of MAX_VERTEX_ATTRIB_STRIDE.");

    MakeCaseApp(gl_error_LINE_WIDTH_LE_ZERO, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if width is less than or equal to zero.");
    MakeCaseApp(gl_error_VIEWPORT_INDEX, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if first + count is greater than the value of MAX_VIEWPORTS.");
    MakeCaseApp(gl_error_VIEWPORT_COUNT_NEGATIVE, GL_INVALID_VALUE, "An INVALID_VALUE error is generated if count is negative.");
    MakeCaseApp(gl_error_SCISSOR_INDEX, GL_INVALID_VALUE, "An INVALID_VALUE error is generated by ScissorArrayv if first + countis greater than the value of MAX_VIEWPORTS.");
    MakeCaseApp(gl_error_SCISSOR_COUNT_NEGATIVE, GL_INVALID_VALUE, "An INVALID_VALUE error is generated by ScissorArrayv if count is negative.");
    MakeCaseApp(gl_error_SCISSOR_WIDTH_HEIGHT_NEGATIVE, GL_INVALID_VALUE, "An INVALID_VALUE error is generated by ScissorIndexed and Scissor if width or height is negative.");

    default: Assert(0); break;
#undef MakeCaseApi
#undef MakeCaseApp
    }
}

void GenerateOther(context *C, GLenum Source, const char *Msg) {
    if(C->DebugCallback == 0) {
        return;
    }
    C->DebugCallback(Source, GL_DEBUG_TYPE_OTHER, 0, 0, strlen(Msg), Msg, C->DebugCallbackUser);
}

int RequireRoomForNewObjects(context *C, u64 Count) {
    if(ArrayRequireRoom(&C->Objects, Count, sizeof(object), INITIAL_OBJECT_CAPACITY)) {
        GenerateErrorMsg(C, GL_OUT_OF_MEMORY, GL_DEBUG_SOURCE_API, "");
        return 1;
    }
    return 0;
}

int RequireRoomForNewCommands(context *C, u64 Count) {
    if(ArrayRequireRoom(&C->Commands, Count, sizeof(command), INITIAL_COMMAND_CAPACITY)) {
        GenerateErrorMsg(C, GL_OUT_OF_MEMORY, GL_DEBUG_SOURCE_API, "");
        return 1;
    }
    return 0;
}

int PushCommand(context *C, command Command) {
    if(RequireRoomForNewCommands(C, 1)) {
        return 1;
    }
    ArrayData(command, C->Commands)[C->Commands.Count++] = Command;

    return 0;
}

int CreateObjects(context *Context, object Template, u64 Count, GLuint *OutHandles) {
    if(RequireRoomForNewObjects(Context, Count)) {
        return 1;
    }

    object *Objects = Context->Objects.Data;
    for(u64 I = 0; I < Count; ++I) {
        u64 Index = Context->NextFreeObjectIndex;

        if(Objects[Index].Type != object_NONE) {
            GenerateErrorMsg(Context, 0, GL_DEBUG_SOURCE_API, "");
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
        
        ++Context->Objects.Count;
    }

    return 0;
}

int CreateObjectsSizei(context *Context, object Template, GLsizei Count, GLuint *OutHandles) {
    if(Count < 0) {
        GenerateErrorMsg(Context, GL_INVALID_ENUM, GL_DEBUG_SOURCE_APPLICATION, "");
        return 1;
    }

    return CreateObjects(Context, Template, (u64)Count, OutHandles);
}

void DeleteObject(context *C, object *Object) {
    object *Objects = C->Objects.Data;
    u64 Index = Object - Objects;
    Assert(0 <= Index && Index < C->Objects.Capacity);
    Assert(Object->Type != object_NONE);

    Object->Type = object_NONE;
    Object->None.NextFreeIndex = C->NextFreeObjectIndex;
    C->NextFreeObjectIndex = Index;
    ++C->FreeObjectCount;
    ++C->Objects.Count;
}

void DeleteObjects(context *Context, u64 Count, const GLuint *Handles, object_type ExpectedType) {
    object *Objects = Context->Objects.Data;
    u64 Capacity = Context->Objects.Capacity;
    for(u64 I = 0; I < Count; ++I) {
        u64 Index = (u64)Handles[I];

        if(Index >= Capacity || Objects[Index].Type == object_NONE) {
            // NOTE(blackedout): Silently ignore, as specified
            continue;
        }

        if(Objects[Index].Type != ExpectedType) {
            GenerateErrorMsg(Context, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, "");
        }

        DeleteObject(Context, Objects + Index);
    }
}

void DeleteObjectsSizei(context *Context, GLsizei Count, const GLuint *Handles, object_type ExpectedType) {
    if(Count < 0) {
        GenerateErrorMsg(Context, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, "");
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
    object *Objects = C->Objects.Data;
    Assert(H < C->Objects.Capacity);
    u64 Index = (u64)H;
    *OutObject = Objects + Index;
}

// NOTE(blackedout): MARK: Check functions

int CheckObjectCreated(context *C, GLuint H) {
    if(H >= C->Objects.Capacity) {
        return 1;
    }
    object *Object = 0;
    GetObject(C, H, &Object);
    return Object->Type == object_NONE;
}

int CheckObjectTypeGet(context *C, GLuint H, object_type Type, object **OutObject) {
    if(H >= C->Objects.Capacity) {
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

int HandledCheckProgramGet(context *C, GLuint H, object **OutObject, const char *Name) {
    CheckGL(0 == CheckObjectType(C, H, object_SHADER), gl_error_PROGRAM_IS_SHADER, 1);
    CheckGL(CheckObjectTypeGet(C, H, object_PROGRAM, OutObject), gl_error_PROGRAM_INVALID, 1);
    return 0;
}

int HandledCheckShaderGet(context *C, GLuint H, object **OutObject, const char *Name) {
    CheckGL(0 == CheckObjectType(C, H, object_PROGRAM), gl_error_SHADER_IS_PROGRAM, 1);
    CheckGL(CheckObjectTypeGet(C, H, object_SHADER, OutObject), gl_error_SHADER_INVALID, 1);
    return 0;
}

// NOTE(blackedout): MARK: Helper function

void SetVertexInputAttributeFormat(context *C, object *Object, int IsCurrent, u32 Index, GLint Size, GLenum Type, GLuint RelativeOffset, u32 IntegerHandlingBits, const char *Name) {
    CheckGL(Index >= C->PipelineStateInfos[pipeline_state_VERTEX_INPUT_ATTRIBUTES].InstanceCount, gl_error_VERTEX_ARRAY_ATTRIB_FORMAT_INDEX);

    // TODO(blackedout): Further input checking
    // Also: "An INVALID_VALUE error is generated if relativeoffset is larger than the value of MAX_VERTEX_ATTRIB_RELATIVE_OFFSET."
    // Doesn't seem to exist in Vulkan

    vertex_array_attribute *Attribute = Object->VertexArray.InputAttributes + Index;
    Attribute->Size = Size;
    Attribute->Type = Type;
    Attribute->RelativeOffset = RelativeOffset;
    SetFlagsU32(&Attribute->Flags, vertex_input_attribute_INTEGER_MASK, IntegerHandlingBits);

    if(IsCurrent) {
        pipeline_state_vertex_input_attribute *State = GetCurrentPipelineState(C, pipeline_state_VERTEX_INPUT_ATTRIBUTES);
        State += Index;
        State->Size = Size;
        State->Type = Type;
        State->RelativeOffset = RelativeOffset;
        SetFlagsU32(&State->Flags, vertex_input_attribute_INTEGER_MASK, IntegerHandlingBits);
    }
}

void SetVertexInputBinding(context *C, object *Object, int IsCurrent, u32 Index, GLuint Vbo, GLintptr Offset, GLsizei Stride, const char *Name) {
    CheckGL(Index >= C->PipelineStateInfos[pipeline_state_VERTEX_INPUT_BINDINGS].InstanceCount, gl_error_VERTEX_ARRAY_VERTEX_BUFFER_INDEX);
    CheckGL(Stride < 0 || Offset < 0 || Stride > C->DeviceInfo.Properties.limits.maxVertexInputBindingStride, gl_error_VERTEX_ARRAY_VERTEX_BUFFER_STRIDE_OFFSET);

    object *ObjectB = 0;
    CheckGL(Vbo != 0 && CheckObjectTypeGet(C, Vbo, object_BUFFER, &ObjectB), gl_error_VERTEX_ARRAY_VERTEX_BUFFER_VBO_INVALID);

    vertex_array_binding NewBinding = {0};
    if(Vbo != 0) {
        NewBinding.Vbo = Vbo;
        NewBinding.Offset = Offset;
        NewBinding.Stride = Stride;
    }

    Object->VertexArray.InputBindings[Index] = NewBinding;
    if(IsCurrent) {
        pipeline_state_vertex_input_binding *State = GetCurrentPipelineState(C, pipeline_state_VERTEX_INPUT_BINDINGS);
        State[Index].IsEnabled = NewBinding.Vbo != 0;
        State[Index].Stride = NewBinding.Stride;
    }
}

void SetVertexInputAttributeBinding(context *C, object *Object, int IsCurrent, u32 AttributeIndex, u32 BindingIndex, const char *Name) {
    CheckGL(AttributeIndex >= C->PipelineStateInfos[pipeline_state_VERTEX_INPUT_ATTRIBUTES].InstanceCount, gl_error_VERTEX_ARRAY_ATTRIB_BINDING_ATTRIB_INDEX);
    CheckGL(BindingIndex >= C->PipelineStateInfos[pipeline_state_VERTEX_INPUT_BINDINGS].InstanceCount, gl_error_VERTEX_ARRAY_ATTRIB_BINDING_BINDING_INDEX);

    Object->VertexArray.InputAttributes[AttributeIndex].BindingIndex = BindingIndex;
    if(IsCurrent) {
        pipeline_state_vertex_input_attribute *State = GetCurrentPipelineState(C, pipeline_state_VERTEX_INPUT_ATTRIBUTES);
        State += AttributeIndex;
        State->BindingIndex = BindingIndex;
    }
}

void SetVertexInputAttributePointer(context *C, object *Object, u32 Index, GLint Size, GLenum Type, GLsizei Stride, const void *Pointer, u32 IntegerHandlingBits, const char *Name) {
    SetVertexInputAttributeFormat(C, Object, 1, Index, Size, Type, 0, IntegerHandlingBits, Name);
    SetVertexInputAttributeBinding(C, Object, 1, Index, Index, Name);

    // TODO(blackedout): If pointer is 0, it is allowed to have no array buffer bound ? 
    buffer_target_info ArrayBufferTargetInfo = {0};
    Assert(0 == GetBufferTargetInfo(GL_ARRAY_BUFFER, &ArrayBufferTargetInfo));
    GLuint BoundVbo = C->BoundBuffers[ArrayBufferTargetInfo.Index];
    Assert(0 == CheckObjectType(C, BoundVbo, object_BUFFER));

    vertex_input_attribute_size_type_info SizeTypeInfo = {0};
    // TODO(blackedout): Assert is not good here
    Assert(0 == GetVertexInputAttributeSizeTypeInfo(Size, Type, &SizeTypeInfo));
    //const char *Msg = "Custom: glVertexAttribPointer size and type combination invalid.";
    //GenerateErrorMsg(C, GL_INVALID_VALUE, GL_DEBUG_SOURCE_APPLICATION, Msg);
    //return;

    GLsizei ActualStride = Stride ? Stride : SizeTypeInfo.ByteCount;
    SetVertexInputBinding(C, Object, 1, Index, BoundVbo, (GLintptr)Pointer, ActualStride, Name);
}

void SetVertexInputAttributeEnabled(context *C, object *Object, int IsCurrent, GLuint Index, int IsEnabled, const char *Name) {
    CheckGL(Index >= C->PipelineStateInfos[pipeline_state_VERTEX_INPUT_ATTRIBUTES].InstanceCount, gl_error_XABLE_VERTEX_ATTRIB_INDEX);
    
    SetFlagsEnabledU32(&Object->VertexArray.InputAttributes[Index].Flags, vertex_input_attribute_IS_ENABLED, IsEnabled);
    if(IsCurrent) {
        pipeline_state_vertex_input_attribute *State = GetCurrentPipelineState(C, pipeline_state_VERTEX_INPUT_ATTRIBUTES);
        SetFlagsEnabledU32(&State[Index].Flags, vertex_input_attribute_IS_ENABLED, IsEnabled);
    }
}

GLboolean IsObjectType(GLuint H, object_type Type, const char *Name) {
    context *C = 0;
    GLboolean Result = GL_FALSE;
    CheckGL(AcquireContext(&C, Name), gl_error_ACQUIRE_CONTEXT, Result);
    int IsWrongType = CheckObjectType(C, H, object_BUFFER);
    ReleaseContext(C, Name);
    Result = IsWrongType ? GL_FALSE : GL_TRUE;
    return Result;
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

    MakeStaticEntry(pipeline_state_HEADER, pipeline_state_header),
    MakeStaticEntry(pipeline_state_CLEAR_COLOR, pipeline_state_clear_color),
    MakeStaticEntry(pipeline_state_CLEAR_DEPTH, pipeline_state_clear_depth),
    MakeStaticEntry(pipeline_state_CLEAR_STENCIL, pipeline_state_clear_stencil),
    MakeDynamicEntry(pipeline_state_VIEWPORT, pipeline_state_viewport, offsetof(VkPhysicalDeviceLimits,  maxViewports)),
    MakeDynamicEntry(pipeline_state_SCISSOR, pipeline_state_scissor, offsetof(VkPhysicalDeviceLimits,  maxViewports)),
    MakeStaticEntry(pipeline_state_FRAMEBUFFER, pipeline_state_framebuffer),
    MakeDynamicEntry(pipeline_state_DRAW_BUFFERS, pipeline_state_draw_buffer, offsetof(VkPhysicalDeviceLimits,  maxColorAttachments)),
    //MakeStaticEntry(pipeline_state_VERTEX_ARRAY, pipeline_state_vertex_array),
    MakeDynamicEntry(pipeline_state_VERTEX_INPUT_ATTRIBUTES, pipeline_state_vertex_input_attribute, offsetof(VkPhysicalDeviceLimits, maxVertexInputAttributes)),
    MakeDynamicEntry(pipeline_state_VERTEX_INPUT_BINDINGS, pipeline_state_vertex_input_binding, offsetof(VkPhysicalDeviceLimits, maxVertexInputBindings)),
    MakeStaticEntry(pipeline_state_PROGRAM, pipeline_state_program),
    MakeStaticEntry(pipeline_state_PRIMITIVE_TYPE, pipeline_state_primitive_type),

#undef MakeDynamicEntry
#undef MakeStaticEntry
};
StaticAssert(ArrayCount(PipelineStateInfoParams) == pipeline_state_COUNT);

static void SetPipelineStateInfos(pipeline_state_info *Infos, const VkPhysicalDeviceLimits *Limits, u32 *OutByteCount) {
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

int CreatePipelineStates(context *C, const VkPhysicalDeviceLimits *Limits) {
    SetPipelineStateInfos(C->PipelineStateInfos, Limits, &C->PipelineStateByteCount);
    C->VertexInputBindingDescriptions = calloc(C->PipelineStateInfos[pipeline_state_VERTEX_INPUT_BINDINGS].InstanceCount, sizeof(VkVertexInputBindingDescription));
    C->VertexInputAttributeDescriptions = calloc(C->PipelineStateInfos[pipeline_state_VERTEX_INPUT_ATTRIBUTES].InstanceCount, sizeof(VkVertexInputAttributeDescription));

    if(ArrayRequireRoom(&C->PipelineStates, 2, C->PipelineStateByteCount, INITIAL_PIPELINE_STATE_CAPACITY)) {
        return 1;
    }
    C->PipelineStates.Count = 1;
    SetDefaultPipelineState(C, 0);
    return 0;
}

void *GetPipelineState(context *C, u64 StateIndex, pipeline_state_type Type) {
    u8 *Base = ArrayData(u8, C->PipelineStates) + StateIndex*C->PipelineStateByteCount;
    return Base + C->PipelineStateInfos[Type].ByteOffset;
}

void ClearPipelineState(context *C, u64 StateIndex, pipeline_state_type Type) {
    pipeline_state_info Info = C->PipelineStateInfos[Type];
    void *State = GetPipelineState(C, StateIndex, Type);
    memset(State, 0, Info.InstanceCount*Info.InstaceByteCount);
}

void CopyPipelineStateToPtr(context *C, void *DstState, u64 SrcStateIndex, pipeline_state_type Type) {
    pipeline_state_info Info = C->PipelineStateInfos[Type];
    void *SrcState = GetPipelineState(C, SrcStateIndex, Type);
    memcpy(DstState, SrcState, Info.InstanceCount*Info.InstaceByteCount);
}

void CopyPipelineStateFromPtr(context *C, u64 DstStateIndex, void *SrcState, pipeline_state_type Type) {
    pipeline_state_info Info = C->PipelineStateInfos[Type];
    void *DstState = GetPipelineState(C, DstStateIndex, Type);
    memcpy(DstState, SrcState, Info.InstanceCount*Info.InstaceByteCount);
}

void CopyPipelineState(context *C, u64 DstStateIndex, u64 SrcStateIndex, pipeline_state_type Type) {
    pipeline_state_info Info = C->PipelineStateInfos[Type];
    void *DstState = GetPipelineState(C, DstStateIndex, Type);
    void *SrcState = GetPipelineState(C, SrcStateIndex, Type);
    memcpy(DstState, SrcState, Info.InstanceCount*Info.InstaceByteCount);
}

int UseCurrentPipelineState(context *C, u32 TypeCount, pipeline_state_type *Types) {
    // TODO(blackedout): Validate if pipeline can be used
    {
        
    }

    for(u32 I = 0; I < TypeCount; ++I) {
        if(Types[I] == pipeline_state_VERTEX_INPUT_BINDINGS) {
            // TODO(blackedout): Since comamnds are only pushed, make the command buffer like a stack
            // That would allow variable sized commands (would make it unnecessary to push a bind for each command)

            // TODO(blackedout): Unbind this also

            // NOTE(blackedout): Bindings are used, push bind commands of currently bound vao
            object *Object = 0;
            // TODO(blackedout): This should be a validation, don't Assert
            Assert(0 == CheckObjectTypeGet(C, C->BoundVao, object_VERTEX_ARRAY, &Object));
            
            for(u32 J = 0; J < C->PipelineStateInfos[pipeline_state_VERTEX_INPUT_BINDINGS].InstanceCount; ++J) {
                object *ObjectB = 0;
                if(Object->VertexArray.InputBindings[J].Vbo == 0) {
                    // NOTE(blackedout): Binding unused, ignore it
                    continue;
                }
                Assert(0 == CheckObjectTypeGet(C, Object->VertexArray.InputBindings[J].Vbo, object_BUFFER, &ObjectB));
                
                command Command = {
                    .Type = command_BIND_VERTEX_BUFFER,
                    .BindVertexBuffer = {
                        .BindingIndex = J,
                        .Buffer = ObjectB->Buffer.Buffer,
                        .Offset = Object->VertexArray.InputBindings[J].Offset
                    }
                };
                // TODO(blackedout): Error handling
                PushCommand(C, Command);
            }
        }
    }

    int FoundMatchingPipeline = 0;
    u32 MatchingPipelineIndex = 0;
    // NOTE(blackedout): First pipeline is the current one
    for(u32 I = 1; I < C->PipelineStates.Count; ++I) {
        int DidMatchAll = 1;
        for(u32 J = 0; J < TypeCount; ++J) {
            pipeline_state_type Type = Types[J];
            pipeline_state_info Info = C->PipelineStateInfos[Type];
            void *State = GetPipelineState(C, I, Type);
            void *CurrState = GetCurrentPipelineState(C, Type);

            // TODO(blackedout): Take special care of state that has enabled flags, such that the disabled stuff is not actually compared

            if(memcmp(State, CurrState, Info.InstanceCount*Info.InstaceByteCount) != 0) {
                DidMatchAll = 0;
                break;
            }
        }
        if(DidMatchAll) {
            FoundMatchingPipeline = 1;
            MatchingPipelineIndex = I;
            break;
        }
    }

    if(FoundMatchingPipeline == 0) {
        if(ArrayRequireRoom(&C->PipelineStates, 1, C->PipelineStateByteCount, INITIAL_PIPELINE_STATE_CAPACITY)) {
            return 1;
        }
        MatchingPipelineIndex = C->PipelineStates.Count++;
        SetDefaultPipelineState(C, MatchingPipelineIndex);
        FoundMatchingPipeline = 1;

        for(u32 J = 0; J < TypeCount; ++J) {
            pipeline_state_type Type = Types[J];
            pipeline_state_info Info = C->PipelineStateInfos[Type];
            void *State = GetPipelineState(C, MatchingPipelineIndex, Type);
            void *CurrState = GetCurrentPipelineState(C, Type);
            memcpy(State, CurrState, Info.InstanceCount*Info.InstaceByteCount);
        }
    }

    if(C->IsPipelineSet == 0 || C->LastPipelineIndex != MatchingPipelineIndex) {
        command Command = {
            .Type = command_BIND_PIPELINE,
            .BindPipeline = MatchingPipelineIndex
        };
        C->IsPipelineSet = 1;
        C->LastPipelineIndex = MatchingPipelineIndex;

        if(PushCommand(C, Command)) {
            return 1;
        }
    }

    return 0;
}

void SetDefaultPipelineState(context *C, u32 PipelineIndex) {
    StaticAssert(pipeline_state_HEADER == 0);
    void *NewPipelineState = GetPipelineState(C, PipelineIndex, pipeline_state_HEADER);
    memset(NewPipelineState, 0, C->PipelineStateByteCount);

    {
        pipeline_state_viewport Viewport = {0};
        Viewport.width = (float)C->DeviceInfo.SurfaceCapabilities.currentExtent.width;
        Viewport.height = (float)C->DeviceInfo.SurfaceCapabilities.currentExtent.height;
        Viewport.maxDepth = 1.0f;
        pipeline_state_viewport *State = GetPipelineState(C, PipelineIndex, pipeline_state_VIEWPORT);
        for(u32 I = 0; I < C->PipelineStateInfos[pipeline_state_VIEWPORT].InstanceCount; ++I) {
            State[I] = Viewport;
        }
    }

    {
        pipeline_state_scissor Scissor = {0};
        Scissor.extent = C->DeviceInfo.SurfaceCapabilities.currentExtent;
        pipeline_state_scissor *State = GetPipelineState(C, PipelineIndex, pipeline_state_SCISSOR);
        for(u32 I = 0; I < C->PipelineStateInfos[pipeline_state_SCISSOR].InstanceCount; ++I) {
            State[I] = Scissor;
        }
    }
}

static void ConvertPipelineVertexInputAttributes(context *C, u32 PipelineIndex, VkVertexInputAttributeDescription *AttributeDescriptions, u32 *OutCount) {
    u32 Count = 0;
    pipeline_state_vertex_input_attribute *State = GetPipelineState(C, PipelineIndex, pipeline_state_VERTEX_INPUT_ATTRIBUTES);
    pipeline_state_info Info = C->PipelineStateInfos[pipeline_state_VERTEX_INPUT_ATTRIBUTES];
    for(u32 I = 0; I < Info.InstanceCount; ++I) {
        if(State[I].Flags & vertex_input_attribute_IS_ENABLED) {
            vertex_input_attribute_size_type_info Info = {0};
            GetVertexInputAttributeSizeTypeInfo(State[I].Size, State[I].Type, &Info);

            VkVertexInputAttributeDescription Desc = {
                .location = I,
                .binding = State[I].BindingIndex,
                .format = Info.VulkanFormat,
                .offset = State[I].RelativeOffset,
            };
            AttributeDescriptions[Count++] = Desc;
        }
    }
    *OutCount = Count;
}

static void ConvertPipelineVertexInputBindings(context *C, u32 PipelineIndex, VkVertexInputBindingDescription *BindingDescriptions, u32 *OutCount) {
    u32 Count = 0;
    pipeline_state_vertex_input_binding *State = GetPipelineState(C, PipelineIndex, pipeline_state_VERTEX_INPUT_BINDINGS);
    pipeline_state_info Info = C->PipelineStateInfos[pipeline_state_VERTEX_INPUT_BINDINGS];
    for(u32 I = 0; I < Info.InstanceCount; ++I) {
        if(State[I].IsEnabled) {
            VkVertexInputBindingDescription Desc = {
                .binding = I,
                .stride = State[I].Stride,
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, // TODO(blackedout)
            };
            BindingDescriptions[Count++] = Desc;
        }
    }
    *OutCount = Count;
}

static void ConvertProgramShaderStages(context *C, u32 PipelineIndex, VkPipelineShaderStageCreateInfo *ShaderStageCreateInfos, u32 *OutCount) {
    pipeline_state_program *State = GetPipelineState(C, PipelineIndex, pipeline_state_PROGRAM);
        
    // NOTE(blackedout): Assert here because validation should have happended in `UseCurrentPipelineState`
    object *Object = 0;
    Assert(0 == CheckObjectTypeGet(C, State->Program, object_PROGRAM, &Object));
    
    u32 ShaderStageCount = Object->Program.AttachedShaderCount;
    for(u32 J = 0; J < ShaderStageCount; ++J) {
        object *ObjectS = Object->Program.AttachedShaders[J];
        shader_type_info TypeInfo = {0};
        GetShaderTypeInfo(ObjectS->Shader.Type, &TypeInfo); // TODO(blackedout): Error handling
        VkPipelineShaderStageCreateInfo ShaderStageCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = 0,
            .flags = 0,
            .stage = TypeInfo.VulkanBit,
            .module = ObjectS->Shader.VulkanModule,
            .pName = "main",
            .pSpecializationInfo = 0
        };
        ShaderStageCreateInfos[J] = ShaderStageCreateInfo;
    }
    *OutCount = ShaderStageCount;
}

int CreatePipeline(context *C, u32 PipelineIndex, VkRenderPass RenderPass) {
    pipeline_state_header *Header = GetPipelineState(C, PipelineIndex, pipeline_state_HEADER);
    if(Header->IsCreated) {
        return 0;
    }
    Header->Layout = VK_NULL_HANDLE;
    Header->Pipeline = VK_NULL_HANDLE;

    int Result = 1;
    
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

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .topology = 0,
        .primitiveRestartEnable = VK_FALSE,
    };
    {
        // TODO(blackedout): Error handling
        pipeline_state_primitive_type *State = GetPipelineState(C, PipelineIndex, pipeline_state_PRIMITIVE_TYPE);
        primitive_info Info = {0};
        GetPrimitiveInfo(State->Type, &Info);
        InputAssemblyStateCreateInfo.topology = Info.VulkanPrimitve;
    }

    VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .viewportCount = C->PipelineStateInfos[pipeline_state_VIEWPORT].InstanceCount,
        .pViewports = GetPipelineState(C, PipelineIndex, pipeline_state_VIEWPORT),
        .scissorCount = C->PipelineStateInfos[pipeline_state_SCISSOR].InstanceCount,
        .pScissors = GetPipelineState(C, PipelineIndex, pipeline_state_SCISSOR),
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
    VulkanCheckGoto(vkCreatePipelineLayout(C->Device, &PipelineLayoutCreateInfo, 0, &Header->Layout), label_Error);

    VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = C->VertexInputBindingDescriptions,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = C->VertexInputAttributeDescriptions,
    };
    ConvertPipelineVertexInputBindings(C, PipelineIndex, C->VertexInputBindingDescriptions, &PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount);
    ConvertPipelineVertexInputAttributes(C, PipelineIndex, C->VertexInputAttributeDescriptions, &PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount);

    VkPipelineShaderStageCreateInfo ShaderStageCreateInfos[PROGRAM_SHADER_CAPACITY] = {0};
    VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .stageCount = 0,
        .pStages = ShaderStageCreateInfos,
        .pVertexInputState = &PipelineVertexInputStateCreateInfo,
        .pInputAssemblyState = &InputAssemblyStateCreateInfo,
        .pTessellationState = 0,
        .pViewportState = &ViewportStateCreateInfo,
        .pRasterizationState = &C->Config.RasterState,
        .pMultisampleState = &PipelineMultiSampleStateCreateInfo,
        .pDepthStencilState = &PipelineDepthStencilStateCreateInfo,
        .pColorBlendState = &PipelineColorBlendStateCreateInfo,
        //.pDynamicState = &PipelineDynamicStateCreateInfo,
        .layout = Header->Layout,
        .renderPass = RenderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };
    ConvertProgramShaderStages(C, PipelineIndex, ShaderStageCreateInfos, &GraphicsPipelineCreateInfo.stageCount);

    VulkanCheckGoto(vkCreateGraphicsPipelines(C->Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, 0, &Header->Pipeline), label_Error);

    Result = 0;
    goto label_Exit;
label_Error:
    vkDestroyPipelineLayout(C->Device, Header->Layout, 0);
    vkDestroyPipeline(C->Device, Header->Pipeline, 0);
label_Exit:
    return Result;
}