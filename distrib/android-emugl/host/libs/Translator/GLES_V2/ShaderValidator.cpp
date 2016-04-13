#include "ShaderValidator.h"

#include <algorithm>
#include <sstream>

const size_t kNotFound = std::string::npos;

const char* test_src =
"precision mediump float;\n"
"lowp varying float x0;\n"
"\n"
"mediump uniform float x1;\n"
"\n"
"float foo() { int x = 3; return 3.0; }\n"
"void main()\n"
"{\n"
"	float result = x0 + x1;\n"
"	gl_FragColor = vec4(result, result, result, 1.0);\n"
"}\n"
"\n";

std::string remove_char(const char* c, const std::string& str) {
    std::string res(str);

    res.erase(std::remove(res.begin(), res.end(), *c), res.end());

    return res;
}

// Removes everything between "in" and "out" characters
// including the in/out characters themselves.
// Only cares about full matches.
std::string remove_in_out(const std::string& src,
                          const std::string& in,
                          const std::string& out,
                          bool* stahp) {
    std::string res;
    size_t p = 0;

    while (p < src.size()) {
        bool found_match = false;
        size_t in_pos = src.find(in, p);
        size_t out_pos;
        if (in_pos != kNotFound) {
            out_pos = src.find(out, in_pos + in.size());
            if (out_pos != kNotFound) {
                found_match = true;
            } else {
                *stahp = true;
                return src;
            }
        } else {
            out_pos = src.find(out, p);
            if (out_pos != kNotFound) {
                *stahp = true;
                return src;
            }
        }

        if (found_match) {
            res.append(src.substr(p, in_pos - p));
            p = out_pos + out.size();
        } else {
            res.append(src.substr(p, src.size() - p));
            p = src.size();
        }
    }

    return res;
}

std::vector<std::string> un_delimit(const std::vector<std::string>& delims,
                                    const std::string& src) {
    std::vector<std::string> res;
    const size_t notfound = std::string::npos;
    size_t p = 0;

    while (p < src.size()) {
        bool found_delim = false;
        size_t first_delim_pos;
        size_t first_delim_size;
        for (size_t i = 0; i < delims.size(); i++) {
            size_t delim_pos = src.find(delims[i], p);
            if (delim_pos != notfound) {
                if (!found_delim) {
                    found_delim = true;
                    first_delim_pos = delim_pos;
                    first_delim_size = delims[i].size();
                } else {
                    if (first_delim_pos > delim_pos) {
                        first_delim_pos = delim_pos;
                        first_delim_size = delims[i].size();
                    }
                }
            }
        }

        if (found_delim) {
            fprintf(stderr, "parseline\n");
            res.push_back(src.substr(p, first_delim_pos - p));
            p = first_delim_pos + first_delim_size;
        } else {
            res.push_back(src.substr(p, src.size() - p));
            p = src.size();
        }
    }

    return res;
}

std::vector<std::string> un_delimit(const std::string& delim,
                                    const std::string& src) {

    std::vector<std::string> res;
    const size_t notfound = std::string::npos;
    size_t delim_size = delim.size();
    size_t p = 0;
    size_t delim_loc = src.size();


    while (p < src.size()) {
        delim_loc = src.find(delim, p);
        if (delim_loc != notfound) {
            res.push_back(src.substr(p, delim_loc - p));
            p = delim_loc + delim_size;
            delim_loc = p;
        } else {
            res.push_back(src.substr(p, src.size() - p));
            p = src.size();
        }
    }

    return res;
}

std::vector<GLSLESVariableDeclaration>
parse_glsles_variable_decls(const char* src) {
    // TODO
    std::vector<GLSLESVariableDeclaration> res;




    return res;
}

std::vector<GLSLESFunctionPrototype>
parse_glsles_function_prototypes(const char* src) {
    // TODO
    std::vector<GLSLESFunctionPrototype> res;
    return res;
}

bool validate_glsles_keywords(const char* src) {
    return true;
}
