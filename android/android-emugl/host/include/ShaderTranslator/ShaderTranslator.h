#pragma once

#include "ShaderLang.h"

#include <GLES2/gl2.h>

extern "C" {

struct st_name_hashing_map
{
    char **user_names;
    char **compiled_names;
    int count;
};

// Shader variables

enum st_variable_datatype
{
    ST_VARIABLE_UNIFORM,
    ST_VARIABLE_ATTRIBUTE,
    ST_VARIABLE_OUTPUT,
    ST_VARIABLE_VARYING,
    ST_VARIABLE_INTERFACE_BLOCK_FIELD,
};

// Varying interpolation qualifier, see section 4.3.9 of the ESSL 3.00.4 spec
enum st_interpolation_qualifier
{
    ST_INTERPOLATION_SMOOTH,
    ST_INTERPOLATION_CENTROID,
    ST_INTERPOLATION_FLAT
};

// Uniform block layout qualifier, see section 4.3.8.3 of the ESSL 3.00.4 spec
enum st_block_layout_type
{
    ST_BLOCKLAYOUT_STANDARD,
    ST_BLOCKLAYOUT_STD140 = ST_BLOCKLAYOUT_STANDARD,
    ST_BLOCKLAYOUT_STD430,  // Shader storage block layout qualifier
    ST_BLOCKLAYOUT_PACKED,
    ST_BLOCKLAYOUT_SHARED
};

// Interface Blocks, see section 4.3.9 of the ESSL 3.10 spec
enum st_block_type
{
    ST_BLOCK_UNIFORM,
    ST_BLOCK_BUFFER,

    // Required in OpenGL ES 3.1 extension GL_OES_shader_io_blocks.
    // TODO(jiawei.shao@intel.com): add BLOCK_OUT.
    ST_BLOCK_IN
};

struct st_shader_variable
{
    st_variable_datatype st_var_type;

    unsigned int type;
    unsigned int precision;
    int location;
    int binding;
    int offset;
    bool readonly;
    bool writeonly;
    bool is_row_major_layout;
    st_interpolation_qualifier interpolation;
    bool is_invariant;
    bool is_built_in;

    char *name;
    char *mapped_name;
    char *struct_name;

    unsigned int *array_sizes;
    int array_sizes_count;

    unsigned int flattened_offset_in_parent_arrays;

    bool static_use;
    bool active;

    st_shader_variable *fields;
    int fields_count;
};

struct st_interface_block
{
    char *name;
    char *mapped_name;
    char *instance_name;
    unsigned int array_size;
    st_block_layout_type layout;

    bool is_row_major_layout;

    int binding;
    bool static_use;
    bool active;
    st_block_type block_type;

    st_shader_variable *fields;
    int fields_count;
};

struct st_work_group_size
{
    int local_size_qualifiers[3];
};

typedef void (*STInitialize_t)(void);
typedef void (*STFinalize_t)(void);

typedef ShBuiltInResources* (*STGenerateResources_t)(ShBuiltInResources *resources);
typedef ShHandle (*STConstructCompiler_t)(GLenum type,
                             ShShaderSpec spec,
                             ShShaderOutput output,
                             ShBuiltInResources *resources);
typedef bool (*STCompile_t)(ShHandle compiler,
               const char *const *shaderStrings,
               size_t numStrings,
               ShCompileOptions compileOptions);
typedef void (*STClearResults_t)(const ShHandle handle);
typedef char *(*STGetInfoLog_t)(const ShHandle handle);
typedef char *(*STGetObjectCode_t)(const ShHandle handle);

typedef st_name_hashing_map (*STGetNameHashingMap_t)(const ShHandle handle);

typedef void (*STGetUniforms_t)(const ShHandle handle,
                   int *out_count,
                   struct st_shader_variable *out_vars);
typedef void (*STGetVaryings_t)(const ShHandle handle,
                   int *out_count,
                   struct st_shader_variable *out_vars);
typedef void (*STGetInputVaryings_t)(const ShHandle handle,
                        int *out_count,
                        struct st_shader_variable *out_vars);
typedef void (*STGetOutputVaryings_t)(const ShHandle handle,
                         int *out_count,
                         struct st_shader_variable *out_vars);
typedef void (*STGetAttributes_t)(const ShHandle handle,
                     int *out_count,
                     struct st_shader_variable *out_vars);
typedef void (*STGetOutputVariables_t)(const ShHandle handle,
                          int *out_count,
                          struct st_shader_variable *out_vars);
typedef void (*STGetInterfaceBlocks_t)(const ShHandle handle,
                          int *out_count,
                          struct st_interface_block *out_ibs);
typedef void (*STGetUniformBlocks_t)(const ShHandle handle,
                        int *out_count,
                        struct st_interface_block *out_ibs);
typedef void (*STGetShaderStorageBlocks_t)(const ShHandle handle,
                              int *out_count,
                              struct st_interface_block *out_ibs);
typedef void (*STGetComputeShaderLocalGroupSize_t)(const ShHandle handle,
                                      struct st_work_group_size *out_wg);
typedef int (*STGetVertexShaderNumViews_t)(const ShHandle handle);

#define LIST_ST_FUNCS(f) \
        f(STInitialize) \
        f(STFinalize) \
        f(STGenerateResources) \
        f(STConstructCompiler) \
        f(STCompile) \
        f(STClearResults) \
        f(STGetInfoLog) \
        f(STGetObjectCode) \
        f(STGetNameHashingMap) \
        f(STGetUniforms) \
        f(STGetVaryings) \
        f(STGetInputVaryings) \
        f(STGetOutputVaryings) \
        f(STGetAttributes) \
        f(STGetOutputVariables) \
        f(STGetInterfaceBlocks) \
        f(STGetUniformBlocks) \
        f(STGetShaderStorageBlocks) \
        f(STGetComputeShaderLocalGroupSize) \
        f(STGetVertexShaderNumViews) \

}
