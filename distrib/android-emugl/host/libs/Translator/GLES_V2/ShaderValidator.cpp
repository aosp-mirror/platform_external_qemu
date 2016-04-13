#include "ShaderValidator.h"

#include <algorithm>
#include <sstream>

const size_t kNotFound = std::string::npos;

const char* test_src =
"precision mediump float;\n"
"lowp varying float x0;\n"
"\n"
"mediump uniform float x1;\n"
"uniform mediump int x2;\n"
"\n"
"float foo(const lowp in float y, mediump out float x,\n"
"inout mediump float r) { int z = 3; return 3.0; }\n"
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

std::string replace_with(const std::string rm,
                         const std::string rep,
                         const std::string& str) {
    std::string res(str);
    size_t p = 0;
    while (p < str.size()) {
        size_t match_pos = res.find(rm,  p);
        if (match_pos != kNotFound) {
            res.replace(match_pos, rm.size(), rep);
            p = match_pos + rep.size();
        } else {
            p = str.size();
        }
    }
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

// Isolates everything between "in" and "out" characters,
// not including the in/out characters themselves.
// Only cares about full matches.
// Only one level deep.
std::vector<std::string> isolate_in_out(const std::string& src,
                          const std::string& in,
                          const std::string& out,
                          bool* stahp) {
    std::vector<std::string> err_res;
    std::vector<std::string> res;
    size_t p = 0;

    while (p < src.size()) {
        bool found_match = false;
        size_t in_pos = src.find(in, p);
        size_t out_pos;
        if (in_pos != kNotFound) {
            out_pos = src.find(out, in_pos + in.size());
            if (out_pos != kNotFound) {
                found_match = true;
                res.push_back(src.substr(in_pos + in.size(), out_pos));
            } else {
                *stahp = true;
                return err_res;
            }
        } else {
            out_pos = src.find(out, p);
            if (out_pos != kNotFound) {
                *stahp = true;
                return err_res;
            }
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

std::vector<std::string> parse_raw_declarations(std::string text) {
    std::string emp("");

    std::string newline("\n");
    std::string scol(";");

    std::string openpar("(");
    std::string closepar(")");
    std::string openbr("{");
    std::string closebr("}");
    std::string brscol("};");

    std::string preprocessed = replace_with(newline, emp,
                                            replace_with(closebr, brscol, text));

    bool err = false;
    std::vector<std::string> decls;
    std::string no_func_impls = remove_in_out(preprocessed,
            openbr,
            closebr,
            &err);

    if (!err) {
        decls = un_delimit(scol, no_func_impls);
    }
    return decls;
}

size_t multi_find(const std::string& src,
                  const std::vector<std::string>& to_find) {

    size_t res = kNotFound;
    for (size_t i = 0; i < to_find.size(); i++) {
        size_t p = src.find(to_find[i]);
        if (p != kNotFound) {
            res = p;
        }
    }
    return res;
}

void validate_keywords_in_decls(const std::vector<std::string>& non_function_decls,
                                bool* err) {
    std::vector<std::string> variance_keywords;

    variance_keywords.push_back(std::string("invariant"));

    std::vector<std::string> storage_keywords;

    storage_keywords.push_back(std::string("const"));
    storage_keywords.push_back(std::string("uniform"));
    storage_keywords.push_back(std::string("attribute"));
    storage_keywords.push_back(std::string("varying"));
    storage_keywords.push_back(std::string("sampler"));

    std::vector<std::string> precision_keywords;

    precision_keywords.push_back(std::string("lowp"));
    precision_keywords.push_back(std::string("mediump"));
    precision_keywords.push_back(std::string("highp"));
    precision_keywords.push_back(std::string("superp"));

    std::vector<std::string> parameter_keywords;

    parameter_keywords.push_back(std::string("in ")); // c.f., "int"
    parameter_keywords.push_back(std::string("out"));
    parameter_keywords.push_back(std::string("inout"));

    for (size_t i = 0; i < non_function_decls.size(); i++) {
        std::string variance, storage, precision, parameter, varname;
        size_t variance_pos, storage_pos, precision_pos, parameter_pos, varname_pos;

        variance_pos = multi_find(non_function_decls[i], variance_keywords);
        storage_pos = multi_find(non_function_decls[i], storage_keywords);
        precision_pos = multi_find(non_function_decls[i], precision_keywords);
        parameter_pos = multi_find(non_function_decls[i], parameter_keywords);
        varname_pos = kNotFound;
        fprintf(stderr, "%s: %d %d %d %d\n", non_function_decls[i].c_str(),
                variance_pos,
                storage_pos,
                precision_pos,
                parameter_pos);

        if (storage_pos != kNotFound &&
                precision_pos != kNotFound) {
            if (storage_pos < precision_pos) {
                if (parameter_pos != kNotFound) {
                    if (parameter_pos > precision_pos) {
                        fprintf(stderr, "%s: INvalid decl (precision_parameter)\n", non_function_decls[i].c_str());
                        *err = true;
                    } else {
                        fprintf(stderr, "%s: valid decl\n", non_function_decls[i].c_str());
                    }
                } else {
                    fprintf(stderr, "%s: valid decl\n", non_function_decls[i].c_str());
                }
            } else {
                fprintf(stderr, "%s: INvalid decl (storage_precision)\n", non_function_decls[i].c_str());
                *err = true;
            }
        } else if (precision_pos != kNotFound &&
                parameter_pos != kNotFound) {
            if (parameter_pos > precision_pos) {
                fprintf(stderr, "%s: INvalid decl (precision_parameter)\n", non_function_decls[i].c_str());
                *err = true;
            }  else {
                fprintf(stderr, "%s: valid decl\n", non_function_decls[i].c_str());
            }
        }
    }
}

void validate_glsles_variable_decls(const std::vector<std::string>& raw_decls,
                                    bool* err) {
    std::string openpar("(");
    std::vector<std::string> non_function_decls;
    for (size_t i = 0; i < raw_decls.size(); i++) {
        if (raw_decls[i].find(openpar) == kNotFound) {
            non_function_decls.push_back(raw_decls[i]);
        }
    }
    validate_keywords_in_decls(non_function_decls, err);
}

void validate_glsles_function_parameters(const std::vector<std::string>& raw_decls,
                                         bool* err) {
    return;
}

bool validate_glsles_keywords(const char* src) {
    return true;

    bool err = false;

    std::string text(src);
    std::vector<std::string> raw_decls = parse_raw_declarations(src);

    // Liberal validation if parsing fails
    if (raw_decls.empty()) { return true; }

    validate_glsles_variable_decls(raw_decls, &err);
    validate_glsles_function_parameters(raw_decls, &err);

    return !err;
}
