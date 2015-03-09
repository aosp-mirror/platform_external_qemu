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

#include "es1xStatistics.h"
#include "es1xDebug.h"
#include "es1xContext.h"

/*---------------------------------------------------------------------------*/

void es1xStatistics_clear(es1xStatistics* statistics)
{
  int i = 0;
  ES1X_ASSERT(statistics);

  /* Clear all statistics */

  for(i=0;i<NUM_STAT_TYPES;i++)
    statistics->entries[i] = 0;
}

/*---------------------------------------------------------------------------*/

void es1xStatistics_increase(void *_context_,
                             es1xStatTypes type)
{
  es1xContext *context = (es1xContext *) _context_;
  if(context == NULL)
    return;


  ES1X_ASSERT(type >= 0 && type < NUM_STAT_TYPES);
  context->statistics.entries[type]++;

  return;
}

/*---------------------------------------------------------------------------*/

int es1xStatistics_get(void *_context_,
                       es1xStatTypes type)
{
  es1xContext *context = (es1xContext *) _context_;
  if (!_context_)
    return 0;

  ES1X_ASSERT(type >= 0 && type < NUM_STAT_TYPES);
  return context->statistics.entries[type];
}

/*---------------------------------------------------------------------------*/

void es1xStatistics_dump(void *_context_)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG(("--- Statistics for context @ %p  with %d active buffer binding---------------\n", _context_,
            (_context_ == NULL ? 0 : context->activeBufferBinding)));
  ES1X_LOG(("Render calls in total:                %d\n", es1xStatistics_get(_context_, STATISTIC_NUM_RENDER_CALLS)));
  ES1X_LOG(("Render calls with dirty context:      %d\n", es1xStatistics_get(_context_, STATISTIC_NUM_RENDER_CALLS_WITH_DIRTY_CONTEXT)));
  ES1X_LOG(("Render calls invalidated:             %d\n", es1xStatistics_get(_context_, STATISTIC_NUM_INVALIDATED_RENDER_CALLS)));
  ES1X_LOG(("Number of created shader programs:    %d\n", es1xStatistics_get(_context_, STATISTIC_NUM_CREATED_PROGRAMS)));
  ES1X_LOG(("Number of times program was changed:  %d\n", es1xStatistics_get(_context_, STATISTIC_NUM_CHANGED_PROGRAMS)));
}
