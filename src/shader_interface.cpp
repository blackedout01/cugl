#include "shader_interface.h"

#include "glslang/Public/ShaderLang.h"
#include "glslang/Public/ResourceLimits.h"
#include "SPIRV/GlslangToSpv.h"

int GlslangShaderCreateAndParse(shader_type Type, const char *SourceBytes, uint64_t SourceByteCount, glslang_shader *OutShader) {
    EShLanguage Language;
#define MakeCase(Key, Value) case (Key): Language = (Value); break
    switch(Type) {
    MakeCase(shader_VERTEX, EShLangVertex);
    MakeCase(shader_FRAGMENT, EShLangFragment);
    MakeCase(shader_COMPUTE, EShLangCompute);
    MakeCase(shader_GEOMETRY, EShLangGeometry);
    MakeCase(shader_TESSELATION_CONTROL, EShLangTessControl);
    MakeCase(shader_TESSELATION_EVALUATE, EShLangTessEvaluation);
    default:
        return glslang_error_INPUT;
    }
#undef MakeCase

    const int DefaultVersion = 100;
    const bool ForwardCompatible = false;
    const glslang::EShClient Client = glslang::EShClientVulkan;
    const glslang::EShTargetClientVersion ClientVersion = glslang::EShTargetVulkan_1_0;
    const EShMessages Messages = EShMsgDefault;

    // TODO(blackedout): better exception handling
    try {
        glslang::TShader *Shader = new glslang::TShader(Language);

        Shader->setStrings(&SourceBytes, 1);
        Shader->setEnvInput(glslang::EShSourceGlsl, Language, Client, DefaultVersion);
        Shader->setEnvClient(Client, ClientVersion);
        Shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

        const TBuiltInResource *DefaultResources = GetDefaultResources();
        
        *OutShader = (glslang_shader)Shader;
        if(!Shader->parse(DefaultResources, DefaultVersion, ForwardCompatible, Messages)) {
            return glslang_error_COMPILATION_FAILED;
        }
    } catch (...) {
        return glslang_error_EXCEPTION;
    }

    return glslang_error_NONE;
}

int GlslangShaderGetLog(glslang_shader Shader, uint64_t *OutLength, const char **OutString) {
    try {
        auto Info = ((glslang::TShader *)Shader)->getInfoLog();

        *OutLength = strlen(Info);
        *OutString = Info;
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    return glslang_error_NONE;
}

void GlslangShaderDelete(glslang_shader Shader) {
    delete ((glslang::TShader *)Shader);
}

int GlslangProgramCreate(glslang_program *OutProgram) {
    try {
        *OutProgram = (glslang_program)new glslang::TProgram();
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    return glslang_error_NONE;
}

int GlslangProgramAddShader(glslang_program Program, glslang_shader Shader) {
    try {
        ((glslang::TProgram *)Program)->addShader((glslang::TShader *)Shader);
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    return glslang_error_NONE;
}

int GlslangProgramLink(glslang_program Program) {
    const EShMessages Messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
    try {
        if(!((glslang::TProgram *)Program)->link(Messages)) {
            return glslang_error_LINKAGE_FAILED;
        }
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    return glslang_error_NONE;
}

int GlslangProgramGetLog(glslang_program Program, uint64_t *OutLength, const char **OutString) {
    try {
        auto Info = ((glslang::TProgram *)Program)->getInfoLog();

        *OutLength = strlen(Info);
        *OutString = Info;
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    return glslang_error_NONE;
}

int GlslangProgramGetLocation(glslang_program Program, const char *Name, int *OutLocation) {
    try {
        ((glslang::TProgram *)Program)->buildReflection();
        *OutLocation = ((glslang::TProgram *)Program)->getReflectionIndex(Name);
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    return glslang_error_NONE;
}

void GlslangProgramDelete(glslang_program Program) {
    delete ((glslang::TProgram *)Program);
}

int GlslangGetSpirv(glslang_program Program, glslang_shader Shader, unsigned char **OutBytes, uint64_t *OutByteCount) {
    try {
        spv::SpvBuildLogger logger;

        glslang::TIntermediate *Intermediate = ((glslang::TProgram *)Program)->getIntermediate(((glslang::TShader *)Shader)->getStage());

        glslang::SpvOptions SpvOptions;
        SpvOptions.validate = true;
        std::vector<unsigned int> spirv;
        glslang::GlslangToSpv(*Intermediate, spirv, &logger, &SpvOptions);

        uint64_t ByteCount = spirv.size()*sizeof(*spirv.data());
        unsigned char *Data = (unsigned char *)malloc(ByteCount);
        if(Data == 0) {
            return glslang_error_OUT_OF_MEMORY;
        }
        memcpy(Data, spirv.data(), ByteCount);

        //printf("%s\n", logger.getAllMessages().c_str());

        *OutBytes = Data;
        *OutByteCount = ByteCount;
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    
    return glslang_error_NONE;

#if 0
    program->loggerMessages = logger.getAllMessages();

    glslang_program_SPIRV_generate(GlslangProgram, GlslangStage);

    bin.size = glslang_program_SPIRV_get_size(program);
    bin.words = malloc(bin.size * sizeof(uint32_t));
    glslang_program_SPIRV_get(program, bin.words);

    const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
    if (spirv_messages)
        printf("(%s) %s\b", fileName, spirv_messages);
#endif
}