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
#include <iostream>
#include <string>
#include <vector>

#define TrueOrReturn1(X) if((X) == 0) return 1

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

enum type {
    type_NONE = 0,
    type_FLOAT,
    type_VEC2,
    type_VEC3,
    type_VEC4,
    type_COUNT
};

const char *TypeStrings[] = {
    [type_NONE] = "",
    [type_FLOAT] = "float",
    [type_VEC2] = "vec2",
    [type_VEC3] = "vec3",
    [type_VEC4] = "vec4",
};
StaticAssert(ArrayCount(TypeStrings) == type_COUNT);

enum storage_qualifier {
    storage_qualifier_NONE = 0,
    storage_qualifier_IN,
    storage_qualifier_OUT,
    storage_qualifier_UNIFORM,
    storage_qualifier_CONST,
    storage_qualifier_COUNT
};

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
    type Type;
    uint32_t NameTokenIndex;

    uint32_t StartTokenIndex;
    uint32_t TokenLength;
};

struct parsed_shader {
    int HasProfileDefinition;
    uint32_t ProfileTokenIndex;
    uint32_t VersionEndTokenIndex;
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

static int TokenEqualsAny(const token &Token, const char **Strings, uint32_t StringCount, uint32_t Offset, uint32_t *OutIndex) {
    for(uint32_t I = Offset; I < StringCount; ++I) {
        if(TokenEquals(Token, Strings[I])) {
            *OutIndex = I;
            return 1;
        }
    }
    return 0;
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
    for(uint32_t I = 0; I < TokenLength; ++I) {
        Buf[I] = Tokens[I].Start[I];
    }
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
    if(TokenEqualsAny(Tokens[I], StorageQualifierStrings, storage_qualifier_COUNT, 1, &MatchIndex)) {
        Var.StorageQualifier = (storage_qualifier)MatchIndex;
        ++I;
        TrueOrReturn1(I < Tokens.size());
    }
    
    Var.Type = type_NONE;
    if(TokenEqualsAny(Tokens[I], TypeStrings, type_COUNT, 1, &MatchIndex)) {
        Var.Type = (type)MatchIndex;
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
    TrueOrReturn1(TokenEqualsAny(Tokens[I], TypeStrings, type_COUNT, 1, &MatchIndex) || TokenEquals(Tokens[I], "void"));
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
            if(I < Tokens.size() && Tokens[I].Type == token_NAME && TokenEqualsAny(Tokens[I], ProfileStrings, 3, 0, &ProfileIndex)) {
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
        } else if(0 == ParseFunction(Tokens, &I)) {
            int X = 0;
        } else {
            Assert(0);
        }
    }

    int i = 0;
    return;
}

static void MakeVulkanCompatible(const std::vector<token> &Tokens, const parsed_shader &ParsedShader, std::string &Out) {
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
    for(auto &Var : ParsedShader.GlobalVariables) {
        if(Var.StorageQualifier == storage_qualifier_UNIFORM) {
            HasUniformContents = 1;
            // TODO(blackedout): -1 in the first one is potentially not good
            auto &TypeToken = Tokens[Var.NameTokenIndex - 1];
            auto &NameToken = Tokens[Var.NameTokenIndex];
            auto &EndToken = Tokens[Var.StartTokenIndex + Var.TokenLength - 1];
            UniformString.append(TypeToken.Start, EndToken.End);
            UniformString += '\n';

            UniformNameSet.insert(std::string(NameToken.Start, NameToken.End));
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

    const int DefaultVersion = 460;
    const bool ForwardCompatible = false;
    const bool ForceDefaultVersionAndProfile = false;
    glslang::EShClient Client = glslang::EShClientOpenGL;
    glslang::EShTargetClientVersion ClientVersion = glslang::EShTargetOpenGL_450;
    const EShMessages Messages = EShMsgDefault;
    const EProfile DefaultProfile = ECoreProfile;
    DirStackFileIncluder Includer;
    const TBuiltInResource *DefaultResources = GetDefaultResources();

    #if 0
    CallbackIncluder callbackIncluder(input->callbacks, input->callbacks_ctx);
    glslang::TShader::Includer& Includer = (input->callbacks.include_local||input->callbacks.include_system)
        ? static_cast<glslang::TShader::Includer&>(callbackIncluder)
        : static_cast<glslang::TShader::Includer&>(dirStackFileIncluder);
    #endif

    // TODO(blackedout): better exception handling
    try {
        glslang::TShader *Shader = new glslang::TShader(Language);
        *OutShader = (glslang_shader)Shader;

        // NOTE(blackedout): First, parse to validate
        Shader->setStrings(&SourceBytes, 1);
        Shader->setEnvInput(glslang::EShSourceGlsl, Language, Client, DefaultVersion);
        Shader->setEnvClient(Client, ClientVersion);
        Shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
        Shader->setAutoMapLocations(true);

        if(!Shader->parse(DefaultResources, DefaultVersion, ForwardCompatible, Messages)) {
            return glslang_error_COMPILATION_FAILED;
        }

        // NOTE(blackedout): Now modify the program to be Vulkan compatible
        std::string PreprocessedSource;
        if(!Shader->preprocess(DefaultResources, DefaultVersion, ECoreProfile, ForceDefaultVersionAndProfile, ForwardCompatible, Messages, &PreprocessedSource, Includer)) {
            return glslang_error_COMPILATION_FAILED;
        }
        
        std::vector<token> Tokens;
        parsed_shader ParsedShader = {};
        LexProgram(PreprocessedSource.c_str(), PreprocessedSource.length(), Tokens);
        ParseProgramGlobals(Tokens, ParsedShader);
        std::string Transformed;
        MakeVulkanCompatible(Tokens, ParsedShader, Transformed);

        std::cout << "Transformed shader code:" << std::endl;
        std::cout << Transformed << std::endl;
        
        // TODO(blackedout): How to reset the shader???
        Shader->~TShader();
        new(Shader) glslang::TShader(Language);

        Client = glslang::EShClientVulkan;
        ClientVersion = glslang::EShTargetVulkan_1_0;
        const char *TransformedC = Transformed.c_str();
        Shader->setStrings(&TransformedC, 1);
        Shader->setEnvInput(glslang::EShSourceGlsl, Language, Client, DefaultVersion);
        Shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
        Shader->setEnvClient(Client, ClientVersion);
        //Shader->setAutoMapLocations(false);

        //const char *PreprocessedSourcePointer = PreprocessedSource.c_str();
        //Shader->setStrings(&PreprocessedSourcePointer, 1);
        
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

        glslang::TProgram *P = (glslang::TProgram *)Program;
        P->buildReflection();
        printf("Num uniform variables: %d\n", P->getNumUniformVariables());

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