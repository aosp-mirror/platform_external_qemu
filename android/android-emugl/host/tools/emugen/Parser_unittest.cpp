/*
* Copyright 2014 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "Parser.h"

#include <gtest/gtest.h>

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

TEST(ParserTest, normalizeTypeDeclaration) {
    static const struct {
        const char* expected;
        const char* input;
    } kData[] = {
        { "char", "char" },
        { "const unsigned int", "   const   unsigned\tint\n" },
        { "char* const**", "char *const* *" },
    };
    const size_t kDataSize = ARRAYLEN(kData);
    for (auto n : kData) {
        std::string result;
        std::string text = "When parsing '";
        text += n.input;
        text += "'";

        result = normalizeTypeDeclaration(n.input);
        EXPECT_STREQ(n.expected, result.c_str()) << text;
    }
}

TEST(ParserTest, parseTypeDeclaration) {
    static const struct {
        const char* input;
        bool expected;
        const char* expectedType;
        const char* expectedError;
    } kData[] = {
            {"const", false, nullptr, "Missing type name"},
            {"const const", false, nullptr, "Missing type name"},
            {"foo", true, "foo", nullptr},
            {"void", true, "void", nullptr},
            {"const foo", true, "const foo", nullptr},
            {"foo *", true, "foo*", nullptr},
            {"char foo", true, "char foo", nullptr},
            {"\tunsigned \t  int\n", true, "unsigned int", nullptr},
            {"const * char", false, nullptr, "Unexpected '*' before type name"},
            {"const char * ", true, "const char*", nullptr},
            {"const void*const * *", true, "const void* const**", nullptr},
    };
    const size_t kDataSize = ARRAYLEN(kData);
    for (const auto& n : kData) {
        std::string varname, vartype, error;
        std::string text = "When parsing '";
        text += n.input;
        text += "'";

        EXPECT_EQ(n.expected, parseTypeDeclaration(n.input, &vartype, &error))
                << text;
        if (n.expected) {
            EXPECT_STREQ(n.expectedType, vartype.c_str()) << text;
        } else {
            EXPECT_STREQ(n.expectedError, error.c_str()) << text;
        }
    }
}

TEST(ParserTest, parseParameterDeclaration) {
    static const struct {
        const char* input;
        bool expected;
        const char* expectedType;
        const char* expectedVariable;
        const char* expectedError;
    } kData[] = {
            {"foo", false, nullptr, nullptr, "Missing variable name"},
            {"const", false, nullptr, nullptr, "Missing type name"},
            {"const foo", false, nullptr, nullptr, "Missing variable name"},
            {"const const", false, nullptr, nullptr, "Missing type name"},
            {"char foo", true, "char", "foo", nullptr},
            {"unsigned   int\t bar\n", true, "unsigned int", "bar", nullptr},
            {"const * char foo", false, nullptr, nullptr,
             "Unexpected '*' before type name"},
            {"const char * foo", true, "const char*", "foo", nullptr},
            {"const void*const *data", true, "const void* const*", "data",
             nullptr},
            {"char foo const", false, nullptr, nullptr,
             "Extra 'const' after variable name"},
            {"int bar*", false, nullptr, nullptr,
             "Extra '*' after variable name"},
    };
    const size_t kDataSize = ARRAYLEN(kData);
    for (const auto& n : kData) {
        std::string varname, vartype, error;
        std::string text = "When parsing '";
        text += n.input;
        text += "'";

        EXPECT_EQ(n.expected, parseParameterDeclaration(n.input, &vartype,
                                                        &varname, &error))
                << text;
        if (n.expected) {
            EXPECT_STREQ(n.expectedType, vartype.c_str()) << text;
            EXPECT_STREQ(n.expectedVariable, varname.c_str()) << text;
        } else {
            EXPECT_STREQ(n.expectedError, error.c_str()) << text;
        }
    }
}
