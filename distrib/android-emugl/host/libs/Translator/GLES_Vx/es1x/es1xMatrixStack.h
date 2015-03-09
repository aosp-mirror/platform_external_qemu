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

 
#ifndef _ES1XMATRIXSTACK_H
#define _ES1XMATRIXSTACK_H

#ifndef _ES1XMATRIX_H
#	include "es1xMatrix.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct 
{
	es1xMatrix4x4*	matrices;
	int				top;
	int				size;
} es1xMatrixStack;

es1xMatrixStack*	es1xMatrixStack_create		(int size);
void				es1xMatrixStack_destroy		(es1xMatrixStack* stack);
void				es1xMatrixStack_loadMatrix	(es1xMatrixStack* stack, const GLfloat* matrix);
void				es1xMatrixStack_pushMatrix	(es1xMatrixStack* stack, es1xMatrix4x4* matrix);
es1xMatrix4x4*		es1xMatrixStack_popMatrix	(es1xMatrixStack* stack);
es1xMatrix4x4*		es1xMatrixStack_peekMatrix	(es1xMatrixStack* stack);

#ifdef __cplusplus
}
#endif

#endif /* _ES1XMATRIXSTACK_H */


