#include "internal.h"
#include "vulkan/vulkan_core.h"

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

int GetVertexInputAttributeSizeTypeInfo(GLint Size, GLenum Type, vertex_input_attribute_size_type_info *OutInfo) {
#define MakeSizeCase(Count, Format) case (Count): OutInfo->VulkanFormat = (Format); break
#define MakeDefaultCase default: OutInfo->VulkanFormat = VK_FORMAT_UNDEFINED; break

#define MakeCase(GlType, BitCount, VulkanType)\
    case GlType:\
        switch(Size) {\
        MakeSizeCase(1, VK_FORMAT_R ## BitCount ## _ ## VulkanType);\
        MakeSizeCase(2, VK_FORMAT_R ## BitCount ## G ## BitCount ## _ ## VulkanType);\
        MakeSizeCase(3, VK_FORMAT_R ## BitCount ## G ## BitCount ## B ## BitCount ## _ ## VulkanType);\
        MakeSizeCase(4, VK_FORMAT_R ## BitCount ## G ## BitCount ## B ## BitCount ## A ## BitCount ## _ ## VulkanType);\
        MakeDefaultCase;\
        }\
        OutInfo->ByteCount = Size*BitCount/8;\
        break
        
    switch(Type) {
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
#undef MakeCase

#undef MakeDefaultCase
#undef MakeSizeCase

    return OutInfo->VulkanFormat == VK_FORMAT_UNDEFINED;
}

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