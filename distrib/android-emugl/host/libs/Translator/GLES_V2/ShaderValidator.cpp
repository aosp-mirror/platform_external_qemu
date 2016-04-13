// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "ShaderValidator.h"

#include <algorithm>
#include <sstream>

const size_t kNotFound = std::string::npos;

// Replaces |rm| with |rep| in |str|.
// Example: rm = "a", rep = "bb", str = "abcd" returns "bbbcd"
static std::string replace_with(const std::string& rm,
                                const std::string& rep,
                                const std::string& str) {
    std::string res;
    size_t p = 0;
    while (p < str.size()) {
        size_t match_pos = str.find(rm,  p);
        if (match_pos != kNotFound) {
            res.append(str.substr(p, match_pos - p));
            res.append(rep);
            p = match_pos + rm.size();
        } else {
            res.append(str.substr(p, str.size() - p));
            p = str.size();
        }
    }
    return res;
}

// Removes everything between "in" and "out" characters
// including the in/out characters themselves.
// Only cares about full matches.
// Only one level deep.
// Example: src = "a (1 2)b(3)",
//          in = "(", out = ")"
//          returns "a b"
// Example: src = "a (1 2 (3))"
//          in = "(", out = ")"
//          returns "a )"
static std::string remove_in_out(const std::string& src,
                                 const std::string& in,
                                 const std::string& out,
                                 bool* err) {
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
                *err = true;
                return src;
            }
        } else {
            out_pos = src.find(out, p);
            if (out_pos != kNotFound) {
                *err = true;
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
// Example: src = "a (1 2)b(3)",
//          in = "(", out = ")"
//          returns vector with ["1 2", "3"]
// Example: src = "a (1 2 (3))"
//          in = "(", out = ")"
//          returns vector with ["1 2 (3"]
static std::vector<std::string> isolate_in_out(const std::string& src,
                          const std::string& in,
                          const std::string& out,
                          bool* err) {
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
                res.push_back(src.substr(in_pos + in.size(), out_pos - in_pos - in.size()));
            } else {
                *err = true;
                return err_res;
            }
        } else {
            out_pos = src.find(out, p);
            if (out_pos != kNotFound) {
                *err = true;
                return err_res;
            }
        }

        if (found_match) {
            p = out_pos + out.size();
        } else {
            p = src.size();
        }
    }

    return res;
}

// Splits a string |src| delimited by |delim|.
// Example: delim = ",", src = "1,2,3"
//          returns vector with ["1","2","3"]
static std::vector<std::string> un_delimit(const std::string& delim,
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

static std::vector<std::string> parse_raw_declarations(std::string text) {
    std::string emp("");
    std::string sp(" ");

    std::string newline("\n");
    std::string scol(";");

    std::string openpar("(");
    std::string openparsp("( ");
    std::string closepar(")");
    std::string openbr("{");
    std::string closebr("}");
    std::string brscol("};");

    text.insert(0, sp);

    std::string preprocessed = replace_with(newline, sp,
                               replace_with(openpar, openparsp,
                               replace_with(closebr, brscol, text)));

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

// Attempts to find any string in to_find in src.
// Returns the position of the last match, or kNotFound
// Example: src = "abcdef", to_find = ["b", "c", "d"]
//          returns 3
static size_t multi_find(const std::string& src,
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

// Validates arrangement of keywords in
// a GLSL ES variable declaration or function parameter.
// With the given parsing functions, this is only designed to pass
// dEQP-GLES2.shaders.qualification_order.*
static void validate_keywords_in_decls(const std::vector<std::string>& decls,
                                       bool* err) {
    std::vector<std::string> variance_keywords;

    variance_keywords.push_back(std::string("invariant"));

    std::vector<std::string> storage_keywords;

    storage_keywords.push_back(std::string(" const "));
    storage_keywords.push_back(std::string(" uniform "));
    storage_keywords.push_back(std::string(" attribute "));
    storage_keywords.push_back(std::string(" varying "));
    storage_keywords.push_back(std::string(" sampler "));

    std::vector<std::string> precision_keywords;

    precision_keywords.push_back(std::string(" lowp "));
    precision_keywords.push_back(std::string(" mediump "));
    precision_keywords.push_back(std::string(" highp "));
    precision_keywords.push_back(std::string(" superp "));

    std::vector<std::string> parameter_keywords;

    parameter_keywords.push_back(std::string(" in ")); // c.f., "int"
    parameter_keywords.push_back(std::string(" out "));
    parameter_keywords.push_back(std::string(" inout "));

    for (size_t i = 0; i < decls.size(); i++) {
        std::string variance, storage, precision, parameter;
        size_t storage_pos, precision_pos, parameter_pos;

        storage_pos = multi_find(decls[i], storage_keywords);
        precision_pos = multi_find(decls[i], precision_keywords);
        parameter_pos = multi_find(decls[i], parameter_keywords);

        if (storage_pos != kNotFound &&
                precision_pos != kNotFound) {
            if (storage_pos < precision_pos) {
                if (parameter_pos != kNotFound) {
                    if (parameter_pos > precision_pos) {
                        *err = true;
                    }
                }
            } else {
                *err = true;
            }
        } else if (precision_pos != kNotFound &&
                parameter_pos != kNotFound) {
            if (parameter_pos > precision_pos) {
                *err = true;
            }
        }
    }
}

static void validate_glsles_variable_decls(
        const std::vector<std::string>& raw_decls,
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

static void validate_glsles_function_parameters(
        const std::vector<std::string>& raw_decls,
        bool* err) {

    bool parse_err = false;
    std::string openpar("(");
    std::string closepar(")");
    std::string comma(",");
    std::vector<std::string> func_param_decls;
    for (size_t i = 0; i < raw_decls.size(); i++) {
        if (raw_decls[i].find(openpar) != kNotFound) {
            std::vector<std::string> func_params =
                isolate_in_out(raw_decls[i], openpar, closepar, &parse_err);
            if (func_params.size() > 0) {
                // Just use the first set of parens
                std::vector<std::string> these_params =
                    un_delimit(comma, func_params[0]);
                func_param_decls.insert(func_param_decls.end(),
                                        these_params.begin(),
                                        these_params.end());
            }
        }
    }

    validate_keywords_in_decls(func_param_decls, err);
}

bool validate_glsles_keywords(const char* src) {
    bool err = false;

    std::vector<std::string> raw_decls = parse_raw_declarations(src);

    // Liberal validation if parsing fails
    if (raw_decls.empty()) { return true; }

    validate_glsles_variable_decls(raw_decls, &err);
    validate_glsles_function_parameters(raw_decls, &err);
    return !err;
}
