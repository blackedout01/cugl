#include "shader_interface.h"

#include "StandAlone/DirStackFileIncluder.h"
#include "glslang/MachineIndependent/Versions.h"
#include "glslang/Public/ShaderLang.h"
#include "glslang/Public/ResourceLimits.h"
#include "SPIRV/GlslangToSpv.h"
#include "internal.h"
#include <cctype>
#include <cstdint>
#include <cstring>
#include <string.h>
#include <string>
#include <set>
#include <vector>
#include <map>

#define TrueOrReturn1(X) if((X) == 0) return 1

static int GlslangLanguageToType(EShLanguage Language, shader_type *OutType) {
    switch(Language) {
#define MakeCase(K, V) case (K): *OutType = (V); return 0;
    MakeCase(EShLangVertex, shader_VERTEX);
    MakeCase(EShLangTessControl, shader_TESSELATION_CONTROL);
    MakeCase(EShLangTessEvaluation, shader_TESSELATION_EVALUATE);
    MakeCase(EShLangGeometry, shader_GEOMETRY);
    MakeCase(EShLangFragment, shader_FRAGMENT);
    MakeCase(EShLangCompute, shader_COMPUTE);
#if 0
    MakeCase(EShLangRayGen, shader_);
    MakeCase(EShLangIntersect, shader_);
    MakeCase(EShLangAnyHit, shader_);
    MakeCase(EShLangClosestHit, shader_);
    MakeCase(EShLangMiss, shader_);
    MakeCase(EShLangCallable, shader_);
    MakeCase(EShLangTask, shader_);
    MakeCase(EShLangMesh, shader_);
#endif
    default: return 1;
#undef MakeCase
    }
}

enum token_type {
    token_NAME,
    token_SINGLE,
    token_PREPROCESSOR,
    token_CHAR,
    token_STRING,
};

struct token {
    token_type Type;
    const uint8_t *Start;
    const uint8_t *End;
};

struct glsl_type_info {
    const char *Name;
    uint32_t ByteCount;
    uint32_t ByteAlignment;
};

// NOTE(blackedout): 16.9.4 - https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#interfaces-resources-layout (2025-11-25)
#define DOUBLE_ALIGNMENT 8
#define FLOAT_ALIGNMENT 4
#define INT_ALIGNMENT 4
#define UINT_ALIGNMENT 4

// TODO(blackedout): Verify these
static glsl_type_info GlslTypeInfos[] = {
    [glsl_type_NONE] = { "", 0, 0 },
    [glsl_type_VOID] = { "void", 0, 0 },

    [glsl_type_DOUBLE] = { "double", 8, DOUBLE_ALIGNMENT },
    [glsl_type_FLOAT] = { "float", 4, FLOAT_ALIGNMENT },
    [glsl_type_INT] = { "int", 4, INT_ALIGNMENT },
    [glsl_type_UINT] = { "uint", 4, UINT_ALIGNMENT },

    [glsl_type_DVEC2] = { "dvec2", 16, 2*DOUBLE_ALIGNMENT },
    [glsl_type_VEC2] = { "vec2", 8, 2*FLOAT_ALIGNMENT },
    [glsl_type_IVEC2] = { "ivec2", 8, 2*INT_ALIGNMENT },
    [glsl_type_UVEC2] = { "uvec2", 8, 2*UINT_ALIGNMENT },
    [glsl_type_DVEC3] = { "dvec3", 24, 4*DOUBLE_ALIGNMENT },
    [glsl_type_VEC3] = { "vec3", 12, 4*FLOAT_ALIGNMENT },
    [glsl_type_IVEC3] = { "ivec3", 12, 4*INT_ALIGNMENT },
    [glsl_type_UVEC3] = { "uvec3", 12, 4*UINT_ALIGNMENT },
    [glsl_type_DVEC4] = { "dvec4", 32, 4*DOUBLE_ALIGNMENT },
    [glsl_type_VEC4] = { "vec4", 16, 4*FLOAT_ALIGNMENT },
    [glsl_type_IVEC4] = { "ivec4", 16, 4*INT_ALIGNMENT },
    [glsl_type_UVEC4] = { "uvec4", 16, 4*UINT_ALIGNMENT },

    // NOTE(blackedout): NxM - N columns, M rows, stored in column major format
    // mat2x3 = vec3[2] as far as the alignment is concerned
    [glsl_type_MAT2] = { "mat2", 2*2*4, 2*FLOAT_ALIGNMENT },
    [glsl_type_MAT2x2] = { "mat2x2", 2*2*4, 2*FLOAT_ALIGNMENT },
    [glsl_type_MAT2x3] = { "mat2x3", 2*3*4, 4*FLOAT_ALIGNMENT },
    [glsl_type_MAT2x4] = { "mat2x4", 2*4*4, 4*FLOAT_ALIGNMENT },
    [glsl_type_MAT3] = { "mat3", 3*3*4, 4*FLOAT_ALIGNMENT },
    [glsl_type_MAT3x3] = { "mat3x3", 3*3*4, 4*FLOAT_ALIGNMENT },
    [glsl_type_MAT3x4] = { "mat3x4", 3*4*4, 4*FLOAT_ALIGNMENT },
    [glsl_type_MAT4] = { "mat4", 4*4*4, 4*FLOAT_ALIGNMENT },
    [glsl_type_MAT4x4] = { "mat4x4", 4*4*4, 4*FLOAT_ALIGNMENT },

    [glsl_type_DMAT2] = { "dmat2", 2*2*8, 2*DOUBLE_ALIGNMENT },
    [glsl_type_DMAT2x2] = { "dmat2x2", 2*2*8, 2*DOUBLE_ALIGNMENT },
    [glsl_type_DMAT2x3] = { "dmat2x3", 2*3*8, 4*DOUBLE_ALIGNMENT },
    [glsl_type_DMAT2x4] = { "dmat2x4", 2*4*8, 4*DOUBLE_ALIGNMENT },
    [glsl_type_DMAT3] = { "dmat3", 3*3*8, 4*DOUBLE_ALIGNMENT },
    [glsl_type_DMAT3x3] = { "dmat3x3", 3*3*8, 4*DOUBLE_ALIGNMENT },
    [glsl_type_DMAT3x4] = { "dmat3x4", 3*4*8, 4*DOUBLE_ALIGNMENT },
    [glsl_type_DMAT4] = { "dmat4", 4*4*8, 4*DOUBLE_ALIGNMENT },
    [glsl_type_DMAT4x4] = { "dmat4x4", 4*4*8, 4*DOUBLE_ALIGNMENT },
};
StaticAssert(ArrayCount(GlslTypeInfos) == glsl_type_COUNT);

#undef UINT_ALIGNMENT
#undef INT_ALIGNMENT
#undef FLOAT_ALIGNMENT
#undef DOUBLE_ALIGNMENT

const char *StorageQualifierStrings[] = {
    [storage_qualifier_NONE] = "",
    [storage_qualifier_IN] = "in",
    [storage_qualifier_OUT] = "out",
    [storage_qualifier_UNIFORM] = "uniform",
    [storage_qualifier_CONST] = "const",
};
StaticAssert(ArrayCount(StorageQualifierStrings) == storage_qualifier_COUNT);

struct variable_layout {
    int LocationSet = 0;
    int BindingSet = 0;
    //uint32_t StdTokenIndex;
    uint32_t Location;
    uint32_t Binding;
};

struct variable {
    int LayoutSet;
    variable_layout Layout;
    storage_qualifier StorageQualifier;
    glsl_type Type;
    uint32_t NameTokenIndex;

    uint32_t StartTokenIndex;
    uint32_t TokenLength;
};

struct parsed_shader {
    int HasProfileDefinition;
    uint32_t ProfileTokenIndex;
    uint32_t VersionEndTokenIndex;
    uint32_t UniformCount;
    std::vector<variable> GlobalVariables;
};

static void SeekSpaceEnd(uint8_t **InOutIt, uint8_t *End) {
    uint8_t *It = *InOutIt;
    while(It < End && isspace(*It)) {
        ++It;
    }
    *InOutIt = It;
}

static void SeekNameEnd(uint8_t **InOutIt, uint8_t *End) {
    uint8_t *It = *InOutIt;
    while(It < End) {
        if((isalnum(*It) || *It == '_') == 0) {
            break;
        }
        ++It;
    }
    *InOutIt = It;
}

static void SeekLineEnd(uint8_t **InOutIt, uint8_t *End) {
    uint8_t *It = *InOutIt;
    while(It < End && *(It++) != '\n');
    if(It < End && *It == '\r') {
        ++It;
    }
    *InOutIt = It;
}

static void SeekCommentEnd(uint8_t **InOutIt, uint8_t *End) {
    uint8_t *It = *InOutIt;
    // NOTE(blackedout): Skip first char to be able to reference previous char, also first char can't be comment end
    if(It < End) {
        ++It;
    }
    while(It < End) {
        if(*It == '/' && It[-1] == '*') {
            ++It;
            break;
        }
        ++It;
    }
    *InOutIt = It;
}

static void SeekNonEscapedCharEnd(uint8_t **InOutIt, uint8_t *End, char C) {
    uint8_t *It = *InOutIt;
    uint64_t BackslashCount = 0;
    while(It < End) {
        if(*It == '\\') {
            ++BackslashCount;
        } else {
            BackslashCount = 0;
        }

        if(*It == C && (BackslashCount & 1) == 0) {
            ++It;
            break;
        }
        ++It;
    }
    *InOutIt = It;
}

static void LexProgram(const char *Bytes, uint64_t ByteCount, std::vector<token> &OutTokens) {
    auto &Tokens = OutTokens;
    Tokens.clear();

    uint8_t *It = (uint8_t *)Bytes;
    uint8_t *ItEnd = (uint8_t *)Bytes + ByteCount;
    while(It < ItEnd) {
        uint8_t *TokenStart = It;

        switch(*It) {
        case '#': {
            ++It;
            SeekNameEnd(&It, ItEnd);
            Tokens.push_back(token {token_PREPROCESSOR, TokenStart, It});
        } break;
        case '\'': {
            ++It;
            SeekNonEscapedCharEnd(&It, ItEnd, '\'');
            Tokens.push_back(token {token_CHAR, TokenStart + 1, It});
        } break;
        case '"': {
            ++It;
            SeekNonEscapedCharEnd(&It, ItEnd, '"');
            Tokens.push_back(token {token_STRING, TokenStart + 1, It});
        } break;
        case '!': case '$': case '%': case '&': case '(': case ')': case '*': case '+':
        case ',': case '-': case '.': case '/': case ':': case ';': case '<': case '=':
        case '>': case '?': case '@': case '[': case '\\': case ']': case '^': case '`':
        case '{': case '|': case '}': case '~': {
            Tokens.push_back(token {token_SINGLE, TokenStart, ++It});
        } break;
        case ' ': case '\t': case '\n': case '\v': case '\f': case '\r': {
            ++It;
            SeekSpaceEnd(&It, ItEnd);
        } break;
        case 0:
            ++It;
            // TODO(blackedout): What to do here??
            break;
        default:
        if(isalnum(*It) || *It == '_') {
            ++It;
            SeekNameEnd(&It, ItEnd);
            Tokens.push_back(token {token_NAME, TokenStart, It});
        } else {
            Assert(0);
        } break;
        }
    }
}

static int TokenEquals(const token &Token, const char *String) {
    uint64_t TokenLength = Token.End - Token.Start;
    uint64_t StringLength = strlen(String);
    if(TokenLength != StringLength) {
        return 0;
    }
    return strncmp((const char *)Token.Start, String, TokenLength) == 0;
}

static int TokenEqualsAny(const token &Token, void *Strings, uint32_t StringCount, uint32_t FirstIndex, uint32_t StringOffset, uint32_t TypeStride, uint32_t *OutIndex) {
    u8 *Bytes = (u8 *)Strings;
    for(uint32_t I = FirstIndex; I < StringCount; ++I) {
        const char *String = *(char **)(Bytes + I*TypeStride + StringOffset);
        if(TokenEquals(Token, String)) {
            *OutIndex = I;
            return 1;
        }
    }
    return 0;
}

static int TokenEqualsAnyString(const token &Token, const char **Strings, uint32_t StringCount, uint32_t FirstIndex, uint32_t *OutIndex) {
    return TokenEqualsAny(Token, Strings, StringCount, FirstIndex, 0, sizeof(const char *), OutIndex);
}

static int TokenEqualsGlslType(const token &Token, uint32_t *OutIndex) {
    return TokenEqualsAny(Token, GlslTypeInfos, glsl_type_COUNT, 1, offsetof(glsl_type_info, Name), sizeof(glsl_type_info), OutIndex);
}

static int ParseIntAssignment(const std::vector<token> &Tokens, uint32_t *InOutTokenIndex, uint32_t *OutInt) {
    uint32_t I = *InOutTokenIndex;

    if((I < Tokens.size() && Tokens[I].Start[0] == '=') == 0) {
        return 1;
    }
    ++I;
    if((I < Tokens.size()) == 0) {
        return 1;
    }
    char Buf[64] = {};
    uint32_t TokenLength = Tokens[I].End - Tokens[I].Start;
    if(TokenLength >= 64) {
        return 1;
    }
    memcpy(Buf, Tokens[I].Start, TokenLength);
    *OutInt = strtoull(Buf, 0, 10);

    *InOutTokenIndex = I;
    return 0;
}

static int ParseLayout(const std::vector<token> &Tokens, uint32_t *InOutTokenIndex, variable_layout &OutLayout) {
    variable_layout &Layout = OutLayout;
    uint32_t I = *InOutTokenIndex;

    if((I < Tokens.size() && Tokens[I].Start[0] == '(') == 0) {
        return 1;
    }
    ++I;
    while(I < Tokens.size()) {
        if(Tokens[I].Type == token_SINGLE) {
            if(Tokens[I].Start[0] == ')') {
                ++I;
                break;
            }
            if(Tokens[I].Start[0] == ',') {
                ++I;
            }
        } else if(Tokens[I].Type == token_NAME) {
            if(TokenEquals(Tokens[I], "location")) {
                ++I;
                if(ParseIntAssignment(Tokens, &I, &Layout.Location)) {
                    return 1;
                }
                ++I;
                Layout.LocationSet = 1;
            } else if(TokenEquals(Tokens[I], "binding")) {
                ++I;
                if(ParseIntAssignment(Tokens, &I, &Layout.Binding)) {
                    return 1;
                }
                ++I;
                Layout.BindingSet = 1;
            } else {
                // NOTE(blackedout): Unknown layout specifier
                Assert(0);
                ++I;
            }
        }
    }

    *InOutTokenIndex = I;
    return 0;
}

static int ParseVariable(const std::vector<token> &Tokens, uint32_t *InOutTokenIndex, variable &OutVar) {
    variable &Var = OutVar;

    uint32_t I = *InOutTokenIndex;
    uint32_t StartI = I;
    TrueOrReturn1(I < Tokens.size() && Tokens[I].Type == token_NAME);

    Var.LayoutSet = 0;
    if(TokenEquals(Tokens[I], "layout")) {
        ++I;
        if(ParseLayout(Tokens, &I, Var.Layout)) {
            return 1;
        }
        Var.LayoutSet = 1;
        TrueOrReturn1(I < Tokens.size());
    }

    uint32_t MatchIndex = 0;
    Var.StorageQualifier = storage_qualifier_NONE;
    if(TokenEqualsAnyString(Tokens[I], StorageQualifierStrings, storage_qualifier_COUNT, 1, &MatchIndex)) {
        Var.StorageQualifier = (storage_qualifier)MatchIndex;
        ++I;
        TrueOrReturn1(I < Tokens.size());
    }
    
    Var.Type = glsl_type_NONE;
    if(TokenEqualsGlslType(Tokens[I], &MatchIndex)) {
        Var.Type = (glsl_type)MatchIndex;
        ++I;
        TrueOrReturn1(I < Tokens.size());
    } else {
        // TODO(blackedout): Try inline struct definition
        //Assert(0);
        return 1;
    }
    
    TrueOrReturn1(Tokens[I].Type == token_NAME); 
    Var.NameTokenIndex = I;
    ++I;
    TrueOrReturn1(I < Tokens.size());
    
    TrueOrReturn1(Tokens[I].Type == token_SINGLE && Tokens[I].Start[0] == ';');
    ++I;

    Var.StartTokenIndex = StartI;
    Var.TokenLength = I - StartI;
    *InOutTokenIndex = I;
    return 0;
}

static int ParseFunction(const std::vector<token> &Tokens, uint32_t *InOutTokenIndex) {
    uint32_t I = *InOutTokenIndex;

    TrueOrReturn1(I < Tokens.size() && Tokens[I].Type == token_NAME);

    uint32_t MatchIndex = 0;
    TrueOrReturn1(TokenEqualsGlslType(Tokens[I], &MatchIndex));
    ++I;
    TrueOrReturn1(I < Tokens.size());

    TrueOrReturn1(Tokens[I].Type == token_NAME);
    ++I;
    TrueOrReturn1(I < Tokens.size());

    TrueOrReturn1(Tokens[I].Type == token_SINGLE && Tokens[I].Start[0] == '(');
    ++I;
    TrueOrReturn1(I < Tokens.size());
    while((Tokens[I].Type == token_SINGLE && Tokens[I].Start[0] == ')') == 0) {
        ++I;
        TrueOrReturn1(I < Tokens.size());
    }
    ++I;
    TrueOrReturn1(I < Tokens.size());

    TrueOrReturn1(Tokens[I].Type == token_SINGLE && Tokens[I].Start[0] == '{');
    ++I;
    TrueOrReturn1(I < Tokens.size());
    uint32_t CurlyDepth = 1;
    while(1) {
        if(Tokens[I].Type == token_SINGLE) {
            if(Tokens[I].Start[0] == '}') {
                TrueOrReturn1(CurlyDepth);
                --CurlyDepth;

                if(CurlyDepth == 0) {
                    ++I;
                    break;
                }
            } else if(Tokens[I].Start[0] == '{') {
                ++CurlyDepth;
            }
        }
        ++I;
        TrueOrReturn1(I < Tokens.size());
    }
    
    *InOutTokenIndex = I;
    return 0;
}

static void ParseProgramGlobals(const std::vector<token> &Tokens, parsed_shader &OutParsed) {
    // NOTE(blackedout): Skip version first
    uint32_t I = 0;
    if(I < Tokens.size() && Tokens[I].Type == token_PREPROCESSOR && TokenEquals(Tokens[I], "#version")) {
        ++I;
        if(I < Tokens.size() && Tokens[I].Type == token_NAME) {
            ++I;
            
            OutParsed.VersionEndTokenIndex = I;
            uint32_t ProfileIndex = 0;
            const char *ProfileStrings[] = { "core", "compatibility", "es" };
            if(I < Tokens.size() && Tokens[I].Type == token_NAME && TokenEqualsAnyString(Tokens[I], ProfileStrings, 3, 0, &ProfileIndex)) {
                OutParsed.ProfileTokenIndex = I;
                OutParsed.HasProfileDefinition = 1;
                ++I;
                OutParsed.VersionEndTokenIndex = I;
            }
        }
    }
    OutParsed.GlobalVariables.clear();

    variable Var;
    while(I < Tokens.size()) {
        if(0 == ParseVariable(Tokens, &I, Var)) {
            OutParsed.GlobalVariables.push_back(Var);
            if(Var.StorageQualifier == storage_qualifier_UNIFORM) {
                ++OutParsed.UniformCount;
            }
        } else if(0 == ParseFunction(Tokens, &I)) {
            int X = 0;
        } else {
            Assert(0);
        }
    }

    int i = 0;
    return;
}

static void MakeVulkanCompatible(shader_type ShaderType, const std::vector<token> &Tokens, const parsed_shader &ParsedShader, std::string &Out, uniform_variable *Uniforms, uint32_t UniformCount) {
    // NOTE(blackedout): IMPORTANT: `GlobalVariables` are expected to be in the order they are found in the program.

    Out.clear();
    std::string &Result = Out;
    Result += "#version 460";
    if(ParsedShader.HasProfileDefinition) {
        const token &Token = Tokens[ParsedShader.ProfileTokenIndex];
        Result.append(Token.Start, Token.End);
    }
    Result += '\n';
    token_type LastTokenType = token_SINGLE;

    std::set<std::string> UniformNameSet;
    int HasUniformContents = 0;
    std::string UniformString = "layout(binding=0) uniform ubo {\n";
    for(uint32_t I = 0; I < UniformCount; ++I) {
        HasUniformContents = 1;
        auto &Var = ParsedShader.GlobalVariables[Uniforms[I].VariableIndices[ShaderType]];
        glsl_type_info TypeInfo = GlslTypeInfos[Uniforms[I].Type];
        
        UniformString.append(TypeInfo.Name);
        UniformString += ' ';
        UniformString.append(Uniforms[I].Name);
        UniformString += ';';
        UniformString += '\n';

        UniformNameSet.insert(std::string(Uniforms[I].Name));
    }
    for(auto &Var : ParsedShader.GlobalVariables) {
        if(Var.StorageQualifier == storage_qualifier_UNIFORM) {
            
        }
    }
    UniformString += "} Ubo;\n";

    const auto AppendTokensUntil = [&](uint32_t &I, uint32_t EndIndex) {
        for(; I < EndIndex; ++I) {
            const token &Token = Tokens[I];
            if(LastTokenType == token_NAME && Token.Type == token_NAME) {
                Result += ' ';
            }
            if(Token.Type == token_NAME) {
                auto TokenString = std::string(Token.Start, Token.End);
                auto It = UniformNameSet.find(TokenString);
                if(It != UniformNameSet.end()) {
                    Result += "Ubo.";
                }
            }
            Result.append(Token.Start, Token.End);
            LastTokenType = Token.Type;
            if(Token.Type == token_SINGLE) {
                if(Token.Start[0] == ';' || Token.Start[0] == '{') {
                    Result += '\n';
                }
            }
        }
    };

    if(HasUniformContents) {
        Result += UniformString;
    }

    uint32_t TokenIndex = ParsedShader.VersionEndTokenIndex;
    for(uint32_t VarIndex = 0; VarIndex < ParsedShader.GlobalVariables.size(); ++VarIndex) {
        const variable &Var = ParsedShader.GlobalVariables[VarIndex];
        if(Var.StorageQualifier == storage_qualifier_UNIFORM) {
            AppendTokensUntil(TokenIndex, Var.StartTokenIndex);
            TokenIndex += Var.TokenLength;
        }
    }
    AppendTokensUntil(TokenIndex, Tokens.size());
}

int GlslangShaderCreateAndParse(shader_type Type, const char *Source, uint64_t SourceLength, glslang_shader *OutShader) {
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

    const int DefaultVersion = 460;
    const EShMessages Messages = EShMsgDefault;
    const TBuiltInResource *DefaultResources = GetDefaultResources();

    // TODO(blackedout): better exception handling
    try {
        glslang_shader Shader = {};
        const auto GlslangShader = new glslang::TShader(Language);
        Shader.Native = GlslangShader;
        Shader.Source = Source;
        Shader.SourceLength = SourceLength;

        // NOTE(blackedout): Parse to validate
        GlslangShader->setStrings(&OutShader->Source, 1);
        GlslangShader->setEnvInput(glslang::EShSourceGlsl, Language, glslang::EShClientOpenGL, DefaultVersion);
        GlslangShader->setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
        GlslangShader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
        GlslangShader->setAutoMapLocations(true);

        *OutShader = Shader;
        if(!GlslangShader->parse(DefaultResources, DefaultVersion, false, Messages)) {
            return glslang_error_COMPILATION_FAILED;
        }
    } catch (...) {
        return glslang_error_EXCEPTION;
    }

    return glslang_error_NONE;
}

int GlslangShaderGetLog(glslang_shader *Shader, uint64_t *OutLength, const char **OutString) {
    try {
        auto GlslangShader = (glslang::TShader *)Shader->Native;
        auto Info = GlslangShader->getInfoLog();

        *OutLength = strlen(Info);
        *OutString = Info;
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    return glslang_error_NONE;
}

void GlslangShaderDelete(glslang_shader *Shader) {
    auto GlslangShader = (glslang::TShader *)Shader->Native;
    delete GlslangShader;
    free((void *)Shader->Source);
    free((void *)Shader->SpirvBytes);
    free((void *)Shader->Variables);
    glslang_shader EmptyShader = {};
    *Shader = EmptyShader;
}

int GlslangProgramCreate(glslang_program *OutProgram) {
    try {
        glslang_program Program = {};
        Program.Native = new glslang::TProgram();
        *OutProgram = Program;
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    return glslang_error_NONE;
}

int GlslangProgramAddShader(glslang_program *Program, glslang_shader *Shader) {
    try {
        auto GlslangProgram = (glslang::TProgram *)Program->Native;
        auto GlslangShader = (glslang::TShader *)Shader->Native;
        //GlslangProgram->addShader(GlslangShader);
        shader_type Type;
        Assert(0 == GlslangLanguageToType(GlslangShader->getStage(), &Type));
        Program->AttachedShaders[Type] = Shader;
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    return glslang_error_NONE;
}

int GlslangProgramLink(glslang_program *Program) {
    const EShMessages Messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
    const int DefaultVersion = 460;
    const bool ForwardCompatible = false;
    const bool ForceDefaultVersionAndProfile = false;
    const EProfile DefaultProfile = ECoreProfile;
    try {
        const TBuiltInResource *DefaultResources = GetDefaultResources();
        std::vector<token> ShaderTokens[shader_COUNT];
        parsed_shader ParsedShaders[shader_COUNT] = {};
        std::string PreprocessedSources[shader_COUNT];
        for(uint32_t I = 0; I < shader_COUNT; ++I) {
            glslang_shader *Shader = Program->AttachedShaders[I];
            if(Shader == 0) {
                continue;
            }

            const auto GlslangShader = (glslang::TShader *)Shader->Native;
            const char *Source = Shader->Source;
            uint64_t SourceLength = Shader->SourceLength;

            #if 0
            CallbackIncluder callbackIncluder(input->callbacks, input->callbacks_ctx);
            glslang::TShader::Includer& Includer = (input->callbacks.include_local||input->callbacks.include_system)
                ? static_cast<glslang::TShader::Includer&>(callbackIncluder)
                : static_cast<glslang::TShader::Includer&>(dirStackFileIncluder);
            #endif

            DirStackFileIncluder Includer;
            // NOTE(blackedout): Now modify the program to be Vulkan compatible
            std::string &PreprocessedSource = PreprocessedSources[I];
            if(!GlslangShader->preprocess(DefaultResources, DefaultVersion, ECoreProfile, ForceDefaultVersionAndProfile, ForwardCompatible, Messages, &PreprocessedSource, Includer)) {
                return glslang_error_COMPILATION_FAILED;
            }
            
            std::vector<token> &Tokens = ShaderTokens[I];
            parsed_shader &ParsedShader = ParsedShaders[I];
            LexProgram(PreprocessedSource.c_str(), PreprocessedSource.length(), Tokens);
            ParseProgramGlobals(Tokens, ParsedShader);

            Shader->VariableCount = ParsedShader.GlobalVariables.size();
            Shader->Variables = (decltype(Shader->Variables))calloc(Shader->VariableCount, sizeof(shader_variable));
            if(Shader->Variables == 0) {
                return glslang_error_OUT_OF_MEMORY;
            }

            uint32_t NameBufByteCount = 0;
            for(uint32_t I = 0; I < ParsedShader.GlobalVariables.size(); ++I) {
                auto &Var = ParsedShader.GlobalVariables[I];
                token &NameToken = Tokens[Var.NameTokenIndex];
                auto NameLength = NameToken.End - NameToken.Start;
                NameBufByteCount += NameLength + 1;
            }

            // TOOD(blackedout): Fix leaks

            Shader->NameBuf = (decltype(Shader->NameBuf))calloc(NameBufByteCount, 1);
            if(Shader->NameBuf == 0) {
                return glslang_error_OUT_OF_MEMORY;
            }

            char *NameIt = Shader->NameBuf;
            for(uint32_t I = 0; I < ParsedShader.GlobalVariables.size(); ++I) {
                auto &Var = ParsedShader.GlobalVariables[I];
                shader_variable DstVar = {
                    .Name = NameIt,
                    .Location = Var.LayoutSet && Var.Layout.LocationSet ? (int)Var.Layout.Location : -1,
                    .ArrayLength = 1,
                    .StorageQualifier = Var.StorageQualifier,
                    .Type = Var.Type
                };
                token &NameToken = Tokens[Var.NameTokenIndex];
                auto NameLength = NameToken.End - NameToken.Start;
                memcpy(NameIt, NameToken.Start, NameLength);
                NameIt += NameLength + 1;

                Shader->Variables[I] = DstVar;
            }
        }

        struct cstr_cmp {
            bool operator()(const char *A, const char *B) const {
                return strcmp(A, B) < 0;
            }
        };
        
        std::set<int> UniformLocations;
        std::map<const char *, size_t, cstr_cmp> UniformNames;
        std::vector<uniform_variable> Uniforms;
        for(uint32_t I = 0; I < shader_COUNT; ++I) {
            glslang_shader *Shader = Program->AttachedShaders[I];
            if(Shader == 0) {
                continue;
            }

            for(uint32_t J = 0; J < Shader->VariableCount; ++J) {
                shader_variable &Var = Shader->Variables[J];
                if(Var.StorageQualifier != storage_qualifier_UNIFORM) {
                    continue;
                }
                uniform_variable *UniformVariable = 0;
                auto It = UniformNames.find(Var.Name);
                if(It != UniformNames.end()) {
                    UniformVariable = Uniforms.data() + It->second;

                    if(UniformVariable->Location == -1) {
                        UniformVariable->Location = Var.Location;
                        UniformVariable->LocationCount = Var.ArrayLength;
                    }
                } else {
                    uniform_variable NewUniformVariable = {
                        .Location = Var.Location,
                        .LocationCount = (int)Var.ArrayLength,
                        .Name = Var.Name,
                        .Type = Var.Type,
                    };
                    Uniforms.push_back(NewUniformVariable);
                    UniformNames[NewUniformVariable.Name] = Uniforms.size() - 1;
                    UniformVariable = Uniforms.data() + (Uniforms.size() - 1);
                }
                if(UniformVariable->Location != -1) {
                    for(int K = 0; K < UniformVariable->LocationCount; ++K) {
                        UniformLocations.insert(UniformVariable->Location + K);
                    }
                }
                
                UniformVariable->VariableIndicesSet = (shader_flags)(UniformVariable->VariableIndicesSet | (1 << I));
                UniformVariable->VariableIndices[I] = J;
            }
        }

        // NOTE(blackedout): Set remaining uniform locations, sort by them, then set byte offsets
        int MinLocation = 0;
        for(auto &Uniform : Uniforms) {
            if(Uniform.Location != -1) {
                continue;
            }

            while(UniformLocations.find(MinLocation) != UniformLocations.end()) {
                ++MinLocation;
            }
            Uniform.Location = MinLocation;
            MinLocation += Uniform.LocationCount;
        }

        std::sort(Uniforms.begin(), Uniforms.end(), [](const uniform_variable &Lhs, const uniform_variable &Rhs) {
            return Lhs.Location < Rhs.Location;
        });

        uint32_t ByteOffset = 0;
        uint32_t UniformLocationCount = 0;
        for(auto &Uniform : Uniforms) {
            glsl_type_info TypeInfo = GlslTypeInfos[Uniform.Type];
            if(ByteOffset % TypeInfo.ByteAlignment) {
                ByteOffset += TypeInfo.ByteAlignment - (ByteOffset % TypeInfo.ByteAlignment);
            }
            Uniform.ByteOffset = ByteOffset;
            ByteOffset += TypeInfo.ByteCount;
            UniformLocationCount += Uniform.LocationCount;
        }
        Program->UniformByteCount = ByteOffset;
        Program->UniformLocationCount = UniformLocationCount;

        const auto GlslangProgram = (glslang::TProgram *)Program->Native;
        Program->Uniforms = (decltype(Program->Uniforms))calloc(Uniforms.size(), sizeof(uniform_variable));
        if(Program->Uniforms == 0) {
            return glslang_error_OUT_OF_MEMORY;
        }
        Program->UniformCount = Uniforms.size();
        memcpy(Program->Uniforms, Uniforms.data(), Uniforms.size()*sizeof(uniform_variable));
        for(uint32_t I = 0; I < shader_COUNT; ++I) {
            glslang_shader *Shader = Program->AttachedShaders[I];
            if(Shader == 0) {
                continue;
            }

            {
                std::string Transformed;
                MakeVulkanCompatible((shader_type)I, ShaderTokens[I], ParsedShaders[I], Transformed, Program->Uniforms, Program->UniformCount);

                Shader->SourceLength = Transformed.length();
                Shader->Source = (decltype(Shader->Source))calloc(Shader->SourceLength + 1, 1);
                if(Shader->Source == 0) {
                    return glslang_error_OUT_OF_MEMORY;
                }
                memcpy((char *)Shader->Source, Transformed.c_str(), Shader->SourceLength);
            }

            printf("Transformed shader code:\n%.*s\n", (int)Shader->SourceLength, Shader->Source);
            
            // TODO(blackedout): How to reset the shader???
            const auto GlslangShader = (glslang::TShader *)Shader->Native;
            const auto Language = GlslangShader->getStage();
            GlslangShader->~TShader();
            new(GlslangShader) glslang::TShader(Language);
            
            GlslangShader->setStrings((const char **)&Shader->Source, 1);
            GlslangShader->setEnvInput(glslang::EShSourceGlsl, Language, glslang::EShClientVulkan, DefaultVersion);
            GlslangShader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
            GlslangShader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);

            if(!GlslangShader->parse(DefaultResources, DefaultVersion, ForwardCompatible, EShMsgDefault)) {
                return glslang_error_COMPILATION_FAILED;
            }
            GlslangProgram->addShader(GlslangShader);
        }
        
        if(!GlslangProgram->link(Messages)) {
            return glslang_error_LINKAGE_FAILED;
        }
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    return glslang_error_NONE;
}

int GlslangProgramGetLog(glslang_program *Program, uint64_t *OutLength, const char **OutString) {
    try {
        const auto GlslangProgram = (glslang::TProgram *)Program->Native;
        auto Info = GlslangProgram->getInfoLog();

        *OutLength = strlen(Info);
        *OutString = Info;
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    return glslang_error_NONE;
}

int GlslangProgramGetLocation(glslang_program *Program, const char *Name, int *OutLocation) {
    try {
        const auto GlslangProgram = (glslang::TProgram *)Program->Native;
        GlslangProgram->buildReflection();
        *OutLocation = GlslangProgram->getReflectionIndex(Name);
    } catch(...) {
        return glslang_error_EXCEPTION;
    }
    return glslang_error_NONE;
}

void GlslangProgramDelete(glslang_program *Program) {

    const auto GlslangProgram = (glslang::TProgram *)Program->Native;
    delete GlslangProgram;
    glslang_program EmptyProgram = {};
    *Program = EmptyProgram;
}

int GlslangGetSpirv(glslang_program *Program, glslang_shader *Shader, unsigned char **OutBytes, uint64_t *OutByteCount) {
    try {
        spv::SpvBuildLogger logger;

        const auto GlslangProgram = (glslang::TProgram *)Program->Native;
        //GlslangProgram->buildReflection();
        //printf("Num uniform variables: %d\n", GlslangProgram->getNumUniformVariables());

        const auto GlslangShader = (glslang::TShader *)Shader->Native;
        glslang::TIntermediate *Intermediate = GlslangProgram->getIntermediate(GlslangShader->getStage());

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