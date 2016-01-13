#ifndef GLES_FUNCTIONS_H
#define GLES_FUNCTIONS_H

#include "gles_common_functions.h"
#include "gles_extensions_functions.h"
#include "gles1_only_functions.h"
#include "gles1_extensions_functions.h"
#include "gles2_only_functions.h"
#include "gles2_extensions_functions.h"
#include "gles3_only_functions.h"

#define LIST_GLES_FUNCTIONS(X,Y) \
    LIST_GLES_COMMON_FUNCTIONS(X) \
    LIST_GLES_EXTENSIONS_FUNCTIONS(Y) \
    LIST_GLES1_ONLY_FUNCTIONS(X) \
    LIST_GLES1_EXTENSIONS_FUNCTIONS(Y) \
    LIST_GLES2_ONLY_FUNCTIONS(X) \
    LIST_GLES2_EXTENSIONS_FUNCTIONS(Y) \
    LIST_GLES3_ONLY_FUNCTIONS(X) \

#endif  // GLES_FUNCTIONS_H
