/*
 * Copyright (C) 2011 The Android Open Source Project
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

 
#ifndef _ES1XUNIFORMVERSIONS_H
#define _ES1XUNIFORMVERSIONS_H

#ifndef _ES1XDEFS_H
#       include "es1xDefs.h"
#endif

typedef enum
{
  UNIFORM_GROUP_UNSORTED = 0,
  UNIFORM_GROUP_FOG,
  UNIFORM_GROUP_VERTEX_PARAMETER,
  UNIFORM_GROUP_POINT_SIZE,
  UNIFORM_GROUP_ALPHA_TEST,
  UNIFORM_GROUP_PMV_MATRIX,
  UNIFORM_GROUP_MODELVIEW_MATRIX,
  UNIFORM_GROUP_INVERSION_MODELVIEW_MATRIX,
  UNIFORM_GROUP_TEXTURE_MATRIX,
  UNIFORM_GROUP_LIGHT         = UNIFORM_GROUP_TEXTURE_MATRIX + ES1X_NUM_SUPPORTED_TEXTURE_UNITS,
  UNIFORM_GROUP_LIGHT_MODEL   = UNIFORM_GROUP_LIGHT + ES1X_NUM_SUPPORTED_LIGHTS,
  UNIFORM_GROUP_CLIP_PLANE,
  UNIFORM_GROUP_MATERIALS     = UNIFORM_GROUP_CLIP_PLANE + ES1X_NUM_SUPPORTED_CLIP_PLANES,
  UNIFORM_GROUP_TEXTURE_ENV,
  NUM_UNIFORM_GROUPS          = UNIFORM_GROUP_TEXTURE_ENV + ES1X_NUM_SUPPORTED_TEXTURE_UNITS
} UniformGroup;

typedef struct
{
  GLint group[NUM_UNIFORM_GROUPS];
} es1xUniformVersions;

#ifdef __cplusplus
extern "C"
{
#endif

ES1X_INLINE void es1xUniformVersions_fill(es1xUniformVersions* versions, GLuint value)
{
  UniformGroup group;

  for(group=(UniformGroup)0;group<(UniformGroup)NUM_UNIFORM_GROUPS;group=(UniformGroup)(group+1))
    versions->group[group] = value;
}

ES1X_INLINE void es1xUniformVersions_init(es1xUniformVersions* versions)
{
  es1xUniformVersions_fill(versions, 0);
}

ES1X_INLINE void es1xUniformVersions_invalidateAll(es1xUniformVersions* versions)
{
  es1xUniformVersions_fill(versions, (GLuint)-1);
}

ES1X_INLINE void es1xUniformVersions_increment(es1xUniformVersions* versions, UniformGroup group)
{
  ES1X_ASSERT(versions);
  ES1X_ASSERT(group >= 0 && group < NUM_UNIFORM_GROUPS);
  versions->group[group]++;
}

ES1X_INLINE GLboolean es1xUniformVersions_differs(const es1xUniformVersions* versions0, const es1xUniformVersions* versions1, UniformGroup group)
{
  ES1X_ASSERT(versions0);
  ES1X_ASSERT(versions1);
  ES1X_ASSERT(group >= 0 && group < NUM_UNIFORM_GROUPS);
  return (GLboolean)(versions0->group[group] != versions1->group[group]);
}

ES1X_INLINE void es1xUniformVersions_update(const es1xUniformVersions* src, es1xUniformVersions* dst, UniformGroup group)
{
  ES1X_ASSERT(src);
  ES1X_ASSERT(dst);
  ES1X_ASSERT(group >= 0 && group < NUM_UNIFORM_GROUPS);
  dst->group[group] = src->group[group];
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ES1XUNIFORMVERSIONS_H */
