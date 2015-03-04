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

 
#include "es1xLight.h"

void es1xLight_init(es1xLight* light)
{
	es1xColor_set	(&light->ambient,			0.0f, 0.0f, 0.0f, 1.0f);
	es1xColor_set	(&light->diffuse,			0.0f, 0.0f, 0.0f, 1.0f);		
	es1xColor_set	(&light->specular,			0.0f, 0.0f, 0.0f, 1.0f);
	es1xVec4_set	(&light->eyePosition,		0.0f, 0.0f, 1.0f, 0.0f);
	es1xVec3_set	(&light->spotEyeDirection,	0.0f, 0.0f, -1.0f);
	
	light->spotExponent				= 0.0f;
	light->spotCutoff				= 180.f;
	
	light->constantAttenuation		= 1.0f;
	light->linearAttenuation		= 0.0f;
	light->quadraticAttenuation		= 0.0f;
}
