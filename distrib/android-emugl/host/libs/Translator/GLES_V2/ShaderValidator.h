#include <string>
#include <vector>

enum GLSLESVariance {
    GLSLES_VARIANCE_INVARIANT = 0,
    GLSLES_VARIANCE_VARIANT = 1
};

enum GLSLESStorage {
    GLSLES_STORAGE_CONST = 0,
    GLSLES_STORAGE_UNIFORM = 1,
    GLSLES_STORAGE_ATTRIBUTE = 2,
    GLSLES_STORAGE_VARYING = 3
};

enum GLSLESPrecision {
    GLSLES_PRECISION_LOWP = 0,
    GLSLES_PRECISION_MEDIUMP = 1,
    GLSLES_PRECISION_HIGHP = 2,
    GLSLES_PRECISION_SUPERP = 3
};

struct GLSLESVariableDeclaration {
    GLSLESVariance variance;
    GLSLESStorage storage;
    GLSLESPrecision precision;
    std::string parameter;
    std::string var_name;
};

struct GLSLESFunctionPrototype {
    std::string name;
    std::vector<GLSLESVariableDeclaration> argument_types;
    std::string return_value;
};

std::vector<GLSLESVariableDeclaration>
parse_glsles_variable_decls(const char* src);

std::vector<GLSLESFunctionPrototype>
parse_glsles_function_prototypes(const char* src);

bool validate_glsles_keywords(const char* src);
