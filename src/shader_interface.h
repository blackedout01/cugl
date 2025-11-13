#ifndef SHADER_INTERFACE_H
#define SHADER_INTERFACE_H

#include "stdint.h"

typedef enum shader_type {
    shader_VERTEX,
    shader_FRAGMENT,
    shader_COMPUTE,
    shader_GEOMETRY,
    shader_TESSELATION_CONTROL,
    shader_TESSELATION_EVALUATE,
} shader_type;

typedef enum glslang_error_type {
    glslang_error_NONE = 0,
    glslang_error_INPUT,
    glslang_error_OUT_OF_MEMORY,
    glslang_error_COMPILATION_FAILED,
    glslang_error_LINKAGE_FAILED,
    glslang_error_EXCEPTION,
} glslang_error_type;

struct glslang_program_empty {};
struct glslang_shader_empty {};

typedef struct glslang_shader_empty *glslang_shader;
typedef struct glslang_program_empty *glslang_program;

#ifdef __cplusplus
extern "C" {
#endif

int GlslangShaderCreateAndParse(shader_type Type, const char *SourceBytes, uint64_t SourceByteCount, glslang_shader *OutShader);
int GlslangShaderGetLog(glslang_shader Shader, uint64_t *OutLength, const char **OutString);
void GlslangShaderDelete(glslang_shader Shader);

int GlslangProgramCreate(glslang_program *OutProgram);
int GlslangProgramAddShader(glslang_program Program, glslang_shader Shader);
int GlslangProgramLink(glslang_program Program);
int GlslangProgramGetLog(glslang_program Program, uint64_t *OutLength, const char **OutString);
int GlslangProgramGetLocation(glslang_program Program, const char *Name, int *OutLocation);
void GlslangProgramDelete(glslang_program Program);
int GlslangGetSpirv(glslang_program Program, glslang_shader Shader, unsigned char **OutBytes, uint64_t *OutByteCount);

#ifdef __cplusplus
}
#endif

#endif