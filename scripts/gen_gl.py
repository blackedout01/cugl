import os, sys
from dataclasses import dataclass

GEN_GL_FIRST_LINE = "// The following part was generated using `scripts/gen_gl.py`"

@dataclass
class Function:
    name: str
    return_type: str
    params: str

@dataclass
class ParsedGladGL:
    functions: list[Function]
    glad_gl_only: str
    glad_source_comment: str


def make_fake_implementation(name: str, return_type: str):
    def r(value: str):
        return f"return {value};"

    if name == "glCheckFramebufferStatus" or name == "glCheckNamedFramebufferStatus":
        return r("GL_FRAMEBUFFER_COMPLETE")
    if name == "glClientWaitSync":
        return r("GL_CONDITION_SATISFIED")
    if name == "glGetShaderiv":
        return "if(pname == GL_COMPILE_STATUS) *params = GL_TRUE;"
    if name == "glGetProgramiv":
        return "if(pname == GL_LINK_STATUS) *params = GL_TRUE;"
    if name == "glFenceSync":
        return r("(GLsync)1")
    if name == "glGetError" or name == "glGetGraphicsResetStatus":
        return r("GL_NO_ERROR")
    if name == "glUnmapBuffer" or name == "glUnmapNamedBuffer":
        return r("GL_TRUE")
    
    if return_type == "GLuint" or return_type == "GLint":
        return r("1")
    if return_type == "GLboolean":
        return r("GL_TRUE")

    return ""


def parse_glad_gl_header(glad_dir: str) -> ParsedGladGL:
    glad_gl_header_lines = []
    khr_contents = ""
    with open(os.path.join(glad_dir, "include", "glad", "gl.h")) as f:
        glad_gl_header_lines = f.readlines()
    with open(os.path.join(glad_dir, "include", "KHR", "khrplatform.h")) as f:
        khr_contents = f.read()

    result = ParsedGladGL([], "", "")

    signature_lines = []
    function_names_by_pfn = {}

    is_in_gl_part = False
    is_in_source_comment = True
    for line in glad_gl_header_lines:
        if is_in_gl_part:
            if line.startswith("#define GL_VERSION_1_0 1"):
                is_in_gl_part = False
            elif line.startswith("#include <KHR/khrplatform.h>"):
                result.glad_gl_only += khr_contents
            else:
                result.glad_gl_only += line
        else:
            if is_in_source_comment:
                result.glad_source_comment += line
                if "*/" in line:
                    is_in_source_comment = False
            if line.startswith("typedef") and "GLAD_API_PTR" in line:
                signature_lines.append(line)
            elif line.startswith("GLAD_API_CALL PFNGL"):
                _, pfn, name = line.split(' ')
                function_names_by_pfn[pfn] = name.strip().removeprefix("glad_").removesuffix(';')
            elif line.startswith("#endif /* GLAD_PLATFORM_H_ */"):
                is_in_gl_part = True

    for signature_line in signature_lines:
        typedef_result, ptr_pfn, named_params = signature_line.split('(')

        pfn = ptr_pfn.removeprefix("GLAD_API_PTR *").strip(')')
        if pfn not in function_names_by_pfn:
            continue

        return_type = typedef_result.removeprefix("typedef").strip()
        named_params = named_params.strip().removesuffix(");")

        function_name = function_names_by_pfn[pfn]
        result.functions.append(Function(function_name, return_type, named_params))

    return result


# NOTE(blackedout): Extract the OpenGL parts from the parsed glad header and put it into a string
# with added function declarations.
def gen_gl_header(parsed: ParsedGladGL) -> str:
    function_decls = ""
    for f in parsed.functions:
        space1 = "" if f.return_type.endswith('*') else ' '
        function_decls += f"{f.return_type}{space1}{f.name}({f.params});\n"

    gl_header = rf"""{GEN_GL_FIRST_LINE}
// It contains the OpenGL parts of `glad/gl.h` and added OpenGL function declarations.
{parsed.glad_source_comment}
#ifndef __gl_h_
#define GLAD_API_PTR
{parsed.glad_gl_only}{function_decls}
#endif
"""
    return gl_header


# NOTE(blackedout): Erase the OpenGL part from `cugl/cugl.h` and replace it with the regenerated
# version that is to be put in `gl_header`.
# IMPORTANT: The OpenGL part is identified by it's first line `GEN_GL_FIRST_LINE`. Don't put it
# elsewhere in the code, everything after will be erassed.
def mod_cugl_header(repo_dir: str, gl_header: str):
    cugl_header = ""
    cugl_header_lines = []
    with open(os.path.join(repo_dir, "include", "cugl", "cugl.h")) as f:
        cugl_header_lines = f.readlines()

    start_found = False
    for line in cugl_header_lines:
        if line.startswith(GEN_GL_FIRST_LINE):
            start_found = True
            break
        else:
            cugl_header += line

    if not start_found:
        print("Couldn't find start of gl header in cugl header. Aborting.")
        return

    cugl_header = rf"""{cugl_header}{gl_header}
#endif
"""
    with open(os.path.join(repo_dir, "include", "cugl", "cugl.h"), "w") as f:
        f.write(cugl_header)
    

# NOTE(blackedout): Generate `fake_glad_load_gl.h` that contains a function declaration `fakeGladLoadGL`,
# which is defined in the second generated file `fake_glad_load_gl.c` and sets the OpenGL function
# pointers of glad to fake (mostly empty) implementations which are also defined in that file.
# See `make_fake_implementation` in here for details of the function implementations.
# The output files are put into the directory specified with `dst_path`.
def gen_fake_gl(parsed: ParsedGladGL, dst_path: str):
    make_fake_name = lambda name: f"fake_{name}"

    function_impls = ""
    for f in parsed.functions:
        impl_code = make_fake_implementation(f.name, f.return_type)
        function_impls += f"{f.return_type} {make_fake_name(f.name)}({f.params}) {{{impl_code}}}\n"
    set_lines = "\n    ".join([f"glad_{f.name} = {make_fake_name(f.name)};" for f in parsed.functions])

    header_output = rf"""// Generated by `scripts/gen_gl.py`
#ifndef FAKE_GLAD_LOAD_GL_H
#define FAKE_GLAD_LOAD_GL_H
void fakeGladLoadGL();
#endif
"""
    
    source_output = rf"""// Generated by `scripts/gen_gl.py`
#include "glad/gl.h"

{function_impls}
void fakeGladLoadGL() {{
    {set_lines}
}}
"""

    with open(os.path.join(dst_path, "fake_glad_load_gl.h"), "w") as f:
        f.write(header_output)
    with open(os.path.join(dst_path, "fake_glad_load_gl.c"), "w") as f:
        f.write(source_output)


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.realpath(sys.argv[0]))
    parsed = parse_glad_gl_header(os.path.join(script_dir, "..", "glad"))

    usage = "Use mod or gen."
    if len(sys.argv) == 1:
        print(f"Too few arguments. {usage}")
        sys.exit()
    elif len(sys.argv) > 2:
        print(f"Too many arguments. {usage}")
        sys.exit()

    if sys.argv[1] == "mod":
        gl_header = gen_gl_header(parsed)
        mod_cugl_header(os.path.join(script_dir, ".."), gl_header)
    elif sys.argv[1] == "gen":
        gen_fake_gl(parsed, os.path.join(script_dir, ".."))
    else:
        print(f"Unknown argument {sys.argv[1]}. {usage}")
