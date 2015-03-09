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

 
#include "es1xMatrixStack.h"
#include "es1xMemory.h"

/*---------------------------------------------------------------------------*/

es1xMatrixStack* es1xMatrixStack_create(int size)
{
  ES1X_ASSERT(size > 0);

  /* Create and initialize matrix stack */
  {
    es1xMatrixStack* stack = (es1xMatrixStack*)es1xMalloc(sizeof(es1xMatrixStack));

    stack->matrices     = (es1xMatrix4x4*)es1xMalloc(sizeof(es1xMatrix4x4) * size);
    stack->top          = 0;
    stack->size         = size;

    es1xMatrix4x4_loadIdentity(&(stack->matrices[stack->top]));
    return stack;
  }
}

/*---------------------------------------------------------------------------*/

void es1xMatrixStack_destroy(es1xMatrixStack* stack)
{
  ES1X_ASSERT(stack);
  ES1X_ASSERT(stack->matrices);

  es1xFree(stack->matrices);
  es1xFree(stack);
}

/*---------------------------------------------------------------------------*/

void es1xMatrixStack_loadMatrix(es1xMatrixStack* stack, const GLfloat* matrix)
{
  ES1X_ASSERT(matrix);
  ES1X_ASSERT(stack);

  /* Load matrix */
  {
    es1xMatrix4x4* top = &(stack->matrices[stack->top]);
    ES1X_ASSERT(top);
    es1xMemCopy(&(top->data[0][0]), matrix, sizeof(GLfloat) * 16);
    ES1X_VOID_STATEMENT;
  }
}

/*---------------------------------------------------------------------------*/

void es1xMatrixStack_pushMatrix(es1xMatrixStack* stack, es1xMatrix4x4* matrix)
{
  ES1X_ASSERT(matrix);
  ES1X_ASSERT(stack);
  ES1X_ASSERT(stack->top + 1 < stack->size);

  stack->top ++;
  es1xMemCopy(&(stack->matrices[stack->top]), matrix, sizeof(es1xMatrix4x4));
}

/*---------------------------------------------------------------------------*/

es1xMatrix4x4* es1xMatrixStack_popMatrix(es1xMatrixStack* stack)
{
  ES1X_ASSERT(stack);
  ES1X_ASSERT(stack->top > 0);
  stack->top --;
  return &(stack->matrices[stack->top]);
}

/*---------------------------------------------------------------------------*/

es1xMatrix4x4* es1xMatrixStack_peekMatrix(es1xMatrixStack* stack)
{
  ES1X_ASSERT(stack);
  return &(stack->matrices[stack->top]);
}

/*---------------------------------------------------------------------------*/
