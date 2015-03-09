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

 
#ifndef _ES1XTYPECONVERTER_H
#define _ES1XTYPECONVERTER_H

#ifndef _ES1XDEFS_H
#	include "es1xDefs.h"
#endif

#ifndef __ES1XSTATEQUERYLOOKUP_H
#	include "es1xStateQueryLookup.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_itof			(void* dst, const void* src, int numElements);
void es1xTypeConverter_itox			(void* dst, const void* src, int numElements);
void es1xTypeConverter_itob			(void* dst, const void* src, int numElements);
void es1xTypeConverter_ftoi			(void* dst, const void* src, int numElements);
void es1xTypeConverter_ctoi			(void* dst, const void* src, int numElements);
void es1xTypeConverter_ftox			(void* dst, const void* src, int numElements);
void es1xTypeConverter_ftob			(void* dst, const void* src, int numElements);
void es1xTypeConverter_xtoi			(void* dst, const void* src, int numElements);
void es1xTypeConverter_xtof			(void* dst, const void* src, int numElements);
void es1xTypeConverter_xtob			(void* dst, const void* src, int numElements);
void es1xTypeConverter_btoi			(void* dst, const void* src, int numElements);
void es1xTypeConverter_btof			(void* dst, const void* src, int numElements);
void es1xTypeConverter_btox			(void* dst, const void* src, int numElements);
void es1xTypeConverter_etoi			(void* dst, const void* src, int numElements, es1xQueryResultMapFunction mapFunction);
void es1xTypeConverter_etof			(void* dst, const void* src, int numElements, es1xQueryResultMapFunction mapFunction);
void es1xTypeConverter_etox			(void* dst, const void* src, int numElements, es1xQueryResultMapFunction mapFunction);
void es1xTypeConverter_etob			(void* dst, const void* src, int numElements, es1xQueryResultMapFunction mapFunction);
void es1xTypeConverter_etoe			(void* dst, const void* src, int numElements, es1xQueryResultMapFunction mapFunction);
void es1xTypeConverter_convert		(void* dst, es1xStateQueryParamType dstType, const void* src, es1xStateQueryParamType srcType, int numElements);
void es1xTypeConverter_convertEntry	(const es1xStateQueryEntry* entry, void* dst, es1xStateQueryParamType dstType, const void* src, es1xStateQueryParamType srcType, int numElements);

/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ES1XTYPECONVERTER_H */
