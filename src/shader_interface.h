#ifndef SHADER_INTERFACE_H
#define SHADER_INTERFACE_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum storage_qualifier {
    storage_qualifier_NONE = 0,
    storage_qualifier_IN,
    storage_qualifier_OUT,
    storage_qualifier_UNIFORM,
    storage_qualifier_CONST,
    storage_qualifier_COUNT
} storage_qualifier;

typedef enum glsl_type {
    glsl_type_NONE = 0,
    glsl_type_VOID,

    glsl_type_DOUBLE,
    glsl_type_FLOAT,
    glsl_type_INT,
    glsl_type_UINT,

    glsl_type_DVEC2,
    glsl_type_VEC2,
    glsl_type_IVEC2,
    glsl_type_UVEC2,
    glsl_type_DVEC3,
    glsl_type_VEC3,
    glsl_type_IVEC3,
    glsl_type_UVEC3,
    glsl_type_DVEC4,
    glsl_type_VEC4,
    glsl_type_IVEC4,
    glsl_type_UVEC4,

    glsl_type_MAT2,
    glsl_type_MAT2x2,
    glsl_type_MAT2x3,
    glsl_type_MAT2x4,
    glsl_type_MAT3,
    glsl_type_MAT3x3,
    glsl_type_MAT3x4,
    glsl_type_MAT4,
    glsl_type_MAT4x4,

    glsl_type_DMAT2,
    glsl_type_DMAT2x2,
    glsl_type_DMAT2x3,
    glsl_type_DMAT2x4,
    glsl_type_DMAT3,
    glsl_type_DMAT3x3,
    glsl_type_DMAT3x4,
    glsl_type_DMAT4,
    glsl_type_DMAT4x4,
    
    glsl_type_COUNT
} glsl_type;

typedef enum shader_type {
    shader_VERTEX,
    shader_FRAGMENT,
    shader_COMPUTE,
    shader_GEOMETRY,
    shader_TESSELATION_CONTROL,
    shader_TESSELATION_EVALUATE,
    shader_COUNT,
} shader_type;

typedef enum shader_flags {
    shader_flag_VERTEX = 1 << shader_VERTEX,
    shader_flag_FRAGMENT = 1 << shader_FRAGMENT,
    shader_flag_COMPUTE = 1 << shader_COMPUTE,
    shader_flag_GEOMETRY = 1 << shader_GEOMETRY,
    shader_flag_TESSELATION_CONTROL = 1 << shader_TESSELATION_CONTROL,
    shader_flag_TESSELATION_EVALUATE = 1 << shader_TESSELATION_EVALUATE,
} shader_flags;

typedef enum glslang_error_type {
    glslang_error_NONE = 0,
    glslang_error_INPUT,
    glslang_error_OUT_OF_MEMORY,
    glslang_error_COMPILATION_FAILED,
    glslang_error_LINKAGE_FAILED,
    glslang_error_EXCEPTION,
} glslang_error_type;

typedef struct shader_variable {
    const char *Name;
    uint32_t ArrayLength;
    int Location;
    storage_qualifier StorageQualifier;
    glsl_type Type;
} shader_variable;

typedef struct uniform_variable {
    int Location;
    int LocationCount;
    const char *Name;
    glsl_type Type;
    uint32_t ByteOffset;
    shader_flags VariableIndicesSet;
    uint32_t VariableIndices[shader_COUNT];
} uniform_variable;

typedef struct glslang_shader {
    void *Native;
    const char *Source;
    uint64_t SourceLength;
    uint8_t *SpirvBytes;
    uint64_t SpirvByteCount;
    shader_variable *Variables;
    uint32_t VariableCount;
    char *NameBuf;
} glslang_shader;

typedef struct glslang_program {
    void *Native;
    glslang_shader *AttachedShaders[shader_COUNT];
    uniform_variable *Uniforms;
    uint32_t UniformCount;
    uint32_t UniformByteCount;
    uint32_t UniformLocationCount;
} glslang_program;

int GlslangShaderCreateAndParse(shader_type Type, const char *Source, uint64_t SourceLength, glslang_shader *OutShader);
int GlslangShaderGetLog(glslang_shader *Shader, uint64_t *OutLength, const char **OutString);
void GlslangShaderDelete(glslang_shader *Shader);

int GlslangProgramCreate(glslang_program *OutProgram);
int GlslangProgramAddShader(glslang_program *Program, glslang_shader *Shader);
int GlslangProgramLink(glslang_program *Program);
int GlslangProgramGetLog(glslang_program *Program, uint64_t *OutLength, const char **OutString);
int GlslangProgramGetLocation(glslang_program *Program, const char *Name, int *OutLocation);
void GlslangProgramDelete(glslang_program *Program);
int GlslangGetSpirv(glslang_program *Program, glslang_shader *Shader, unsigned char **OutBytes, uint64_t *OutByteCount);

#ifdef __cplusplus
}
#endif

#endif