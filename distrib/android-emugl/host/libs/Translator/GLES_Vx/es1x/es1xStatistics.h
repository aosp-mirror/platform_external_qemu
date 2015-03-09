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

 
#ifndef _ES1XSTATISTICS_H
#define _ES1XSTATISTICS_H

#ifndef _ES1XDEFS_H
#       include "es1xDefs.h"
#endif

typedef enum
{
  STATISTIC_NUM_RENDER_CALLS = 0,
  STATISTIC_NUM_RENDER_CALLS_WITH_DIRTY_CONTEXT,
  STATISTIC_NUM_INVALIDATED_RENDER_CALLS,
  STATISTIC_NUM_CREATED_PROGRAMS,
  STATISTIC_NUM_CHANGED_PROGRAMS,
  NUM_STAT_TYPES,
} es1xStatTypes;

#ifdef ES1X_SUPPORT_STATISTICS
#       define ES1X_INCREASE_STATISTICS(C, S)      es1xStatistics_increase(C, S)
#else
#       define ES1X_INCREASE_STATISTICS(C, S)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _es1xStatistics_
{
  int entries[NUM_STAT_TYPES];
} es1xStatistics;

void    es1xStatistics_clear    (es1xStatistics* statistics);
void    es1xStatistics_increase (void *_context_, es1xStatTypes type);
int     es1xStatistics_get      (void *_context_, es1xStatTypes type);
void    es1xStatistics_dump     (void *_context_);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ES1X_STATISTICS_H */
